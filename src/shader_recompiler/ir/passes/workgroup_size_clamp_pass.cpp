// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <bit>
#include <optional>
#include "common/div_ceil.h"
#include "common/logging/log.h"
#include "shader_recompiler/ir/program.h"
#include "shader_recompiler/profile.h"

namespace Shader::Optimization {

// Vulkan drivers cap the number of invocations a compute workgroup may have (commonly 1024 on
// AMD, NVIDIA and Intel), but a guest can declare a bigger one and rely on hardware executing it
// as several waves that all still see the same WorkgroupId. Some shaders build a linear thread
// index out of that id with the idiom `WorkgroupId * declared_group_size + LocalInvocationId`,
// where the multiply is often compiled down to a left shift when the size is a power of two. When
// we recognize exactly that idiom, we can shrink the X dimension of the workgroup to something the
// host supports, grow the dispatch's group count by the same factor, and patch the one constant
// that assumed the original size, so every invocation still computes the same index it originally
// would have. Rasterizer::DispatchDirect/DispatchIndirect apply the resulting
// Info::workgroup_split_factor to the guest's dispatch dimensions.
//
// This is deliberately narrow: a real hardware barrier can't be split across separate real
// workgroups, so this only helps because the pattern above has each invocation address only the
// data at its own linear index, with no dependency on what any other invocation loads or stores.
// We can't prove that in general, so we only apply the transform when we find precisely this
// idiom; anything else (multiple oversized dimensions, no matching pattern, shared/LDS memory in
// use) is left alone and PipelineCache::GetComputePipeline instead skips the pipeline outright,
// exactly as it did before this pass existed.

namespace {

struct ScaleMatch {
    IR::Inst* inst;
    bool is_shift;
};

// Looks for a single direct use of a `GetAttributeU32 WorkgroupId, #dim` result as the left-hand
// side of a ShiftLeftLogical32 or IMul32 by an immediate equal to `size`. Returns nullopt if no
// such use exists, or if the pattern is ambiguous (more than one candidate), since guessing wrong
// here would silently corrupt addressing instead of just failing to speed things up.
std::optional<ScaleMatch> FindSizeScale(IR::Program& program, u32 dim, u32 size) {
    std::optional<ScaleMatch> match;
    for (IR::Block* const block : program.blocks) {
        for (IR::Inst& inst : block->Instructions()) {
            if (inst.GetOpcode() != IR::Opcode::GetAttributeU32 ||
                inst.Arg(0).Attribute() != IR::Attribute::WorkgroupId || inst.Arg(1).U32() != dim) {
                continue;
            }
            for (const auto& [user, operand] : inst.Uses()) {
                if (operand != 0 || !user->Arg(1).IsImmediate()) {
                    continue;
                }
                const bool is_shift = user->GetOpcode() == IR::Opcode::ShiftLeftLogical32;
                const bool is_mul = user->GetOpcode() == IR::Opcode::IMul32;
                if (!is_shift && !is_mul) {
                    continue;
                }
                const u32 scale = is_shift ? (u32(1) << user->Arg(1).U32()) : user->Arg(1).U32();
                if (scale != size) {
                    continue;
                }
                if (match.has_value()) {
                    // More than one candidate; too ambiguous to safely patch.
                    return std::nullopt;
                }
                match = ScaleMatch{.inst = user, .is_shift = is_shift};
            }
        }
    }
    return match;
}

} // namespace

void WorkgroupSizeClampPass(IR::Program& program, RuntimeInfo& runtime_info,
                            const Profile& profile) {
    if (program.info.sw_stage != SwStage::Compute) {
        return;
    }
    auto& cs_info = runtime_info.hw.cs;
    const auto& size = cs_info.workgroup_size;
    const u64 invocations = u64(size[0]) * size[1] * size[2];
    const bool fits = invocations <= profile.max_compute_workgroup_invocations &&
                      size[0] <= profile.max_compute_workgroup_size[0] &&
                      size[1] <= profile.max_compute_workgroup_size[1] &&
                      size[2] <= profile.max_compute_workgroup_size[2];
    if (fits) {
        return;
    }
    // Only handle a single oversized X dimension with Y and Z already within limits, and only
    // when the shader has no shared/LDS memory: a real barrier can't be split across separate
    // real workgroups, so we can't safely reason about correctness once LDS is involved.
    const u64 yz_invocations = u64(size[1]) * size[2];
    if (cs_info.shared_memory_size != 0 || size[1] > profile.max_compute_workgroup_size[1] ||
        size[2] > profile.max_compute_workgroup_size[2] ||
        yz_invocations > profile.max_compute_workgroup_invocations) {
        return;
    }
    const u32 original_x = size[0];
    const u32 max_x = std::min<u32>(
        profile.max_compute_workgroup_size[0],
        u32(profile.max_compute_workgroup_invocations / std::max<u64>(1, yz_invocations)));
    if (max_x == 0) {
        return;
    }
    const u32 split_factor = Common::DivCeil(original_x, max_x);
    if (split_factor <= 1 || original_x % split_factor != 0) {
        // Doesn't divide evenly into equal-sized real workgroups; bail out rather than guess.
        return;
    }
    const u32 new_x = original_x / split_factor;

    const std::optional<ScaleMatch> scale = FindSizeScale(program, 0, original_x);
    if (!scale.has_value()) {
        // Couldn't find (or safely confirm) the expected idiom; leave pipeline creation to fail
        // safely instead of guessing at how the shader builds its addressing.
        return;
    }

    // A ShiftLeftLogical32 match means original_x was a power of two (1 << shift == original_x),
    // and since split_factor evenly divides it, new_x is one too.
    if (scale->is_shift) {
        scale->inst->SetArg(1, IR::Value(u32(std::countr_zero(new_x))));
    } else {
        scale->inst->SetArg(1, IR::Value(new_x));
    }
    cs_info.workgroup_size[0] = new_x;
    program.info.workgroup_split_factor = split_factor;

    LOG_WARNING(Render_Recompiler,
                "Compute shader {:#x} workgroup size {}x{}x{} exceeds device limits; splitting X "
                "into {} dispatches of {}x{}x{} each",
                program.info.pgm_hash, original_x, size[1], size[2], split_factor, new_x, size[1],
                size[2]);
}

} // namespace Shader::Optimization
