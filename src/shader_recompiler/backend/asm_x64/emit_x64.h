// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <vector>
#include <xbyak/xbyak.h>
#include <xbyak/xbyak_util.h>
#include "shader_recompiler/ir/program.h"

namespace Shader::Backend::X64 {

void EmitX64(const IR::Program& program, Xbyak::CodeGenerator& c);

} // namespace Shader::Backend::X64