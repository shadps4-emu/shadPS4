// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/backend/spirv/emit_spirv_instructions.h"
#include "shader_recompiler/backend/spirv/spirv_emit_context.h"

namespace Shader::Backend::SPIRV {
namespace {
std::pair<Id, Id> AtomicArgs(EmitContext& ctx) {
    const Id scope{ctx.ConstU32(static_cast<u32>(spv::Scope::Device))};
    const Id semantics{ctx.u32_zero_value};
    return {scope, semantics};
}

Id SharedAtomicU32(EmitContext& ctx, Id offset, Id value,
                   Id (Sirit::Module::*atomic_func)(Id, Id, Id, Id, Id)) {
    const Id shift_id{ctx.ConstU32(2U)};
    const Id index{ctx.OpShiftRightArithmetic(ctx.U32[1], offset, shift_id)};
    const Id pointer{ctx.OpAccessChain(ctx.shared_u32, ctx.shared_memory_u32, index)};
    const auto [scope, semantics]{AtomicArgs(ctx)};
    return (ctx.*atomic_func)(ctx.U32[1], pointer, scope, semantics, value);
}

Id SharedAtomicU32_IncDec(EmitContext& ctx, Id offset,
                          Id (Sirit::Module::*atomic_func)(Id, Id, Id, Id)) {
    const Id shift_id{ctx.ConstU32(2U)};
    const Id index{ctx.OpShiftRightArithmetic(ctx.U32[1], offset, shift_id)};
    const Id pointer{ctx.OpAccessChain(ctx.shared_u32, ctx.shared_memory_u32, index)};
    const auto [scope, semantics]{AtomicArgs(ctx)};
    return (ctx.*atomic_func)(ctx.U32[1], pointer, scope, semantics);
}

Id BufferAtomicU32BoundsCheck(EmitContext& ctx, Id index, Id buffer_size, auto emit_func) {
    if (Sirit::ValidId(buffer_size)) {
        // Bounds checking enabled, wrap in a conditional branch to make sure that
        // the atomic is not mistakenly executed when the index is out of bounds.
        const Id in_bounds = ctx.OpULessThan(ctx.U1[1], index, buffer_size);
        const Id ib_label = ctx.OpLabel();
        const Id oob_label = ctx.OpLabel();
        const Id end_label = ctx.OpLabel();
        ctx.OpSelectionMerge(end_label, spv::SelectionControlMask::MaskNone);
        ctx.OpBranchConditional(in_bounds, ib_label, oob_label);
        ctx.AddLabel(ib_label);
        const Id ib_result = emit_func();
        ctx.OpBranch(end_label);
        ctx.AddLabel(oob_label);
        const Id oob_result = ctx.u32_zero_value;
        ctx.OpBranch(end_label);
        ctx.AddLabel(end_label);
        return ctx.OpPhi(ctx.U32[1], ib_result, ib_label, oob_result, oob_label);
    }
    // Bounds checking not enabled, just perform the atomic operation.
    return emit_func();
}

Id BufferAtomicU32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address, Id value,
                   Id (Sirit::Module::*atomic_func)(Id, Id, Id, Id, Id)) {
    const auto& buffer = ctx.buffers[handle];
    if (Sirit::ValidId(buffer.offset)) {
        address = ctx.OpIAdd(ctx.U32[1], address, buffer.offset);
    }
    const Id index = ctx.OpShiftRightLogical(ctx.U32[1], address, ctx.ConstU32(2u));
    const auto [id, pointer_type] = buffer[EmitContext::BufferAlias::U32];
    const Id ptr = ctx.OpAccessChain(pointer_type, id, ctx.u32_zero_value, index);
    const auto [scope, semantics]{AtomicArgs(ctx)};
    return BufferAtomicU32BoundsCheck(ctx, index, buffer.size_dwords, [&] {
        return (ctx.*atomic_func)(ctx.U32[1], ptr, scope, semantics, value);
    });
}

Id ImageAtomicU32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id coords, Id value,
                  Id (Sirit::Module::*atomic_func)(Id, Id, Id, Id, Id)) {
    const auto& texture = ctx.images[handle & 0xFFFF];
    const Id pointer{ctx.OpImageTexelPointer(ctx.image_u32, texture.id, coords, ctx.ConstU32(0U))};
    const auto [scope, semantics]{AtomicArgs(ctx)};
    return (ctx.*atomic_func)(ctx.U32[1], pointer, scope, semantics, value);
}

Id ImageAtomicF32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id coords, Id value,
                  Id (Sirit::Module::*atomic_func)(Id, Id, Id, Id, Id)) {
    const auto& texture = ctx.images[handle & 0xFFFF];
    const Id pointer{ctx.OpImageTexelPointer(ctx.image_f32, texture.id, coords, ctx.ConstU32(0U))};
    const auto [scope, semantics]{AtomicArgs(ctx)};
    return (ctx.*atomic_func)(ctx.F32[1], pointer, scope, semantics, value);
}
} // Anonymous namespace

