// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/exception.h"
#include "shader_recompiler/backend/asm_x64/x64_emit_context.h"
#include "shader_recompiler/backend/asm_x64/x64_utils.h"

using namespace Xbyak;
using namespace Xbyak::util;

namespace Shader::Backend::X64 {

void EmitBitCastU16F16(EmitContext& ctx, const Operands& dest, const Operands& src) {
    // We handle 16-bit floats in general purpose registers
    MovGP(ctx, dest[0], src[0]);
}

void EmitBitCastU32F32(EmitContext& ctx, const Operands& dest, const Operands& src) {
    if (src[0].isMEM()) {
        MovGP(ctx, dest[0], src[0]);
    } else if (dest[0].isMEM()) {
        ctx.Code().movd(dest[0].getAddress(), src[0].getReg().cvt128());
    } else {
        ctx.Code().movd(dword[rsp - 4], src[0].getReg().cvt128());
        MovGP(ctx, dest[0], dword[rsp - 4]);
    }
}

void EmitBitCastU64F64(EmitContext& ctx, const Operands& dest, const Operands& src) {
    if (src[0].isMEM()) {
        MovGP(ctx, dest[0], src[0]);
    } else if (dest[0].isMEM()) {
        ctx.Code().movq(dest[0].getAddress(), src[0].getReg().cvt128());
    } else {
        ctx.Code().movq(qword[rsp - 8], src[0].getReg().cvt128());
        MovGP(ctx, dest[0], qword[rsp - 8]);
    }
}

void EmitBitCastF16U16(EmitContext& ctx, const Operands& dest, const Operands& src) {
    MovGP(ctx, dest[0], src[0]);
}

void EmitBitCastF32U32(EmitContext& ctx, const Operands& dest, const Operands& src) {
    if (dest[0].isMEM()) {
        MovGP(ctx, dest[0], src[0]);
    } else if (src[0].isMEM()) {
        ctx.Code().movd(dest[0].getReg().cvt128(), src[0].getAddress());
    } else {
        MovGP(ctx, dword[rsp - 4], src[0]);
        ctx.Code().movd(dest[0].getReg().cvt128(), dword[rsp - 4]);
    }
}

void EmitBitCastF64U64(EmitContext& ctx, const Operands& dest, const Operands& src) {
    if (dest[0].isMEM()) {
        MovGP(ctx, dest[0], src[0]);
    } else if (src[0].isMEM()) {
        ctx.Code().movq(dest[0].getReg().cvt128(), src[0].getAddress());
    } else {
        MovGP(ctx, qword[rsp - 8], src[0]);
        ctx.Code().mov(dest[0].getReg().cvt128(), qword[rsp - 8]);
    }
}

void EmitPackUint2x32(EmitContext& ctx, const Operands& dest, const Operands& src) {
    const bool is_mem = dest[0].isMEM() && (src[0].isMEM() || src[1].isMEM());
    Reg tmp = is_mem ? ctx.TempGPReg(false) : dest[0].getReg();
    MovGP(ctx, tmp, src[1]);
    ctx.Code().shl(tmp, 32);
    ctx.Code().or_(tmp, src[0]);
    MovGP(ctx, dest[0], tmp);
}

void EmitUnpackUint2x32(EmitContext& ctx, const Operands& dest, const Operands& src) {
    Reg src0 = src[0].isMEM() ? ctx.TempGPReg() : src[0].getReg();
    MovGP(ctx, src0, src[0]);
    Reg dest1 = dest[1].isMEM() ? ctx.TempGPReg(false) : dest[1].getReg().changeBit(64);
    MovGP(ctx, dest1, src0);
    ctx.Code().shr(dest1, 32);
    MovGP(ctx, dest[1], dest1);
    MovGP(ctx, dest[0], src0.cvt32());
}

void EmitPackFloat2x32(EmitContext& ctx, const Operands& dest, const Operands& src) {
    Xmm tmp = dest[0].isMEM() ? ctx.TempXmmReg(false) : dest[0].getReg().cvt128();
    MovFloat(ctx, tmp, src[0]);
    ctx.Code().pinsrd(tmp, src[1], 1);
    MovFloat(ctx, dest[0], tmp);
}

void EmitPackUnorm2x16(EmitContext& ctx) {
    throw NotImplementedException("PackUnorm2x16");
}

void EmitUnpackUnorm2x16(EmitContext& ctx) {
    throw NotImplementedException("UnpackUnorm2x16");
}

void EmitPackSnorm2x16(EmitContext& ctx) {
    throw NotImplementedException("PackSnorm2x16");
}

void EmitUnpackSnorm2x16(EmitContext& ctx) {
    throw NotImplementedException("UnpackSnorm2x16");
}

void EmitPackUint2x16(EmitContext& ctx) {
    throw NotImplementedException("PackUint2x16");
}

void EmitUnpackUint2x16(EmitContext& ctx) {
    throw NotImplementedException("UnpackUint2x16");
}

void EmitPackSint2x16(EmitContext& ctx) {
    throw NotImplementedException("PackSint2x16");
}

void EmitUnpackSint2x16(EmitContext& ctx) {
    throw NotImplementedException("UnpackSint2x16");
}

void EmitPackHalf2x16(EmitContext& ctx) {
    throw NotImplementedException("PackHalf2x16");
}

void EmitUnpackHalf2x16(EmitContext& ctx) {
    throw NotImplementedException("UnpackHalf2x16");
}

void EmitPackUnorm4x8(EmitContext& ctx) {
    throw NotImplementedException("PackUnorm4x8");
}

void EmitUnpackUnorm4x8(EmitContext& ctx) {
    throw NotImplementedException("UnpackUnorm4x8");
}

void EmitPackSnorm4x8(EmitContext& ctx) {
    throw NotImplementedException("PackSnorm4x8");
}

void EmitUnpackSnorm4x8(EmitContext& ctx) {
    throw NotImplementedException("UnpackSnorm4x8");
}

void EmitPackUint4x8(EmitContext& ctx) {
    throw NotImplementedException("PackUint4x8");
}

void EmitUnpackUint4x8(EmitContext& ctx) {
    throw NotImplementedException("UnpackUint4x8");
}

void EmitPackSint4x8(EmitContext& ctx) {
    throw NotImplementedException("PackSint4x8");
}

void EmitUnpackSint4x8(EmitContext& ctx) {
    throw NotImplementedException("UnpackSint4x8");
}

void EmitPackUfloat10_11_11(EmitContext& ctx) {
    throw NotImplementedException("PackUfloat10_11_11");
}

void EmitUnpackUfloat10_11_11(EmitContext& ctx) {
    throw NotImplementedException("UnpackUfloat10_11_11");
}

void EmitPackUnorm2_10_10_10(EmitContext& ctx) {
    throw NotImplementedException("PackUnorm2_10_10_10");
}

void EmitUnpackUnorm2_10_10_10(EmitContext& ctx) {
    throw NotImplementedException("UnpackUnorm2_10_10_10");
}

void EmitPackSnorm2_10_10_10(EmitContext& ctx) {
    throw NotImplementedException("PackSnorm2_10_10_10");
}

void EmitUnpackSnorm2_10_10_10(EmitContext& ctx) {
    throw NotImplementedException("UnpackSnorm2_10_10_10");
}

void EmitPackUint2_10_10_10(EmitContext& ctx) {
    throw NotImplementedException("PackUint2_10_10_10");
}

void EmitUnpackUint2_10_10_10(EmitContext& ctx) {
    throw NotImplementedException("UnpackUint2_10_10_10");
}

void EmitPackSint2_10_10_10(EmitContext& ctx) {
    throw NotImplementedException("PackSint2_10_10_10");
}

void EmitUnpackSint2_10_10_10(EmitContext& ctx) {
    throw NotImplementedException("UnpackSint2_10_10_10");
}

}