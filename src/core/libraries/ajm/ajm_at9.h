// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <fstream>
#include "common/types.h"
#include "core/libraries/ajm/ajm_instance.h"

extern "C" {
#include <structures.h>
}

namespace Libraries::Ajm {

constexpr unsigned int SCE_AT9_CONFIG_DATA_SIZE = 4;

struct SceAjmDecAt9InitializeParameters {
    u8 config_data[SCE_AT9_CONFIG_DATA_SIZE];
    u32 reserved;
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

    std::tuple<u32, u32, u32> Decode(const u8* in_buf, u32 in_size, u8* out_buf,
                                     u32 out_size) override;
};

} // namespace Libraries::Ajm
