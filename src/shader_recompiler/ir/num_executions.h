// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "shader_recompiler/ir/type.h"

// Get the number of times an instruction will be executed.
// 0 if it cannot be determined statically.

namespace Shader::IR {

u64 GetNumExecutions(const Inst* inst);

} // namespace Shader::IR
