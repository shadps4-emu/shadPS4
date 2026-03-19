// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <optional>
#include <span>
#include <string>

#include "common/types.h"
#include "shader_recompiler/runtime_info.h"

namespace VideoCore::Render {

class IShaderDebugProvider {
public:
    virtual ~IShaderDebugProvider() = default;

    virtual std::string GetShaderName(Shader::Stage stage, u64 hash,
                                      std::optional<size_t> perm = {}) const = 0;
    virtual std::optional<u64> ReplaceShader(u64 module_handle,
                                             std::span<const u32> backend_binary) = 0;
};

} // namespace VideoCore::Render
