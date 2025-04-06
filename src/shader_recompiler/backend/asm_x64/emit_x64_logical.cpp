// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/backend/asm_x64/x64_emit_context.h"
#include "shader_recompiler/backend/asm_x64/x64_utils.h"

namespace Shader::Backend::X64 {

using namespace Xbyak;
using namespace Xbyak::util;

void EmitLogicalOr(EmitContext& ctx, const Operands& dest, const Operands& op1, const Operands& op2) {
    Reg tmp = dest[0].isMEM() ? ctx.TempGPReg().cvt8() : dest[0].getReg().cvt8();
    MovGP(ctx, tmp, op1[0]);
    ctx.Code().or_(tmp, op2[0]);
    MovGP(ctx, dest[0], tmp);
}

void EmitLogicalAnd(EmitContext& ctx, const Operands& dest, const Operands& op1, const Operands& op2) {
    Reg tmp = dest[0].isMEM() ? ctx.TempGPReg().cvt8() : dest[0].getReg().cvt8();
    MovGP(ctx, tmp, op1[0]);
    ctx.Code().and_(tmp, op2[0]);
    MovGP(ctx, dest[0], tmp);
}

void EmitLogicalXor(EmitContext& ctx, const Operands& dest, const Operands& op1, const Operands& op2) {
    Reg tmp = dest[0].isMEM() ? ctx.TempGPReg().cvt8() : dest[0].getReg().cvt8();
    MovGP(ctx, tmp, op1[0]);
    ctx.Code().xor_(tmp, op2[0]);
    MovGP(ctx, dest[0], tmp);
}

void EmitLogicalNot(EmitContext& ctx, const Operands& dest, const Operands& op) {
    Reg tmp = dest[0].isMEM() ? ctx.TempGPReg().cvt8() : dest[0].getReg().cvt8();
    MovGP(ctx, tmp, op[0]);
    ctx.Code().not_(tmp);
    MovGP(ctx, dest[0], tmp);
}

} // namespace Shader::Backend::X64