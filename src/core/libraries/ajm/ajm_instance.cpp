// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ajm_aac.h"
#include "ajm_at9.h"
#include "ajm_instance.h"
#include "ajm_mp3.h"
#include "ajm_result.h"

#include <magic_enum/magic_enum.hpp>

namespace Libraries::Ajm {

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
        m_codec = std::make_unique<AjmAt9Decoder>(
            AjmFormatEncoding(flags.format), AjmAt9CodecFlags(flags.codec), u32(flags.channels));
        break;
    }
    case AjmCodecType::Mp3Dec: {
        m_codec = std::make_unique<AjmMp3Decoder>(
            AjmFormatEncoding(flags.format), AjmMp3CodecFlags(flags.codec), u32(flags.channels));
        break;
    }
    case AjmCodecType::M4aacDec: {
        m_codec = std::make_unique<AjmAacDecoder>(
            AjmFormatEncoding(flags.format), AjmAacCodecFlags(flags.codec), u32(flags.channels));
        break;
    }
    default:
        UNREACHABLE_MSG("Unimplemented codec type {}", magic_enum::enum_name(codec_type));
    }
}

void AjmInstance::Reset() {
    m_total_samples = 0;
    m_gapless.Reset();
    m_codec->Reset();
}

void AjmInstance::ExecuteJob(AjmJob& job) {
    const auto control_flags = job.flags.control_flags;
    job.output.p_result->result = 0;
    if (True(control_flags & AjmJobControlFlags::Reset)) {
        LOG_TRACE(Lib_Ajm, "Resetting instance {}", job.instance_id);
        Reset();
    }
    if (job.input.init_params.has_value()) {
        LOG_TRACE(Lib_Ajm, "Initializing instance {}", job.instance_id);
        auto& params = job.input.init_params.value();
        m_codec->Initialize(&params, sizeof(params));
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

        const auto samples_processed =
            m_gapless.init.total_samples - m_gapless.current.total_samples;
        if (params.total_samples != 0 || params.skip_samples == 0) {
            if (params.total_samples >= samples_processed) {
                const auto sample_difference =
                    s64(m_gapless.init.total_samples) - params.total_samples;

                m_gapless.init.total_samples = params.total_samples;
                m_gapless.current.total_samples -= sample_difference;
            } else {
                LOG_WARNING(Lib_Ajm, "ORBIS_AJM_RESULT_INVALID_PARAMETER");
                job.output.p_result->result |= ORBIS_AJM_RESULT_INVALID_PARAMETER;
            }
        }

        const auto samples_skipped = m_gapless.init.skip_samples - m_gapless.current.skip_samples;
        if (params.skip_samples != 0 || params.total_samples == 0) {
            if (params.skip_samples >= samples_skipped) {
                const auto sample_difference =
                    s32(m_gapless.init.skip_samples) - params.skip_samples;

                m_gapless.init.skip_samples = params.skip_samples;
                m_gapless.current.skip_samples -= sample_difference;
            } else {
                LOG_WARNING(Lib_Ajm, "ORBIS_AJM_RESULT_INVALID_PARAMETER");
                job.output.p_result->result |= ORBIS_AJM_RESULT_INVALID_PARAMETER;
            }
        }
    }

    std::span<u8> in_buf(job.input.buffer);
    SparseOutputBuffer out_buf(job.output.buffers);
    auto in_size = in_buf.size();
    auto out_size = out_buf.Size();
    u32 frames_decoded = 0;

    if (!job.input.buffer.empty()) {
        for (;;) {
            if (m_flags.gapless_loop && m_gapless.IsEnd()) {
                m_gapless.Reset();
                m_total_samples = 0;
            }
            if (!HasEnoughSpace(out_buf)) {
                LOG_TRACE(Lib_Ajm, "ORBIS_AJM_RESULT_NOT_ENOUGH_ROOM ({} < {})", out_buf.Size(),
                          m_codec->GetNextFrameSize(m_gapless));
                job.output.p_result->result |= ORBIS_AJM_RESULT_NOT_ENOUGH_ROOM;
            }
            if (in_buf.size() < m_codec->GetMinimumInputSize()) {
                job.output.p_result->result |= ORBIS_AJM_RESULT_PARTIAL_INPUT;
            }
            if (job.output.p_result->result != 0) {
                break;
            }
            const auto result = m_codec->ProcessData(in_buf, out_buf, m_gapless);
            if (result.is_reset) {
                m_total_samples = 0;
            } else {
                m_total_samples += result.samples_written;
            }
            frames_decoded += result.frames_decoded;
            if (result.result != 0) {
                job.output.p_result->result |= result.result;
                job.output.p_result->internal_result = result.internal_result;
                break;
            }
            if (False(job.flags.run_flags & AjmJobRunFlags::MultipleFrames)) {
                break;
            }
        }
    }

    if (job.output.p_mframe) {
        job.output.p_mframe->num_frames = frames_decoded;
    }
    if (job.output.p_stream) {
        job.output.p_stream->input_consumed = in_size - in_buf.size();
        job.output.p_stream->output_written = out_size - out_buf.Size();
        job.output.p_stream->total_decoded_samples = m_total_samples;
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
    if (m_gapless.IsEnd()) {
        return true;
    }
    return output.Size() >= m_codec->GetNextFrameSize(m_gapless);
}

} // namespace Libraries::Ajm
