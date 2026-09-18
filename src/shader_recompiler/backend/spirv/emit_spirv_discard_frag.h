// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <vector>
#include "common/types.h"
#include "shader_recompiler/runtime_info.h"

namespace Shader::Backend::SPIRV {

[[nodiscard]] std::vector<u32> EmitDiscardFragmentShader(std::array<Shader::OutputMap, 3> outputs);

} // namespace Shader::Backend::SPIRV
