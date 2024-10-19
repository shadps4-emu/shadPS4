// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <fmt/format.h>
#include "common/assert.h"
#include "core/libraries/ajm/ajm_at9.h"
#include "error_codes.h"

extern "C" {
#include <decoder.h>
#include <libatrac9.h>
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
    Atrac9ReleaseHandle(handle);
    handle = Atrac9GetHandle();
    ASSERT_MSG(handle, "Atrac9GetHandle failed");
    decoded_samples = 0;
    static int filename = 0;
    file.close();
    file.open(fmt::format("inst{}_{}.raw", index, ++filename), std::ios::out | std::ios::binary);
}

std::tuple<u32, u32, u32> AjmAt9Decoder::Decode(const u8* in_buf, u32 in_size, u8* out_buf,
                                                u32 out_size) {
    if (in_size <= 0 || out_size <= 0) {
        return std::tuple(in_size, out_size, 0);
    }

    Atrac9InitDecoder(handle, config_data);

    const auto decoder_handle = static_cast<Atrac9Handle*>(handle);

    int frame_count;
    int bytes_used;

    Atrac9CodecInfo codec_info;
    Atrac9GetCodecInfo(handle, &codec_info);

    decoded_samples = 0;

    const auto size = codec_info.channels * codec_info.frameSamples * sizeof(u16);

    for (frame_count = 0; frame_count < decoder_handle->Config.FramesPerSuperframe; frame_count++) {
        short pcm_buffer[size];
        int ret = Atrac9Decode(decoder_handle, in_buf, pcm_buffer, &bytes_used);
        ASSERT_MSG(ret == At9Status::ERR_SUCCESS, "Atrac9Decode failed");
        in_buf += bytes_used;
        in_size -= bytes_used;
        std::memcpy(out_buf, pcm_buffer, size);
        file.write(reinterpret_cast<const char*>(pcm_buffer), size);
        out_buf += size;
        out_size -= size;
        decoded_samples += decoder_handle->Config.FrameSamples;
    }

    LOG_TRACE(Lib_Ajm, "Decoded {} samples, frame count: {}", decoded_samples, frame_count);

    return std::tuple(in_size, out_size, frame_count);
}

} // namespace Libraries::Ajm
