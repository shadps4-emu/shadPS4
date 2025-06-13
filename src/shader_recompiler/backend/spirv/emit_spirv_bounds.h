// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "shader_recompiler/backend/spirv/spirv_emit_context.h"

namespace Shader::Backend::SPIRV {

template <u32 bit_size, u32 num_components = 1, bool is_float = false>
std::tuple<Id, Id> ResolveTypeAndZero(EmitContext& ctx) {
    Id result_type{};
    Id zero_value{};
    if constexpr (bit_size == 64 && num_components == 1 && !is_float) {
        result_type = ctx.U64;
        zero_value = ctx.u64_zero_value;
    } else if constexpr (bit_size == 32) {
        if (is_float) {
            result_type = ctx.F32[num_components];
            zero_value = ctx.f32_zero_value;
        } else {
            result_type = ctx.U32[num_components];
            zero_value = ctx.u32_zero_value;
        }
    } else if constexpr (bit_size == 16 && num_components == 1 && !is_float) {
        result_type = ctx.U16;
        zero_value = ctx.u16_zero_value;
    } else if constexpr (bit_size == 8 && num_components == 1 && !is_float) {
        result_type = ctx.U8;
        zero_value = ctx.u8_zero_value;
    } else {
        static_assert(false, "Type not supported.");
    }
    if (num_components > 1) {
        std::array<Id, num_components> zero_ids;
        zero_ids.fill(zero_value);
        zero_value = ctx.ConstantComposite(result_type, zero_ids);
    }
    return {result_type, zero_value};
}

template <u32 bit_size, u32 num_components = 1, bool is_float = false>
auto AccessBoundsCheck(EmitContext& ctx, Id index, Id buffer_size, auto emit_func) {
    if (Sirit::ValidId(buffer_size)) {
        // Bounds checking enabled, wrap in a conditional branch to make sure that
        // the atomic is not mistakenly executed when the index is out of bounds.
        auto compare_index = index;
        if (num_components > 1) {
            compare_index = ctx.OpIAdd(ctx.U32[1], index, ctx.ConstU32(num_components - 1));
        }
        const Id in_bounds = ctx.OpULessThan(ctx.U1[1], compare_index, buffer_size);
        const Id ib_label = ctx.OpLabel();
        const Id end_label = ctx.OpLabel();
        ctx.OpSelectionMerge(end_label, spv::SelectionControlMask::MaskNone);
        ctx.OpBranchConditional(in_bounds, ib_label, end_label);
        const auto last_label = ctx.last_label;
        ctx.AddLabel(ib_label);
        const auto ib_result = emit_func();
        ctx.OpBranch(end_label);
        ctx.AddLabel(end_label);
        if (Sirit::ValidId(ib_result)) {
            const auto [result_type, zero_value] =
                ResolveTypeAndZero<bit_size, num_components, is_float>(ctx);
            return ctx.OpPhi(result_type, ib_result, ib_label, zero_value, last_label);
        } else {
            return Id{0};
        }
    }
    // Bounds checking not enabled, just perform the atomic operation.
    return emit_func();
}

template <u32 bit_size, u32 num_components = 1, bool is_float = false>
static Id LoadAccessBoundsCheck(EmitContext& ctx, Id index, Id buffer_size, Id result) {
    if (Sirit::ValidId(buffer_size)) {
        // Bounds checking enabled, wrap in a select.
        auto compare_index = index;
        if (num_components > 1) {
            compare_index = ctx.OpIAdd(ctx.U32[1], index, ctx.ConstU32(num_components - 1));
        }
        const Id in_bounds = ctx.OpULessThan(ctx.U1[1], compare_index, buffer_size);
        const auto [result_type, zero_value] =
            ResolveTypeAndZero<bit_size, num_components, is_float>(ctx);
        return ctx.OpSelect(result_type, in_bounds, result, zero_value);
    }
    // Bounds checking not enabled, just return the plain value.
    return result;
}

} // namespace Shader::Backend::SPIRV
