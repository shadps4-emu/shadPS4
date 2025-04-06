// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/exception.h"
#include "shader_recompiler/backend/asm_x64/x64_emit_context.h"
#include "shader_recompiler/backend/asm_x64/x64_utils.h"

namespace Shader::Backend::X64 {

using namespace Xbyak;
using namespace Xbyak::util;

namespace {

static bool EmitSaveRegTemp(EmitContext ctx, const Reg& save, const Operand& dest) {
    if (dest.getIdx() == save.getIdx()) {
        // Destination is reg, no need to save
        return false;
    }
    ctx.Code().push(save);
    return true;
}

static void EmitRestoreRegTemp(EmitContext ctx, const Reg& save) {
    ctx.Code().pop(save);
}

} // namespace

void EmitIAdd32(EmitContext& ctx, const Operands& dest, const Operands& op1, const Operands& op2) {
    if (dest[0].isREG() && op1[0].isREG() && op2[0].isREG()) {
        ctx.Code().lea(dest[0].getReg(), ptr[op1[0].getReg() + op2[0].getReg()]);
    } else {
        Reg tmp = dest[0].isMEM() ? ctx.TempGPReg(false).cvt32() : dest[0].getReg().cvt32();
        MovGP(ctx, tmp, op1[0]);
        ctx.Code().add(tmp, op2[0]);
        MovGP(ctx, dest[0], tmp); 
    }
}

void EmitIAdd64(EmitContext& ctx, const Operands& dest, const Operands& op1, const Operands& op2) {
    if (dest[0].isREG() && op1[0].isREG() && op2[0].isREG()) {
        ctx.Code().lea(dest[0].getReg(), ptr[op1[0].getReg() + op2[0].getReg()]);
    } else {
        Reg tmp = dest[0].isMEM() ? ctx.TempGPReg(false) : dest[0].getReg();
        MovGP(ctx, tmp, op1[0]);
        ctx.Code().add(tmp, op2[0]);
        MovGP(ctx, dest[0], tmp); 
    }
}

void EmitIAddCary32(EmitContext& ctx, const Operands& dest, const Operands& op1, const Operands& op2) {
    Reg tmp = dest[0].isMEM() ? ctx.TempGPReg(false).cvt32() : dest[0].getReg().cvt32();
    Operand carry = dest[1];
    carry.setBit(1);
    MovGP(ctx, tmp, op1[0]);
    ctx.Code().add(tmp, op2[0]);
    ctx.Code().setc(carry);
}

void EmitISub32(EmitContext& ctx, const Operands& dest, const Operands& op1, const Operands& op2) {
    Reg tmp = dest[0].isMEM() ? ctx.TempGPReg(false).cvt32() : dest[0].getReg().cvt32();
    MovGP(ctx, tmp, op1[0]);
    ctx.Code().sub(tmp, op2[0]);
    MovGP(ctx, dest[0], tmp);
}

void EmitISub64(EmitContext& ctx, const Operands& dest, const Operands& op1, const Operands& op2) {
    Reg tmp = dest[0].isMEM() ? ctx.TempGPReg(false) : dest[0].getReg();
    MovGP(ctx, tmp, op1[0]);
    ctx.Code().sub(tmp, op2[0]);
    MovGP(ctx, dest[0], tmp);
}

void EmitSMulExt(EmitContext& ctx) {
    throw NotImplementedException("SMulExtended");
}

void EmitUMulExt(EmitContext& ctx) {
    throw NotImplementedException("UMulExtended");
}

void EmitIMul32(EmitContext& ctx, const Operands& dest, const Operands& op1, const Operands& op2) {
    Reg tmp = dest[0].isMEM() ? ctx.TempGPReg(false).cvt32() : dest[0].getReg().cvt32();
    MovGP(ctx, tmp, op1[0]);
    ctx.Code().imul(tmp, op2[0]);
    MovGP(ctx, dest[0], tmp);
}

void EmitIMul64(EmitContext& ctx, const Operands& dest, const Operands& op1, const Operands& op2) {
    Reg tmp = dest[0].isMEM() ? ctx.TempGPReg(false) : dest[0].getReg();
    MovGP(ctx, tmp, op1[0]);
    ctx.Code().imul(tmp, op2[0]);
    MovGP(ctx, dest[0], tmp);
}

void EmitSDiv32(EmitContext& ctx, const Operands& dest, const Operands& op1, const Operands& op2) {
    bool rax_saved = EmitSaveRegTemp(ctx, rax, dest[0]);
    bool rdx_saved = EmitSaveRegTemp(ctx, rdx, dest[0]);
    Reg tmp = op2[0].getReg().cvt32();
    while (tmp.getIdx() == rax.getIdx()) {
        tmp = ctx.TempGPReg().cvt32();
    }
    MovGP(ctx, tmp, op2[0]);
    MovGP(ctx, eax, op1[0]);
    ctx.Code().idiv(tmp);
    MovGP(ctx, dest[0], eax);
    if (rdx_saved) {
        EmitRestoreRegTemp(ctx, rdx);
    }
    if (rax_saved) {
        EmitRestoreRegTemp(ctx, rax);
    }
}

void EmitUDiv32(EmitContext& ctx, const Operands& dest, const Operands& op1, const Operands& op2) {
    bool rax_saved = EmitSaveRegTemp(ctx, rax, dest[0]);
    bool rdx_saved = EmitSaveRegTemp(ctx, rdx, dest[0]);
    Reg tmp = op2[0].getReg().cvt32();
    while (tmp.getIdx() == rax.getIdx()) {
        tmp = ctx.TempGPReg().cvt32();
    }
    MovGP(ctx, tmp, op2[0]);
    MovGP(ctx, eax, op1[0]);
    ctx.Code().div(tmp);
    MovGP(ctx, dest[0], eax);
    if (rdx_saved) {
        EmitRestoreRegTemp(ctx, rdx);
    }
    if (rax_saved) {
        EmitRestoreRegTemp(ctx, rax);
    }
}

void EmitSMod32(EmitContext& ctx, const Operands& dest, const Operands& op1, const Operands& op2) {
    bool rax_saved = EmitSaveRegTemp(ctx, rax, dest[0]);
    bool rdx_saved = EmitSaveRegTemp(ctx, rdx, dest[0]);
    Reg tmp = op2[0].getReg().cvt32();
    while (tmp.getIdx() == rax.getIdx()) {
        tmp = ctx.TempGPReg().cvt32();
    }
    MovGP(ctx, tmp, op2[0]);
    MovGP(ctx, eax, op1[0]);
    ctx.Code().idiv(tmp);
    MovGP(ctx, dest[0], edx);
    if (rdx_saved) {
        EmitRestoreRegTemp(ctx, rdx);
    }
    if (rax_saved) {
        EmitRestoreRegTemp(ctx, rax);
    }
}

void EmitUMod32(EmitContext& ctx, const Operands& dest, const Operands& op1, const Operands& op2) {
    bool rax_saved = EmitSaveRegTemp(ctx, rax, dest[0]);
    bool rdx_saved = EmitSaveRegTemp(ctx, rdx, dest[0]);
    Reg tmp = op2[0].getReg().cvt32();
    while (tmp.getIdx() == rax.getIdx()) {
        tmp = ctx.TempGPReg().cvt32();
    }
    MovGP(ctx, tmp, op2[0]);
    MovGP(ctx, eax, op1[0]);
    ctx.Code().div(tmp);
    MovGP(ctx, dest[0], edx);
    if (rdx_saved) {
        EmitRestoreRegTemp(ctx, rdx);
    }
    if (rax_saved) {
        EmitRestoreRegTemp(ctx, rax);
    }
}

void EmitINeg32(EmitContext& ctx, const Operands& dest, const Operands& op) {
    Reg tmp = dest[0].isMEM() ? ctx.TempGPReg(false).cvt32() : dest[0].getReg().cvt32();
    MovGP(ctx, tmp, op[0]);
    ctx.Code().neg(tmp);
    MovGP(ctx, dest[0], tmp);
}

void EmitINeg64(EmitContext& ctx, const Operands& dest, const Operands& op) {
    Reg tmp = dest[0].isMEM() ? ctx.TempGPReg(false) : dest[0].getReg();
    MovGP(ctx, tmp, op[0]);
    ctx.Code().neg(tmp);
    MovGP(ctx, dest[0], tmp);
}

void EmitIAbs32(EmitContext& ctx, const Operands& dest, const Operands& op) {
    Label done;
    Reg tmp = dest[0].isMEM() ? ctx.TempGPReg(false).cvt32() : dest[0].getReg().cvt32();
    MovGP(ctx, tmp, op[0]);
    ctx.Code().cmp(tmp, 0);
    ctx.Code().jns(done);
    ctx.Code().neg(tmp);
    ctx.Code().L(done);
    MovGP(ctx, dest[0], tmp);
}

void EmitShiftLeftLogical32(EmitContext& ctx, const Operands& dest, const Operands& base, const Operands& shift) {
    bool rcx_saved = EmitSaveRegTemp(ctx, rcx, dest[0]);
    Reg tmp = dest[0].getIdx() == rcx.getIdx() ? ctx.TempGPReg(false).cvt32() : dest[0].getReg().cvt32();
    MovGP(ctx, tmp, base[0]);
    MovGP(ctx, cl, shift[0]);
    ctx.Code().shl(tmp, cl);
    MovGP(ctx, dest[0], tmp);
    if (rcx_saved) {
        EmitRestoreRegTemp(ctx, rcx);
    }
}

void EmitShiftLeftLogical64(EmitContext& ctx, const Operands& dest, const Operands& base, const Operands& shift) {
    bool rcx_saved = EmitSaveRegTemp(ctx, rcx, dest[0]);
    Reg tmp = dest[0].getIdx() == rcx.getIdx() ? ctx.TempGPReg(false) : dest[0].getReg();
    MovGP(ctx, tmp, base[0]);
    MovGP(ctx, cl, shift[0]);
    ctx.Code().shl(tmp, cl);
    MovGP(ctx, dest[0], tmp);
    if (rcx_saved) {
        EmitRestoreRegTemp(ctx, rcx);
    }
}

void EmitShiftRightLogical32(EmitContext& ctx, const Operands& dest, const Operands& base, const Operands& shift) {
    bool rcx_saved = EmitSaveRegTemp(ctx, rcx, dest[0]);
    Reg tmp = dest[0].isMEM() ? ctx.TempGPReg(false).cvt32() : dest[0].getReg().cvt32();
    MovGP(ctx, tmp, base[0]);
    MovGP(ctx, cl, shift[0]);
    ctx.Code().shr(tmp, cl);
    MovGP(ctx, dest[0], tmp);
    if (rcx_saved) {
        EmitRestoreRegTemp(ctx, rcx);
    }
}

void EmitShiftRightLogical64(EmitContext& ctx, const Operands& dest, const Operands& base, const Operands& shift) {
    bool rcx_saved = EmitSaveRegTemp(ctx, rcx, dest[0]);
    Reg tmp = dest[0].isMEM() ? ctx.TempGPReg(false) : dest[0].getReg();
    MovGP(ctx, tmp, base[0]);
    MovGP(ctx, cl, shift[0]);
    ctx.Code().shr(tmp, cl);
    MovGP(ctx, dest[0], tmp);
    if (rcx_saved) {
        EmitRestoreRegTemp(ctx, rcx);
    }
}

void EmitShiftRightArithmetic32(EmitContext& ctx, const Operands& dest, const Operands& base, const Operands& shift) {
    bool rcx_saved = EmitSaveRegTemp(ctx, rcx, dest[0]);
    Reg tmp = dest[0].isMEM() ? ctx.TempGPReg(false).cvt32() : dest[0].getReg().cvt32();
    MovGP(ctx, tmp, base[0]);
    MovGP(ctx, cl, shift[0]);
    ctx.Code().sar(tmp, cl);
    MovGP(ctx, dest[0], tmp);
    if (rcx_saved) {
        EmitRestoreRegTemp(ctx, rcx);
    }
}

void EmitShiftRightArithmetic64(EmitContext& ctx, const Operands& dest, const Operands& base, const Operands& shift) {
    bool rcx_saved = EmitSaveRegTemp(ctx, rcx, dest[0]);
    Reg tmp = dest[0].isMEM() ? ctx.TempGPReg(false) : dest[0].getReg();
    MovGP(ctx, tmp, base[0]);
    MovGP(ctx, cl, shift[0]);
    ctx.Code().sar(tmp, cl);
    MovGP(ctx, dest[0], tmp);
    if (rcx_saved) {
        EmitRestoreRegTemp(ctx, rcx);
    }
}

void EmitBitwiseAnd32(EmitContext& ctx, const Operands& dest, const Operands& op1, const Operands& op2) {
    Reg tmp = dest[0].isMEM() ? ctx.TempGPReg(false).cvt32() : dest[0].getReg().cvt32();
    MovGP(ctx, tmp, op1[0]);
    ctx.Code().and_(tmp, op2[0]);
    MovGP(ctx, dest[0], tmp);
}

void EmitBitwiseAnd64(EmitContext& ctx, const Operands& dest, const Operands& op1, const Operands& op2) {
    Reg tmp = dest[0].isMEM() ? ctx.TempGPReg(false) : dest[0].getReg();
    MovGP(ctx, tmp, op1[0]);
    ctx.Code().and_(tmp, op2[0]);
    MovGP(ctx, dest[0], tmp);
}

void EmitBitwiseOr32(EmitContext& ctx, const Operands& dest, const Operands& op1, const Operands& op2) {
    Reg tmp = dest[0].isMEM() ? ctx.TempGPReg(false).cvt32() : dest[0].getReg().cvt32();
    MovGP(ctx, tmp, op1[0]);
    ctx.Code().or_(tmp, op2[0]);
    MovGP(ctx, dest[0], tmp);
}

void EmitBitwiseOr64(EmitContext& ctx, const Operands& dest, const Operands& op1, const Operands& op2) {
    Reg tmp = dest[0].isMEM() ? ctx.TempGPReg(false) : dest[0].getReg();
    MovGP(ctx, tmp, op1[0]);
    ctx.Code().or_(tmp, op2[0]);
    MovGP(ctx, dest[0], tmp);
}

void EmitBitwiseXor32(EmitContext& ctx, const Operands& dest, const Operands& op1, const Operands& op2) {
    Reg tmp = dest[0].isMEM() ? ctx.TempGPReg(false).cvt32() : dest[0].getReg().cvt32();
    MovGP(ctx, tmp, op1[0]);
    ctx.Code().xor_(tmp, op2[0]);
    MovGP(ctx, dest[0], tmp);
}

void EmitBitFieldInsert(EmitContext& ctx) {
    throw NotImplementedException("BitFieldInsert");
}

void EmitBitFieldSExtract(EmitContext& ctx) {
    throw NotImplementedException("BitFieldSExtract");
}

void EmitBitFieldUExtract(EmitContext& ctx) {
    throw NotImplementedException("BitFieldUExtract");
}

void EmitBitReverse32(EmitContext& ctx) {
    throw NotImplementedException("BitReverse32");
}

void EmitBitCount32(EmitContext& ctx) {
    throw NotImplementedException("BitCount32");
}

void EmitBitCount64(EmitContext& ctx) {
    throw NotImplementedException("BitCount64");
}

void EmitBitwiseNot32(EmitContext& ctx, const Operands& dest, const Operands& op) {
    Reg tmp = dest[0].isMEM() ? ctx.TempGPReg(false).cvt32() : dest[0].getReg().cvt32();
    MovGP(ctx, tmp, op[0]);
    ctx.Code().not_(tmp);
    MovGP(ctx, dest[0], tmp);
}

void EmitFindSMsb32(EmitContext& ctx) {
    throw NotImplementedException("FindSMsb32");
}

void EmitFindUMsb32(EmitContext& ctx) {
    throw NotImplementedException("FindUMsb32");
}

void EmitFindILsb32(EmitContext& ctx) {
    throw NotImplementedException("FindILsb32");
}

void EmitFindILsb64(EmitContext& ctx) {
    throw NotImplementedException("FindILsb64");
}

void EmitSMin32(EmitContext& ctx, const Operands& dest, const Operands& op1, const Operands& op2) {
    Reg tmp = dest[0].isMEM() ? ctx.TempGPReg(false).cvt32() : dest[0].getReg().cvt32();
    MovGP(ctx, tmp, op1[0]);
    ctx.Code().cmp(tmp, op2[0]);
    ctx.Code().cmovg(tmp, op2[0]);
    MovGP(ctx, dest[0], tmp);
}

void EmitUMin32(EmitContext& ctx, const Operands& dest, const Operands& op1, const Operands& op2) {
    Reg tmp = dest[0].isMEM() ? ctx.TempGPReg(false).cvt32() : dest[0].getReg().cvt32();
    MovGP(ctx, tmp, op1[0]);
    ctx.Code().cmp(tmp, op2[0]);
    ctx.Code().cmova(tmp, op2[0]);
    MovGP(ctx, dest[0], tmp);
}

void EmitSMax32(EmitContext& ctx, const Operands& dest, const Operands& op1, const Operands& op2) {
    Reg tmp = dest[0].isMEM() ? ctx.TempGPReg(false).cvt32() : dest[0].getReg().cvt32();
    MovGP(ctx, tmp, op1[0]);
    ctx.Code().cmp(tmp, op2[0]);
    ctx.Code().cmovl(tmp, op2[0]);
    MovGP(ctx, dest[0], tmp);
}

void EmitUMax32(EmitContext& ctx, const Operands& dest, const Operands& op1, const Operands& op2) {
    Reg tmp = dest[0].isMEM() ? ctx.TempGPReg(false).cvt32() : dest[0].getReg().cvt32();
    MovGP(ctx, tmp, op1[0]);
    ctx.Code().cmp(tmp, op2[0]);
    ctx.Code().cmovb(tmp, op2[0]);
    MovGP(ctx, dest[0], tmp);
}

void EmitSClamp32(EmitContext& ctx, const Operands& dest, const Operands& value, const Operands& min, const Operands& max) {
    Reg tmp = dest[0].isMEM() ? ctx.TempGPReg(false).cvt32() : dest[0].getReg().cvt32();
    MovGP(ctx, tmp, value[0]);
    ctx.Code().cmp(tmp, min[0]);
    ctx.Code().cmovl(tmp, min[0]);
    ctx.Code().cmp(tmp, max[0]);
    ctx.Code().cmovg(tmp, max[0]);
    MovGP(ctx, dest[0], tmp);
}

void EmitUClamp32(EmitContext& ctx, const Operands& dest, const Operands& value, const Operands& min, const Operands& max) {
    Reg tmp = dest[0].isMEM() ? ctx.TempGPReg(false).cvt32() : dest[0].getReg().cvt32();
    MovGP(ctx, tmp, value[0]);
    ctx.Code().cmp(tmp, min[0]);
    ctx.Code().cmovb(tmp, min[0]);
    ctx.Code().cmp(tmp, max[0]);
    ctx.Code().cmova(tmp, max[0]);
    MovGP(ctx, dest[0], tmp);
}

void EmitSLessThan32(EmitContext& ctx, const Operands& dest, const Operands& lhs, const Operands& rhs) {
    Reg tmp = lhs[0].isMEM() && rhs[0].isMEM() ? ctx.TempGPReg(false).cvt32() : lhs[0].getReg().cvt32();
    MovGP(ctx, tmp, lhs[0]);
    ctx.Code().cmp(tmp, rhs[0]);
    ctx.Code().setl(dest[0]);
}

void EmitSLessThan64(EmitContext& ctx, const Operands& dest, const Operands& lhs, const Operands& rhs) {
    Reg tmp = lhs[0].isMEM() && rhs[0].isMEM() ? ctx.TempGPReg(false) : lhs[0].getReg();
    MovGP(ctx, tmp, lhs[0]);
    ctx.Code().cmp(tmp, rhs[0]);
    ctx.Code().setl(dest[0]);
}

void EmitULessThan32(EmitContext& ctx, const Operands& dest, const Operands& lhs, const Operands& rhs) {
    Reg tmp = lhs[0].isMEM() && rhs[0].isMEM() ? ctx.TempGPReg(false).cvt32() : lhs[0].getReg().cvt32();
    MovGP(ctx, tmp, lhs[0]);
    ctx.Code().cmp(tmp, rhs[0]);
    ctx.Code().setb(dest[0]);
}

void EmitULessThan64(EmitContext& ctx, const Operands& dest, const Operands& lhs, const Operands& rhs) {
    Reg tmp = lhs[0].isMEM() && rhs[0].isMEM() ? ctx.TempGPReg(false) : lhs[0].getReg();
    MovGP(ctx, tmp, lhs[0]);
    ctx.Code().cmp(tmp, rhs[0]);
    ctx.Code().setb(dest[0]);
}

void EmitIEqual32(EmitContext& ctx, const Operands& dest, const Operands& lhs, const Operands& rhs) {
    Reg tmp = lhs[0].isMEM() && rhs[0].isMEM() ? ctx.TempGPReg(false).cvt32() : lhs[0].getReg().cvt32();
    MovGP(ctx, tmp, lhs[0]);
    ctx.Code().cmp(tmp, rhs[0]);
    ctx.Code().sete(dest[0]);
}

void EmitIEqual64(EmitContext& ctx, const Operands& dest, const Operands& lhs, const Operands& rhs) {
    Reg tmp = lhs[0].isMEM() && rhs[0].isMEM() ? ctx.TempGPReg(false) : lhs[0].getReg();
    MovGP(ctx, tmp, lhs[0]);
    ctx.Code().cmp(tmp, rhs[0]);
    ctx.Code().sete(dest[0]);
}

void EmitSLessThanEqual(EmitContext& ctx, const Operands& dest, const Operands& lhs, const Operands& rhs) {
    Reg tmp = lhs[0].isMEM() && rhs[0].isMEM() ? ctx.TempGPReg(false).cvt32() : lhs[0].getReg().cvt32();
    MovGP(ctx, tmp, lhs[0]);
    ctx.Code().cmp(tmp, rhs[0]);
    ctx.Code().setle(dest[0]);
}

void EmitULessThanEqual(EmitContext& ctx, const Operands& dest, const Operands& lhs, const Operands& rhs) {
    Reg tmp = lhs[0].isMEM() && rhs[0].isMEM() ? ctx.TempGPReg(false).cvt32() : lhs[0].getReg().cvt32();
    MovGP(ctx, tmp, lhs[0]);
    ctx.Code().cmp(tmp, rhs[0]);
    ctx.Code().setbe(dest[0]);
}

void EmitSGreaterThan(EmitContext& ctx, const Operands& dest, const Operands& lhs, const Operands& rhs) {
    Reg tmp = lhs[0].isMEM() && rhs[0].isMEM() ? ctx.TempGPReg(false).cvt32() : lhs[0].getReg().cvt32();
    MovGP(ctx, tmp, lhs[0]);
    ctx.Code().cmp(tmp, rhs[0]);
    ctx.Code().setg(dest[0]);
}

void EmitUGreaterThan(EmitContext& ctx, const Operands& dest, const Operands& lhs, const Operands& rhs) {
    Reg tmp = lhs[0].isMEM() && rhs[0].isMEM() ? ctx.TempGPReg(false).cvt32() : lhs[0].getReg().cvt32();
    MovGP(ctx, tmp, lhs[0]);
    ctx.Code().cmp(tmp, rhs[0]);
    ctx.Code().seta(dest[0]);
}

void EmitINotEqual32(EmitContext& ctx, const Operands& dest, const Operands& lhs, const Operands& rhs) {
    Reg tmp = lhs[0].isMEM() && rhs[0].isMEM() ? ctx.TempGPReg(false).cvt32() : lhs[0].getReg().cvt32();
    MovGP(ctx, tmp, lhs[0]);
    ctx.Code().cmp(tmp, rhs[0]);
    ctx.Code().setne(dest[0]);
}

void EmitINotEqual64(EmitContext& ctx, const Operands& dest, const Operands& lhs, const Operands& rhs) {
    Reg tmp = lhs[0].isMEM() && rhs[0].isMEM() ? ctx.TempGPReg(false) : lhs[0].getReg();
    MovGP(ctx, tmp, lhs[0]);
    ctx.Code().cmp(tmp, rhs[0]);
    ctx.Code().setne(dest[0]);
}

void EmitSGreaterThanEqual(EmitContext& ctx, const Operands& dest, const Operands& lhs, const Operands& rhs) {
    Reg tmp = lhs[0].isMEM() && rhs[0].isMEM() ? ctx.TempGPReg(false).cvt32() : lhs[0].getReg().cvt32();
    MovGP(ctx, tmp, lhs[0]);
    ctx.Code().cmp(tmp, rhs[0]);
    ctx.Code().setge(dest[0]);
}

void EmitUGreaterThanEqual(EmitContext& ctx, const Operands& dest, const Operands& lhs, const Operands& rhs) {
    Reg tmp = lhs[0].isMEM() && rhs[0].isMEM() ? ctx.TempGPReg(false).cvt32() : lhs[0].getReg().cvt32();
    MovGP(ctx, tmp, lhs[0]);
    ctx.Code().cmp(tmp, rhs[0]);
    ctx.Code().setae(dest[0]);
}

} // namespace Shader::Backend::X64