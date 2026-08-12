// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "shader_recompiler/ir/breadth_first_search.h"
#include "shader_recompiler/ir/passes/ir_passes.h"
#include "shader_recompiler/ir/passes/resource_pass.h"
#include "shader_recompiler/ir/program.h"
#include "shader_recompiler/profile.h"

namespace Shader::Optimization {

std::pair<IR::Inst*, bool> CheckDisableAnisoLod0Pattern(IR::Inst* inst) {
    // Find sample source trying to disable anisotropy for lod0.
    // Assuming S# is in UD s[12:15] and T# is in s[4:11]
    // The next pattern:
    //  s_bfe_u32     s0, s7,  $0x0008000c
    //  s_and_b32     s1, s12, $0xfffff1ff
    //  s_cmp_eq_u32  s0, 0
    //  s_cselect_b32 s0, s1, s12
    // is used to disable anisotropy in the sampler if the sampled texture doesn't have mips

    if (inst->GetOpcode() != IR::Opcode::SelectU32) {
        return {inst, false};
    }

    // Select should be based on zero check
    const auto* prod0 = inst->Arg(0).InstRecursive();
    if (prod0->GetOpcode() != IR::Opcode::IEqual32 ||
        !(prod0->Arg(1).IsImmediate() && prod0->Arg(1).U32() == 0u)) {
        return {inst, false};
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
        return {inst, false};
    }

    // Make sure mask is masking out anisotropy
    const auto* prod1 = inst->Arg(1).InstRecursive();
    if (prod1->GetOpcode() != IR::Opcode::BitwiseAnd32 || prod1->Arg(1).U32() != 0xfffff1ff) {
        return {inst, false};
    }

    // We're working on the first dword of s#
    auto* prod2 = inst->Arg(2).InstRecursive();
    if (prod2->GetOpcode() != IR::Opcode::GetUserData &&
        prod2->GetOpcode() != IR::Opcode::ReadConst && prod2->GetOpcode() != IR::Opcode::Phi) {
        return {inst, false};
    }

    return {prod2, true};
}

IR::Inst* FindSharpSource(IR::Inst* handle, const IR::Block& current_parent) {
    auto finding = IR::DominatingBreadthFirstSearch(
        handle, current_parent, false, [](IR::Inst* inst) -> std::optional<IR::Inst*> {
            if (inst->GetOpcode() == IR::Opcode::GetUserData ||
                inst->GetOpcode() == IR::Opcode::ReadConst ||
                inst->GetOpcode() == IR::Opcode::ReadConstBuffer) {
                return inst;
            }
            return std::nullopt;
        });

    if (!finding) {
        // We defer the assert to the resource patching pass, since sometimes the sharp is not
        // required (e.g. for fmask)
        return nullptr;
    }

    auto sharp_source = finding.value();
    return sharp_source;
}

void MarkReadConstBufferSharpSources(IR::Inst& first, IR::Block& block, u32 count) {
    auto first_handle = FindSharpSource(&first, block)->Arg(0);
    auto it = block.Instructions().iterator_to(first);
    auto end = block.Instructions().end();
    u32 marked_count = 0;
    for (; it != end && marked_count < count; ++it) {
        IR::Inst& inst = *it;
        if (inst.GetOpcode() == IR::Opcode::SetScalarRegister ||
            inst.GetOpcode() == IR::Opcode::IAdd32) {
            continue;
        }
        auto source = FindSharpSource(&inst, block);
        if (source && source->GetOpcode() == IR::Opcode::ReadConstBuffer) {
            ASSERT(source->Arg(0) == first_handle);
            marked_count++;
            auto flags = source->Flags<IR::BufferInstInfo>();
            flags.sharp_source.Assign(1u);
            source->SetFlags(flags);
            continue;
        }
        break;
    }
    ASSERT(marked_count == count);
}

void DiscoverBufferSharp(IR::Block& block, IR::Inst& inst, ResourceDiscoveryList& sharp_usages) {
    IR::Inst* handle = inst.Arg(0).InstRecursive();
    if (handle->AreAllArgsImmediates()) {
        // For inmediates, add a sharp usage with null sharp source.
        sharp_usages.emplace_back(ResourceDiscovery{&inst, &block, nullptr});
    } else {
        IR::Inst* buffer_handle = handle->Arg(0).InstRecursive();
        IR::Inst* sharp_source = FindSharpSource(buffer_handle, block);
        if (sharp_source && sharp_source->GetOpcode() == IR::Opcode::ReadConstBuffer) {
            MarkReadConstBufferSharpSources(*sharp_source, *sharp_source->GetParent(), 4);
        }
        sharp_usages.emplace_back(ResourceDiscovery{&inst, &block, sharp_source});
    }
}

void DiscoverImageSharp(IR::Block& block, IR::Inst& inst, ResourceDiscoveryList& sharp_usages) {
    IR::Inst* image_handle = inst.Arg(0).InstRecursive();
    IR::Inst* sharp_source = FindSharpSource(image_handle, block);
    IR::Inst* sampler_sharp_source = nullptr;
    bool disable_aniso = false;

    if (inst.GetOpcode() == IR::Opcode::ImageSampleRaw) {
        const IR::Inst* sampler = inst.Arg(1).InstRecursive();
        if (!sampler->AreAllArgsImmediates()) {
            auto [sampler_handle, found] =
                CheckDisableAnisoLod0Pattern(sampler->Arg(0).InstRecursive());
            sampler_sharp_source = FindSharpSource(sampler_handle, block);
            disable_aniso = found;
        }
    }

    if (sharp_source && sharp_source->GetOpcode() == IR::Opcode::ReadConstBuffer) {
        const auto texture_flags = inst.Flags<IR::TextureInstInfo>();
        const auto is_r128 = texture_flags.is_r128.Value();
        MarkReadConstBufferSharpSources(*sharp_source, *sharp_source->GetParent(), is_r128 ? 4 : 8);
    }
    if (sampler_sharp_source && sampler_sharp_source->GetOpcode() == IR::Opcode::ReadConstBuffer) {
        MarkReadConstBufferSharpSources(*sampler_sharp_source, *sampler_sharp_source->GetParent(),
                                        4);
    }

    sharp_usages.emplace_back(ResourceDiscovery{
        &inst,
        &block,
        sharp_source,
        sampler_sharp_source,
        disable_aniso,
    });
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