// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/div_ceil.h"
#include "shader_recompiler/backend/spirv/emit_spirv_bounds.h"
#include "shader_recompiler/backend/spirv/emit_spirv_instructions.h"
#include "shader_recompiler/backend/spirv/spirv_emit_context.h"

namespace Shader::Backend::SPIRV {

namespace {
using PointerType = EmitContext::PointerType;
using PointerSize = EmitContext::PointerSize;

std::pair<Id, Id> AtomicArgs(EmitContext& ctx) {
    const Id scope{ctx.ConstU32(static_cast<u32>(spv::Scope::Device))};
    const Id semantics{ctx.u32_zero_value};
    return {scope, semantics};
}

Id SharedAtomicU32(EmitContext& ctx, Id offset, Id value,
                   Id (Sirit::Module::*atomic_func)(Id, Id, Id, Id, Id)) {
    const Id shift_id{ctx.ConstU32(2U)};
    const Id index{ctx.OpShiftRightLogical(ctx.U32[1], offset, shift_id)};
    const u32 num_elements{Common::DivCeil(ctx.runtime_info.cs_info.shared_memory_size, 4u)};
    const Id pointer{ctx.EmitSharedMemoryAccess(ctx.shared_u32, ctx.shared_memory_u32, index)};
    const auto [scope, semantics]{AtomicArgs(ctx)};
    return AccessBoundsCheck<32>(ctx, index, ctx.ConstU32(num_elements), [&] {
        return (ctx.*atomic_func)(ctx.U32[1], pointer, scope, semantics, value);
    });
}

Id SharedAtomicU32IncDec(EmitContext& ctx, Id offset,
                         Id (Sirit::Module::*atomic_func)(Id, Id, Id, Id)) {
    const Id shift_id{ctx.ConstU32(2U)};
    const Id index{ctx.OpShiftRightLogical(ctx.U32[1], offset, shift_id)};
    const u32 num_elements{Common::DivCeil(ctx.runtime_info.cs_info.shared_memory_size, 4u)};
    const Id pointer{ctx.EmitSharedMemoryAccess(ctx.shared_u32, ctx.shared_memory_u32, index)};
    const auto [scope, semantics]{AtomicArgs(ctx)};
    return AccessBoundsCheck<32>(ctx, index, ctx.ConstU32(num_elements), [&] {
        return (ctx.*atomic_func)(ctx.U32[1], pointer, scope, semantics);
    });
}

Id SharedAtomicU64(EmitContext& ctx, Id offset, Id value,
                   Id (Sirit::Module::*atomic_func)(Id, Id, Id, Id, Id)) {
    const Id shift_id{ctx.ConstU32(3U)};
    const Id index{ctx.OpShiftRightLogical(ctx.U32[1], offset, shift_id)};
    const u32 num_elements{Common::DivCeil(ctx.runtime_info.cs_info.shared_memory_size, 8u)};
    const Id pointer{ctx.EmitSharedMemoryAccess(ctx.shared_u64, ctx.shared_memory_u64, index)};
    const auto [scope, semantics]{AtomicArgs(ctx)};
    return AccessBoundsCheck<64>(ctx, index, ctx.ConstU32(num_elements), [&] {
        return (ctx.*atomic_func)(ctx.U64, pointer, scope, semantics, value);
    });
}

Id SharedAtomicU64IncDec(EmitContext& ctx, Id offset,
                         Id (Sirit::Module::*atomic_func)(Id, Id, Id, Id)) {
    const Id shift_id{ctx.ConstU32(3U)};
    const Id index{ctx.OpShiftRightLogical(ctx.U32[1], offset, shift_id)};
    const u32 num_elements{Common::DivCeil(ctx.runtime_info.cs_info.shared_memory_size, 8u)};
    const Id pointer{ctx.EmitSharedMemoryAccess(ctx.shared_u64, ctx.shared_memory_u64, index)};
    const auto [scope, semantics]{AtomicArgs(ctx)};
    return AccessBoundsCheck<64>(ctx, index, ctx.ConstU32(num_elements), [&] {
        return (ctx.*atomic_func)(ctx.U64, pointer, scope, semantics);
    });
}

template <bool is_float = false>
Id BufferAtomicU32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address, Id value,
                   Id (Sirit::Module::*atomic_func)(Id, Id, Id, Id, Id)) {
    const auto& buffer = ctx.buffers[handle];
    const Id type = is_float ? ctx.F32[1] : ctx.U32[1];
    if (const Id offset = buffer.Offset(PointerSize::B32); Sirit::ValidId(offset)) {
        address = ctx.OpIAdd(ctx.U32[1], address, offset);
    }
    const auto [id, pointer_type] = buffer.Alias(PointerType::U32);
    const Id ptr = ctx.OpAccessChain(pointer_type, id, ctx.u32_zero_value, address);
    const auto [scope, semantics]{AtomicArgs(ctx)};
    return AccessBoundsCheck<32, 1, is_float>(ctx, address, buffer.Size(PointerSize::B32), [&] {
        return (ctx.*atomic_func)(type, ptr, scope, semantics, value);
    });
}

Id BufferAtomicU32IncDec(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address,
                         Id (Sirit::Module::*atomic_func)(Id, Id, Id, Id)) {
    const auto& buffer = ctx.buffers[handle];
    if (const Id offset = buffer.Offset(PointerSize::B32); Sirit::ValidId(offset)) {
        address = ctx.OpIAdd(ctx.U32[1], address, offset);
    }
    const auto [id, pointer_type] = buffer.Alias(PointerType::U32);
    const Id ptr = ctx.OpAccessChain(pointer_type, id, ctx.u32_zero_value, address);
    const auto [scope, semantics]{AtomicArgs(ctx)};
    return AccessBoundsCheck<32>(ctx, address, buffer.Size(PointerSize::B32), [&] {
        return (ctx.*atomic_func)(ctx.U32[1], ptr, scope, semantics);
    });
}

Id BufferAtomicU32CmpSwap(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address, Id value,
                          Id cmp_value,
                          Id (Sirit::Module::*atomic_func)(Id, Id, Id, Id, Id, Id, Id)) {
    const auto& buffer = ctx.buffers[handle];
    if (const Id offset = buffer.Offset(PointerSize::B32); Sirit::ValidId(offset)) {
        address = ctx.OpIAdd(ctx.U32[1], address, offset);
    }
    const auto [id, pointer_type] = buffer.Alias(PointerType::U32);
    const Id ptr = ctx.OpAccessChain(pointer_type, id, ctx.u32_zero_value, address);
    const auto [scope, semantics]{AtomicArgs(ctx)};
    return AccessBoundsCheck<32>(ctx, address, buffer.Size(PointerSize::B32), [&] {
        return (ctx.*atomic_func)(ctx.U32[1], ptr, scope, semantics, semantics, value, cmp_value);
    });
}

Id BufferAtomicU64(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address, Id value,
                   Id (Sirit::Module::*atomic_func)(Id, Id, Id, Id, Id)) {
    const auto& buffer = ctx.buffers[handle];
    if (const Id offset = buffer.Offset(PointerSize::B64); Sirit::ValidId(offset)) {
        address = ctx.OpIAdd(ctx.U32[1], address, offset);
    }
    const auto [id, pointer_type] = buffer.Alias(PointerType::U64);
    const Id ptr = ctx.OpAccessChain(pointer_type, id, ctx.u32_zero_value, address);
    const auto [scope, semantics]{AtomicArgs(ctx)};
    return AccessBoundsCheck<64>(ctx, address, buffer.Size(PointerSize::B64), [&] {
        return (ctx.*atomic_func)(ctx.U64, ptr, scope, semantics, value);
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

Id EmitSharedAtomicIAdd64(EmitContext& ctx, Id offset, Id value) {
    return SharedAtomicU64(ctx, offset, value, &Sirit::Module::OpAtomicIAdd);
}

Id EmitSharedAtomicUMax32(EmitContext& ctx, Id offset, Id value) {
    return SharedAtomicU32(ctx, offset, value, &Sirit::Module::OpAtomicUMax);
}

Id EmitSharedAtomicUMax64(EmitContext& ctx, Id offset, Id value) {
    return SharedAtomicU64(ctx, offset, value, &Sirit::Module::OpAtomicUMax);
}

Id EmitSharedAtomicSMax32(EmitContext& ctx, Id offset, Id value) {
    return SharedAtomicU32(ctx, offset, value, &Sirit::Module::OpAtomicSMax);
}

Id EmitSharedAtomicSMax64(EmitContext& ctx, Id offset, Id value) {
    return SharedAtomicU64(ctx, offset, value, &Sirit::Module::OpAtomicSMax);
}

Id EmitSharedAtomicUMin32(EmitContext& ctx, Id offset, Id value) {
    return SharedAtomicU32(ctx, offset, value, &Sirit::Module::OpAtomicUMin);
}

Id EmitSharedAtomicUMin64(EmitContext& ctx, Id offset, Id value) {
    return SharedAtomicU64(ctx, offset, value, &Sirit::Module::OpAtomicUMin);
}

Id EmitSharedAtomicSMin32(EmitContext& ctx, Id offset, Id value) {
    return SharedAtomicU32(ctx, offset, value, &Sirit::Module::OpAtomicSMin);
}

Id EmitSharedAtomicSMin64(EmitContext& ctx, Id offset, Id value) {
    return SharedAtomicU64(ctx, offset, value, &Sirit::Module::OpAtomicSMin);
}

Id EmitSharedAtomicAnd32(EmitContext& ctx, Id offset, Id value) {
    return SharedAtomicU32(ctx, offset, value, &Sirit::Module::OpAtomicAnd);
}

Id EmitSharedAtomicAnd64(EmitContext& ctx, Id offset, Id value) {
    return SharedAtomicU64(ctx, offset, value, &Sirit::Module::OpAtomicAnd);
}

Id EmitSharedAtomicOr32(EmitContext& ctx, Id offset, Id value) {
    return SharedAtomicU32(ctx, offset, value, &Sirit::Module::OpAtomicOr);
}

Id EmitSharedAtomicOr64(EmitContext& ctx, Id offset, Id value) {
    return SharedAtomicU64(ctx, offset, value, &Sirit::Module::OpAtomicOr);
}

Id EmitSharedAtomicXor32(EmitContext& ctx, Id offset, Id value) {
    return SharedAtomicU32(ctx, offset, value, &Sirit::Module::OpAtomicXor);
}

Id EmitSharedAtomicXor64(EmitContext& ctx, Id offset, Id value) {
    return SharedAtomicU64(ctx, offset, value, &Sirit::Module::OpAtomicXor);
}

Id EmitSharedAtomicISub32(EmitContext& ctx, Id offset, Id value) {
    return SharedAtomicU32(ctx, offset, value, &Sirit::Module::OpAtomicISub);
}

Id EmitSharedAtomicISub64(EmitContext& ctx, Id offset, Id value) {
    return SharedAtomicU64(ctx, offset, value, &Sirit::Module::OpAtomicISub);
}

Id EmitSharedAtomicInc32(EmitContext& ctx, Id offset) {
    return SharedAtomicU32IncDec(ctx, offset, &Sirit::Module::OpAtomicIIncrement);
}

Id EmitSharedAtomicInc64(EmitContext& ctx, Id offset) {
    return SharedAtomicU64IncDec(ctx, offset, &Sirit::Module::OpAtomicIIncrement);
}

Id EmitSharedAtomicDec32(EmitContext& ctx, Id offset) {
    return SharedAtomicU32IncDec(ctx, offset, &Sirit::Module::OpAtomicIDecrement);
}

Id EmitSharedAtomicDec64(EmitContext& ctx, Id offset) {
    return SharedAtomicU64IncDec(ctx, offset, &Sirit::Module::OpAtomicIDecrement);
}

Id EmitBufferAtomicIAdd32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address, Id value) {
    return BufferAtomicU32(ctx, inst, handle, address, value, &Sirit::Module::OpAtomicIAdd);
}

Id EmitBufferAtomicIAdd64(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address, Id value) {
    return BufferAtomicU64(ctx, inst, handle, address, value, &Sirit::Module::OpAtomicIAdd);
}

Id EmitBufferAtomicISub32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address, Id value) {
    return BufferAtomicU32(ctx, inst, handle, address, value, &Sirit::Module::OpAtomicISub);
}

Id EmitBufferAtomicSMin32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address, Id value) {
    return BufferAtomicU32(ctx, inst, handle, address, value, &Sirit::Module::OpAtomicSMin);
}

Id EmitBufferAtomicSMin64(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address, Id value) {
    return BufferAtomicU64(ctx, inst, handle, address, value, &Sirit::Module::OpAtomicSMin);
}

Id EmitBufferAtomicUMin32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address, Id value) {
    return BufferAtomicU32(ctx, inst, handle, address, value, &Sirit::Module::OpAtomicUMin);
}

