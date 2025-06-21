// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/backend/spirv/emit_spirv_instructions.h"
#include "shader_recompiler/backend/spirv/spirv_emit_context.h"

namespace Shader::Backend::SPIRV {

Id SubgroupScope(EmitContext& ctx) {
    return ctx.ConstU32(static_cast<u32>(spv::Scope::Subgroup));
}

Id EmitWarpId(EmitContext& ctx) {
    UNREACHABLE();
}

Id EmitLaneId(EmitContext& ctx) {
    return ctx.OpLoad(ctx.U32[1], ctx.subgroup_local_invocation_id);
}

Id EmitQuadShuffle(EmitContext& ctx, Id value, Id index) {
    return ctx.OpGroupNonUniformQuadBroadcast(ctx.U32[1], SubgroupScope(ctx), value, index);
}

Id EmitReadFirstLane(EmitContext& ctx, Id value) {
    return ctx.OpGroupNonUniformBroadcastFirst(ctx.U32[1], SubgroupScope(ctx), value);
}

Id EmitReadLane(EmitContext& ctx, Id value, Id lane) {
    // TODO: proper implementation would need to ensure that `lane` is active in the subgroup
    // by tracking EXEC register more closely, extracting the predicate used, and using
    // it as a parameter to OpGroupNonUniformBallot. If the condition is not satisfied,
    // the result is undefined. It may result in device loss
    //
    // Excerpt from SPIR-V specification:
    // The resulting value is undefined if Id is not part of the scope restricted tangle,
    // or is greater than or equal to the size of the scope.
    return ctx.OpGroupNonUniformBroadcastFirst(ctx.U32[1], SubgroupScope(ctx), value);
    // return ctx.OpGroupNonUniformBroadcast(ctx.U32[1], SubgroupScope(ctx), value,
    //                                       lane);
}

Id EmitWriteLane(EmitContext& ctx, Id value, Id write_value, u32 lane) {
    return ctx.u32_zero_value;
}

} // namespace Shader::Backend::SPIRV
