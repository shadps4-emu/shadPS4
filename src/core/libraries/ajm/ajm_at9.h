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

constexpr s32 ORBIS_AJM_DEC_AT9_MAX_CHANNELS = 8;

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
    u8 config_data[ORBIS_AT9_CONFIG_DATA_SIZE];
    u32 superframe_bytes_remain{};
    u32 num_frames{};

    explicit AjmAt9Decoder();
    ~AjmAt9Decoder() override;

    void Reset() override;

    void Initialize(const void* buffer, u32 buffer_size) override;

    void GetCodecInfo(void* out_info) override;
    u32 GetCodecInfoSize() override {
        return sizeof(AjmSidebandDecAt9CodecInfo);
    }

    void Decode(const AjmJobInput* input, AjmJobOutput* output) override;

private:
    void ResetCodec();
};

} // namespace Libraries::Ajm
