// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <bit>
#include <optional>

#include <boost/container/small_vector.hpp>

#include "shader_recompiler/ir/basic_block.h"
#include "shader_recompiler/ir/ir_emitter.h"
#include "shader_recompiler/ir/program.h"
#include "shader_recompiler/runtime_info.h"

namespace Shader::Optimization {
namespace {

struct SharpLocation {
    IR::ScalarReg eud_ptr;
    u32 index_dwords;

    auto operator<=>(const SharpLocation&) const = default;
};

bool IsResourceInstruction(const IR::Inst& inst) {
    switch (inst.GetOpcode()) {
    case IR::Opcode::ReadConstBuffer:
    case IR::Opcode::ReadConstBufferF32:
    case IR::Opcode::ImageSampleExplicitLod:
    case IR::Opcode::ImageSampleImplicitLod:
    case IR::Opcode::ImageSampleDrefExplicitLod:
    case IR::Opcode::ImageSampleDrefImplicitLod:
    case IR::Opcode::ImageFetch:
    case IR::Opcode::ImageGather:
    case IR::Opcode::ImageGatherDref:
    case IR::Opcode::ImageQueryDimensions:
    case IR::Opcode::ImageQueryLod:
    case IR::Opcode::ImageGradient:
    case IR::Opcode::ImageRead:
    case IR::Opcode::ImageWrite:
        return true;
    default:
        return false;
    }
}

/*class Descriptors {
public:
    explicit Descriptors(TextureDescriptors& texture_descriptors_)
        : texture_descriptors{texture_descriptors_} {}

    u32 Add(const TextureDescriptor& desc) {
        const u32 index{Add(texture_descriptors, desc, [&desc](const auto& existing) {
            return desc.type == existing.type && desc.is_depth == existing.is_depth &&
                   desc.has_secondary == existing.has_secondary &&
                   desc.cbuf_index == existing.cbuf_index &&
                   desc.cbuf_offset == existing.cbuf_offset &&
                   desc.shift_left == existing.shift_left &&
                   desc.secondary_cbuf_index == existing.secondary_cbuf_index &&
                   desc.secondary_cbuf_offset == existing.secondary_cbuf_offset &&
                   desc.secondary_shift_left == existing.secondary_shift_left &&
                   desc.count == existing.count && desc.size_shift == existing.size_shift;
        })};
        // TODO: Read this from TIC
        texture_descriptors[index].is_multisample |= desc.is_multisample;
        return index;
    }

private:
    template <typename Descriptors, typename Descriptor, typename Func>
    static u32 Add(Descriptors& descriptors, const Descriptor& desc, Func&& pred) {
        // TODO: Handle arrays
        const auto it{std::ranges::find_if(descriptors, pred)};
        if (it != descriptors.end()) {
            return static_cast<u32>(std::distance(descriptors.begin(), it));
        }
        descriptors.push_back(desc);
        return static_cast<u32>(descriptors.size()) - 1;
    }

    TextureDescriptors& texture_descriptors;
};*/

} // Anonymous namespace

SharpLocation TrackSharp(const IR::Value& handle) {
    IR::Inst* inst = handle.InstRecursive();
    if (inst->GetOpcode() == IR::Opcode::GetScalarRegister) {
        return SharpLocation{
            .eud_ptr = IR::ScalarReg::Max,
            .index_dwords = inst->Arg(0).U32(),
        };
    }
    ASSERT_MSG(inst->GetOpcode() == IR::Opcode::ReadConst, "Sharp load not from constant memory");

    // Retrieve offset from base.
    IR::Inst* addr = inst->Arg(0).InstRecursive();
    u32 dword_offset = addr->Arg(1).U32();
    addr = addr->Arg(0).InstRecursive();
    ASSERT_MSG(addr->Arg(1).IsImmediate(), "Bindless not supported");
    dword_offset += addr->Arg(1).U32() >> 2;

    // Retrieve SGPR that holds sbase
    inst = addr->Arg(0).InstRecursive()->Arg(0).InstRecursive();
    ASSERT_MSG(inst->GetOpcode() == IR::Opcode::GetScalarRegister,
               "Nested resource loads not supported");
    const IR::ScalarReg base = inst->Arg(0).ScalarReg();

    // Return retrieved location.
    return SharpLocation{
        .eud_ptr = base,
        .index_dwords = dword_offset,
    };
}

void ResourceTrackingPass(IR::BlockList& program) {
    for (IR::Block* const block : program) {
        for (IR::Inst& inst : block->Instructions()) {
            if (!IsResourceInstruction(inst)) {
                continue;
            }
            printf("ff\n");
            IR::Inst* producer = inst.Arg(0).InstRecursive();
            const auto loc = TrackSharp(producer->Arg(0));
            fmt::print("Found resource s[{}:{}] is_eud = {}\n", loc.index_dwords,
                       loc.index_dwords + 4, loc.eud_ptr != IR::ScalarReg::Max);
        }
    }
}

} // namespace Shader::Optimization
