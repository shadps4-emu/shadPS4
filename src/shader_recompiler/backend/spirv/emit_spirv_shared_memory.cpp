// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/backend/spirv/emit_spirv_instructions.h"
#include "shader_recompiler/backend/spirv/spirv_emit_context.h"
#include "shader_recompiler/ir/microinstruction.h"
#include "shader_recompiler/ir/shared_memory.h"

namespace Shader::Backend::SPIRV {

using PointerType = EmitContext::PointerType;
using SharedFlags = SharedMemory::Flags;

Id EmitLoadSharedU16(EmitContext& ctx, IR::Inst* inst, Id offset) {
    bool is_scratch = inst->Flags<SharedFlags>().is_scratch.Value();
    const Id pointer = ctx.EmitSharedMemoryAccess(offset, PointerType::U16, is_scratch);
    return ctx.OpLoad(ctx.U16, pointer);
}

Id EmitLoadSharedU32(EmitContext& ctx, IR::Inst* inst, Id offset) {
    bool is_scratch = inst->Flags<SharedFlags>().is_scratch.Value();
    const Id pointer = ctx.EmitSharedMemoryAccess(offset, PointerType::U32, is_scratch);
    return ctx.OpLoad(ctx.U32[1], pointer);
}

Id EmitLoadSharedU64(EmitContext& ctx, IR::Inst* inst, Id offset) {
    bool is_scratch = inst->Flags<SharedFlags>().is_scratch.Value();
    const Id pointer = ctx.EmitSharedMemoryAccess(offset, PointerType::U64, is_scratch);
    return ctx.OpLoad(ctx.U64, pointer);
}

void EmitWriteSharedU16(EmitContext& ctx, IR::Inst* inst, Id offset, Id value) {
    bool is_scratch = inst->Flags<SharedFlags>().is_scratch.Value();
    const Id pointer = ctx.EmitSharedMemoryAccess(offset, PointerType::U16, is_scratch);
    ctx.OpStore(pointer, value);
}

void EmitWriteSharedU32(EmitContext& ctx, IR::Inst* inst, Id offset, Id value) {
    bool is_scratch = inst->Flags<SharedFlags>().is_scratch.Value();
    const Id pointer = ctx.EmitSharedMemoryAccess(offset, PointerType::U32, is_scratch);
    ctx.OpStore(pointer, value);
}

void EmitWriteSharedU64(EmitContext& ctx, IR::Inst* inst, Id offset, Id value) {
    bool is_scratch = inst->Flags<SharedFlags>().is_scratch.Value();
    const Id pointer = ctx.EmitSharedMemoryAccess(offset, PointerType::U64, is_scratch);
    ctx.OpStore(pointer, value);
}

} // namespace Shader::Backend::SPIRV