Id EmitBufferAtomicUMin64(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address, Id value) {
    return BufferAtomicU64(ctx, inst, handle, address, value, &Sirit::Module::OpAtomicUMin);
}

Id EmitBufferAtomicFMin32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address, Id value) {
    if (ctx.profile.supports_buffer_fp32_atomic_min_max) {
        return BufferAtomicU32<true>(ctx, inst, handle, address, value,
                                     &Sirit::Module::OpAtomicFMin);
    }

    const auto u32_value = ctx.OpBitcast(ctx.U32[1], value);
    const auto sign_bit_set =
        ctx.OpBitFieldUExtract(ctx.U32[1], u32_value, ctx.ConstU32(31u), ctx.ConstU32(1u));

    const auto result = ctx.OpSelect(
        ctx.F32[1], sign_bit_set,
        EmitBitCastF32U32(ctx, EmitBufferAtomicUMax32(ctx, inst, handle, address, u32_value)),
        EmitBitCastF32U32(ctx, EmitBufferAtomicSMin32(ctx, inst, handle, address, u32_value)));

    return result;
}

Id EmitBufferAtomicSMax32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address, Id value) {
    return BufferAtomicU32(ctx, inst, handle, address, value, &Sirit::Module::OpAtomicSMax);
}

Id EmitBufferAtomicSMax64(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address, Id value) {
    return BufferAtomicU64(ctx, inst, handle, address, value, &Sirit::Module::OpAtomicSMax);
}

