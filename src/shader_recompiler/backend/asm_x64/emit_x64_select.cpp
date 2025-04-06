// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/backend/asm_x64/x64_emit_context.h"
#include "shader_recompiler/backend/asm_x64/x64_utils.h"

namespace Shader::Backend::X64 {

using namespace Xbyak;
using namespace Xbyak::util;

void EmitSelectU1(EmitContext& ctx, const Operands& dest, const Operands& cond, const Operands& true_value, const Operands& false_value) {
    Label false_label, end_label;
    Reg tmp = cond[0].isMEM() ? ctx.TempGPReg().cvt8() : cond[0].getReg().cvt8();
    MovGP(ctx, tmp, cond[0]);
    ctx.Code().test(tmp, tmp);
    ctx.Code().jz(false_label);
    MovGP(ctx, dest[0], true_value[0]);
    ctx.Code().jmp(end_label);
    ctx.Code().L(false_label);
    MovGP(ctx, dest[0], false_value[0]);
    ctx.Code().L(end_label);
}

void EmitSelectU8(EmitContext& ctx, const Operands& dest, const Operands& cond, const Operands& true_value, const Operands& false_value) {
    EmitSelectU1(ctx, dest, cond, true_value, false_value);
}

void EmitSelectU16(EmitContext& ctx, const Operands& dest, const Operands& cond, const Operands& true_value, const Operands& false_value) {
    EmitSelectU1(ctx, dest, cond, true_value, false_value);
}

void EmitSelectU32(EmitContext& ctx, const Operands& dest, const Operands& cond, const Operands& true_value, const Operands& false_value) {
    EmitSelectU1(ctx, dest, cond, true_value, false_value);
}

void EmitSelectU64(EmitContext& ctx, const Operands& dest, const Operands& cond, const Operands& true_value, const Operands& false_value) {
    EmitSelectU1(ctx, dest, cond, true_value, false_value);
}

void EmitSelectF16(EmitContext& ctx, const Operands& dest, const Operands& cond, const Operands& true_value, const Operands& false_value) {
    EmitSelectU1(ctx, dest, cond, true_value, false_value);
}

void EmitSelectF32(EmitContext& ctx, const Operands& dest, const Operands& cond, const Operands& true_value, const Operands& false_value) {
    Label false_label, end_label;
    Reg tmp = cond[0].isMEM() ? ctx.TempGPReg().cvt8() : cond[0].getReg().cvt8();
    MovGP(ctx, tmp, cond[0]);
    ctx.Code().test(tmp, tmp);
    ctx.Code().jz(false_label);
    MovFloat(ctx, dest[0], true_value[0]);
    ctx.Code().jmp(end_label);
    ctx.Code().L(false_label);
    MovFloat(ctx, dest[0], false_value[0]);
    ctx.Code().L(end_label);
}

void EmitSelectF64(EmitContext& ctx, const Operands& dest, const Operands& cond, const Operands& true_value, const Operands& false_value) {
    Label false_label, end_label;
    Reg tmp = cond[0].isMEM() ? ctx.TempGPReg().cvt8() : cond[0].getReg().cvt8();
    MovGP(ctx, tmp, cond[0]);
    ctx.Code().test(tmp, tmp);
    ctx.Code().jz(false_label);
    MovDouble(ctx, dest[0], true_value[0]);
    ctx.Code().jmp(end_label);
    ctx.Code().L(false_label);
    MovDouble(ctx, dest[0], false_value[0]);
    ctx.Code().L(end_label);
}

} // namespace Shader::Backend::X64