// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/backend/spirv/emit_spirv_instructions.h"
#include "shader_recompiler/backend/spirv/spirv_emit_context.h"

namespace Shader::Backend::SPIRV {

namespace {
using PointerType = EmitContext::PointerType;
using PointerSize = EmitContext::PointerSize;

using AtomicFunc = Id (Sirit::Module::*)(Id, Id, Id, Id, Id);
using AtomicIncDecFunc = Id (Sirit::Module::*)(Id, Id, Id, Id);
using AtomicCmpSwpFunc = Id (Sirit::Module::*)(Id, Id, Id, Id, Id, Id, Id);

static std::pair<Id, Id> SharedAtomicArgs(EmitContext& ctx) {
    const Id scope{ctx.ConstU32(static_cast<u32>(spv::Scope::Workgroup))};
    const Id semantics{ctx.u32_zero_value};
    return {scope, semantics};
}

static std::pair<Id, Id> BufferAtomicArgs(EmitContext& ctx) {
    const Id scope{ctx.ConstU32(static_cast<u32>(spv::Scope::Device))};
    const Id semantics{ctx.u32_zero_value};
    return {scope, semantics};
}

static Id GetImageAtomicPointer(EmitContext& ctx, u32 handle, Id coords, bool is_float) {
    const auto& texture = ctx.images[handle & 0xFFFF];
    const Id spv_pointer_type = is_float ? ctx.image_f32 : ctx.image_u32;
    return ctx.OpImageTexelPointer(spv_pointer_type, texture.id, coords, ctx.ConstU32(0U));
}

static Id SharedAtomicB32(EmitContext& ctx, Id offset, Id value, AtomicFunc atomic_func,
                          bool is_float = false) {
    const Id type = is_float ? ctx.F32[1] : ctx.U32[1];
    PointerType pointer_type = is_float ? PointerType::F32 : PointerType::U32;
    const Id pointer = ctx.EmitSharedMemoryAccess(offset, PointerType::U32);
    const auto [scope, semantics]{SharedAtomicArgs(ctx)};
    return (ctx.*atomic_func)(ctx.U32[1], pointer, scope, semantics, value);
}

Id SharedAtomicU32IncDec(EmitContext& ctx, Id offset, AtomicIncDecFunc atomic_func) {
    const Id pointer = ctx.EmitSharedMemoryAccess(offset, PointerType::U32);
    const auto [scope, semantics]{SharedAtomicArgs(ctx)};
    return (ctx.*atomic_func)(ctx.U32[1], pointer, scope, semantics);
}

Id SharedAtomicU64(EmitContext& ctx, Id offset, Id value, AtomicFunc atomic_func) {
    const Id pointer = ctx.EmitSharedMemoryAccess(offset, PointerType::U64);
    const auto [scope, semantics]{SharedAtomicArgs(ctx)};
    return (ctx.*atomic_func)(ctx.U64, pointer, scope, semantics, value);
}

Id SharedAtomicU64IncDec(EmitContext& ctx, Id offset, AtomicIncDecFunc atomic_func) {
    const Id pointer = ctx.EmitSharedMemoryAccess(offset, PointerType::U64);
    const auto [scope, semantics]{SharedAtomicArgs(ctx)};
    return (ctx.*atomic_func)(ctx.U64, pointer, scope, semantics);
}

Id SharedAtomicU32CmpSwap(EmitContext& ctx, Id offset, Id value, Id cmp_value) {
    const Id pointer = ctx.EmitSharedMemoryAccess(offset, PointerType::U32);
    const auto [scope, semantics]{SharedAtomicArgs(ctx)};
    return ctx.OpAtomicCompareExchange(ctx.U32[1], pointer, scope, semantics, semantics, value,
                                       cmp_value);
}

Id SharedAtomicU64CmpSwap(EmitContext& ctx, Id offset, Id value, Id cmp_value) {
    const Id pointer = ctx.EmitSharedMemoryAccess(offset, PointerType::U64);
    const auto [scope, semantics]{SharedAtomicArgs(ctx)};
    return ctx.OpAtomicCompareExchange(ctx.U64, pointer, scope, semantics, semantics, value,
                                       cmp_value);
}

Id BufferAtomicB32(EmitContext& ctx, u32 handle, Id address, Id value, AtomicFunc atomic_func,
                   bool is_float = false) {
    const Id type = is_float ? ctx.F32[1] : ctx.U32[1];
    PointerType pointer_type = is_float ? PointerType::F32 : PointerType::U32;
    const Id ptr = ctx.EmitBufferAccess(handle, address, pointer_type);
    const auto [scope, semantics]{BufferAtomicArgs(ctx)};
    return (ctx.*atomic_func)(type, ptr, scope, semantics, value);
}

Id BufferAtomicU32IncDec(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address,
                         AtomicIncDecFunc atomic_func) {
    const Id ptr = ctx.EmitBufferAccess(handle, address, PointerType::U32);
    const auto [scope, semantics]{BufferAtomicArgs(ctx)};
    return (ctx.*atomic_func)(ctx.U32[1], ptr, scope, semantics);
}

Id BufferAtomicU32CmpSwap(EmitContext& ctx, u32 handle, Id address, Id value, Id cmp_value,
                          AtomicCmpSwpFunc atomic_func) {
    const Id ptr = ctx.EmitBufferAccess(handle, address, PointerType::U32);
    const auto [scope, semantics]{BufferAtomicArgs(ctx)};
    return (ctx.*atomic_func)(ctx.U32[1], ptr, scope, semantics, semantics, value, cmp_value);
}

Id BufferAtomicU64(EmitContext& ctx, u32 handle, Id address, Id value, AtomicFunc atomic_func) {
    const Id ptr = ctx.EmitBufferAccess(handle, address, PointerType::U64);
    const auto [scope, semantics]{BufferAtomicArgs(ctx)};
    return (ctx.*atomic_func)(ctx.U64, ptr, scope, semantics, value);
}

Id FixImageAtomicCoords(EmitContext& ctx, Id coords, AmdGpu::ImageType image_type) {
    switch (image_type) {
    case AmdGpu::ImageType::Color1D: {
        // Lowered to 2D with height 1
        const auto x = coords;
        return ctx.OpCompositeConstruct(ctx.U32[2], x, ctx.u32_zero_value);
    }
    case AmdGpu::ImageType::Color1DArray: {
        // Lowered to 2D array with height 1
        const auto x = ctx.OpCompositeExtract(ctx.U32[1], coords, 0U);
        const auto slice = ctx.OpCompositeExtract(ctx.U32[1], coords, 1U);
        return ctx.OpCompositeConstruct(ctx.U32[3], x, ctx.u32_zero_value, slice);
    }
    default:
        return coords;
    }
}

Id ImageAtomicB32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id coords, Id value,
                  AtomicFunc atomic_func, bool is_float = false) {
    const Id pointer{GetImageAtomicPointer(ctx, handle, coords, is_float)};
    const auto [scope, semantics]{BufferAtomicArgs(ctx)};
    return (ctx.*atomic_func)(ctx.U32[1], pointer, scope, semantics, value);
}

Id ImageAtomicU32CmpSwap(EmitContext& ctx, IR::Inst* inst, u32 handle, Id coords, Id value,
                         Id cmp_value, AtomicCmpSwpFunc atomic_func) {
    const Id pointer{GetImageAtomicPointer(ctx, handle, coords, false)};
    const auto [scope, semantics]{BufferAtomicArgs(ctx)};
    return (ctx.*atomic_func)(ctx.U32[1], pointer, scope, semantics, semantics, value, cmp_value);
}
} // Anonymous namespace