Id EmitBufferAtomicUMax32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address, Id value) {
    return BufferAtomicU32(ctx, inst, handle, address, value, &Sirit::Module::OpAtomicUMax);
}

Id EmitBufferAtomicUMax64(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address, Id value) {
    return BufferAtomicU64(ctx, inst, handle, address, value, &Sirit::Module::OpAtomicUMax);
}

Id EmitBufferAtomicFMax32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address, Id value) {
    if (ctx.profile.supports_buffer_fp32_atomic_min_max) {
        return BufferAtomicU32<true>(ctx, inst, handle, address, value,
                                     &Sirit::Module::OpAtomicFMax);
    }

    const auto u32_value = ctx.OpBitcast(ctx.U32[1], value);
    const auto sign_bit_set =
        ctx.OpBitFieldUExtract(ctx.U32[1], u32_value, ctx.ConstU32(31u), ctx.ConstU32(1u));

    const auto result = ctx.OpSelect(
        ctx.F32[1], sign_bit_set,
        EmitBitCastF32U32(ctx, EmitBufferAtomicUMin32(ctx, inst, handle, address, u32_value)),
        EmitBitCastF32U32(ctx, EmitBufferAtomicSMax32(ctx, inst, handle, address, u32_value)));

    return result;
}

Id EmitBufferAtomicInc32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address) {
    return BufferAtomicU32IncDec(ctx, inst, handle, address, &Sirit::Module::OpAtomicIIncrement);
}

