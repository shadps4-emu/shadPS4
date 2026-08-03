// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <utility>

#include "shader_recompiler/backend/spirv/spirv_emit_context.h"

namespace Shader::Backend::SPIRV {

inline Id SharedMemoryAccessInBounds(EmitContext& ctx, Id byte_offset, u32 allocation_size,
                                     u32 access_size) {
    ASSERT(access_size > 0);
    // Express the complete-access check as offset < valid_start_count. This avoids overflowing a
    // dynamic offset while still requiring the final byte of a multi-byte access to fit.
    const u32 valid_start_count =
        allocation_size >= access_size ? allocation_size - access_size + 1 : 0;
    return ctx.OpULessThan(ctx.U1[1], byte_offset, ctx.ConstU32(valid_start_count));
}

template <typename Emit>
Id EmitCheckedSharedResult(EmitContext& ctx, Id byte_offset, u32 allocation_size, u32 access_size,
                           Id result_type, Id zero_value, Emit&& emit) {
    const Id in_bounds = SharedMemoryAccessInBounds(ctx, byte_offset, allocation_size, access_size);
    const Id access_label = ctx.OpLabel();
    const Id merge_label = ctx.OpLabel();
    const Id out_of_bounds_predecessor = ctx.last_label;

    ctx.OpSelectionMerge(merge_label, spv::SelectionControlMask::MaskNone);
    ctx.OpBranchConditional(in_bounds, access_label, merge_label);

    ctx.AddLabel(access_label);
    // The callback constructs the Workgroup pointer as well as performing the access. Keeping it
    // in this block avoids an out-of-range OpAccessChain on the rejected path.
    const Id result = std::forward<Emit>(emit)();
    ctx.OpBranch(merge_label);

    ctx.AddLabel(merge_label);
    return ctx.OpPhi(result_type, result, access_label, zero_value, out_of_bounds_predecessor);
}

template <typename Emit>
void EmitCheckedSharedWrite(EmitContext& ctx, Id byte_offset, u32 allocation_size, u32 access_size,
                            Emit&& emit) {
    const Id in_bounds = SharedMemoryAccessInBounds(ctx, byte_offset, allocation_size, access_size);
    const Id access_label = ctx.OpLabel();
    const Id merge_label = ctx.OpLabel();

    ctx.OpSelectionMerge(merge_label, spv::SelectionControlMask::MaskNone);
    ctx.OpBranchConditional(in_bounds, access_label, merge_label);

    ctx.AddLabel(access_label);
    // As above, do not construct the Workgroup pointer until the legal branch is active.
    std::forward<Emit>(emit)();
    ctx.OpBranch(merge_label);

    ctx.AddLabel(merge_label);
}

} // namespace Shader::Backend::SPIRV