Id EmitSharedAtomicIAdd32(EmitContext& ctx, Id offset, Id value) {
    return SharedAtomicB32(ctx, offset, value, &Sirit::Module::OpAtomicIAdd);
}

Id EmitSharedAtomicIAdd64(EmitContext& ctx, Id offset, Id value) {
    return SharedAtomicU64(ctx, offset, value, &Sirit::Module::OpAtomicIAdd);
}

Id EmitSharedAtomicUMax32(EmitContext& ctx, Id offset, Id value) {
    return SharedAtomicB32(ctx, offset, value, &Sirit::Module::OpAtomicUMax);
}

Id EmitSharedAtomicUMax64(EmitContext& ctx, Id offset, Id value) {
    return SharedAtomicU64(ctx, offset, value, &Sirit::Module::OpAtomicUMax);
}

Id EmitSharedAtomicSMax32(EmitContext& ctx, Id offset, Id value) {
    return SharedAtomicB32(ctx, offset, value, &Sirit::Module::OpAtomicSMax);
}

Id EmitSharedAtomicSMax64(EmitContext& ctx, Id offset, Id value) {
    return SharedAtomicU64(ctx, offset, value, &Sirit::Module::OpAtomicSMax);
}

Id EmitSharedAtomicUMin32(EmitContext& ctx, Id offset, Id value) {
    return SharedAtomicB32(ctx, offset, value, &Sirit::Module::OpAtomicUMin);
}