Id EmitBufferAtomicDec32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address) {
    return BufferAtomicU32IncDec(ctx, inst, handle, address, &Sirit::Module::OpAtomicIDecrement);
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

Id EmitBufferAtomicCmpSwap32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address, Id value,
                             Id cmp_value) {
    return BufferAtomicU32CmpSwap(ctx, inst, handle, address, value, cmp_value,
                                  &Sirit::Module::OpAtomicCompareExchange);
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
    const auto [id, pointer_type] = buffer.Alias(PointerType::U32);
    const Id ptr = ctx.OpAccessChain(pointer_type, id, ctx.u32_zero_value, ctx.ConstU32(gds_addr));
    const auto [scope, semantics]{AtomicArgs(ctx)};
    return ctx.OpAtomicIIncrement(ctx.U32[1], ptr, scope, semantics);
}

Id EmitDataConsume(EmitContext& ctx, u32 gds_addr, u32 binding) {
    const auto& buffer = ctx.buffers[binding];
    const auto [id, pointer_type] = buffer.Alias(PointerType::U32);
    const Id ptr = ctx.OpAccessChain(pointer_type, id, ctx.u32_zero_value, ctx.ConstU32(gds_addr));
    const auto [scope, semantics]{AtomicArgs(ctx)};
    return ctx.OpAtomicIDecrement(ctx.U32[1], ptr, scope, semantics);
}

} // namespace Shader::Backend::SPIRV
