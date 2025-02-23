// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/ajm/ajm_at9.h"
#include "core/libraries/ajm/ajm_instance.h"
#include "core/libraries/ajm/ajm_mp3.h"

#include <magic_enum/magic_enum.hpp>

namespace Libraries::Ajm {

constexpr int ORBIS_AJM_RESULT_NOT_INITIALIZED = 0x00000001;
constexpr int ORBIS_AJM_RESULT_INVALID_DATA = 0x00000002;
constexpr int ORBIS_AJM_RESULT_INVALID_PARAMETER = 0x00000004;
constexpr int ORBIS_AJM_RESULT_PARTIAL_INPUT = 0x00000008;
constexpr int ORBIS_AJM_RESULT_NOT_ENOUGH_ROOM = 0x00000010;
constexpr int ORBIS_AJM_RESULT_STREAM_CHANGE = 0x00000020;
constexpr int ORBIS_AJM_RESULT_TOO_MANY_CHANNELS = 0x00000040;
constexpr int ORBIS_AJM_RESULT_UNSUPPORTED_FLAG = 0x00000080;
constexpr int ORBIS_AJM_RESULT_SIDEBAND_TRUNCATED = 0x00000100;
constexpr int ORBIS_AJM_RESULT_PRIORITY_PASSED = 0x00000200;
constexpr int ORBIS_AJM_RESULT_CODEC_ERROR = 0x40000000;
constexpr int ORBIS_AJM_RESULT_FATAL = 0x80000000;

u8 GetPCMSize(AjmFormatEncoding format) {
    switch (format) {
    case AjmFormatEncoding::S16:
        return sizeof(s16);
    case AjmFormatEncoding::S32:
        return sizeof(s32);
    case AjmFormatEncoding::Float:
        return sizeof(float);
    default:
        UNREACHABLE();
    }
}

AjmInstance::AjmInstance(AjmCodecType codec_type, AjmInstanceFlags flags) : m_flags(flags) {
    switch (codec_type) {
    case AjmCodecType::At9Dec: {
        m_codec = std::make_unique<AjmAt9Decoder>(AjmFormatEncoding(flags.format),
                                                  AjmAt9CodecFlags(flags.codec));
        break;
    }
    case AjmCodecType::Mp3Dec: {
        m_codec = std::make_unique<AjmMp3Decoder>(AjmFormatEncoding(flags.format),
                                                  AjmMp3CodecFlags(flags.codec));
        break;
    }
    default:
        UNREACHABLE_MSG("Unimplemented codec type {}", magic_enum::enum_name(codec_type));
    }
}

void AjmInstance::ExecuteJob(AjmJob& job) {
    const auto control_flags = job.flags.control_flags;
    if (True(control_flags & AjmJobControlFlags::Reset)) {
        LOG_TRACE(Lib_Ajm, "Resetting instance {}", job.instance_id);
        m_format = {};
        m_gapless = {};
        m_resample_parameters = {};
        m_total_samples = 0;
        m_codec->Reset();
    }
    if (job.input.init_params.has_value()) {
        LOG_TRACE(Lib_Ajm, "Initializing instance {}", job.instance_id);
        auto& params = job.input.init_params.value();
        m_codec->Initialize(&params, sizeof(params));
        is_initialized = true;
    }
    if (job.input.resample_parameters.has_value()) {
        LOG_ERROR(Lib_Ajm, "Unimplemented: resample parameters");
        m_resample_parameters = job.input.resample_parameters.value();
    }
    if (job.input.format.has_value()) {
        LOG_ERROR(Lib_Ajm, "Unimplemented: format parameters");
        m_format = job.input.format.value();
    }
    if (job.input.gapless_decode.has_value()) {
        auto& params = job.input.gapless_decode.value();
        if (params.total_samples != 0) {
            const auto max = std::max(params.total_samples, m_gapless.init.total_samples);
            m_gapless.current.total_samples += max - m_gapless.init.total_samples;
            m_gapless.init.total_samples = max;
        }
        if (params.skip_samples != 0) {
            const auto max = std::max(params.skip_samples, m_gapless.init.skip_samples);
            m_gapless.current.skip_samples += max - m_gapless.init.skip_samples;
            m_gapless.init.skip_samples = max;
        }
    }

    if (!is_initialized) {
        return;
    }

    if (!job.input.buffer.empty() && !job.output.buffers.empty()) {
        std::span<u8> in_buf(job.input.buffer);
        SparseOutputBuffer out_buf(job.output.buffers);

        u32 frames_decoded = 0;
        auto in_size = in_buf.size();
        auto out_size = out_buf.Size();
        while (!in_buf.empty() && !out_buf.IsEmpty() && !m_gapless.IsEnd()) {
            if (!HasEnoughSpace(out_buf)) {
                if (job.output.p_mframe == nullptr || frames_decoded == 0) {
                    job.output.p_result->result = ORBIS_AJM_RESULT_NOT_ENOUGH_ROOM;
                    break;
                }
            }

            const auto [nframes, nsamples] = m_codec->ProcessData(in_buf, out_buf, m_gapless);
            frames_decoded += nframes;
            m_total_samples += nsamples;

            if (False(job.flags.run_flags & AjmJobRunFlags::MultipleFrames)) {
                break;
            }
        }

        if (m_gapless.IsEnd()) {
            in_buf = in_buf.subspan(in_buf.size());
            m_gapless.current.total_samples = m_gapless.init.total_samples;
            m_gapless.current.skip_samples = m_gapless.init.skip_samples;
            m_codec->Reset();
        }
        if (job.output.p_mframe) {
            job.output.p_mframe->num_frames = frames_decoded;
        }
        if (job.output.p_stream) {
            job.output.p_stream->input_consumed = in_size - in_buf.size();
            job.output.p_stream->output_written = out_size - out_buf.Size();
            job.output.p_stream->total_decoded_samples = m_total_samples;
        }
    }

    if (job.output.p_format != nullptr) {
        *job.output.p_format = m_codec->GetFormat();
    }
    if (job.output.p_gapless_decode != nullptr) {
        *job.output.p_gapless_decode = m_gapless.current;
    }
    if (job.output.p_codec_info != nullptr) {
        m_codec->GetInfo(job.output.p_codec_info);
    }
}

bool AjmInstance::HasEnoughSpace(const SparseOutputBuffer& output) const {
    return output.Size() >= m_codec->GetNextFrameSize(m_gapless);
}

} // namespace Libraries::Ajm
