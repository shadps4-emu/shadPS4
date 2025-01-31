// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/backend/spirv/emit_spirv_instructions.h"
#include "shader_recompiler/backend/spirv/spirv_emit_context.h"

namespace Shader::Backend::SPIRV {

Id EmitLoadSharedU32(EmitContext& ctx, Id offset) {
    const Id shift_id{ctx.ConstU32(2U)};
    const Id index{ctx.OpShiftRightArithmetic(ctx.U32[1], offset, shift_id)};
    if (ctx.info.has_emulated_shared_memory) {
        const Id pointer =
            ctx.OpAccessChain(ctx.shared_u32, ctx.shared_memory_u32, ctx.u32_zero_value, index);
        return ctx.OpLoad(ctx.U32[1], pointer);
    } else {
        const Id pointer = ctx.OpAccessChain(ctx.shared_u32, ctx.shared_memory_u32, index);
        return ctx.OpLoad(ctx.U32[1], pointer);
    }
}

Id EmitLoadSharedU64(EmitContext& ctx, Id offset) {
    const Id shift_id{ctx.ConstU32(2U)};
    const Id base_index{ctx.OpShiftRightArithmetic(ctx.U32[1], offset, shift_id)};
    const Id next_index{ctx.OpIAdd(ctx.U32[1], base_index, ctx.ConstU32(1U))};
    if (ctx.info.has_emulated_shared_memory) {
        const Id lhs_pointer{ctx.OpAccessChain(ctx.shared_u32, ctx.shared_memory_u32,
                                               ctx.u32_zero_value, base_index)};
        const Id rhs_pointer{ctx.OpAccessChain(ctx.shared_u32, ctx.shared_memory_u32,
                                               ctx.u32_zero_value, next_index)};
        return ctx.OpCompositeConstruct(ctx.U32[2], ctx.OpLoad(ctx.U32[1], lhs_pointer),
                                        ctx.OpLoad(ctx.U32[1], rhs_pointer));
    } else {
        const Id lhs_pointer{ctx.OpAccessChain(ctx.shared_u32, ctx.shared_memory_u32, base_index)};
        const Id rhs_pointer{ctx.OpAccessChain(ctx.shared_u32, ctx.shared_memory_u32, next_index)};
        return ctx.OpCompositeConstruct(ctx.U32[2], ctx.OpLoad(ctx.U32[1], lhs_pointer),
                                        ctx.OpLoad(ctx.U32[1], rhs_pointer));
    }
}

Id EmitLoadSharedU128(EmitContext& ctx, Id offset) {
    const Id shift_id{ctx.ConstU32(2U)};
    const Id base_index{ctx.OpShiftRightArithmetic(ctx.U32[1], offset, shift_id)};
    std::array<Id, 4> values{};
    for (u32 i = 0; i < 4; ++i) {
        const Id index{i == 0 ? base_index : ctx.OpIAdd(ctx.U32[1], base_index, ctx.ConstU32(i))};
        if (ctx.info.has_emulated_shared_memory) {
            const Id pointer{ctx.OpAccessChain(ctx.shared_u32, ctx.shared_memory_u32,
                                               ctx.u32_zero_value, index)};
            values[i] = ctx.OpLoad(ctx.U32[1], pointer);
        } else {
            const Id pointer{ctx.OpAccessChain(ctx.shared_u32, ctx.shared_memory_u32, index)};
            values[i] = ctx.OpLoad(ctx.U32[1], pointer);
        }
    }
    return ctx.OpCompositeConstruct(ctx.U32[4], values);
}

void EmitWriteSharedU32(EmitContext& ctx, Id offset, Id value) {
    const Id shift{ctx.ConstU32(2U)};
    const Id word_offset{ctx.OpShiftRightArithmetic(ctx.U32[1], offset, shift)};
    if (ctx.info.has_emulated_shared_memory) {
        const Id pointer = ctx.OpAccessChain(ctx.shared_u32, ctx.shared_memory_u32,
                                             ctx.u32_zero_value, word_offset);
        ctx.OpStore(pointer, value);
    } else {
        const Id pointer = ctx.OpAccessChain(ctx.shared_u32, ctx.shared_memory_u32, word_offset);
        ctx.OpStore(pointer, value);
    }
}

void EmitWriteSharedU64(EmitContext& ctx, Id offset, Id value) {
    const Id shift{ctx.ConstU32(2U)};
    const Id word_offset{ctx.OpShiftRightArithmetic(ctx.U32[1], offset, shift)};
    const Id next_offset{ctx.OpIAdd(ctx.U32[1], word_offset, ctx.ConstU32(1U))};
    if (ctx.info.has_emulated_shared_memory) {
        const Id lhs_pointer{ctx.OpAccessChain(ctx.shared_u32, ctx.shared_memory_u32,
                                               ctx.u32_zero_value, word_offset)};
        const Id rhs_pointer{ctx.OpAccessChain(ctx.shared_u32, ctx.shared_memory_u32,
                                               ctx.u32_zero_value, next_offset)};
        ctx.OpStore(lhs_pointer, ctx.OpCompositeExtract(ctx.U32[1], value, 0U));
        ctx.OpStore(rhs_pointer, ctx.OpCompositeExtract(ctx.U32[1], value, 1U));
    } else {
        const Id lhs_pointer{ctx.OpAccessChain(ctx.shared_u32, ctx.shared_memory_u32, word_offset)};
        const Id rhs_pointer{ctx.OpAccessChain(ctx.shared_u32, ctx.shared_memory_u32, next_offset)};
        ctx.OpStore(lhs_pointer, ctx.OpCompositeExtract(ctx.U32[1], value, 0U));
        ctx.OpStore(rhs_pointer, ctx.OpCompositeExtract(ctx.U32[1], value, 1U));
    }
}

void EmitWriteSharedU128(EmitContext& ctx, Id offset, Id value) {
    const Id shift{ctx.ConstU32(2U)};
    const Id base_index{ctx.OpShiftRightArithmetic(ctx.U32[1], offset, shift)};
    for (u32 i = 0; i < 4; ++i) {
        const Id index{i == 0 ? base_index : ctx.OpIAdd(ctx.U32[1], base_index, ctx.ConstU32(i))};
        if (ctx.info.has_emulated_shared_memory) {
            const Id pointer{ctx.OpAccessChain(ctx.shared_u32, ctx.shared_memory_u32,
                                               ctx.u32_zero_value, index)};
            ctx.OpStore(pointer, ctx.OpCompositeExtract(ctx.U32[1], value, i));
        } else {
            const Id pointer{ctx.OpAccessChain(ctx.shared_u32, ctx.shared_memory_u32, index)};
            ctx.OpStore(pointer, ctx.OpCompositeExtract(ctx.U32[1], value, i));
        }
    }
}

} // namespace Shader::Backend::SPIRV
