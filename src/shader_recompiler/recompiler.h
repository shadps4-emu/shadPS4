// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "shader_recompiler/pools.h"
#include "shader_recompiler/ir/basic_block.h"
#include "shader_recompiler/ir/program.h"

namespace Shader {

struct Profile;
struct RuntimeInfo;

[[nodiscard]] IR::Program TranslateProgram(std::span<const u32> code, Pools& pools, Info& info,
                                           RuntimeInfo& runtime_info, const Profile& profile);

} // namespace Shader
