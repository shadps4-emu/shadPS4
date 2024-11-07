// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/ajm/ajm_at9.h"
#include "core/libraries/ajm/ajm_instance.h"
#include "core/libraries/ajm/ajm_mp3.h"

#include <magic_enum.hpp>

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

AjmInstance::AjmInstance(AjmCodecType codec_type, AjmInstanceFlags flags) : m_flags(flags) {
    switch (codec_type) {
    case AjmCodecType::At9Dec: {
        m_codec = std::make_unique<AjmAt9Decoder>(AjmFormatEncoding(flags.format),
                                                  AjmAt9CodecFlags(flags.codec));
        break;
    }
    case AjmCodecType::Mp3Dec: {
        m_codec = std::make_unique<AjmMp3Decoder>(AjmFormatEncoding(flags.format));
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
        m_gapless_samples = 0;
        m_total_samples = 0;
        m_codec->Reset();
    }
    if (job.input.init_params.has_value()) {
        LOG_TRACE(Lib_Ajm, "Initializing instance {}", job.instance_id);
        auto& params = job.input.init_params.value();
        m_codec->Initialize(&params, sizeof(params));
    }
    if (job.input.resample_parameters.has_value()) {
        UNREACHABLE_MSG("Unimplemented: resample parameters");
        m_resample_parameters = job.input.resample_parameters.value();
    }
    if (job.input.format.has_value()) {
        UNREACHABLE_MSG("Unimplemented: format parameters");
        m_format = job.input.format.value();
    }
    if (job.input.gapless_decode.has_value()) {
        auto& params = job.input.gapless_decode.value();
        m_gapless.total_samples = params.total_samples;
        m_gapless.skip_samples = params.skip_samples;
    }

    if (!job.input.buffer.empty() && !job.output.buffers.empty()) {
        u32 frames_decoded = 0;
        std::span<u8> in_buf(job.input.buffer);
        SparseOutputBuffer out_buf(job.output.buffers);

        auto in_size = in_buf.size();
        auto out_size = out_buf.Size();
        while (!in_buf.empty() && !out_buf.IsEmpty() && !IsGaplessEnd()) {
            const auto samples_remain =
                m_gapless.total_samples != 0
                    ? std::optional<u32>{m_gapless.total_samples - m_gapless_samples}
                    : std::optional<u32>{};
            const auto [nframes, nsamples] =
                m_codec->ProcessData(in_buf, out_buf, m_gapless, samples_remain);
            frames_decoded += nframes;
            m_total_samples += nsamples;
            m_gapless_samples += nsamples;
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

    if (m_flags.gapless_loop && m_gapless.total_samples != 0 &&
        m_gapless_samples >= m_gapless.total_samples) {
        m_gapless_samples = 0;
        m_gapless.skipped_samples = 0;
        m_codec->Reset();
    }
    if (job.output.p_format != nullptr) {
        *job.output.p_format = m_codec->GetFormat();
    }
    if (job.output.p_gapless_decode != nullptr) {
        *job.output.p_gapless_decode = m_gapless;
    }
    if (job.output.p_codec_info != nullptr) {
        m_codec->GetInfo(job.output.p_codec_info);
    }
}

bool AjmInstance::IsGaplessEnd() {
    return m_gapless.total_samples != 0 && m_gapless_samples >= m_gapless.total_samples;
}

} // namespace Libraries::Ajm
