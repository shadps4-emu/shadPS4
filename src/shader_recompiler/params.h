// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <span>
#include "common/types.h"

namespace Shader {

/**
 * Compilation parameters used to identify and locate a guest shader program.
 */
struct ShaderParams {
    static constexpr u32 NumShaderUserData = 16;

    std::span<const u32, NumShaderUserData> user_data;
    std::span<const u32> code;
    u64 hash;

    VAddr Base() const noexcept {
        return reinterpret_cast<VAddr>(code.data());
    }
};

} // namespace Shader