Id EmitSharedAtomicIAdd32(EmitContext& ctx, Id offset, Id value) {
    return SharedAtomicU32(ctx, offset, value, &Sirit::Module::OpAtomicIAdd);
}

Id EmitSharedAtomicUMax32(EmitContext& ctx, Id offset, Id value) {
    return SharedAtomicU32(ctx, offset, value, &Sirit::Module::OpAtomicUMax);
}

Id EmitSharedAtomicSMax32(EmitContext& ctx, Id offset, Id value) {
    return SharedAtomicU32(ctx, offset, value, &Sirit::Module::OpAtomicSMax);
}

Id EmitSharedAtomicUMin32(EmitContext& ctx, Id offset, Id value) {
    return SharedAtomicU32(ctx, offset, value, &Sirit::Module::OpAtomicUMin);
}

Id EmitSharedAtomicSMin32(EmitContext& ctx, Id offset, Id value) {
    return SharedAtomicU32(ctx, offset, value, &Sirit::Module::OpAtomicSMin);
}

Id EmitSharedAtomicAnd32(EmitContext& ctx, Id offset, Id value) {
    return SharedAtomicU32(ctx, offset, value, &Sirit::Module::OpAtomicAnd);
}

Id EmitSharedAtomicOr32(EmitContext& ctx, Id offset, Id value) {
    return SharedAtomicU32(ctx, offset, value, &Sirit::Module::OpAtomicOr);
}

Id EmitSharedAtomicXor32(EmitContext& ctx, Id offset, Id value) {
    return SharedAtomicU32(ctx, offset, value, &Sirit::Module::OpAtomicXor);
}

Id EmitSharedAtomicISub32(EmitContext& ctx, Id offset, Id value) {
    return SharedAtomicU32(ctx, offset, value, &Sirit::Module::OpAtomicISub);
}

Id EmitSharedAtomicIIncrement32(EmitContext& ctx, Id offset) {
    return SharedAtomicU32_IncDec(ctx, offset, &Sirit::Module::OpAtomicIIncrement);
}

Id EmitSharedAtomicIDecrement32(EmitContext& ctx, Id offset) {
    return SharedAtomicU32_IncDec(ctx, offset, &Sirit::Module::OpAtomicIDecrement);
}

Id EmitBufferAtomicIAdd32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address, Id value) {
    return BufferAtomicU32(ctx, inst, handle, address, value, &Sirit::Module::OpAtomicIAdd);
}

Id EmitBufferAtomicSMin32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address, Id value) {
    return BufferAtomicU32(ctx, inst, handle, address, value, &Sirit::Module::OpAtomicSMin);
}

Id EmitBufferAtomicUMin32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address, Id value) {
    return BufferAtomicU32(ctx, inst, handle, address, value, &Sirit::Module::OpAtomicUMin);
}

Id EmitBufferAtomicSMax32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address, Id value) {
    return BufferAtomicU32(ctx, inst, handle, address, value, &Sirit::Module::OpAtomicSMax);
}

Id EmitBufferAtomicUMax32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address, Id value) {
    return BufferAtomicU32(ctx, inst, handle, address, value, &Sirit::Module::OpAtomicUMax);
}

Id EmitBufferAtomicInc32(EmitContext&, IR::Inst*, u32, Id, Id) {
    // TODO
    UNREACHABLE_MSG("Unsupported BUFFER_ATOMIC opcode: ", IR::Opcode::BufferAtomicInc32);
}

Id EmitBufferAtomicDec32(EmitContext&, IR::Inst*, u32, Id, Id) {
    // TODO
    UNREACHABLE_MSG("Unsupported BUFFER_ATOMIC opcode: ", IR::Opcode::BufferAtomicDec32);
}

Id EmitBufferAtomicAnd32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address, Id value) {
    return BufferAtomicU32(ctx, inst, handle, address, value, &Sirit::Module::OpAtomicAnd);
}

Id EmitBufferAtomicOr32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address, Id value) {
    return BufferAtomicU32(ctx, inst, handle, address, value, &Sirit::Module::OpAtomicOr);
}

Id EmitBufferAtomicXor32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address, Id value) {
    return BufferAtomicU32(ctx, inst, handle, address, value, &Sirit::Module::OpAtomicXor);
}

Id EmitBufferAtomicSwap32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address, Id value) {
    return BufferAtomicU32(ctx, inst, handle, address, value, &Sirit::Module::OpAtomicExchange);
}

Id EmitImageAtomicIAdd32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id coords, Id value) {
    return ImageAtomicU32(ctx, inst, handle, coords, value, &Sirit::Module::OpAtomicIAdd);
}

