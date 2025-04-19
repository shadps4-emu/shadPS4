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
    Reg tmp = ctx.TempGPReg();
    ctx.Code().lea(tmp, ptr[ctx.UserData() + offset]);
    MovGP( ctx, dest[0], dword[tmp]);
}

void EmitSetUserData(EmitContext& ctx, const Operands& offset, const Operands& value) {
    Reg tmp = ctx.TempGPReg();
    MovGP(ctx, tmp, offset[0]);
    ctx.Code().lea(tmp, ptr[ctx.UserData() + tmp * 4]);
    MovGP(ctx, dword[tmp], value[0]);
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
    Reg tmp = dest[0].IsMem() ? ctx.TempGPReg() : dest[0].Reg().changeBit(64);
    Reg off_tmp = offset[0].IsMem() ? ctx.TempGPReg() : offset[0].Reg().changeBit(64);
    MovGP(ctx, tmp, base[1]);
    MovGP(ctx, off_tmp, offset[0]);
    ctx.Code().shl(tmp, 32);
    ctx.Code().or_(tmp, base[0].Op());
    ctx.Code().lea(tmp, ptr[tmp + off_tmp * 4]);
    MovGP(ctx, dest[0], dword[tmp]);
}

void EmitReadConstBuffer(EmitContext& ctx, const Operands& dest, const Operands& handle, const Operands& offset) {
    Reg tmp = dest[0].IsMem() ? ctx.TempGPReg() : dest[0].Reg().changeBit(64);
    // Reconstruct base address
    Reg off_tmp = ctx.TempGPReg();
    MovGP(ctx, tmp, handle[1]);
    ctx.Code().and_(tmp, 0xFFF);
    ctx.Code().shl(tmp, 32);
    MovGP(ctx, off_tmp.cvt32(), handle[0]);
    ctx.Code().and_(off_tmp.cvt32(), 0xFFFFFFFF);
    ctx.Code().or_(tmp, off_tmp);
    // TODO: we should correctly clamp the offset
    MovGP(ctx, off_tmp, offset[0]);
    ctx.Code().lea(tmp, ptr[tmp + off_tmp * 4]);
    MovGP(ctx, dest[0], dword[tmp]);

}

void EmitReadStepRate(EmitContext& ctx) {
    throw NotImplementedException("ReadStepRate");
}

void EmitGetAttribute(EmitContext& ctx, const Operands& dest) {
    LOG_WARNING(Render_Recompiler, "GetAttribute stubbed, setting to 0.0");
    if (dest[0].IsMem()) {
        ctx.Code().mov(dest[0].Mem(), 0);
    } else {
        ctx.Code().pxor(dest[0].Xmm(), dest[0].Xmm());
    }
}

void EmitGetAttributeU32(EmitContext& ctx, const Operands& dest) {
    LOG_WARNING(Render_Recompiler, "GetAttributeU32 stubbed, setting to 0");
    if (dest[0].IsMem()) {
        ctx.Code().mov(dest[0].Mem(), 0);
    } else {
        ctx.Code().xor_(dest[0].Reg(), dest[0].Reg());
    }
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

} // namespace Shader::Backend::X64