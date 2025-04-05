// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/backend/asm_x64/x64_emit_context.h"
#include "shader_recompiler/backend/asm_x64/x64_utils.h"

namespace Shader::Backend::X64 {

using namespace Xbyak;
using namespace Xbyak::util;

void EmitConvertS16F16(EmitContext& ctx, const Operands& dest, const Operands& src) {
    Xmm tmp_xmm = ctx.TempXmmReg(false);
    Reg tmp_reg = dest[0].isMEM() ? ctx.TempGPReg().cvt32() : dest[0].getReg().cvt32();
    EmitInlineF16ToF32(ctx, tmp_xmm, src[0]);
    ctx.Code().cvttss2si(tmp_reg, tmp_xmm);
    ctx.Code().and_(tmp_reg, 0xFFFF);
    MovGP(ctx, dest[0], tmp_reg);
}

void EmitConvertS16F32(EmitContext& ctx, const Operands& dest, const Operands& src) {
    Reg tmp = dest[0].isMEM() ? ctx.TempGPReg(false).cvt32() : dest[0].getReg().cvt32();
    ctx.Code().cvttss2si(tmp, src[0]);
    ctx.Code().and_(tmp, 0xFFFF);
    MovGP(ctx, dest[0], tmp);
}

void EmitConvertS16F64(EmitContext& ctx, const Operands& dest, const Operands& src) {
    Reg tmp = dest[0].isMEM() ? ctx.TempGPReg(false).cvt32() : dest[0].getReg().cvt32();
    ctx.Code().cvttsd2si(tmp, src[0]);
    ctx.Code().and_(tmp, 0xFFFF);
    MovGP(ctx, dest[0], tmp);
}

void EmitConvertS32F16(EmitContext& ctx, const Operands& dest, const Operands& src) {
    Xmm tmp_xmm = ctx.TempXmmReg(false);
    Reg tmp_reg = dest[0].isMEM() ? ctx.TempGPReg().cvt32() : dest[0].getReg().cvt32();
    EmitInlineF16ToF32(ctx, tmp_xmm, src[0]);
    ctx.Code().cvttss2si(tmp_reg, tmp_xmm);
    MovGP(ctx, dest[0], tmp_reg);
}

void EmitConvertS32F32(EmitContext& ctx, const Operands& dest, const Operands& src) {
    Reg tmp = dest[0].isMEM() ? ctx.TempGPReg(false).cvt32() : dest[0].getReg().cvt32();
    ctx.Code().cvttss2si(tmp, src[0]);
    MovGP(ctx, dest[0], tmp);
}

void EmitConvertS32F64(EmitContext& ctx, const Operands& dest, const Operands& src) {
    Reg tmp = dest[0].isMEM() ? ctx.TempGPReg(false).cvt32() : dest[0].getReg().cvt32();
    ctx.Code().cvttsd2si(tmp, src[0]);
    MovGP(ctx, dest[0], tmp);
}

void EmitConvertS64F16(EmitContext& ctx, const Operands& dest, const Operands& src) {
    Xmm tmp_xmm = ctx.TempXmmReg(false);
    Reg tmp_reg = dest[0].isMEM() ? ctx.TempGPReg() : dest[0].getReg();
    EmitInlineF16ToF32(ctx, tmp_xmm, src[0]);
    ctx.Code().cvttss2si(tmp_reg, tmp_xmm);
    MovGP(ctx, dest[0], tmp_reg);
}

void EmitConvertS64F32(EmitContext& ctx, const Operands& dest, const Operands& src) {
    Reg tmp = dest[0].isMEM() ? ctx.TempGPReg(false) : dest[0].getReg();
    ctx.Code().cvttss2si(tmp, src[0]);
    MovGP(ctx, dest[0], tmp);
}

void EmitConvertS64F64(EmitContext& ctx, const Operands& dest, const Operands& src) {
    Reg tmp = dest[0].isMEM() ? ctx.TempGPReg(false) : dest[0].getReg();
    ctx.Code().cvttsd2si(tmp, src[0]);
    MovGP(ctx, dest[0], tmp);
}

void EmitConvertU16F16(EmitContext& ctx, const Operands& dest, const Operands& src) {
    EmitConvertS16F16(ctx, dest, src);
}

void EmitConvertU16F32(EmitContext& ctx, const Operands& dest, const Operands& src) {
    EmitConvertS16F32(ctx, dest, src);
}

void EmitConvertU16F64(EmitContext& ctx, const Operands& dest, const Operands& src) {
    EmitConvertS16F64(ctx, dest, src);
}

void EmitConvertU32F16(EmitContext& ctx, const Operands& dest, const Operands& src) {
    EmitConvertS32F16(ctx, dest, src);
}

void EmitConvertU32F32(EmitContext& ctx, const Operands& dest, const Operands& src) {
    EmitConvertS32F32(ctx, dest, src);
}

void EmitConvertU32F64(EmitContext& ctx, const Operands& dest, const Operands& src) {
    EmitConvertS32F64(ctx, dest, src);
}

void EmitConvertU64F16(EmitContext& ctx, const Operands& dest, const Operands& src) {
    EmitConvertS64F16(ctx, dest, src);
}

void EmitConvertU64F32(EmitContext& ctx, const Operands& dest, const Operands& src) {
    EmitConvertS64F32(ctx, dest, src);
}

void EmitConvertU64F64(EmitContext& ctx, const Operands& dest, const Operands& src) {
    EmitConvertS64F64(ctx, dest, src);
}

void EmitConvertU64U32(EmitContext& ctx, const Operands& dest, const Operands& src) {
    MovGP(ctx, dest[0], src[0]);
}

void EmitConvertU32U64(EmitContext& ctx, const Operands& dest, const Operands& src) {
    MovGP(ctx, dest[0], src[0]);
}

void EmitConvertF16F32(EmitContext& ctx, const Operands& dest, const Operands& src) {
    EmitInlineF32ToF16(ctx, dest[0], src[0]);
}

void EmitConvertF32F16(EmitContext& ctx, const Operands& dest, const Operands& src) {
    EmitInlineF16ToF32(ctx, dest[0], src[0]);
}

void EmitConvertF32F64(EmitContext& ctx, const Operands& dest, const Operands& src) {
    Xmm tmp = dest[0].isMEM() ? ctx.TempXmmReg(false) : dest[0].getReg().cvt128();
    ctx.Code().cvtsd2ss(tmp, src[0]);
    MovFloat(ctx, dest[0], tmp);
}

void EmitConvertF64F32(EmitContext& ctx, const Operands& dest, const Operands& src) {
    Xmm tmp = dest[0].isMEM() ? ctx.TempXmmReg(false) : dest[0].getReg().cvt128();
    ctx.Code().cvtss2sd(tmp, src[0]);
    MovDouble(ctx, dest[0], tmp);
}

void EmitConvertF16S8(EmitContext& ctx, const Operands& dest, const Operands& src) {
    Reg tmp_reg = dest[0].isMEM() ? ctx.TempGPReg().cvt32() : dest[0].getReg().cvt32();
    Xmm tmp_xmm = ctx.TempXmmReg(false);
    ctx.Code().movsx(tmp_reg, src[0]);
    ctx.Code().cvtsi2ss(tmp_xmm, tmp_reg);
    EmitInlineF32ToF16(ctx, dest[0], tmp_xmm);
}

void EmitConvertF16S16(EmitContext& ctx, const Operands& dest, const Operands& src) {
    Reg tmp_reg = dest[0].isMEM() ? ctx.TempGPReg().cvt32() : dest[0].getReg().cvt32();
    Xmm tmp_xmm = ctx.TempXmmReg(false);
    ctx.Code().movsx(tmp_reg, src[0]);
    ctx.Code().cvtsi2ss(tmp_xmm, tmp_reg);
    EmitInlineF32ToF16(ctx, dest[0], tmp_xmm);
}

void EmitConvertF16S32(EmitContext& ctx, const Operands& dest, const Operands& src) {
    Xmm tmp = ctx.TempXmmReg(false);
    ctx.Code().cvtsi2ss(tmp, src[0]);
    EmitInlineF32ToF16(ctx, dest[0], tmp);
}

void EmitConvertF16S64(EmitContext& ctx, const Operands& dest, const Operands& src) {
    Xmm tmp = ctx.TempXmmReg(false);
    ctx.Code().cvtsi2ss(tmp, src[0]);
    EmitInlineF32ToF16(ctx, dest[0], tmp);
}

void EmitConvertF16U8(EmitContext& ctx, const Operands& dest, const Operands& src) {
    EmitConvertF16S8(ctx, dest, src);
}

void EmitConvertF16U16(EmitContext& ctx, const Operands& dest, const Operands& src) {
    EmitConvertF16S16(ctx, dest, src);
}

void EmitConvertF16U32(EmitContext& ctx, const Operands& dest, const Operands& src) {
    EmitConvertF16S32(ctx, dest, src);
}

void EmitConvertF16U64(EmitContext& ctx, const Operands& dest, const Operands& src) {
    EmitConvertF16S64(ctx, dest, src);
}

void EmitConvertF32S8(EmitContext& ctx, const Operands& dest, const Operands& src) {
    Reg tmp_reg = ctx.TempGPReg(false).cvt32();
    Xmm tmp_xmm = dest[0].isMEM() ? ctx.TempXmmReg(false) : dest[0].getReg().cvt128();
    ctx.Code().movsx(tmp_reg, src[0]);
    ctx.Code().cvtsi2ss(tmp_xmm, tmp_reg);
    MovFloat(ctx, dest[0], tmp_xmm);
}

void EmitConvertF32S16(EmitContext& ctx, const Operands& dest, const Operands& src) {
    Reg tmp_reg = ctx.TempGPReg(false).cvt32();
    Xmm tmp_xmm = dest[0].isMEM() ? ctx.TempXmmReg(false) : dest[0].getReg().cvt128();
    ctx.Code().movsx(tmp_reg, src[0]);
    ctx.Code().cvtsi2ss(tmp_xmm, tmp_reg);
    MovFloat(ctx, dest[0], tmp_xmm);
}

void EmitConvertF32S32(EmitContext& ctx, const Operands& dest, const Operands& src) {
    Xmm tmp = dest[0].isMEM() ? ctx.TempXmmReg(false) : dest[0].getReg().cvt128();
    ctx.Code().cvtsi2ss(tmp, src[0]);
    MovFloat(ctx, dest[0], tmp);
}

void EmitConvertF32S64(EmitContext& ctx, const Operands& dest, const Operands& src) {
    Xmm tmp = dest[0].isMEM() ? ctx.TempXmmReg(false) : dest[0].getReg().cvt128();
    ctx.Code().cvtsi2ss(tmp, src[0]);
    MovFloat(ctx, dest[0], tmp);
}

void EmitConvertF32U8(EmitContext& ctx, const Operands& dest, const Operands& src) {
    EmitConvertF32S8(ctx, dest, src);
}

void EmitConvertF32U16(EmitContext& ctx, const Operands& dest, const Operands& src) {
    EmitConvertF32S16(ctx, dest, src);
}

void EmitConvertF32U32(EmitContext& ctx, const Operands& dest, const Operands& src) {
    EmitConvertF32S32(ctx, dest, src);
}

void EmitConvertF32U64(EmitContext& ctx, const Operands& dest, const Operands& src) {
    EmitConvertF32S64(ctx, dest, src);
}

void EmitConvertF64S8(EmitContext& ctx, const Operands& dest, const Operands& src) {
    Reg tmp_reg = ctx.TempGPReg(false).cvt32();
    Xmm tmp_xmm = dest[0].isMEM() ? ctx.TempXmmReg(false) : dest[0].getReg().cvt128();
    ctx.Code().movsx(tmp_reg, src[0]);
    ctx.Code().cvtsi2sd(tmp_xmm, tmp_reg);
    MovDouble(ctx, dest[0], tmp_xmm);
}

void EmitConvertF64S16(EmitContext& ctx, const Operands& dest, const Operands& src) {
    Reg tmp_reg = ctx.TempGPReg(false).cvt32();
    Xmm tmp_xmm = dest[0].isMEM() ? ctx.TempXmmReg(false) : dest[0].getReg().cvt128();
    ctx.Code().movsx(tmp_reg, src[0]);
    ctx.Code().cvtsi2sd(tmp_xmm, tmp_reg);
    MovDouble(ctx, dest[0], tmp_xmm);
}

void EmitConvertF64S32(EmitContext& ctx, const Operands& dest, const Operands& src) {
    Xmm tmp = dest[0].isMEM() ? ctx.TempXmmReg(false) : dest[0].getReg().cvt128();
    ctx.Code().cvtsi2sd(tmp, src[0]);
    MovDouble(ctx, dest[0], tmp);
}

void EmitConvertF64S64(EmitContext& ctx, const Operands& dest, const Operands& src) {
    Xmm tmp = dest[0].isMEM() ? ctx.TempXmmReg(false) : dest[0].getReg().cvt128();
    ctx.Code().cvtsi2sd(tmp, src[0]);
    MovDouble(ctx, dest[0], tmp);
}

void EmitConvertF64U8(EmitContext& ctx, const Operands& dest, const Operands& src) {
    EmitConvertF64S8(ctx, dest, src);
}

void EmitConvertF64U16(EmitContext& ctx, const Operands& dest, const Operands& src) {
    EmitConvertF64S16(ctx, dest, src);
}

void EmitConvertF64U32(EmitContext& ctx, const Operands& dest, const Operands& src) {
    EmitConvertF64S32(ctx, dest, src);
}

void EmitConvertF64U64(EmitContext& ctx, const Operands& dest, const Operands& src) {
    EmitConvertF64S64(ctx, dest, src);
}

void EmitConvertU16U32(EmitContext& ctx, const Operands& dest, const Operands& src) {
    MovGP(ctx, dest[0], src[0]);
}

void EmitConvertU32U16(EmitContext& ctx, const Operands& dest, const Operands& src) {
    MovGP(ctx, dest[0], src[0]);
}

}

