// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/backend/spirv/emit_spirv_instructions.h"
#include "shader_recompiler/backend/spirv/spirv_emit_context.h"

namespace Shader::Backend::SPIRV {

using PointerType = EmitContext::PointerType;
using SharedMemory::Section;
using SharedFlags = SharedMemory::Flags;

Id EmitLoadSharedU16(EmitContext& ctx, IR::Inst* inst, Id offset) {
    Section section = inst->Flags<SharedFlags>().section.Value();
    const Id pointer = ctx.EmitSharedMemoryAccess(offset, PointerType::U16, section);
    return ctx.OpLoad(ctx.U16, pointer);
}

Id EmitLoadSharedU32(EmitContext& ctx, IR::Inst* inst, Id offset) {
    Section section = inst->Flags<SharedFlags>().section.Value();
    const Id pointer = ctx.EmitSharedMemoryAccess(offset, PointerType::U32, section);
    return ctx.OpLoad(ctx.U32[1], pointer);
}

Id EmitLoadSharedU64(EmitContext& ctx, IR::Inst* inst, Id offset) {
    Section section = inst->Flags<SharedFlags>().section.Value();
    const Id pointer = ctx.EmitSharedMemoryAccess(offset, PointerType::U64, section);
    return ctx.OpLoad(ctx.U64, pointer);
}

void EmitWriteSharedU16(EmitContext& ctx, IR::Inst* inst, Id offset, Id value) {
    Section section = inst->Flags<SharedFlags>().section.Value();
    const Id pointer = ctx.EmitSharedMemoryAccess(offset, PointerType::U16, section);
    ctx.OpStore(pointer, value);
}

void EmitWriteSharedU32(EmitContext& ctx, IR::Inst* inst, Id offset, Id value) {
    Section section = inst->Flags<SharedFlags>().section.Value();
    const Id pointer = ctx.EmitSharedMemoryAccess(offset, PointerType::U32, section);
    ctx.OpStore(pointer, value);
}

void EmitWriteSharedU64(EmitContext& ctx, IR::Inst* inst, Id offset, Id value) {
    Section section = inst->Flags<SharedFlags>().section.Value();
    const Id pointer = ctx.EmitSharedMemoryAccess(offset, PointerType::U64, section);
    ctx.OpStore(pointer, value);
}

void EmitPredicatedWriteSharedU32(EmitContext& ctx, IR::Inst* inst, Id offset, Id value, Id cond) {
    Section section = inst->Flags<SharedFlags>().section.Value();
    const Id if_label{ctx.OpLabel()};
    const Id merge_label{ctx.OpLabel()};
    ctx.OpSelectionMerge(merge_label, spv::SelectionControlMask::MaskNone);
    ctx.OpBranchConditional(cond, if_label, merge_label);
    ctx.AddLabel(if_label);
    const Id pointer{ctx.EmitSharedMemoryAccess(offset, PointerType::U32, section)};
    ctx.OpStore(pointer, value);
    ctx.OpBranch(merge_label);
    ctx.AddLabel(merge_label);
}

} // namespace Shader::Backend::SPIRV
