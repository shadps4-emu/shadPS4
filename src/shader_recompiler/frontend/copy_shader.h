// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <map>
#include <span>

#include "common/types.h"
#include "shader_recompiler/ir/attribute.h"

namespace Shader {

struct CopyShaderData {
    std::map<u32, std::pair<Shader::IR::Attribute, u32>> attr_map;
    u32 num_attrs{0};
    u32 output_vertices{0};
};

CopyShaderData ParseCopyShader(std::span<const u32> code);

} // namespace Shader
