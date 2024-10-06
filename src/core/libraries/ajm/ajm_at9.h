// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <fstream>
#include <guiddef.h>
#include "common/types.h"
#include "core/libraries/ajm/ajm_instance.h"

extern "C" {
#include <structures.h>
}

namespace Libraries::Ajm {

constexpr unsigned int SCE_AT9_CONFIG_DATA_SIZE = 4;

#pragma pack(push, 1)
struct WavHeader {
    /* RIFF Chunk Descriptor */
    char RIFF[4];       // RIFF Header Magic header
    uint32_t ChunkSize; // RIFF Chunk Size
    char WAVE[4];       // WAVE Header
    /* "fmt" sub-chunk */
    char fmt[4];            // FMT header
    uint32_t Subchunk1Size; // Size of the fmt chunk
    uint16_t AudioFormat;   // Audio format 1=PCM,6=mulaw,7=alaw,     257=IBM Mu-Law, 258=IBM A-Law,
                            // 259=ADPCM
    uint16_t NumOfChan;     // Number of channels 1=Mono 2=Sterio
    uint32_t SamplesPerSec; // Sampling Frequency in Hz
    uint32_t bytesPerSec;   // bytes per second
    uint16_t blockAlign;    // 2=16-bit mono, 4=16-bit stereo
    uint16_t bitsPerSample; // Number of bits per sample
    u16 cbSize;             // Extension size
    u16 samplesPerBlock;
    u32 channelMask;
    GUID subFormat;
    u32 versionInfo;
    u8 configData[SCE_AT9_CONFIG_DATA_SIZE]; // the juicy part
    u32 reserved;
    /* "fact" sub-chunk */
    char Subchunk2ID[4];    // "fact"  string
    uint32_t Subchunk2Size; // Sampled data length
};
#pragma pack(pop)

static_assert(sizeof(WavHeader) == 80);

struct AjmAt9Decoder final : AjmInstance {
    void* handle;
    bool decoder_initialized = false;
    std::fstream file;
    int length;

    explicit AjmAt9Decoder();
    ~AjmAt9Decoder() override;

    void Reset() override;

    std::tuple<u32, u32, u32> Decode(const u8* in_buf, u32 in_size, u8* out_buf,
                                     u32 out_size) override;
};

} // namespace Libraries::Ajm
