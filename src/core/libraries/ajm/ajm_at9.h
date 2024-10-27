// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstring>
#include <fstream>
#include "common/assert.h"
#include "common/types.h"
#include "core/libraries/ajm/ajm_instance.h"

extern "C" {
#include <structures.h>
}

namespace Libraries::Ajm {

constexpr u32 SCE_AT9_CONFIG_DATA_SIZE = 4;
constexpr s32 SCE_AJM_DEC_AT9_MAX_CHANNELS = 8;

struct AjmDecAt9InitializeParameters {
    u8 config_data[SCE_AT9_CONFIG_DATA_SIZE];
    u32 reserved;
};

struct AjmSidebandDecAt9CodecInfo {
    u32 uiSuperFrameSize;
    u32 uiFramesInSuperFrame;
    u32 uiNextFrameSize;
    u32 uiFrameSamples;
};

struct AjmAt9Decoder final : AjmInstance {
    void* handle;
    bool decoder_initialized = false;
    std::fstream file;
    int length;
    u8 config_data[SCE_AT9_CONFIG_DATA_SIZE];

    explicit AjmAt9Decoder();
    ~AjmAt9Decoder() override;

    void Reset() override;

    void Initialize(const void* buffer, u32 buffer_size) override;

    void GetCodecInfo(void* out_info) override;
    u32 GetCodecInfoSize() override {
        return sizeof(AjmSidebandDecAt9CodecInfo);
    }

    std::tuple<u32, u32, u32> Decode(const u8* in_buf, u32 in_size, u8* out_buf,
                                     u32 out_size) override;
};

} // namespace Libraries::Ajm
