// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/exception.h"
#include "shader_recompiler/backend/asm_x64/emit_x64_instructions.h"
#include "shader_recompiler/backend/asm_x64/x64_emit_context.h"
#include "shader_recompiler/backend/asm_x64/x64_utils.h"

namespace Shader::Backend::X64 {

using namespace Xbyak;
using namespace Xbyak::util;


void EmitFPAbs16(EmitContext& ctx, const Operands& dest, const Operands& src) {
    MovGP(ctx, dest[0], src[0]);
    ctx.Code().and_(dest[0].Op(), 0x7FFF);
}

void EmitFPAbs32(EmitContext& ctx, const Operands& dest, const Operands& src) {
    Reg reg_tmp = ctx.TempXmmReg();
    Xmm xmm_tmp = dest[0].IsMem() ? ctx.TempXmmReg() : dest[0].Xmm();
    ctx.Code().mov(reg_tmp, 0x7FFFFFFF);
    ctx.Code().movd(xmm_tmp, reg_tmp);
    ctx.Code().andps(xmm_tmp, src[0].Op());
    MovFloat(ctx, dest[0], xmm_tmp);
}

void EmitFPAbs64(EmitContext& ctx, const Operands& dest, const Operands& src) {
    Reg reg_tmp = ctx.TempGPReg();
    Xmm xmm_tmp = dest[0].IsMem() ? ctx.TempXmmReg() : dest[0].Xmm();
    ctx.Code().mov(reg_tmp, 0x7FFFFFFFFFFFFFFF);
    ctx.Code().movq(xmm_tmp, reg_tmp);
    ctx.Code().andpd(xmm_tmp, src[0].Op());
    MovFloat(ctx, dest[0], xmm_tmp);
}

void EmitFPAdd16(EmitContext& ctx, const Operands& dest, const Operands& op1, const Operands& op2) {
    Xmm tmp1 = ctx.TempXmmReg();
    Xmm tmp2 = ctx.TempXmmReg();
    EmitInlineF16ToF32(ctx, tmp1, op1[0].Op());
    EmitInlineF16ToF32(ctx, tmp2, op2[0].Op());
    ctx.Code().addss(tmp1, tmp2);
    EmitInlineF32ToF16(ctx, dest[0].Op(), tmp1);
}

void EmitFPAdd32(EmitContext& ctx, const Operands& dest, const Operands& op1, const Operands& op2) {
    Xmm tmp = dest[0].IsMem() ? ctx.TempXmmReg() : dest[0].Xmm();
    MovFloat(ctx, tmp, op1[0]);
    ctx.Code().addss(tmp, op2[0].Op());
    MovFloat(ctx, dest[0], tmp);
}

void EmitFPAdd64(EmitContext& ctx, const Operands& dest, const Operands& op1, const Operands& op2) {
    Xmm tmp = dest[0].IsMem() ? ctx.TempXmmReg() : dest[0].Xmm();
    MovDouble(ctx, tmp, op1[0]);
    ctx.Code().addsd(tmp, op2[0].Op());
    MovDouble(ctx, dest[0], tmp);
}

void EmitFPSub32(EmitContext& ctx, const Operands& dest, const Operands& op1, const Operands& op2) {
    Xmm tmp = dest[0].IsMem() ? ctx.TempXmmReg() : dest[0].Xmm();
    MovFloat(ctx, tmp, op1[0]);
    ctx.Code().subss(tmp, op2[0].Op());
    MovFloat(ctx, dest[0], tmp);
}

void EmitFPFma16(EmitContext& ctx, const Operands& dest, const Operands& op1, const Operands& op2, const Operands& op3) {
    Xmm tmp1 = ctx.TempXmmReg();
    Xmm tmp2 = ctx.TempXmmReg();
    Xmm tmp3 = ctx.TempXmmReg();
    EmitInlineF16ToF32(ctx, tmp1, op1[0].Op());
    EmitInlineF16ToF32(ctx, tmp2, op2[0].Op());
    EmitInlineF16ToF32(ctx, tmp3, op3[0].Op());
    ctx.Code().vfmadd132ss(tmp3, tmp1, tmp2);
    EmitInlineF32ToF16(ctx, dest[0].Op(), tmp3);
}

void EmitFPFma32(EmitContext& ctx, const Operands& dest, const Operands& op1, const Operands& op2, const Operands& op3) {
    Xmm tmp1 = dest[0].IsMem() ? ctx.TempXmmReg() : dest[0].Xmm();
    Xmm tmp2 = op2[0].IsMem() ? ctx.TempXmmReg() : op2[0].Xmm();
    MovFloat(ctx, tmp1, op3[0]);
    MovFloat(ctx, tmp2, op2[0]);
    ctx.Code().vfmadd132ss(tmp2, tmp1, op1[0].Op());
    MovFloat(ctx, dest[0], tmp2);
}

void EmitFPFma64(EmitContext& ctx, const Operands& dest, const Operands& op1, const Operands& op2, const Operands& op3) {
    Xmm tmp1 = dest[0].IsMem() ? ctx.TempXmmReg() : dest[0].Xmm();
    Xmm tmp2 = op2[0].IsMem() ? ctx.TempXmmReg() : op2[0].Xmm();
    MovDouble(ctx, tmp1, op3[0]);
    MovDouble(ctx, tmp2, op2[0]);
    ctx.Code().vfmadd132sd(tmp2, tmp1, op1[0].Op());
    MovDouble(ctx, dest[0], tmp2);
}

void EmitFPMax32(EmitContext& ctx, const Operands& dest, const Operands& op1, const Operands& op2, bool is_legacy) {
    if (is_legacy) {
        Xmm tmp1 = ctx.TempXmmReg();
        Xmm tmp2 = ctx.TempXmmReg();
        MovFloat(ctx, tmp1, op1[0].Op());
        MovFloat(ctx, tmp2, op1[0].Op());
        ctx.Code().maxss(tmp2, op2[0].Op());
        ctx.Code().cmpunordss(tmp1, tmp1);
        ctx.Code().andps(tmp1, op2[0].Op());
        ctx.Code().orps(tmp2, tmp1);
        MovFloat(ctx, dest[0], tmp2);
    } else {
        Xmm tmp = dest[0].IsMem() ? ctx.TempXmmReg() : dest[0].Xmm();
        MovFloat(ctx, tmp, op1[0]);
        ctx.Code().maxss(tmp, op2[0].Op());
        MovFloat(ctx, dest[0], tmp);
    }
}

void EmitFPMax64(EmitContext& ctx, const Operands& dest, const Operands& op1, const Operands& op2) {
    Xmm tmp = dest[0].IsMem() ? ctx.TempXmmReg() : dest[0].Xmm();
    MovDouble(ctx, tmp, op1[0]);
    ctx.Code().maxsd(tmp, op2[0].Op());
    MovDouble(ctx, dest[0], tmp);
}

void EmitFPMin32(EmitContext& ctx, const Operands& dest, const Operands& op1, const Operands& op2, bool is_legacy) {
    if (is_legacy) {
        Xmm tmp1 = ctx.TempXmmReg();
        Xmm tmp2 = ctx.TempXmmReg();
        MovFloat(ctx, tmp1, op1[0].Op());
        MovFloat(ctx, tmp2, op1[0].Op());
        ctx.Code().minss(tmp2, op2[0].Op());
        ctx.Code().cmpunordss(tmp1, tmp1);
        ctx.Code().andps(tmp1, op2[0].Op());
        ctx.Code().orps(tmp2, tmp1);
        MovFloat(ctx, dest[0], tmp2);
    } else {
        Xmm tmp = dest[0].IsMem() ? ctx.TempXmmReg() : dest[0].Xmm();
        MovFloat(ctx, tmp, op1[0]);
        ctx.Code().minss(tmp, op2[0].Op());
        MovFloat(ctx, dest[0], tmp);
    }
}

void EmitFPMin64(EmitContext& ctx, const Operands& dest, const Operands& op1, const Operands& op2) {
    Xmm tmp = dest[0].IsMem() ? ctx.TempXmmReg() : dest[0].Xmm();
    MovDouble(ctx, tmp, op1[0]);
    ctx.Code().minsd(tmp, op2[0].Op());
    MovDouble(ctx, dest[0], tmp);
}

void EmitFPMinTri32(EmitContext& ctx, const Operands& dest, const Operands& op1, const Operands& op2, const Operands& op3) {
    Xmm tmp = dest[0].IsMem() ? ctx.TempXmmReg() : dest[0].Xmm();
    MovFloat(ctx, tmp, op1[0]);
    ctx.Code().minss(tmp, op2[0].Op());
    ctx.Code().minss(tmp, op3[0].Op());
    MovFloat(ctx, dest[0], tmp);
}

void EmitFPMaxTri32(EmitContext& ctx, const Operands& dest, const Operands& op1, const Operands& op2, const Operands& op3) {
    Xmm tmp = dest[0].IsMem() ? ctx.TempXmmReg() : dest[0].Xmm();
    MovFloat(ctx, tmp, op1[0]);
    ctx.Code().maxss(tmp, op2[0].Op());
    ctx.Code().maxss(tmp, op3[0].Op());
    MovFloat(ctx, dest[0], tmp);
}

void EmitFPMedTri32(EmitContext& ctx, const Operands& dest, const Operands& op1, const Operands& op2, const Operands& op3) {
    Xmm tmp = dest[0].IsMem() ? ctx.TempXmmReg() : dest[0].Xmm();
    Xmm tmp2 = ctx.TempXmmReg();
    MovFloat(ctx, tmp2, op1[0]);
    ctx.Code().maxss(tmp2, op2[0].Op());
    ctx.Code().minss(tmp2, op3[0].Op());
    MovFloat(ctx, tmp, op1[0]);
    ctx.Code().minss(tmp, op2[0].Op());
    ctx.Code().maxss(tmp, tmp2);
    MovFloat(ctx, dest[0], tmp);
}

void EmitFPMul16(EmitContext& ctx, const Operands& dest, const Operands& op1, const Operands& op2) {
    Xmm tmp1 = ctx.TempXmmReg();
    Xmm tmp2 = ctx.TempXmmReg();
    EmitInlineF16ToF32(ctx, tmp1, op1[0].Op());
    EmitInlineF16ToF32(ctx, tmp2, op2[0].Op());
    ctx.Code().mulss(tmp1, tmp2);
    EmitInlineF32ToF16(ctx, dest[0].Op(), tmp1);
}

void EmitFPMul32(EmitContext& ctx, const Operands& dest, const Operands& op1, const Operands& op2) {
    Xmm tmp = dest[0].IsMem() ? ctx.TempXmmReg() : dest[0].Xmm();
    MovFloat(ctx, tmp, op1[0]);
    ctx.Code().mulss(tmp, op2[0].Op());
    MovFloat(ctx, dest[0], tmp);
}

void EmitFPMul64(EmitContext& ctx, const Operands& dest, const Operands& op1, const Operands& op2) {
    Xmm tmp = dest[0].IsMem() ? ctx.TempXmmReg() : dest[0].Xmm();
    MovDouble(ctx, tmp, op1[0]);
    ctx.Code().mulsd(tmp, op2[0].Op());
    MovDouble(ctx, dest[0], tmp);
}

void EmitFPDiv32(EmitContext& ctx, const Operands& dest, const Operands& op1, const Operands& op2) {
    Xmm tmp = dest[0].IsMem() ? ctx.TempXmmReg() : dest[0].Xmm();
    MovFloat(ctx, tmp, op1[0]);
    ctx.Code().divss(tmp, op2[0].Op());
    MovFloat(ctx, dest[0], tmp);
}

void EmitFPDiv64(EmitContext& ctx, const Operands& dest, const Operands& op1, const Operands& op2) {
    Xmm tmp = dest[0].IsMem() ? ctx.TempXmmReg() : dest[0].Xmm();
    MovDouble(ctx, tmp, op1[0]);
    ctx.Code().divsd(tmp, op2[0].Op());
    MovDouble(ctx, dest[0], tmp);
}

void EmitFPNeg16(EmitContext& ctx, const Operands& dest, const Operands& op1) {
    MovGP(ctx, dest[0], op1[0]);
    ctx.Code().xor_(dest[0].Op(), 0x8000);
}

void EmitFPNeg32(EmitContext& ctx, const Operands& dest, const Operands& op1) {
    Xmm tmp_xmm = dest[0].IsMem() ? ctx.TempXmmReg() : dest[0].Xmm();
    Reg tmp_reg = ctx.TempGPReg().cvt32();
    ctx.Code().mov(tmp_reg, 0x80000000);
    ctx.Code().movd(tmp_xmm, tmp_reg);
    ctx.Code().xorps(tmp_xmm, op1[0].Op());
    MovFloat(ctx, dest[0], tmp_xmm);
}

void EmitFPNeg64(EmitContext& ctx, const Operands& dest, const Operands& op1) {
    Xmm tmp_xmm = dest[0].IsMem() ? ctx.TempXmmReg() : dest[0].Xmm();
    Reg tmp_reg = ctx.TempXmmReg();
    ctx.Code().mov(tmp_reg, 0x8000000000000000);
    ctx.Code().movq(tmp_xmm, tmp_reg);
    ctx.Code().xorpd(tmp_xmm, op1[0].Op());
    MovDouble(ctx, dest[0], tmp_xmm);
    
}

void EmitFPSin(EmitContext& ctx) {
    throw NotImplementedException("FPSin");
}

void EmitFPCos(EmitContext& ctx) {
    throw NotImplementedException("FPCos");
}

void EmitFPExp2(EmitContext& ctx) {
    throw NotImplementedException("FPExp2");
}

void EmitFPLdexp(EmitContext& ctx) {
    throw NotImplementedException("FPLdexp");
}

void EmitFPLog2(EmitContext& ctx) {
    throw NotImplementedException("FPLog2");
}

void EmitFPRecip32(EmitContext& ctx, const Operands& dest, const Operands& op1) {
    Xmm tmp = dest[0].IsMem() ? ctx.TempXmmReg() : dest[0].Xmm();
    ctx.Code().rcpss(tmp, op1[0].Op());
    MovFloat(ctx, dest[0], tmp);
}

void EmitFPRecip64(EmitContext& ctx, const Operands& dest, const Operands& op1) {
    Xmm tmp_xmm = dest[0].IsMem() ? ctx.TempXmmReg() : dest[0].Xmm();
    Reg tmp_reg = ctx.TempGPReg();
    ctx.Code().mov(tmp_reg, 1);
    ctx.Code().cvtsi2sd(tmp_xmm, tmp_reg);
    ctx.Code().divsd(tmp_xmm, op1[0].Op());
    MovDouble(ctx, dest[0], tmp_xmm);
}

void EmitFPRecipSqrt32(EmitContext& ctx, const Operands& dest, const Operands& op1) {
    Xmm tmp = dest[0].IsMem() ? ctx.TempXmmReg() : dest[0].Xmm();
    ctx.Code().rsqrtss(tmp, op1[0].Op());
    MovFloat(ctx, dest[0], tmp);
}

void EmitFPRecipSqrt64(EmitContext& ctx, const Operands& dest, const Operands& op1) {
    Xmm tmp_xmm = dest[0].IsMem() ? ctx.TempXmmReg() : dest[0].Xmm();
    Reg tmp_reg = ctx.TempGPReg();
    ctx.Code().mov(tmp_reg, 1);
    ctx.Code().cvtsi2sd(tmp_xmm, tmp_reg);
    ctx.Code().divsd(tmp_xmm, op1[0].Op());
    ctx.Code().sqrtsd(tmp_xmm, tmp_xmm);
    MovDouble(ctx, dest[0], tmp_xmm);
}

void EmitFPSqrt(EmitContext& ctx, const Operands& dest, const Operands& op1) {
    Xmm tmp = dest[0].IsMem() ? ctx.TempXmmReg() : dest[0].Xmm();
    ctx.Code().sqrtss(tmp, op1[0].Op());
    MovFloat(ctx, dest[0], tmp);
}

void EmitFPSaturate16(EmitContext& ctx) {
    throw NotImplementedException("FPSaturate16");
}

void EmitFPSaturate32(EmitContext& ctx) {
    throw NotImplementedException("FPSaturate32");
}

void EmitFPSaturate64(EmitContext& ctx) {
    throw NotImplementedException("FPSaturate64");
}

void EmitFPClamp16(EmitContext& ctx, const Operands& dest, const Operands& op, const Operands& min, const Operands& max) {
    Xmm tmp1 = ctx.TempXmmReg();
    Xmm tmp2 = ctx.TempXmmReg();
    Xmm tmp3 = ctx.TempXmmReg();
    EmitInlineF16ToF32(ctx, tmp1, op[0].Op());
    EmitInlineF16ToF32(ctx, tmp2, min[0].Op());
    EmitInlineF16ToF32(ctx, tmp3, max[0].Op());
    ctx.Code().maxss(tmp1, tmp2);
    ctx.Code().minss(tmp1, tmp3);
    EmitInlineF32ToF16(ctx, dest[0].Op(), tmp1); 
}

void EmitFPClamp32(EmitContext& ctx, const Operands& dest, const Operands& op, const Operands& min, const Operands& max) {
    Xmm tmp = dest[0].IsMem() ? ctx.TempXmmReg() : dest[0].Xmm();
    MovFloat(ctx, tmp, op[0]);
    ctx.Code().maxss(tmp, min[0].Op());
    ctx.Code().minss(tmp, max[0].Op());
    MovFloat(ctx, dest[0], tmp);
}

void EmitFPClamp64(EmitContext& ctx, const Operands& dest, const Operands& op, const Operands& min, const Operands& max) {
    Xmm tmp = dest[0].IsMem() ? ctx.TempXmmReg() : dest[0].Xmm();
    MovDouble(ctx, tmp, op[0]);
    ctx.Code().maxsd(tmp, min[0].Op());
    ctx.Code().minsd(tmp, max[0].Op());
    MovDouble(ctx, dest[0], tmp);
}

void EmitFPRoundEven16(EmitContext& ctx, const Operands& dest, const Operands& op1) {
    Xmm tmp = ctx.TempXmmReg();
    EmitInlineF16ToF32(ctx, tmp, op1[0].Op());
    ctx.Code().roundss(tmp, tmp, 0x00);
    EmitInlineF32ToF16(ctx, dest[0].Op(), tmp);
}

void EmitFPRoundEven32(EmitContext& ctx, const Operands& dest, const Operands& op1) {
    Xmm tmp = dest[0].IsMem() ? ctx.TempXmmReg() : dest[0].Xmm();
    ctx.Code().roundss(tmp, op1[0].Op(), 0x00);
    MovFloat(ctx, dest[0], tmp);
}

void EmitFPRoundEven64(EmitContext& ctx, const Operands& dest, const Operands& op1) {
    Xmm tmp = dest[0].IsMem() ? ctx.TempXmmReg() : dest[0].Xmm();
    ctx.Code().roundsd(tmp, op1[0].Op(), 0x00);
    MovDouble(ctx, dest[0], tmp);
}

void EmitFPFloor16(EmitContext& ctx, const Operands& dest, const Operands& op1) {
    Xmm tmp = ctx.TempXmmReg();
    EmitInlineF16ToF32(ctx, tmp, op1[0].Op());
    ctx.Code().roundss(tmp, tmp, 0x01);
    EmitInlineF32ToF16(ctx, dest[0].Op(), tmp);
}

void EmitFPFloor32(EmitContext& ctx, const Operands& dest, const Operands& op1) {
    Xmm tmp = dest[0].IsMem() ? ctx.TempXmmReg() : dest[0].Xmm();
    ctx.Code().roundss(tmp, op1[0].Op(), 0x01);
    MovFloat(ctx, dest[0], tmp);
}

void EmitFPFloor64(EmitContext& ctx, const Operands& dest, const Operands& op1) {
    Xmm tmp = dest[0].IsMem() ? ctx.TempXmmReg() : dest[0].Xmm();
    ctx.Code().roundsd(tmp, op1[0].Op(), 0x01);
    MovDouble(ctx, dest[0], tmp);
}

void EmitFPCeil16(EmitContext& ctx, const Operands& dest, const Operands& op1) {
    Xmm tmp = ctx.TempXmmReg();
    EmitInlineF16ToF32(ctx, tmp, op1[0].Op());
    ctx.Code().roundss(tmp, tmp, 0x02);
    EmitInlineF32ToF16(ctx, dest[0].Op(), tmp);
}

void EmitFPCeil32(EmitContext& ctx, const Operands& dest, const Operands& op1) {
    Xmm tmp = dest[0].IsMem() ? ctx.TempXmmReg() : dest[0].Xmm();
    ctx.Code().roundss(tmp, op1[0].Op(), 0x02);
    MovFloat(ctx, dest[0], tmp);
}

void EmitFPCeil64(EmitContext& ctx, const Operands& dest, const Operands& op1) {
    Xmm tmp = dest[0].IsMem() ? ctx.TempXmmReg() : dest[0].Xmm();
    ctx.Code().roundsd(tmp, op1[0].Op(), 0x02);
    MovDouble(ctx, dest[0], tmp);
}

void EmitFPTrunc16(EmitContext& ctx, const Operands& dest, const Operands& op) {
    Xmm tmp_xmm = ctx.TempXmmReg();
    Reg tmp_reg = dest[0].IsMem() ? ctx.TempGPReg().cvt32() : dest[0].Reg().cvt32();
    EmitInlineF16ToF32(ctx, tmp_xmm, op[0].Op());
    ctx.Code().cvttss2si(tmp_reg, tmp_xmm);
    ctx.Code().cvtsi2ss(tmp_xmm, tmp_reg);
    EmitInlineF32ToF16(ctx, dest[0].Op(), tmp_xmm);
}

void EmitFPTrunc32(EmitContext& ctx, const Operands& dest, const Operands& op) {
    Xmm tmp_xmm = dest[0].IsMem() ? ctx.TempXmmReg() : dest[0].Xmm();
    Reg tmp_reg = ctx.TempGPReg().cvt32();
    ctx.Code().cvttss2si(tmp_reg, op[0].Op());
    ctx.Code().cvtsi2ss(tmp_xmm, tmp_reg);
    MovFloat(ctx, dest[0], tmp_xmm);
}

void EmitFPTrunc64(EmitContext& ctx, const Operands& dest, const Operands& op) {
    Xmm tmp_xmm = dest[0].IsMem() ? ctx.TempXmmReg() : dest[0].Xmm();
    Reg tmp_reg = ctx.TempGPReg();
    ctx.Code().cvttsd2si(tmp_reg, op[0].Op());
    ctx.Code().cvtsi2sd(tmp_xmm, tmp_reg);
    MovDouble(ctx, dest[0], tmp_xmm);
}

void EmitFPFract32(EmitContext& ctx, const Operands& dest, const Operands& op) {
    Xmm tmp = dest[0].IsMem() ? ctx.TempXmmReg() : dest[0].Xmm();
    Xmm tmp2 = ctx.TempXmmReg();
    MovFloat(ctx, tmp, op[0]);
    ctx.Code().roundss(tmp2, tmp, 0x01);
    ctx.Code().subss(tmp, tmp2);
    MovFloat(ctx, dest[0], tmp);
}

void EmitFPFract64(EmitContext& ctx, const Operands& dest, const Operands& op) {
    Xmm tmp = dest[0].IsMem() ? ctx.TempXmmReg() : dest[0].Xmm();
    Xmm tmp2 = ctx.TempXmmReg();
    MovDouble(ctx, tmp, op[0]);
    ctx.Code().roundsd(tmp2, tmp, 0x01);
    ctx.Code().subsd(tmp, tmp2);
    MovDouble(ctx, dest[0], tmp);
}

void EmitFPFrexpSig32(EmitContext& ctx) {
    throw NotImplementedException("FPFrexpSig32");
}

void EmitFPFrexpSig64(EmitContext& ctx) {
    throw NotImplementedException("FPFrexpSig64");
}

void EmitFPFrexpExp32(EmitContext& ctx) {
    throw NotImplementedException("FPFrexpExp32");
}

void EmitFPFrexpExp64(EmitContext& ctx) {
    throw NotImplementedException("FPFrexpExp64");
}

void EmitFPOrdEqual16(EmitContext& ctx, const Operands& dest, const Operands& lhs, const Operands& rhs) {
    Label not_nan;
    EmitFPUnordEqual16(ctx, dest, lhs, rhs);
    ctx.Code().jnp(not_nan);
    ctx.Code().mov(dest[0].Op(), 0);
    ctx.Code().L(not_nan);
}

void EmitFPOrdEqual32(EmitContext& ctx, const Operands& dest, const Operands& lhs, const Operands& rhs) {
    Label not_nan;
    EmitFPUnordEqual32(ctx, dest, lhs, rhs);
    ctx.Code().jnp(not_nan);
    ctx.Code().mov(dest[0].Op(), 0);
    ctx.Code().L(not_nan);
}

void EmitFPOrdEqual64(EmitContext& ctx, const Operands& dest, const Operands& lhs, const Operands& rhs) {
    Label not_nan;
    EmitFPUnordEqual64(ctx, dest, lhs, rhs);
    ctx.Code().jnp(not_nan);
    ctx.Code().mov(dest[0].Op(), 0);
    ctx.Code().L(not_nan);
}

void EmitFPUnordEqual16(EmitContext& ctx, const Operands& dest, const Operands& lhs, const Operands& rhs) {
    Xmm tmp1 = ctx.TempXmmReg();
    Xmm tmp2 = ctx.TempXmmReg();
    EmitInlineF16ToF32(ctx, tmp1, lhs[0].Op());
    EmitInlineF16ToF32(ctx, tmp2, rhs[0].Op());
    ctx.Code().ucomiss(tmp1, tmp2);
    ctx.Code().sete(dest[0].Op());
}

void EmitFPUnordEqual32(EmitContext& ctx, const Operands& dest, const Operands& lhs, const Operands& rhs) {
    Xmm tmp = lhs[0].IsMem() ? ctx.TempXmmReg() : lhs[0].Xmm();
    MovFloat(ctx, tmp, lhs[0]);
    ctx.Code().ucomiss(tmp, rhs[0].Op());
    ctx.Code().sete(dest[0].Op());
}

void EmitFPUnordEqual64(EmitContext& ctx, const Operands& dest, const Operands& lhs, const Operands& rhs) {
    Xmm tmp = lhs[0].IsMem() ? ctx.TempXmmReg() : lhs[0].Xmm();
    MovDouble(ctx, tmp, lhs[0]);
    ctx.Code().ucomisd(tmp, rhs[0].Op());
    ctx.Code().sete(dest[0].Op());
}

void EmitFPOrdNotEqual16(EmitContext& ctx, const Operands& dest, const Operands& lhs, const Operands& rhs) {
    Label not_nan;
    EmitFPUnordNotEqual16(ctx, dest, lhs, rhs);
    ctx.Code().jnp(not_nan);
    ctx.Code().mov(dest[0].Op(), 0);
    ctx.Code().L(not_nan);
}

void EmitFPOrdNotEqual32(EmitContext& ctx, const Operands& dest, const Operands& lhs, const Operands& rhs) {
    Label not_nan;
    Xmm tmp = lhs[0].IsMem() ? ctx.TempXmmReg() : lhs[0].Xmm();
    ctx.Code().jnp(not_nan);
    ctx.Code().mov(dest[0].Op(), 0);
    ctx.Code().L(not_nan);
}

void EmitFPOrdNotEqual64(EmitContext& ctx, const Operands& dest, const Operands& lhs, const Operands& rhs) {
    Label not_nan;
    EmitFPUnordNotEqual64(ctx, dest, lhs, rhs);
    ctx.Code().jnp(not_nan);
    ctx.Code().mov(dest[0].Op(), 0);
    ctx.Code().L(not_nan);
}

void EmitFPUnordNotEqual16(EmitContext& ctx, const Operands& dest, const Operands& lhs, const Operands& rhs) {
    Xmm tmp1 = ctx.TempXmmReg();
    Xmm tmp2 = ctx.TempXmmReg();
    EmitInlineF16ToF32(ctx, tmp1, lhs[0].Op());
    EmitInlineF16ToF32(ctx, tmp2, rhs[0].Op());
    ctx.Code().ucomiss(tmp1, tmp2);
    ctx.Code().setne(dest[0].Op());
}

void EmitFPUnordNotEqual32(EmitContext& ctx, const Operands& dest, const Operands& lhs, const Operands& rhs) {
    Xmm tmp = lhs[0].IsMem() ? ctx.TempXmmReg() : lhs[0].Xmm();
    MovFloat(ctx, tmp, lhs[0]);
    ctx.Code().ucomiss(tmp, rhs[0].Op());
    ctx.Code().setne(dest[0].Op());
}

void EmitFPUnordNotEqual64(EmitContext& ctx, const Operands& dest, const Operands& lhs, const Operands& rhs) {
    Xmm tmp = lhs[0].IsMem() ? ctx.TempXmmReg() : lhs[0].Xmm();
    MovDouble(ctx, tmp, lhs[0]);
    ctx.Code().ucomisd(tmp, rhs[0].Op());
    ctx.Code().setne(dest[0].Op());
}

void EmitFPOrdLessThan16(EmitContext& ctx, const Operands& dest, const Operands& lhs, const Operands& rhs) {
    Label not_nan;
    EmitFPUnordLessThan16(ctx, dest, lhs, rhs);
    ctx.Code().jnp(not_nan);
    ctx.Code().mov(dest[0].Op(), 0);
    ctx.Code().L(not_nan);
}

void EmitFPOrdLessThan32(EmitContext& ctx, const Operands& dest, const Operands& lhs, const Operands& rhs) {
    Label not_nan;
    EmitFPUnordLessThan32(ctx, dest, lhs, rhs);
    ctx.Code().jnp(not_nan);
    ctx.Code().mov(dest[0].Op(), 0);
    ctx.Code().L(not_nan);
}

void EmitFPOrdLessThan64(EmitContext& ctx, const Operands& dest, const Operands& lhs, const Operands& rhs) {
    Label not_nan;
    EmitFPUnordLessThan64(ctx, dest, lhs, rhs);
    ctx.Code().jnp(not_nan);
    ctx.Code().mov(dest[0].Op(), 0);
    ctx.Code().L(not_nan);
}

void EmitFPUnordLessThan16(EmitContext& ctx, const Operands& dest, const Operands& lhs, const Operands& rhs) {
    Xmm tmp1 = ctx.TempXmmReg();
    Xmm tmp2 = ctx.TempXmmReg();
    EmitInlineF16ToF32(ctx, tmp1, lhs[0].Op());
    EmitInlineF16ToF32(ctx, tmp2, rhs[0].Op());
    ctx.Code().ucomiss(tmp1, tmp2);
    ctx.Code().setb(dest[0].Op());
}

void EmitFPUnordLessThan32(EmitContext& ctx, const Operands& dest, const Operands& lhs, const Operands& rhs) {
    Xmm tmp = lhs[0].IsMem() ? ctx.TempXmmReg() : lhs[0].Xmm();
    MovFloat(ctx, tmp, lhs[0]);
    ctx.Code().ucomiss(tmp, rhs[0].Op());
    ctx.Code().setb(dest[0].Op());
}

void EmitFPUnordLessThan64(EmitContext& ctx, const Operands& dest, const Operands& lhs, const Operands& rhs) {
    Xmm tmp = lhs[0].IsMem() ? ctx.TempXmmReg() : lhs[0].Xmm();
    MovDouble(ctx, tmp, lhs[0]);
    ctx.Code().ucomisd(tmp, rhs[0].Op());
    ctx.Code().setb(dest[0].Op());
}

void EmitFPOrdGreaterThan16(EmitContext& ctx, const Operands& dest, const Operands& lhs, const Operands& rhs) {
    Label not_nan;
    EmitFPUnordGreaterThan16(ctx, dest, lhs, rhs);
    ctx.Code().jnp(not_nan);
    ctx.Code().mov(dest[0].Op(), 0);
    ctx.Code().L(not_nan);
}

void EmitFPOrdGreaterThan32(EmitContext& ctx, const Operands& dest, const Operands& lhs, const Operands& rhs) {
    Label not_nan;
    EmitFPUnordGreaterThan32(ctx, dest, lhs, rhs);
    ctx.Code().jnp(not_nan);
    ctx.Code().mov(dest[0].Op(), 0);
    ctx.Code().L(not_nan);
}

void EmitFPOrdGreaterThan64(EmitContext& ctx, const Operands& dest, const Operands& lhs, const Operands& rhs) {
    Label not_nan;
    EmitFPUnordGreaterThan64(ctx, dest, lhs, rhs);
    ctx.Code().jnp(not_nan);
    ctx.Code().mov(dest[0].Op(), 0);
    ctx.Code().L(not_nan);
}

void EmitFPUnordGreaterThan16(EmitContext& ctx, const Operands& dest, const Operands& lhs, const Operands& rhs) {
    Xmm tmp1 = ctx.TempXmmReg();
    Xmm tmp2 = ctx.TempXmmReg();
    EmitInlineF16ToF32(ctx, tmp1, lhs[0].Op());
    EmitInlineF16ToF32(ctx, tmp2, rhs[0].Op());
    ctx.Code().ucomiss(tmp1, tmp2);
    ctx.Code().seta(dest[0].Op());
}

void EmitFPUnordGreaterThan32(EmitContext& ctx, const Operands& dest, const Operands& lhs, const Operands& rhs) {
    Xmm tmp = lhs[0].IsMem() ? ctx.TempXmmReg() : lhs[0].Xmm();
    MovFloat(ctx, tmp, lhs[0]);
    ctx.Code().ucomiss(tmp, rhs[0].Op());
    ctx.Code().seta(dest[0].Op());
}

void EmitFPUnordGreaterThan64(EmitContext& ctx, const Operands& dest, const Operands& lhs, const Operands& rhs) {
    Xmm tmp = lhs[0].IsMem() ? ctx.TempXmmReg() : lhs[0].Xmm();
    MovDouble(ctx, tmp, lhs[0]);
    ctx.Code().ucomisd(tmp, rhs[0].Op());
    ctx.Code().seta(dest[0].Op());
}

void EmitFPOrdLessThanEqual16(EmitContext& ctx, const Operands& dest, const Operands& lhs, const Operands& rhs) {
    Label not_nan;
    EmitFPUnordLessThanEqual16(ctx, dest, lhs, rhs);
    ctx.Code().jnp(not_nan);
    ctx.Code().mov(dest[0].Op(), 0);
    ctx.Code().L(not_nan);
}

void EmitFPOrdLessThanEqual32(EmitContext& ctx, const Operands& dest, const Operands& lhs, const Operands& rhs) {
    Label not_nan;
    EmitFPUnordLessThanEqual32(ctx, dest, lhs, rhs);
    ctx.Code().jnp(not_nan);
    ctx.Code().mov(dest[0].Op(), 0);
    ctx.Code().L(not_nan);
}

void EmitFPOrdLessThanEqual64(EmitContext& ctx, const Operands& dest, const Operands& lhs, const Operands& rhs) {
    Label not_nan;
    EmitFPUnordLessThanEqual64(ctx, dest, lhs, rhs);
    ctx.Code().jnp(not_nan);
    ctx.Code().mov(dest[0].Op(), 0);
    ctx.Code().L(not_nan);
}

void EmitFPUnordLessThanEqual16(EmitContext& ctx, const Operands& dest, const Operands& lhs, const Operands& rhs) {
    Xmm tmp1 = ctx.TempXmmReg();
    Xmm tmp2 = ctx.TempXmmReg();
    EmitInlineF16ToF32(ctx, tmp1, lhs[0].Op());
    EmitInlineF16ToF32(ctx, tmp2, rhs[0].Op());
    ctx.Code().ucomiss(tmp1, tmp2);
    ctx.Code().setbe(dest[0].Op());
}

void EmitFPUnordLessThanEqual32(EmitContext& ctx, const Operands& dest, const Operands& lhs, const Operands& rhs) {
    Xmm tmp = lhs[0].IsMem() ? ctx.TempXmmReg() : lhs[0].Xmm();
    MovFloat(ctx, tmp, lhs[0]);
    ctx.Code().ucomiss(tmp, rhs[0].Op());
    ctx.Code().setbe(dest[0].Op());
}

void EmitFPUnordLessThanEqual64(EmitContext& ctx, const Operands& dest, const Operands& lhs, const Operands& rhs) {
    Xmm tmp = lhs[0].IsMem() ? ctx.TempXmmReg() : lhs[0].Xmm();
    MovDouble(ctx, tmp, lhs[0]);
    ctx.Code().ucomisd(tmp, rhs[0].Op());
    ctx.Code().setbe(dest[0].Op());
}

void EmitFPOrdGreaterThanEqual16(EmitContext& ctx, const Operands& dest, const Operands& lhs, const Operands& rhs) {
    Label not_nan;
    EmitFPUnordGreaterThanEqual16(ctx, dest, lhs, rhs);
    ctx.Code().jnp(not_nan);
    ctx.Code().mov(dest[0].Op(), 0);
    ctx.Code().L(not_nan);
}

void EmitFPOrdGreaterThanEqual32(EmitContext& ctx, const Operands& dest, const Operands& lhs, const Operands& rhs) {
    Label not_nan;
    EmitFPUnordGreaterThanEqual32(ctx, dest, lhs, rhs);
    ctx.Code().jnp(not_nan);
    ctx.Code().mov(dest[0].Op(), 0);
    ctx.Code().L(not_nan);
}

void EmitFPOrdGreaterThanEqual64(EmitContext& ctx, const Operands& dest, const Operands& lhs, const Operands& rhs) {
    Label not_nan;
    EmitFPUnordGreaterThanEqual64(ctx, dest, lhs, rhs);
    ctx.Code().jnp(not_nan);
    ctx.Code().mov(dest[0].Op(), 0);
    ctx.Code().L(not_nan);
}

void EmitFPUnordGreaterThanEqual16(EmitContext& ctx, const Operands& dest, const Operands& lhs, const Operands& rhs) {
    Xmm tmp1 = ctx.TempXmmReg();
    Xmm tmp2 = ctx.TempXmmReg();
    EmitInlineF16ToF32(ctx, tmp1, lhs[0].Op());
    EmitInlineF16ToF32(ctx, tmp2, rhs[0].Op());
    ctx.Code().ucomiss(tmp1, tmp2);
    ctx.Code().setae(dest[0].Op());
}

void EmitFPUnordGreaterThanEqual32(EmitContext& ctx, const Operands& dest, const Operands& lhs, const Operands& rhs) {
    Xmm tmp = lhs[0].IsMem() ? ctx.TempXmmReg() : lhs[0].Xmm();
    MovFloat(ctx, tmp, lhs[0]);
    ctx.Code().ucomiss(tmp, rhs[0].Op());
    ctx.Code().setae(dest[0].Op());
}

void EmitFPUnordGreaterThanEqual64(EmitContext& ctx, const Operands& dest, const Operands& lhs, const Operands& rhs) {
    Xmm tmp = lhs[0].IsMem() ? ctx.TempXmmReg() : lhs[0].Xmm();
    MovDouble(ctx, tmp, lhs[0]);
    ctx.Code().ucomisd(tmp, rhs[0].Op());
    ctx.Code().setae(dest[0].Op());
}

void EmitFPIsNan16(EmitContext& ctx, const Operands& dest, const Operands& op) {
    Xmm tmp = ctx.TempXmmReg();
    EmitInlineF16ToF32(ctx, tmp, op[0].Op());
    ctx.Code().ucomiss(tmp, tmp);
    ctx.Code().setp(dest[0].Op());
}

void EmitFPIsNan32(EmitContext& ctx, const Operands& dest, const Operands& op) {
    Xmm tmp = dest[0].IsMem() ? ctx.TempXmmReg() : dest[0].Xmm();
    MovFloat(ctx, tmp, op[0]);
    ctx.Code().ucomiss(tmp, tmp);
    ctx.Code().setp(dest[0].Op());
}

void EmitFPIsNan64(EmitContext& ctx, const Operands& dest, const Operands& op) {
    Xmm tmp = dest[0].IsMem() ? ctx.TempXmmReg() : dest[0].Xmm();
    MovDouble(ctx, tmp, op[0]);
    ctx.Code().ucomisd(tmp, tmp);
    ctx.Code().setp(dest[0].Op());
}

void EmitFPIsInf32(EmitContext& ctx) {
    throw NotImplementedException("FPIsInf32");
}

void EmitFPIsInf64(EmitContext& ctx) {
    throw NotImplementedException("FPIsInf64");
}

void EmitFPCmpClass32(EmitContext&) {
    UNREACHABLE();
}

} // namespace Shader::Backend::X64