Id EmitSharedAtomicUMin64(EmitContext& ctx, Id offset, Id value) {
    return SharedAtomicU64(ctx, offset, value, &Sirit::Module::OpAtomicUMin);
}

Id EmitSharedAtomicSMin32(EmitContext& ctx, Id offset, Id value) {
    return SharedAtomicB32(ctx, offset, value, &Sirit::Module::OpAtomicSMin);
}

Id EmitSharedAtomicSMin64(EmitContext& ctx, Id offset, Id value) {
    return SharedAtomicU64(ctx, offset, value, &Sirit::Module::OpAtomicSMin);
}

Id EmitSharedAtomicAnd32(EmitContext& ctx, Id offset, Id value) {
    return SharedAtomicB32(ctx, offset, value, &Sirit::Module::OpAtomicAnd);
}

Id EmitSharedAtomicAnd64(EmitContext& ctx, Id offset, Id value) {
    return SharedAtomicU64(ctx, offset, value, &Sirit::Module::OpAtomicAnd);
}

Id EmitSharedAtomicOr32(EmitContext& ctx, Id offset, Id value) {
    return SharedAtomicB32(ctx, offset, value, &Sirit::Module::OpAtomicOr);
}

Id EmitSharedAtomicOr64(EmitContext& ctx, Id offset, Id value) {
    return SharedAtomicU64(ctx, offset, value, &Sirit::Module::OpAtomicOr);
}

Id EmitSharedAtomicXor32(EmitContext& ctx, Id offset, Id value) {
    return SharedAtomicB32(ctx, offset, value, &Sirit::Module::OpAtomicXor);
}

Id EmitSharedAtomicXor64(EmitContext& ctx, Id offset, Id value) {
    return SharedAtomicU64(ctx, offset, value, &Sirit::Module::OpAtomicXor);
}

Id EmitSharedAtomicISub32(EmitContext& ctx, Id offset, Id value) {
    return SharedAtomicB32(ctx, offset, value, &Sirit::Module::OpAtomicISub);
}

Id EmitSharedAtomicISub64(EmitContext& ctx, Id offset, Id value) {
    return SharedAtomicU64(ctx, offset, value, &Sirit::Module::OpAtomicISub);
}

Id EmitSharedAtomicCmpSwap32(EmitContext& ctx, Id offset, Id value, Id cmp_value) {
    return SharedAtomicU32CmpSwap(ctx, offset, value, cmp_value);
}

Id EmitSharedAtomicCmpSwap64(EmitContext& ctx, Id offset, Id value, Id cmp_value) {
    return SharedAtomicU64CmpSwap(ctx, offset, value, cmp_value);
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
    return BufferAtomicB32(ctx, handle, address, value, &Sirit::Module::OpAtomicIAdd);
}

Id EmitBufferAtomicIAdd64(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address, Id value) {
    return BufferAtomicU64(ctx, handle, address, value, &Sirit::Module::OpAtomicIAdd);
}

Id EmitBufferAtomicISub32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address, Id value) {
    return BufferAtomicB32(ctx, handle, address, value, &Sirit::Module::OpAtomicISub);
}

Id EmitBufferAtomicSMin32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address, Id value) {
    return BufferAtomicB32(ctx, handle, address, value, &Sirit::Module::OpAtomicSMin);
}

Id EmitBufferAtomicSMin64(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address, Id value) {
    return BufferAtomicU64(ctx, handle, address, value, &Sirit::Module::OpAtomicSMin);
}

Id EmitBufferAtomicUMin32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address, Id value) {
    return BufferAtomicB32(ctx, handle, address, value, &Sirit::Module::OpAtomicUMin);
}

Id EmitBufferAtomicUMin64(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address, Id value) {
    return BufferAtomicU64(ctx, handle, address, value, &Sirit::Module::OpAtomicUMin);
}

Id EmitBufferAtomicSMax32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address, Id value) {
    return BufferAtomicB32(ctx, handle, address, value, &Sirit::Module::OpAtomicSMax);
}

Id EmitBufferAtomicSMax64(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address, Id value) {
    return BufferAtomicU64(ctx, handle, address, value, &Sirit::Module::OpAtomicSMax);
}

Id EmitBufferAtomicUMax32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address, Id value) {
    return BufferAtomicB32(ctx, handle, address, value, &Sirit::Module::OpAtomicUMax);
}

Id EmitBufferAtomicUMax64(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address, Id value) {
    return BufferAtomicU64(ctx, handle, address, value, &Sirit::Module::OpAtomicUMax);
}

