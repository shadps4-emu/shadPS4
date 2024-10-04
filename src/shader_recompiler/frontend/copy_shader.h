// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <span>
#include <unordered_map>

#include "common/types.h"
#include "shader_recompiler/ir/attribute.h"

namespace Shader {

struct CopyShaderData {
    std::unordered_map<u32, Shader::IR::Attribute> attr_map;
    u32 num_attrs{0};
};

CopyShaderData ParseCopyShader(const std::span<const u32>& code);

} // namespace Shader
