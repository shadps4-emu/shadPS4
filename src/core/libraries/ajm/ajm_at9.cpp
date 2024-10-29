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
    num_frames = 0;
    decoded_samples = 0;
    gapless = {};

    Atrac9ReleaseHandle(handle);
    handle = Atrac9GetHandle();
    Atrac9InitDecoder(handle, config_data);

    Atrac9CodecInfo codec_info;
    Atrac9GetCodecInfo(handle, &codec_info);
    bytes_remain = codec_info.superframeSize;
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

std::tuple<u32, u32> AjmAt9Decoder::Decode(const u8* in_buf, u32 in_size_in, u8* out_buf,
                                           u32 out_size_in, AjmJobOutput* output) {
    const auto decoder_handle = static_cast<Atrac9Handle*>(handle);
    Atrac9CodecInfo codec_info;
    Atrac9GetCodecInfo(handle, &codec_info);

    int bytes_used = 0;
    int num_superframes = 0;

    u32 in_size = in_size_in;
    u32 out_size = out_size_in;

    const auto ShouldDecode = [&] {
        if (in_size == 0 || out_size == 0) {
            return false;
        }
        if (gapless.total_samples != 0 && gapless.total_samples < decoded_samples) {
            return false;
        }
        return true;
    };

    const auto written_size = codec_info.channels * codec_info.frameSamples * sizeof(u16);
    std::vector<s16> pcm_buffer(written_size >> 1);
    while (ShouldDecode()) {
        u32 ret = Atrac9Decode(decoder_handle, in_buf, pcm_buffer.data(), &bytes_used);
        ASSERT_MSG(ret == At9Status::ERR_SUCCESS, "Atrac9Decode failed ret = {:#x}", ret);
        in_buf += bytes_used;
        in_size -= bytes_used;
        if (output->p_mframe) {
            ++output->p_mframe->num_frames;
        }
        num_frames++;
        bytes_remain -= bytes_used;
        if (gapless.skipped_samples < gapless.skip_samples) {
            gapless.skipped_samples += decoder_handle->Config.FrameSamples;
            if (gapless.skipped_samples > gapless.skip_samples) {
                const auto size = gapless.skipped_samples - gapless.skip_samples;
                const auto start = decoder_handle->Config.FrameSamples - size;
                memcpy(out_buf, pcm_buffer.data() + start, size * sizeof(s16));
                out_buf += size * sizeof(s16);
                out_size -= size * sizeof(s16);
            }
        } else {
            memcpy(out_buf, pcm_buffer.data(), written_size);
            out_buf += written_size;
            out_size -= written_size;
        }
        decoded_samples += decoder_handle->Config.FrameSamples;
        if ((num_frames % codec_info.framesInSuperframe) == 0) {
            in_buf += bytes_remain;
            in_size -= bytes_remain;
            bytes_remain = codec_info.superframeSize;
            num_superframes++;
        }
    }

    if (gapless.total_samples == decoded_samples) {
        decoded_samples = 0;
        if (flags.gapless_loop) {
            gapless.skipped_samples = 0;
        }
    }

    LOG_TRACE(Lib_Ajm, "Decoded {} samples, frame count: {}", decoded_samples, num_frames);
    return std::tuple(in_size, out_size);
}

} // namespace Libraries::Ajm