Id EmitBufferAtomicInc32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address) {
    return BufferAtomicU32IncDec(ctx, inst, handle, address, &Sirit::Module::OpAtomicIIncrement);
}

Id EmitBufferAtomicDec32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address) {
    return BufferAtomicU32IncDec(ctx, inst, handle, address, &Sirit::Module::OpAtomicIDecrement);
}

Id EmitBufferAtomicAnd32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address, Id value) {
    return BufferAtomicB32(ctx, handle, address, value, &Sirit::Module::OpAtomicAnd);
}

Id EmitBufferAtomicOr32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address, Id value) {
    return BufferAtomicB32(ctx, handle, address, value, &Sirit::Module::OpAtomicOr);
}

Id EmitBufferAtomicXor32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address, Id value) {
    return BufferAtomicB32(ctx, handle, address, value, &Sirit::Module::OpAtomicXor);
}

Id EmitBufferAtomicSwap32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address, Id value) {
    return BufferAtomicB32(ctx, handle, address, value, &Sirit::Module::OpAtomicExchange);
}

Id EmitBufferAtomicCmpSwap32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address, Id value,
                             Id cmp_value) {
    return BufferAtomicU32CmpSwap(ctx, handle, address, value, cmp_value,
                                  &Sirit::Module::OpAtomicCompareExchange);
}

Id EmitBufferAtomicFCmpSwap32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address, Id value,
                              Id cmp_value) {
    const auto u32_value = ctx.OpBitcast(ctx.U32[1], value);
    const auto u32_cmp = ctx.OpBitcast(ctx.U32[1], cmp_value);
    const auto result = BufferAtomicU32CmpSwap(ctx, handle, address, u32_value, u32_cmp,
                                               &Sirit::Module::OpAtomicCompareExchange);
    return ctx.OpBitcast(ctx.F32[1], result);
}

Id EmitImageAtomicIAdd32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id coords, Id value) {
    return ImageAtomicB32(ctx, inst, handle, coords, value, &Sirit::Module::OpAtomicIAdd);
}

Id EmitImageAtomicSMin32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id coords, Id value) {
    return ImageAtomicB32(ctx, inst, handle, coords, value, &Sirit::Module::OpAtomicSMin);
}

Id EmitImageAtomicUMin32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id coords, Id value) {
    return ImageAtomicB32(ctx, inst, handle, coords, value, &Sirit::Module::OpAtomicUMin);
}

Id EmitImageAtomicSMax32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id coords, Id value) {
    return ImageAtomicB32(ctx, inst, handle, coords, value, &Sirit::Module::OpAtomicSMax);
}

Id EmitImageAtomicUMax32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id coords, Id value) {
    return ImageAtomicB32(ctx, inst, handle, coords, value, &Sirit::Module::OpAtomicUMax);
}

Id EmitImageAtomicInc32(EmitContext&, IR::Inst*, u32, Id, Id) {
    UNREACHABLE_MSG("SPIR-V Instruction");
}

Id EmitImageAtomicDec32(EmitContext&, IR::Inst*, u32, Id, Id) {
    UNREACHABLE_MSG("SPIR-V Instruction");
}

Id EmitImageAtomicAnd32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id coords, Id value) {
    return ImageAtomicB32(ctx, inst, handle, coords, value, &Sirit::Module::OpAtomicAnd);
}

Id EmitImageAtomicOr32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id coords, Id value) {
    return ImageAtomicB32(ctx, inst, handle, coords, value, &Sirit::Module::OpAtomicOr);
}

Id EmitImageAtomicXor32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id coords, Id value) {
    return ImageAtomicB32(ctx, inst, handle, coords, value, &Sirit::Module::OpAtomicXor);
}

Id EmitImageAtomicExchange32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id coords, Id value) {
    return ImageAtomicB32(ctx, inst, handle, coords, value, &Sirit::Module::OpAtomicExchange);
}

Id EmitImageAtomicCmpSwap32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id coords, Id value,
                            Id cmp_value) {
    return ImageAtomicU32CmpSwap(ctx, inst, handle, coords, value, cmp_value,
                                 &Sirit::Module::OpAtomicCompareExchange);
}

Id EmitDataAppend(EmitContext& ctx, Id gds_dw_offset, Id exec) {
    UNREACHABLE_MSG("SPIR-V Instruction");
}

Id EmitDataConsume(EmitContext& ctx, Id gds_dw_offset, Id exec) {
    UNREACHABLE_MSG("SPIR-V Instruction");
}

