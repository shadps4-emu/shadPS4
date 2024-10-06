// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <fmt/format.h>
#include "common/assert.h"
#include "core/libraries/ajm/ajm_at9.h"

extern "C" {
#include <libatrac9.h>
}

namespace Libraries::Ajm {

AjmAt9Decoder::AjmAt9Decoder() {
    handle = Atrac9GetHandle();
    ASSERT_MSG(handle, "Atrac9GetHandle failed");
    AjmAt9Decoder::Reset();
}

AjmAt9Decoder::~AjmAt9Decoder() {}

void AjmAt9Decoder::Reset() {
    decoded_samples = 0;
    static int filename = 0;
    file.close();
    file.open(fmt::format("inst{}_{}.raw", index, ++filename), std::ios::out | std::ios::binary);
}

std::tuple<u32, u32, u32> AjmAt9Decoder::Decode(const u8* in_buf, u32 in_size, u8* out_buf,
                                                u32 out_size) {
    auto ParseWavHeader = [](const uint8_t* buf, WavHeader* header) {
        std::memcpy(header, buf, sizeof(WavHeader));
    };

    if (in_size <= 0 || out_size <= 0) {
        return std::tuple(in_size, out_size, 0);
    }

    WavHeader header{};
    ParseWavHeader(in_buf - 0x64, &header);

    if (!decoder_initialized) {
        Atrac9InitDecoder(handle, header.configData);
        decoder_initialized = true;
    }

    // todo: do something with decoded data :p

    int bytes_used;
    Atrac9Decode(handle, in_buf, nullptr, &bytes_used);

    Atrac9CodecInfo codec_info;
    Atrac9GetCodecInfo(handle, &codec_info);

    return std::tuple(in_size, out_size, 0);
}

} // namespace Libraries::Ajm
