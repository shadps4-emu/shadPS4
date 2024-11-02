// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <vector>
#include <fmt/format.h>
#include "common/assert.h"
#include "core/libraries/ajm/ajm_at9.h"
#include "error_codes.h"

extern "C" {
#include <decoder.h>
#include <libatrac9.h>
#include <libavutil/samplefmt.h>
}

namespace Libraries::Ajm {

AjmAt9Decoder::AjmAt9Decoder() {
    handle = Atrac9GetHandle();
    ASSERT_MSG(handle, "Atrac9GetHandle failed");
    AjmAt9Decoder::Reset();
}

AjmAt9Decoder::~AjmAt9Decoder() {
    Atrac9ReleaseHandle(handle);
}

void AjmAt9Decoder::Reset() {
    total_decoded_samples = 0;
    gapless = {};

    ResetCodec();
}

void AjmAt9Decoder::ResetCodec() {
    Atrac9ReleaseHandle(handle);
    handle = Atrac9GetHandle();
    Atrac9InitDecoder(handle, config_data);

    Atrac9CodecInfo codec_info;
    Atrac9GetCodecInfo(handle, &codec_info);
    num_frames = 0;
    superframe_bytes_remain = codec_info.superframeSize;
    gapless.skipped_samples = 0;
    gapless_decoded_samples = 0;
}

void AjmAt9Decoder::Initialize(const void* buffer, u32 buffer_size) {
    ASSERT_MSG(buffer_size == sizeof(AjmDecAt9InitializeParameters),
               "Incorrect At9 initialization buffer size {}", buffer_size);
    const auto params = reinterpret_cast<const AjmDecAt9InitializeParameters*>(buffer);
    std::memcpy(config_data, params->config_data, ORBIS_AT9_CONFIG_DATA_SIZE);
    AjmAt9Decoder::Reset();
}

void AjmAt9Decoder::GetCodecInfo(void* out_info) {
    Atrac9CodecInfo decoder_codec_info;
    Atrac9GetCodecInfo(handle, &decoder_codec_info);

    auto* codec_info = reinterpret_cast<AjmSidebandDecAt9CodecInfo*>(out_info);
    codec_info->uiFrameSamples = decoder_codec_info.frameSamples;
    codec_info->uiFramesInSuperFrame = decoder_codec_info.framesInSuperframe;
    codec_info->uiNextFrameSize = static_cast<Atrac9Handle*>(handle)->Config.FrameBytes;
    codec_info->uiSuperFrameSize = decoder_codec_info.superframeSize;
}

void AjmAt9Decoder::Decode(const AjmJobInput* input, AjmJobOutput* output) {
    Atrac9CodecInfo codec_info;
    Atrac9GetCodecInfo(handle, &codec_info);

    size_t out_buffer_index = 0;
    std::span<const u8> in_buf(input->buffer);
    std::span<u8> out_buf = output->buffers[out_buffer_index];
    const auto should_decode = [&] {
        if (in_buf.empty() || out_buf.empty()) {
            return false;
        }
        if (gapless.total_samples && gapless.total_samples < gapless_decoded_samples) {
            return false;
        }
        return true;
    };

    const auto write_output = [&](std::span<s16> pcm) {
        while (!pcm.empty()) {
            auto size = std::min(pcm.size() * sizeof(u16), out_buf.size());
            std::memcpy(out_buf.data(), pcm.data(), size);
            pcm = pcm.subspan(size >> 1);
            out_buf = out_buf.subspan(size);
            if (out_buf.empty()) {
                out_buffer_index += 1;
                if (out_buffer_index >= output->buffers.size()) {
                    return pcm.empty();
                }
                out_buf = output->buffers[out_buffer_index];
            }
        }
        return true;
    };

    int num_superframes = 0;
    const auto pcm_frame_size = codec_info.channels * codec_info.frameSamples * sizeof(u16);
    std::vector<s16> pcm_buffer(pcm_frame_size >> 1);
    while (should_decode()) {
        int bytes_used = 0;
        u32 ret = Atrac9Decode(handle, in_buf.data(), pcm_buffer.data(), &bytes_used);
        ASSERT_MSG(ret == At9Status::ERR_SUCCESS, "Atrac9Decode failed ret = {:#x}", ret);
        in_buf = in_buf.subspan(bytes_used);
        superframe_bytes_remain -= bytes_used;
        const size_t samples_remain =
            gapless.total_samples != 0
                ? (gapless.total_samples - gapless_decoded_samples) * codec_info.channels
                : std::numeric_limits<size_t>::max();
        bool written = false;
        if (gapless.skipped_samples < gapless.skip_samples) {
            gapless.skipped_samples += codec_info.frameSamples;
            if (gapless.skipped_samples > gapless.skip_samples) {
                const u32 nsamples = gapless.skipped_samples - gapless.skip_samples;
                const auto start = codec_info.frameSamples - nsamples;
                written = write_output({pcm_buffer.data() + start, nsamples * codec_info.channels});
                gapless.skipped_samples = gapless.skip_samples;
                total_decoded_samples += nsamples;
                if (gapless.total_samples != 0) {
                    gapless_decoded_samples += nsamples;
                }
            }
        } else {
            const auto pcm_size = std::min(pcm_buffer.size(), samples_remain);
            const auto nsamples = pcm_size / codec_info.channels;
            written = write_output({pcm_buffer.data(), pcm_size});
            total_decoded_samples += nsamples;
            if (gapless.total_samples != 0) {
                gapless_decoded_samples += nsamples;
            }
        }

        num_frames += 1;
        if ((num_frames % codec_info.framesInSuperframe) == 0) {
            num_frames = 0;
            if (superframe_bytes_remain) {
                if (output->p_stream) {
                    output->p_stream->input_consumed += superframe_bytes_remain;
                }
                in_buf = in_buf.subspan(superframe_bytes_remain);
            }
            superframe_bytes_remain = codec_info.superframeSize;
            num_superframes += 1;
        }
        if (output->p_stream) {
            output->p_stream->input_consumed += bytes_used;
            if (written) {
                output->p_stream->output_written +=
                    std::min(pcm_frame_size, samples_remain * sizeof(16));
            }
        }
        if (output->p_mframe) {
            output->p_mframe->num_frames += 1;
        }
    }

    if (flags.gapless_loop && gapless.total_samples != 0 &&
        gapless_decoded_samples >= gapless.total_samples) {
        ResetCodec();
    }

    if (output->p_stream) {
        output->p_stream->total_decoded_samples = total_decoded_samples;
    }

    LOG_TRACE(Lib_Ajm, "Decoded buffer, in remain = {}, out remain = {}", in_buf.size(),
              out_buf.size());
}

} // namespace Libraries::Ajm
