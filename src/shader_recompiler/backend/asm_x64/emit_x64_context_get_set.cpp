// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/exception.h"
#include "shader_recompiler/backend/asm_x64/x64_emit_context.h" 
#include "shader_recompiler/backend/asm_x64/x64_utils.h"

namespace Shader::Backend::X64 {

using namespace Xbyak;
using namespace Xbyak::util;

void EmitGetUserData(EmitContext& ctx, const Operands& dest, IR::ScalarReg reg) {
    const u32 offset = static_cast<u32>(reg) << 2;
    Reg& tmp = ctx.TempGPReg();
    ctx.Code().lea(tmp, ptr[ctx.UserData() + offset]);
    MovGP( ctx, dest[0], ptr[tmp]);
}

void EmitSetUserData(EmitContext& ctx, const Operands& offset, const Operands& value) {
    Reg& tmp = ctx.TempGPReg();
    ctx.Code().mov(tmp, offset[0]);
    ctx.Code().shl(tmp, 2);
    ctx.Code().lea(tmp, ptr[ctx.UserData() + tmp]);
    MovGP(ctx, ptr[tmp], value[0]);
}

void EmitGetThreadBitScalarReg(EmitContext& ctx) {
    UNREACHABLE_MSG("Unreachable instruction");
}

void EmitSetThreadBitScalarReg(EmitContext& ctx) {
    UNREACHABLE_MSG("Unreachable instruction");
}

void EmitGetScalarRegister(EmitContext&) {
    UNREACHABLE_MSG("Unreachable instruction");
}

void EmitSetScalarRegister(EmitContext&) {
    UNREACHABLE_MSG("Unreachable instruction");
}

void EmitGetVectorRegister(EmitContext& ctx) {
    UNREACHABLE_MSG("Unreachable instruction");
}

void EmitSetVectorRegister(EmitContext& ctx) {
    UNREACHABLE_MSG("Unreachable instruction");
}

void EmitSetGotoVariable(EmitContext&) {
    UNREACHABLE_MSG("Unreachable instruction");
}

void EmitGetGotoVariable(EmitContext&) {
    UNREACHABLE_MSG("Unreachable instruction");
}

void EmitReadConst(EmitContext& ctx, const Operands& dest, const Operands& base, const Operands& offset) {
    Reg& tmp = ctx.TempGPReg(false);
    ctx.Code().mov(tmp, base[1]);
    ctx.Code().shl(tmp, 32);
    ctx.Code().or_(tmp, base[0]);
    if (offset[0].isMEM()) {
        ctx.Code().add(tmp, offset[0]);
    } else {
        ctx.Code().lea(tmp, ptr[tmp + offset[0].getReg()]);
    }
    MovGP(ctx, dest[0], ptr[tmp]);
}

void EmitReadConstBuffer(EmitContext& ctx) {
    throw NotImplementedException("ReadConstBuffer");
}

void EmitReadStepRate(EmitContext& ctx) {
    throw NotImplementedException("ReadStepRate");
}

void EmitGetAttribute(EmitContext& ctx) {
    throw NotImplementedException("GetAttribute");
}

void EmitGetAttributeU32(EmitContext& ctx) {
    throw NotImplementedException("GetAttributeU32");
}

void EmitSetAttribute(EmitContext& ctx) {
    throw NotImplementedException("SetAttribute");
}

void EmitGetTessGenericAttribute(EmitContext& ctx) {
    throw NotImplementedException("GetTessGenericAttribute");
}

void EmitReadTcsGenericOuputAttribute(EmitContext& ctx) {
    throw NotImplementedException("ReadTcsGenericOuputAttribute");
}

void EmitSetTcsGenericAttribute(EmitContext& ctx) {
    throw NotImplementedException("SetTcsGenericAttribute");
}

void EmitGetPatch(EmitContext& ctx) {
    throw NotImplementedException("GetPatch");
}

void EmitSetPatch(EmitContext& ctx) {
    throw NotImplementedException("SetPatch");
}

void EmitLoadBufferU8(EmitContext& ctx) {
    throw NotImplementedException("LoadBufferU8");
}

void EmitLoadBufferU16(EmitContext& ctx) {
    throw NotImplementedException("LoadBufferU16");
}

void EmitLoadBufferU32(EmitContext& ctx) {
    throw NotImplementedException("LoadBufferU32");
}

void EmitLoadBufferU32x2(EmitContext& ctx) {
    throw NotImplementedException("LoadBufferU32x2");
}

void EmitLoadBufferU32x3(EmitContext& ctx) {
    throw NotImplementedException("LoadBufferU32x3");
}

void EmitLoadBufferU32x4(EmitContext& ctx) {
    throw NotImplementedException("LoadBufferU32x4");
}

void EmitLoadBufferF32(EmitContext& ctx) {
    throw NotImplementedException("LoadBufferF32");
}

void EmitLoadBufferF32x2(EmitContext& ctx) {
    throw NotImplementedException("LoadBufferF32x2");
}

void EmitLoadBufferF32x3(EmitContext& ctx) {
    throw NotImplementedException("LoadBufferF32x3");
}

void EmitLoadBufferF32x4(EmitContext& ctx) {
    throw NotImplementedException("LoadBufferF32x4");
}

void EmitLoadBufferFormatF32(EmitContext& ctx) {
    throw NotImplementedException("LoadBufferFormatF32");
}

void EmitStoreBufferU8(EmitContext& ctx) {
    throw NotImplementedException("StoreBufferU8");
}

void EmitStoreBufferU16(EmitContext& ctx) {
    throw NotImplementedException("StoreBufferU16");
}

void EmitStoreBufferU32(EmitContext& ctx) {
    throw NotImplementedException("StoreBufferU32");
}

void EmitStoreBufferU32x2(EmitContext& ctx) {
    throw NotImplementedException("StoreBufferU32x2");
}

void EmitStoreBufferU32x3(EmitContext& ctx) {
    throw NotImplementedException("StoreBufferU32x3");
}

void EmitStoreBufferU32x4(EmitContext& ctx) {
    throw NotImplementedException("StoreBufferU32x4");
}

void EmitStoreBufferF32(EmitContext& ctx) {
    throw NotImplementedException("StoreBufferF32");
}

void EmitStoreBufferF32x2(EmitContext& ctx) {
    throw NotImplementedException("StoreBufferF32x2");
}

void EmitStoreBufferF32x3(EmitContext& ctx) {
    throw NotImplementedException("StoreBufferF32x3");
}

void EmitStoreBufferF32x4(EmitContext& ctx) {
    throw NotImplementedException("StoreBufferF32x4");
}

void EmitStoreBufferFormatF32(EmitContext& ctx) {
    throw NotImplementedException("StoreBufferFormatF32");
}

}