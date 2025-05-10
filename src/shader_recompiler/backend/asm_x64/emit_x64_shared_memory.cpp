// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/exception.h"
#include "shader_recompiler/backend/asm_x64/x64_emit_context.h"

namespace Shader::Backend::X64 {

void EmitLoadSharedU32(EmitContext& ctx, const Operands& dest, const Operands& offset) {
    LOG_WARNING(Render_Recompiler, "EmitLoadSharedU32 stubbed, setting to 0");
    if (dest[0].IsMem()) {
        ctx.Code().mov(dest[0].Mem(), 0);
    } else {
        ctx.Code().xor_(dest[0].Reg(), dest[0].Reg());
    }
}

void EmitLoadSharedU64(EmitContext& ctx, const Operands& dest, const Operands& offset) {
    LOG_WARNING(Render_Recompiler, "EmitLoadSharedU64 stubbed, setting to 0");
    if (dest[0].IsMem()) {
        ctx.Code().mov(dest[0].Mem(), 0);
    } else {
        ctx.Code().xor_(dest[0].Reg(), dest[0].Reg());
    }
    if (dest[1].IsMem()) {
        ctx.Code().mov(dest[1].Mem(), 0);
    } else {
        ctx.Code().xor_(dest[1].Reg(), dest[1].Reg());
    }
}

void EmitWriteSharedU32(EmitContext& ctx) {
    throw NotImplementedException("WriteSharedU32");
}

void EmitWriteSharedU64(EmitContext& ctx) {
    throw NotImplementedException("WriteSharedU64");
}
}