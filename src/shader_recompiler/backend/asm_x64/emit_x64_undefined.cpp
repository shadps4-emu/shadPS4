// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/backend/asm_x64/x64_emit_context.h"

namespace Shader::Backend::X64 {

void EmitUndefU1(EmitContext& ctx) {
    UNREACHABLE_MSG("x64 Instruction");
}

void EmitUndefU8(EmitContext&) {
    UNREACHABLE_MSG("x64 Instruction");
}

void EmitUndefU16(EmitContext&) {
    UNREACHABLE_MSG("x64 Instruction");
}

void EmitUndefU32(EmitContext& ctx) {
    UNREACHABLE_MSG("x64 Instruction");
}

void EmitUndefU64(EmitContext&) {
    UNREACHABLE_MSG("x64 Instruction");
}

} // namespace Shader::Backend::X64