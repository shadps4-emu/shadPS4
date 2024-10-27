// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

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
    decoded_samples = 0;
}

void AjmAt9Decoder::Initialize(const void* buffer, u32 buffer_size) {
    Atrac9ReleaseHandle(handle);
    handle = Atrac9GetHandle();
    ASSERT_MSG(buffer_size == sizeof(AjmDecAt9InitializeParameters),
               "Incorrect At9 initialization buffer size {}", buffer_size);
    const auto params = reinterpret_cast<const AjmDecAt9InitializeParameters*>(buffer);
    std::memcpy(config_data, params->config_data, SCE_AT9_CONFIG_DATA_SIZE);
    Atrac9InitDecoder(handle, config_data);
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

std::tuple<u32, u32, u32> AjmAt9Decoder::Decode(const u8* in_buf, u32 in_size, u8* out_buf,
                                                u32 out_size) {
    if (in_size <= 0 || out_size <= 0) {
        return std::tuple(in_size, out_size, 0);
    }

    const auto decoder_handle = static_cast<Atrac9Handle*>(handle);
    Atrac9CodecInfo codec_info;
    Atrac9GetCodecInfo(handle, &codec_info);

    int bytes_used = 0;
    int frame_count = 0;
    int bytes_remain = codec_info.superframeSize;

    const auto written_size = codec_info.channels * codec_info.frameSamples * sizeof(u16);
    for (frame_count = 0;
         frame_count < decoder_handle->Config.FramesPerSuperframe && in_size > 0 && out_size > 0;
         frame_count++) {
        u32 ret = Atrac9Decode(decoder_handle, in_buf, (short*)out_buf, &bytes_used);
        ASSERT_MSG(ret == At9Status::ERR_SUCCESS, "Atrac9Decode failed ret = {:#x}", ret);
        in_buf += bytes_used;
        in_size -= bytes_used;
        bytes_remain -= bytes_used;
        out_buf += written_size;
        out_size -= written_size;
        decoded_samples += decoder_handle->Config.FrameSamples;
    }

    in_size -= bytes_remain;

    LOG_TRACE(Lib_Ajm, "Decoded {} samples, frame count: {}", decoded_samples, frame_count);
    return std::tuple(in_size, out_size, frame_count);
}

} // namespace Libraries::Ajm
