// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "shader_recompiler/ir/dominance_search.h"
#include "shader_recompiler/ir/passes/ir_passes.h"
#include "shader_recompiler/ir/passes/resource_pass.h"
#include "shader_recompiler/ir/program.h"
#include "shader_recompiler/profile.h"

namespace Shader::Optimization {

static bool IsSharpSource(const IR::Inst* inst) {
    return inst->GetOpcode() == IR::Opcode::GetUserData ||
           inst->GetOpcode() == IR::Opcode::ReadConst ||
           inst->GetOpcode() == IR::Opcode::ReadConstBuffer;
}

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
    const auto* prod0 = inst->Arg(0).Inst();
    if (prod0->GetOpcode() != IR::Opcode::IEqual32 ||
        !(prod0->Arg(1).IsImmediate() && prod0->Arg(1).U32() == 0u)) {
        return {inst, false};
    }

    auto* prod0_arg0 = prod0->Arg(0).Inst();
    ASSERT(prod0_arg0->GetOpcode() != IR::Opcode::Phi);

    // The bits range is for lods (note that constants are changed after constant propagation pass)
    if (prod0_arg0->GetOpcode() != IR::Opcode::BitFieldUExtract ||
        !(prod0_arg0->Arg(1).IsImmediate() && prod0_arg0->Arg(1).U32() == 12) ||
        !(prod0_arg0->Arg(2).IsImmediate() && prod0_arg0->Arg(2).U32() == 8)) {
        return {inst, false};
    }

    // Make sure mask is masking out anisotropy
    const auto* prod1 = inst->Arg(1).Inst();
    if (prod1->GetOpcode() != IR::Opcode::BitwiseAnd32 || prod1->Arg(1).U32() != 0xfffff1ff) {
        return {inst, false};
    }

    // We're working on the first dword of s#
    auto* prod2 = inst->Arg(2).Inst();
    ASSERT(prod2->GetOpcode() != IR::Opcode::Phi);
    return {prod2, true};
}

IR::Inst* FindSharpSource(IR::Inst* handle) {
    ASSERT(IsSharpSource(handle));
    return handle;
}

void MarkReadConstBufferSharpSources(const IR::Inst* handle, const IR::Inst** out_sharp_dwords = nullptr) {
    for (size_t arg = 0; arg < handle->NumArgs(); ++arg) {
        IR::Inst* sharp_dword = handle->Arg(arg).Inst();
        auto source = FindSharpSource(sharp_dword);
        if (source && source->GetOpcode() == IR::Opcode::ReadConstBuffer) {
            auto flags = source->Flags<IR::BufferInstInfo>();
            flags.sharp_source.Assign(1u);
            source->SetFlags(flags);
        }
        if (out_sharp_dwords) {
            *(out_sharp_dwords++) = source;
        }
    }
}

void DiscoverBufferSharp(IR::Block& block, IR::Inst& inst, ResourceDiscoveryList& sharp_usages) {
    IR::Inst* handle = inst.Arg(0).Inst();
    auto& resource = sharp_usages.emplace_back(&inst);

    if (!handle->AreAllArgsImmediates()) {
        MarkReadConstBufferSharpSources(handle, resource.sharp_dwords.data());
        resource.num_dwords = handle->NumArgs();
    }
}

void DiscoverImageSharp(IR::Block& block, IR::Inst& inst, ResourceDiscoveryList& sharp_usages) {
    IR::Inst* image_handle = inst.Arg(0).Inst();
    ASSERT(image_handle->GetOpcode() == IR::Opcode::ImageHandle);
    auto& resource = sharp_usages.emplace_back(&inst);

    if (inst.GetOpcode() == IR::Opcode::ImageSampleRaw) {
        const IR::Inst* sampler = inst.Arg(1).Inst();
        if (!sampler->AreAllArgsImmediates()) {
            auto [sampler_handle, found] =
                CheckDisableAnisoLod0Pattern(sampler->Arg(0).Inst());
            resource.sampler_sharp_source = FindSharpSource(sampler_handle);
            resource.disable_aniso = found;
            MarkReadConstBufferSharpSources(sampler);
        }
    }

    IR::Inst* tsharp_low = image_handle->Arg(0).Inst();
    MarkReadConstBufferSharpSources(tsharp_low, resource.sharp_dwords.data());
    resource.num_dwords = tsharp_low->NumArgs();

    if (auto tsharp_high = image_handle->Arg(1).TryInst()) {
        MarkReadConstBufferSharpSources(tsharp_high, resource.sharp_dwords.data() + tsharp_low->NumArgs());
        resource.num_dwords += tsharp_high->NumArgs();
    }
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
                sharp_usages.emplace_back(&inst);
            }
        }
    }
    return sharp_usages;
}

} // namespace Shader::Optimization