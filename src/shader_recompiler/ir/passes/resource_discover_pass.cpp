// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <queue>
#include "shader_recompiler/frontend/control_flow_graph.h"
#include "shader_recompiler/ir/passes/ir_passes.h"
#include "shader_recompiler/ir/passes/resource_pass.h"
#include "shader_recompiler/ir/program.h"
#include "shader_recompiler/profile.h"

namespace Shader::Optimization {

const IR::Inst* TryDisableAnisoLod0(const IR::Inst* inst) {
    // Find sample source trying to disable anisotropy for lod0.
    // Assuming S# is in UD s[12:15] and T# is in s[4:11]
    // The next pattern:
    //  s_bfe_u32     s0, s7,  $0x0008000c
    //  s_and_b32     s1, s12, $0xfffff1ff
    //  s_cmp_eq_u32  s0, 0
    //  s_cselect_b32 s0, s1, s12
    // is used to disable anisotropy in the sampler if the sampled texture doesn't have mips

    if (inst->GetOpcode() != IR::Opcode::SelectU32) {
        return inst;
    }

    // Select should be based on zero check
    const auto* prod0 = inst->Arg(0).InstRecursive();
    if (prod0->GetOpcode() != IR::Opcode::IEqual32 ||
        !(prod0->Arg(1).IsImmediate() && prod0->Arg(1).U32() == 0u)) {
        return inst;
    }

    // The bitfield extract might be hidden by phi sometimes
    auto* prod0_arg0 = prod0->Arg(0).InstRecursive();
    if (prod0_arg0->GetOpcode() == IR::Opcode::Phi) {
        auto arg0 = prod0_arg0->Arg(0);
        auto arg1 = prod0_arg0->Arg(1);
        if (!arg0.IsImmediate() &&
            arg0.InstRecursive()->GetOpcode() == IR::Opcode::BitFieldUExtract) {
            prod0_arg0 = arg0.InstRecursive();
        } else if (!arg1.IsImmediate() &&
                   arg1.InstRecursive()->GetOpcode() == IR::Opcode::BitFieldUExtract) {
            prod0_arg0 = arg1.InstRecursive();
        }
    }

    // The bits range is for lods (note that constants are changed after constant propagation pass)
    if (prod0_arg0->GetOpcode() != IR::Opcode::BitFieldUExtract ||
        !(prod0_arg0->Arg(1).IsImmediate() && prod0_arg0->Arg(1).U32() == 12) ||
        !(prod0_arg0->Arg(2).IsImmediate() && prod0_arg0->Arg(2).U32() == 8)) {
        return inst;
    }

    // Make sure mask is masking out anisotropy
    const auto* prod1 = inst->Arg(1).InstRecursive();
    if (prod1->GetOpcode() != IR::Opcode::BitwiseAnd32 || prod1->Arg(1).U32() != 0xfffff1ff) {
        return inst;
    }

    // We're working on the first dword of s#
    const auto* prod2 = inst->Arg(2).InstRecursive();
    if (prod2->GetOpcode() != IR::Opcode::GetUserData &&
        prod2->GetOpcode() != IR::Opcode::ReadConst && prod2->GetOpcode() != IR::Opcode::Phi) {
        return inst;
    }

    return prod2;
}

bool IsSharpSource(const IR::Inst* inst) {
    return inst->GetOpcode() == IR::Opcode::GetUserData ||
           inst->GetOpcode() == IR::Opcode::ReadConst ||
           inst->GetOpcode() == IR::Opcode::ReadConstBuffer;
}

bool IsCfgBlockDominatedBy(const Shader::Gcn::Block* maybe_dominator,
                           const Shader::Gcn::Block* block, const Shader::Gcn::Block* dest_block) {
    if (block == maybe_dominator) {
        return true;
    }

    boost::container::small_vector<const Shader::Gcn::Block*, 8> visited;
    std::queue<const Shader::Gcn::Block*> queue;
    queue.push(block);

    while (!queue.empty()) {
        const Shader::Gcn::Block* block{queue.front()};
        queue.pop();
        if (block == dest_block) {
            return false;
        }
        if (block == maybe_dominator) {
            continue;
        }
        if (block->branch_false && !std::ranges::contains(visited, block->branch_false)) {
            visited.push_back(block->branch_false);
            queue.push(block->branch_false);
        }
        if (block->branch_true && !std::ranges::contains(visited, block->branch_true)) {
            visited.push_back(block->branch_true);
            queue.push(block->branch_true);
        }
    }

    return true;
}

const IR::Inst* FindSharpSource(IR::Inst* handle, const IR::Block& current_parent, u32 pc = 0) {
    if (IsSharpSource(handle)) {
        return const_cast<IR::Inst*>(handle);
    }

    boost::container::small_vector<IR::Inst*, 8> visited, sources;
    std::queue<IR::Inst*> queue;
    queue.push(handle);

    while (!queue.empty()) {
        IR::Inst* inst{queue.front()};
        queue.pop();
        if (IsSharpSource(inst)) {
            sources.push_back(inst);
            continue;
        }
        if (inst->GetOpcode() != IR::Opcode::Phi) {
            continue;
        }
        for (size_t arg = inst->NumArgs(); arg--;) {
            const IR::Value arg_value = inst->Arg(arg);
            if (arg_value.IsImmediate()) {
                continue;
            }
            IR::Inst* arg_inst = arg_value.InstRecursive();
            if (std::ranges::find(visited, arg_inst) == visited.end()) {
                visited.push_back(arg_inst);
                queue.push(arg_inst);
            }
        }
    }
    if (sources.empty()) {
        UNREACHABLE_MSG("Unable to find sharp sources pc={:#x}", pc);
    }

    // Perform dominance analysis on found sources and eliminate ones that don't pass
    // If a sharp source is dominated by another, the former can be eliminated.
    size_t num_sources = sources.size();
    for (s32 i = 0; i < num_sources;) {
        const IR::Block* block = sources[i]->GetParent();
        ASSERT(block->cfg_block);
        bool was_removed = false;
        for (s32 j = 0; j < num_sources;) {
            const IR::Block* dominator = sources[j]->GetParent();
            ASSERT(dominator->cfg_block);
            if (i != j && IsCfgBlockDominatedBy(dominator->cfg_block, block->cfg_block,
                                                current_parent.cfg_block)) {
                std::swap(sources[i], sources[num_sources - 1]);
                --num_sources;
                sources.pop_back();
                was_removed = true;
                break;
            } else {
                ++j;
            }
        }
        if (!was_removed) {
            ++i;
        }
    }

    ASSERT_MSG(sources.size() == 1, "Unable to deduce sharp source");

    IR::Inst* sharp_source = sources[0];
    if (sharp_source->GetOpcode() == IR::Opcode::ReadConstBuffer) {
        // Set flag so that the flattening pass knows to flatten this instruction.
        auto flags = sharp_source->Flags<IR::BufferInstInfo>();
        flags.sharp_source.Assign(1u);
        sharp_source->SetFlags(flags);
    }

    return sharp_source;
}

void DiscoverBufferSharp(IR::Block& block, IR::Inst& inst, ResourceDiscoveryList& sharp_usages) {
    IR::Inst* handle = inst.Arg(0).InstRecursive();
    if (handle->AreAllArgsImmediates()) {
        // For inmediates, add a sharp usage with null sharp source.
        sharp_usages.emplace_back(ResourceDiscovery{&inst, &block, nullptr});
    } else {
        IR::Inst* buffer_handle = handle->Arg(0).InstRecursive();
        const auto inst_info = inst.Flags<IR::BufferInstInfo>();
        const IR::Inst* sharp_source = FindSharpSource(buffer_handle, block, inst_info.pc);
        sharp_usages.emplace_back(ResourceDiscovery{&inst, &block, sharp_source});
    }
}

void DiscoverImageSharp(IR::Block& block, IR::Inst& inst, ResourceDiscoveryList& sharp_usages) {
    IR::Inst* image_handle = inst.Arg(0).InstRecursive();
    const auto inst_info = inst.Flags<IR::TextureInstInfo>();
    const IR::Inst* sharp_source = FindSharpSource(image_handle, block, inst_info.pc);
    const IR::Inst* sampler_sharp_source = nullptr;

    if (inst.GetOpcode() == IR::Opcode::ImageSampleRaw) {
        const IR::Inst* sampler = inst.Arg(1).InstRecursive();
        if (!sampler->AreAllArgsImmediates()) {
            sampler_sharp_source =
                FindSharpSource(sampler->Arg(0).InstRecursive(), block, inst_info.pc);
        }
    }

    sharp_usages.emplace_back(ResourceDiscovery{&inst, &block, sharp_source, sampler_sharp_source});
}

ResourceDiscoveryList ResourceDiscoverPass(IR::Program& program, const Profile& profile) {
    ResourceDiscoveryList sharp_usages;

    for (IR::Block* const block : program.blocks) {
        for (IR::Inst& inst : block->Instructions()) {
            if (IsBufferInstruction(inst)) {
                DiscoverBufferSharp(*block, inst, sharp_usages);
            } else if (IsImageInstruction(inst)) {
                DiscoverImageSharp(*block, inst, sharp_usages);
            } else if (IsDataRingInstruction(inst)) {
                sharp_usages.emplace_back(ResourceDiscovery{&inst, block, nullptr});
            }
        }
    }

    return sharp_usages;
}

} // namespace Shader::Optimization