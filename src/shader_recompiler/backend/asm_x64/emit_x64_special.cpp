// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/backend/asm_x64/x64_emit_context.h"
#include "shader_recompiler/backend/asm_x64/x64_utils.h"

namespace Shader::Backend::X64 {

using namespace Xbyak;
using namespace Xbyak::util;

void EmitPrologue(EmitContext& ctx) {
    ctx.Prologue();
}

void ConvertDepthMode(EmitContext& ctx) {
    
}

void ConvertPositionToClipSpace(EmitContext& ctx) {
    
}

void EmitEpilogue(EmitContext& ctx) {
    ctx.SetEndFlag();
}

void EmitDiscard(EmitContext& ctx) {
    ctx.SetEndFlag();
}

void EmitDiscardCond(EmitContext& ctx, const Operands& condition) {
    Reg tmp = condition[0].isMEM() ? ctx.TempGPReg().cvt8() : condition[0].getReg().cvt8();
    MovGP(ctx, tmp, condition[0]);
    ctx.Code().test(tmp, tmp);
    ctx.Code().jnz(ctx.EndLabel());
}

void EmitEmitVertex(EmitContext& ctx) {
    
}

void EmitEmitPrimitive(EmitContext& ctx) {
    
}

void EmitEndPrimitive(EmitContext& ctx) {
    
}

void EmitDebugPrint(EmitContext& ctx) {
    
}

} // namespace Shader::Backend::X64