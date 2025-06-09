// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/backend/spirv/emit_spirv_instructions.h"
#include "shader_recompiler/backend/spirv/spirv_emit_context.h"

namespace Shader::Backend::SPIRV {

template <u32 bit_size>
auto AccessBoundsCheck(EmitContext& ctx, Id index, Id buffer_size, auto emit_func) {
    Id zero_value{};
    Id result_type{};
    if constexpr (bit_size == 64) {
        zero_value = ctx.u64_zero_value;
        result_type = ctx.U64;
    } else if constexpr (bit_size == 32) {
        zero_value = ctx.u32_zero_value;
        result_type = ctx.U32[1];
    } else if constexpr (bit_size == 16) {
        zero_value = ctx.u16_zero_value;
        result_type = ctx.U16;
    } else {
        static_assert(false, "type not supported");
    }
    if (Sirit::ValidId(buffer_size)) {
        // Bounds checking enabled, wrap in a conditional branch to make sure that
        // the atomic is not mistakenly executed when the index is out of bounds.
        const Id in_bounds = ctx.OpULessThan(ctx.U1[1], index, buffer_size);
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
            return ctx.OpPhi(result_type, ib_result, ib_label, zero_value, last_label);
        } else {
            return Id{0};
        }
    }
    // Bounds checking not enabled, just perform the atomic operation.
    return emit_func();
}

} // namespace Shader::Backend::SPIRV