static Id FMinMaxFallback(EmitContext& ctx, Id ptr, Id value, bool is_min) {
    const auto [scope, semantics]{BufferAtomicArgs(ctx)};
    const Id true_label{ctx.OpLabel()};
    const Id false_label{ctx.OpLabel()};
    const Id merge_label{ctx.OpLabel()};

    const auto u32_value = ctx.OpBitcast(ctx.U32[1], value);
    const auto sign_bit_set = ctx.OpINotEqual(
        ctx.U1[1],
        ctx.OpBitFieldUExtract(ctx.U32[1], u32_value, ctx.ConstU32(31u), ctx.ConstU32(1u)),
        ctx.u32_zero_value);

    ctx.OpSelectionMerge(merge_label, spv::SelectionControlMask::MaskNone);
    ctx.OpBranchConditional(sign_bit_set, true_label, false_label);

    ctx.AddLabel(true_label);
    const Id sign_bit_set_atomic =
        is_min ? ctx.OpAtomicUMax(ctx.U32[1], ptr, scope, semantics, u32_value)
               : ctx.OpAtomicUMin(ctx.U32[1], ptr, scope, semantics, u32_value);
    ctx.OpBranch(merge_label);

    ctx.AddLabel(false_label);
    const Id sign_bit_unset_atomic =
        is_min ? ctx.OpAtomicSMin(ctx.U32[1], ptr, scope, semantics, u32_value)
               : ctx.OpAtomicSMax(ctx.U32[1], ptr, scope, semantics, u32_value);
    ctx.OpBranch(merge_label);

    ctx.AddLabel(merge_label);
    Id result =
        ctx.OpPhi(ctx.U32[1], sign_bit_set_atomic, true_label, sign_bit_unset_atomic, false_label);
    return ctx.OpBitcast(ctx.F32[1], result);
}

static Id ImageAtomicFMinMax(EmitContext& ctx, IR::Inst* inst, u32 handle, Id coords, Id value,
                             bool is_min) {
    if (ctx.profile.supports_image_fp32_atomic_min_max) {
        auto atomic_fn = is_min ? &Sirit::Module::OpAtomicFMin : &Sirit::Module::OpAtomicFMax;
        return ImageAtomicB32(ctx, inst, handle, coords, value, atomic_fn, true);
    } else {
        const Id ptr{GetImageAtomicPointer(ctx, handle, coords, false)};
        return FMinMaxFallback(ctx, ptr, value, is_min);
    }
}

Id EmitImageAtomicFMax32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id coords, Id value) {
    return ImageAtomicFMinMax(ctx, inst, handle, coords, value, false);
}

Id EmitImageAtomicFMin32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id coords, Id value) {
    return ImageAtomicFMinMax(ctx, inst, handle, coords, value, true);
}

static Id BufferAtomicFMinMax(EmitContext& ctx, u32 handle, Id address, Id value, bool is_min) {
    if (ctx.profile.supports_buffer_fp32_atomic_min_max) {
        return BufferAtomicB32(ctx, handle, address, value,
                               is_min ? &Sirit::Module::OpAtomicFMin : &Sirit::Module::OpAtomicFMax,
                               true);
    } else {
        const Id ptr{ctx.EmitBufferAccess(handle, address, PointerType::U32)};
        return FMinMaxFallback(ctx, ptr, value, is_min);
    }
}

Id EmitBufferAtomicFMin32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address, Id value) {
    return BufferAtomicFMinMax(ctx, handle, address, value, true);
}

Id EmitBufferAtomicFMax32(EmitContext& ctx, IR::Inst* inst, u32 handle, Id address, Id value) {
    return BufferAtomicFMinMax(ctx, handle, address, value, false);
}

static Id SharedAtomicFMinMax(EmitContext& ctx, Id offset, Id value, bool is_min) {
    if (ctx.profile.supports_shared_fp32_atomic_min_max) {
        auto atomic_fn = is_min ? &Sirit::Module::OpAtomicFMin : &Sirit::Module::OpAtomicFMax;
        return SharedAtomicB32(ctx, offset, value, atomic_fn, true);
    } else {
        const Id ptr{ctx.EmitSharedMemoryAccess(offset, PointerType::U32)};
        return FMinMaxFallback(ctx, ptr, value, is_min);
    }
}

Id EmitSharedAtomicFMin32(EmitContext& ctx, Id offset, Id value) {
    return SharedAtomicFMinMax(ctx, offset, value, true);
}

Id EmitSharedAtomicFMax32(EmitContext& ctx, Id offset, Id value) {
    return SharedAtomicFMinMax(ctx, offset, value, false);
}

} // namespace Shader::Backend::SPIRV