Id EmitImageAtomicSMin32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id coords, Id value) {
    return ImageAtomicU32(ctx, inst, handle, coords, value, &Sirit::Module::OpAtomicSMin);
}

Id EmitImageAtomicUMin32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id coords, Id value) {
    return ImageAtomicU32(ctx, inst, handle, coords, value, &Sirit::Module::OpAtomicUMin);
}

Id EmitImageAtomicSMax32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id coords, Id value) {
    return ImageAtomicU32(ctx, inst, handle, coords, value, &Sirit::Module::OpAtomicSMax);
}

Id EmitImageAtomicUMax32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id coords, Id value) {
    return ImageAtomicU32(ctx, inst, handle, coords, value, &Sirit::Module::OpAtomicUMax);
}

Id EmitImageAtomicFMax32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id coords, Id value) {
    if (ctx.profile.supports_image_fp32_atomic_min_max) {
        return ImageAtomicF32(ctx, inst, handle, coords, value, &Sirit::Module::OpAtomicFMax);
    }

    const auto u32_value = ctx.OpBitcast(ctx.U32[1], value);
    const auto sign_bit_set =
        ctx.OpBitFieldUExtract(ctx.U32[1], u32_value, ctx.ConstU32(31u), ctx.ConstU32(1u));

    const auto result = ctx.OpSelect(
        ctx.F32[1], sign_bit_set,
        EmitBitCastF32U32(ctx, EmitImageAtomicUMin32(ctx, inst, handle, coords, u32_value)),
        EmitBitCastF32U32(ctx, EmitImageAtomicSMax32(ctx, inst, handle, coords, u32_value)));

    return result;
}

Id EmitImageAtomicFMin32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id coords, Id value) {
    if (ctx.profile.supports_image_fp32_atomic_min_max) {
        return ImageAtomicF32(ctx, inst, handle, coords, value, &Sirit::Module::OpAtomicFMin);
    }

    const auto u32_value = ctx.OpBitcast(ctx.U32[1], value);
    const auto sign_bit_set =
        ctx.OpBitFieldUExtract(ctx.U32[1], u32_value, ctx.ConstU32(31u), ctx.ConstU32(1u));

    const auto result = ctx.OpSelect(
        ctx.F32[1], sign_bit_set,
        EmitBitCastF32U32(ctx, EmitImageAtomicUMax32(ctx, inst, handle, coords, u32_value)),
        EmitBitCastF32U32(ctx, EmitImageAtomicSMin32(ctx, inst, handle, coords, u32_value)));

    return result;
}

Id EmitImageAtomicInc32(EmitContext&, IR::Inst*, u32, Id, Id) {
    // TODO: This is not yet implemented
    throw NotImplementedException("SPIR-V Instruction");
}

Id EmitImageAtomicDec32(EmitContext&, IR::Inst*, u32, Id, Id) {
    // TODO: This is not yet implemented
    throw NotImplementedException("SPIR-V Instruction");
}

Id EmitImageAtomicAnd32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id coords, Id value) {
    return ImageAtomicU32(ctx, inst, handle, coords, value, &Sirit::Module::OpAtomicAnd);
}

Id EmitImageAtomicOr32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id coords, Id value) {
    return ImageAtomicU32(ctx, inst, handle, coords, value, &Sirit::Module::OpAtomicOr);
}

Id EmitImageAtomicXor32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id coords, Id value) {
    return ImageAtomicU32(ctx, inst, handle, coords, value, &Sirit::Module::OpAtomicXor);
}

Id EmitImageAtomicExchange32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id coords, Id value) {
    return ImageAtomicU32(ctx, inst, handle, coords, value, &Sirit::Module::OpAtomicExchange);
}

Id EmitDataAppend(EmitContext& ctx, u32 gds_addr, u32 binding) {
    const auto& buffer = ctx.buffers[binding];
    const auto [id, pointer_type] = buffer[EmitContext::BufferAlias::U32];
    const Id ptr = ctx.OpAccessChain(pointer_type, id, ctx.u32_zero_value, ctx.ConstU32(gds_addr));
    const auto [scope, semantics]{AtomicArgs(ctx)};
    return ctx.OpAtomicIIncrement(ctx.U32[1], ptr, scope, semantics);
}

Id EmitDataConsume(EmitContext& ctx, u32 gds_addr, u32 binding) {
    const auto& buffer = ctx.buffers[binding];
    const auto [id, pointer_type] = buffer[EmitContext::BufferAlias::U32];
    const Id ptr = ctx.OpAccessChain(pointer_type, id, ctx.u32_zero_value, ctx.ConstU32(gds_addr));
    const auto [scope, semantics]{AtomicArgs(ctx)};
    return ctx.OpAtomicIDecrement(ctx.U32[1], ptr, scope, semantics);
}

} // namespace Shader::Backend::SPIRV
