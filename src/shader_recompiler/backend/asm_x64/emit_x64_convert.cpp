// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/exception.h"
#include "shader_recompiler/backend/asm_x64/x64_emit_context.h"
#include "shader_recompiler/backend/asm_x64/x64_utils.h"

namespace Shader::Backend::X64 {

using namespace Xbyak;
using namespace Xbyak::util;

namespace {

static void EmitInlineF16ToF32(EmitContext& ctx, const Operand& dest, const Operand& src) {
    CodeGenerator& c = ctx.Code();
    Label nonzero_exp, zero_mantissa, norm_loop, norm_done, normal, done;
    Reg sign = ctx.TempGPReg().cvt32();
    Reg exponent = ctx.TempGPReg().cvt32();
    Reg mantissa = ctx.TempGPReg().cvt32();

    c.movzx(mantissa, src);
    
    // Extract sign, exponent, and mantissa
    c.mov(sign, mantissa);
    c.and_(sign, 0x8000);
    c.shl(sign, 16);
    c.mov(exponent, mantissa);
    c.and_(exponent, 0x7C00);
    c.shr(exponent, 10);
    c.and_(mantissa, 0x03FF);

    // Check for zero exponent and mantissa
    c.test(exponent, exponent);
    c.jnz(nonzero_exp);
    c.test(mantissa, mantissa);
    c.jz(zero_mantissa);

    // Nromalize subnormal number
    c.mov(exponent, 1);
    c.L(norm_loop);
    c.test(mantissa, 0x400);
    c.jnz(norm_done);
    c.shl(mantissa, 1);
    c.dec(exponent);
    c.jmp(norm_loop);
    c.L(norm_done);
    c.and_(mantissa, 0x03FF);
    c.jmp(normal);

    // Zero mantissa
    c.L(zero_mantissa);
    c.and_(mantissa, sign);
    c.jmp(done);

    // Non-zero exponent
    c.L(nonzero_exp);
    c.cmp(exponent, 0x1F);
    c.jne(normal);

    // Infinite or NaN
    c.shl(mantissa, 13);
    c.or_(mantissa, sign);
    c.or_(mantissa, 0x7F800000);
    c.jmp(done);

    // Normal number
    c.L(normal);
    c.add(exponent, 112);
    c.shl(exponent, 23);
    c.shl(mantissa, 13);
    c.or_(mantissa, sign);
    c.or_(mantissa, exponent);

    c.L(done);
    if (dest.isMEM()) {
        c.mov(dest, mantissa);
    } else {
        c.movd(dest.getReg().cvt128(), mantissa);
    }
}

static void EmitInlineF32ToF16(EmitContext& ctx, const Operand& dest, const Operand& src) {
    CodeGenerator& c = ctx.Code();
    Label zero_exp, underflow, overflow, done;
    Reg sign = ctx.TempGPReg().cvt32();
    Reg exponent = ctx.TempGPReg().cvt32();
    Reg mantissa = dest.isMEM() ? ctx.TempGPReg().cvt32() : dest.getReg().cvt32();

    if (src.isMEM()) {
        c.mov(mantissa, src);
    } else {
        c.movd(mantissa, src.getReg().cvt128());
    }

    // Extract sign, exponent, and mantissa
    c.mov(exponent, mantissa);
    c.mov(sign, mantissa);
    c.and_(exponent, 0x7F800000);
    c.and_(mantissa, 0x007FFFFF);
    c.shr(exponent, 23);
    c.shl(mantissa, 3);
    c.shr(sign, 16);
    c.and_(sign, 0x8000);

    // Subnormal numbers will be zero
    c.test(exponent, exponent);
    c.jz(zero_exp);

    // Check for overflow and underflow
    c.sub(exponent, 112);
    c.cmp(exponent, 0);
    c.jle(underflow);
    c.cmp(exponent, 0x1F);
    c.jge(overflow);

    // Normal number
    c.shl(exponent, 10);
    c.shr(mantissa, 13);
    c.or_(mantissa, exponent);
    c.or_(mantissa, sign);
    c.jmp(done);

    // Undeflow
    c.L(underflow);
    c.xor_(mantissa, mantissa);
    c.jmp(done);

    // Overflow
    c.L(overflow);
    c.mov(mantissa, 0x7C00);
    c.or_(mantissa, sign);
    c.jmp(done);

    // Zero value
    c.L(zero_exp);
    c.and_(mantissa, sign);

    c.L(done);
    if (dest.isMEM()) {
        c.mov(dest, mantissa);
    } else {
        c.and_(mantissa, 0xFFFF);
    }
}

}

void EmitConvertS16F16(EmitContext& ctx, const Operands& dest, const Operands& src) {
    Xmm tmp_xmm = ctx.TempXmmReg(false);
    Reg tmp_reg = dest[0].isMEM() ? ctx.TempGPReg().cvt32() : dest[0].getReg().cvt32();
    EmitInlineF16ToF32(ctx, tmp_xmm, src[0]);
    ctx.Code().cvttss2si(tmp_reg, tmp_xmm);
    ctx.Code().and_(tmp_reg, 0xFFFF);
    if (dest[0].isMEM()) {
        ctx.Code().mov(dest[0], tmp_reg.cvt16());
    }
}

void EmitConvertS16F32(EmitContext& ctx, const Operands& dest, const Operands& src) {
    Reg tmp = dest[0].isMEM() ? ctx.TempGPReg(false).cvt32() : dest[0].getReg().cvt32();
    ctx.Code().cvttss2si(tmp, src[0]);
    ctx.Code().and_(tmp, 0xFFFF);
    if (dest[0].isMEM()) {
        ctx.Code().mov(dest[0], tmp.cvt16());
    }
}

void EmitConvertS16F64(EmitContext& ctx, const Operands& dest, const Operands& src) {
    Reg tmp = dest[0].isMEM() ? ctx.TempGPReg(false).cvt32() : dest[0].getReg().cvt32();
    ctx.Code().cvttsd2si(tmp, src[0]);
    ctx.Code().and_(tmp, 0xFFFF);
    if (dest[0].isMEM()) {
        ctx.Code().mov(dest[0], tmp.cvt16());
    }
}

void EmitConvertS32F16(EmitContext& ctx, const Operands& dest, const Operands& src) {
    Xmm tmp_xmm = ctx.TempXmmReg(false);
    Reg tmp_reg = dest[0].isMEM() ? ctx.TempGPReg().cvt32() : dest[0].getReg().cvt32();
    EmitInlineF16ToF32(ctx, tmp_xmm, src[0]);
    ctx.Code().cvttss2si(tmp_reg, tmp_xmm);
    if (dest[0].isMEM()) {
        ctx.Code().mov(dest[0], tmp_reg);
    }
}

void EmitConvertS32F32(EmitContext& ctx, const Operands& dest, const Operands& src) {
    Reg tmp = dest[0].isMEM() ? ctx.TempGPReg(false).cvt32() : dest[0].getReg().cvt32();
    ctx.Code().cvttss2si(tmp, src[0]);
    if (dest[0].isMEM()) {
        ctx.Code().mov(dest[0], tmp);
    }
}

void EmitConvertS32F64(EmitContext& ctx, const Operands& dest, const Operands& src) {
    Reg tmp = dest[0].isMEM() ? ctx.TempGPReg(false).cvt32() : dest[0].getReg().cvt32();
    ctx.Code().cvttsd2si(tmp, src[0]);
    if (dest[0].isMEM()) {
        ctx.Code().mov(dest[0], tmp);
    }
}

void EmitConvertS64F16(EmitContext& ctx, const Operands& dest, const Operands& src) {
    Xmm tmp_xmm = ctx.TempXmmReg(false);
    Reg tmp_reg = dest[0].isMEM() ? ctx.TempGPReg() : dest[0].getReg();
    EmitInlineF16ToF32(ctx, tmp_xmm, src[0]);
    ctx.Code().cvttss2si(tmp_reg, tmp_xmm);
    if (dest[0].isMEM()) {
        ctx.Code().mov(dest[0], tmp_reg);
    }
}

void EmitConvertS64F32(EmitContext& ctx, const Operands& dest, const Operands& src) {
    Reg tmp = dest[0].isMEM() ? ctx.TempGPReg(false) : dest[0].getReg();
    ctx.Code().cvttss2si(tmp, src[0]);
    if (dest[0].isMEM()) {
        ctx.Code().mov(dest[0], tmp);
    }
}

void EmitConvertS64F64(EmitContext& ctx, const Operands& dest, const Operands& src) {
    Reg tmp = dest[0].isMEM() ? ctx.TempGPReg(false) : dest[0].getReg();
    ctx.Code().cvttsd2si(tmp, src[0]);
    if (dest[0].isMEM()) {
        ctx.Code().mov(dest[0], tmp);
    }
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
    if (dest[0].isMEM()) {
        ctx.Code().mov(dest[0], tmp);
    }
}

void EmitConvertF64F32(EmitContext& ctx, const Operands& dest, const Operands& src) {
    Xmm tmp = dest[0].isMEM() ? ctx.TempXmmReg(false) : dest[0].getReg().cvt128();
    ctx.Code().cvtss2sd(tmp, src[0]);
    if (dest[0].isMEM()) {
        ctx.Code().mov(dest[0], tmp);
    }
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
    if (dest[0].isMEM()) {
        ctx.Code().mov(dest[0], tmp_xmm);
    }
}

void EmitConvertF32S16(EmitContext& ctx, const Operands& dest, const Operands& src) {
    Reg tmp_reg = ctx.TempGPReg(false).cvt32();
    Xmm tmp_xmm = dest[0].isMEM() ? ctx.TempXmmReg(false) : dest[0].getReg().cvt128();
    ctx.Code().movsx(tmp_reg, src[0]);
    ctx.Code().cvtsi2ss(tmp_xmm, tmp_reg);
    if (dest[0].isMEM()) {
        ctx.Code().mov(dest[0], tmp_xmm);
    }
}

void EmitConvertF32S32(EmitContext& ctx, const Operands& dest, const Operands& src) {
    Xmm tmp = dest[0].isMEM() ? ctx.TempXmmReg(false) : dest[0].getReg().cvt128();
    ctx.Code().cvtsi2ss(tmp, src[0]);
    if (dest[0].isMEM()) {
        ctx.Code().mov(dest[0], tmp);
    }
}

void EmitConvertF32S64(EmitContext& ctx, const Operands& dest, const Operands& src) {
    Xmm tmp = dest[0].isMEM() ? ctx.TempXmmReg(false) : dest[0].getReg().cvt128();
    ctx.Code().cvtsi2ss(tmp, src[0]);
    if (dest[0].isMEM()) {
        ctx.Code().mov(dest[0], tmp);
    }
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
    if (dest[0].isMEM()) {
        ctx.Code().mov(dest[0], tmp_xmm);
    }
}

void EmitConvertF64S16(EmitContext& ctx, const Operands& dest, const Operands& src) {
    Reg tmp_reg = ctx.TempGPReg(false).cvt32();
    Xmm tmp_xmm = dest[0].isMEM() ? ctx.TempXmmReg(false) : dest[0].getReg().cvt128();
    ctx.Code().movsx(tmp_reg, src[0]);
    ctx.Code().cvtsi2sd(tmp_xmm, tmp_reg);
    if (dest[0].isMEM()) {
        ctx.Code().mov(dest[0], tmp_xmm);
    }
}

void EmitConvertF64S32(EmitContext& ctx, const Operands& dest, const Operands& src) {
    Xmm tmp = dest[0].isMEM() ? ctx.TempXmmReg(false) : dest[0].getReg().cvt128();
    ctx.Code().cvtsi2sd(tmp, src[0]);
    if (dest[0].isMEM()) {
        ctx.Code().mov(dest[0], tmp);
    }
}

void EmitConvertF64S64(EmitContext& ctx, const Operands& dest, const Operands& src) {
    Xmm tmp = dest[0].isMEM() ? ctx.TempXmmReg(false) : dest[0].getReg().cvt128();
    ctx.Code().cvtsi2sd(tmp, src[0]);
    if (dest[0].isMEM()) {
        ctx.Code().mov(dest[0], tmp);
    }
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

