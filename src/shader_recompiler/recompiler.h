// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/object_pool.h"
#include "shader_recompiler/ir/basic_block.h"
#include "shader_recompiler/ir/program.h"

namespace Shader {

struct Profile;

[[nodiscard]] IR::Program TranslateProgram(Common::ObjectPool<IR::Inst>& inst_pool,
                                           Common::ObjectPool<IR::Block>& block_pool,
                                           std::span<const u32> code, const Info&& info,
                                           const Profile& profile);

} // namespace Shader
