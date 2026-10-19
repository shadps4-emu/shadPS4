// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <optional>
#include <unordered_set>
#include <vector>
#include "shader_recompiler/ir/breadth_first_search.h"
#include "shader_recompiler/ir/ir_emitter.h"
#include "shader_recompiler/ir/program.h"
#include "shader_recompiler/profile.h"

namespace Shader::Optimization {

static bool IsLoadShared(const IR::Inst& inst) {
    return inst.GetOpcode() == IR::Opcode::LoadSharedU16 ||
           inst.GetOpcode() == IR::Opcode::LoadSharedU32 ||
           inst.GetOpcode() == IR::Opcode::LoadSharedU64;
}

static bool IsWriteShared(const IR::Inst& inst) {
    const IR::Opcode opcode = inst.GetOpcode();
    if (opcode >= IR::Opcode::SharedAtomicIAdd32 && opcode <= IR::Opcode::SharedAtomicCmpSwap64) {
        return !inst.Flags<bool>();
    }
    return opcode == IR::Opcode::WriteSharedU16 || opcode == IR::Opcode::WriteSharedU32 ||
           opcode == IR::Opcode::WriteSharedU64;
}

// Inserts barriers when a shared memory write and read occur in the same basic block.
static void EmitBarrierInBlock(IR::Block* block, bool subgroup = false) {
    const auto emit_barrier = [subgroup](IR::IREmitter& ir) {
        if (subgroup) {
            ir.SubgroupBarrier();
        } else {
            ir.Barrier();
        }
    };
    enum class BarrierAction : u32 {
        None,
        BarrierOnWrite,
        BarrierOnRead,
    };
    BarrierAction action{};
    for (IR::Inst& inst : block->Instructions()) {
        if (IsLoadShared(inst)) {
            if (action == BarrierAction::BarrierOnRead) {
                IR::IREmitter ir{*block, IR::Block::InstructionList::s_iterator_to(inst)};
                emit_barrier(ir);
            }
            action = BarrierAction::BarrierOnWrite;
            continue;
        }
        if (IsWriteShared(inst)) {
            if (action == BarrierAction::BarrierOnWrite) {
                IR::IREmitter ir{*block, IR::Block::InstructionList::s_iterator_to(inst)};
                emit_barrier(ir);
            }
            action = BarrierAction::BarrierOnRead;
        }
    }
    if (action != BarrierAction::None) {
        IR::IREmitter ir{*block, --block->end()};
        emit_barrier(ir);
    }
}

using NodeSet = std::unordered_set<const IR::Block*>;

static void EmitBarrierAtBlockStart(IR::Block* block) {
    auto insert_point = std::ranges::find_if_not(block->Instructions(), IR::IsPhi);
    IR::IREmitter ir{*block, insert_point};
    ir.Barrier();
}

static bool IsDivergent(const IR::U1& cond) {
    return IR::BreadthFirstSearch(cond, [](IR::Inst* inst) -> std::optional<bool> {
               if (inst->GetOpcode() == IR::Opcode::GetAttributeU32 &&
                   inst->Arg(0).Attribute() == IR::Attribute::LocalInvocationId) {
                   return true;
               }
               return std::nullopt;
           }) == true;
}

// Inserts a barrier after divergent conditional blocks to avoid undefined
// behavior when some threads write and others read from shared memory.
static void EmitBarrierInMergeBlock(const IR::AbstractSyntaxNode::Data& data,
                                    NodeSet& divergence_end, u32& divergence_depth) {
    const IR::U1 cond = data.if_node.cond;
    if (IsDivergent(cond)) {
        if (divergence_depth == 0) {
            EmitBarrierAtBlockStart(data.if_node.merge);
        }
        ++divergence_depth;
        divergence_end.emplace(data.if_node.merge);
    }
}

// A barrier inside a loop is invalid when different invocations leave on different iterations.
// Mark such loops so their shared-memory synchronization can be deferred to the merge block.
static NodeSet FindDivergentLoops(const IR::AbstractSyntaxList& syntax_list) {
    NodeSet divergent_loops;
    for (const IR::AbstractSyntaxNode& node : syntax_list) {
        switch (node.type) {
        case IR::AbstractSyntaxNode::Type::Repeat:
            if (IsDivergent(node.data.repeat.cond)) {
                divergent_loops.emplace(node.data.repeat.merge);
            }
            break;
        case IR::AbstractSyntaxNode::Type::Break:
            if (IsDivergent(node.data.break_node.cond)) {
                divergent_loops.emplace(node.data.break_node.merge);
            }
            break;
        default:
            break;
        }
    }
    return divergent_loops;
}

// Returns the number of invocations if the value is unique for each invocation in the workgroup.
static std::optional<u32> InvocationIndexCount(const IR::Value& value,
                                               const RuntimeInfo& runtime_info) {
    if (value.IsImmediate()) {
        return std::nullopt;
    }
    const IR::Inst* inst = value.Inst();
    if (inst->GetOpcode() != IR::Opcode::GetAttributeU32) {
        return std::nullopt;
    }
    const auto& size = runtime_info.hw.cs.workgroup_size;
    switch (inst->Arg(0).Attribute()) {
    case IR::Attribute::LocalInvocationIndex:
        return size[0] * size[1] * size[2];
    case IR::Attribute::LocalInvocationId: {
        const u32 comp = inst->Arg(1).U32();
        for (u32 i = 0; i < 3; ++i) {
            if (i != comp && size[i] != 1) {
                return std::nullopt;
            }
        }
        return size[comp];
    }
    default:
        return std::nullopt;
    }
}

struct SharedSlotAccess {
    u64 stride;
    u64 offset;
    u64 size;
    u64 count;
};

// Matches a shared memory address of the form index * stride + offset.
static std::optional<SharedSlotAccess> MatchSharedSlot(IR::Value address,
                                                       const RuntimeInfo& runtime_info) {
    u64 offset{};
    while (!address.IsImmediate()) {
        const IR::Inst* inst = address.Inst();
        if (inst->GetOpcode() != IR::Opcode::IAdd32) {
            break;
        }
        // Addresses are 32-bit, a negative constant wraps around.
        if (inst->Arg(1).IsImmediate()) {
            offset = u32(offset + inst->Arg(1).U32());
            address = inst->Arg(0);
        } else if (inst->Arg(0).IsImmediate()) {
            offset = u32(offset + inst->Arg(0).U32());
            address = inst->Arg(1);
        } else {
            break;
        }
    }
    u64 stride = 1;
    if (!address.IsImmediate()) {
        const IR::Inst* inst = address.Inst();
        if (inst->GetOpcode() == IR::Opcode::ShiftLeftLogical32 && inst->Arg(1).IsImmediate() &&
            inst->Arg(1).U32() < 16) {
            stride = u64{1} << inst->Arg(1).U32();
            address = inst->Arg(0);
        } else if (inst->GetOpcode() == IR::Opcode::IMul32 && inst->Arg(1).IsImmediate()) {
            stride = inst->Arg(1).U32();
            address = inst->Arg(0);
        } else if (inst->GetOpcode() == IR::Opcode::IMul32 && inst->Arg(0).IsImmediate()) {
            stride = inst->Arg(0).U32();
            address = inst->Arg(1);
        }
    }
    const auto count = InvocationIndexCount(address, runtime_info);
    if (!count || stride == 0) {
        return std::nullopt;
    }
    return SharedSlotAccess{.stride = stride, .offset = offset, .count = *count};
}

static std::optional<u32> SharedAccessSize(const IR::Inst& inst) {
    switch (inst.GetOpcode()) {
    case IR::Opcode::LoadSharedU16:
    case IR::Opcode::WriteSharedU16:
        return 2;
    case IR::Opcode::LoadSharedU32:
    case IR::Opcode::WriteSharedU32:
        return 4;
    case IR::Opcode::LoadSharedU64:
    case IR::Opcode::WriteSharedU64:
        return 8;
    default:
        break;
    }
    const IR::Opcode opcode = inst.GetOpcode();
    if (opcode >= IR::Opcode::SharedAtomicIAdd32 && opcode <= IR::Opcode::SharedAtomicCmpSwap64) {
        return inst.Type() == IR::Type::U64 ? 8 : 4;
    }
    return std::nullopt;
}

// Returns true when each invocation only accesses its own bytes of shared memory: every address
// is a unique invocation index times a stride plus a constant, each access stays inside one
// stride-sized slot, and accesses that can overlap use the same slots. Such shaders never
// exchange data through shared memory, so they do not need barriers emulating wave lockstep.
static bool IsSharedMemoryInvocationPrivate(const IR::Program& program,
                                            const RuntimeInfo& runtime_info) {
    std::vector<SharedSlotAccess> accesses;
    for (const IR::Block* block : program.blocks) {
        for (const IR::Inst& inst : block->Instructions()) {
            if (!IsLoadShared(inst) && !IsWriteShared(inst) && !SharedAccessSize(inst)) {
                continue;
            }
            const auto size = SharedAccessSize(inst);
            if (!size || inst.Flags<bool>()) {
                // Unknown access or GDS.
                return false;
            }
            auto access = MatchSharedSlot(inst.Arg(0), runtime_info);
            if (!access) {
                return false;
            }
            // The SPIR-V backend indexes shared memory by address >> log2(size).
            if (access->stride % *size != 0) {
                return false;
            }
            access->offset &= ~u64{*size - 1};
            access->size = *size;
            if (access->offset % access->stride + access->size > access->stride) {
                return false;
            }
            // Every invocation must stay inside the declared shared memory without wrapping.
            const u64 end = access->offset + (access->count - 1) * access->stride + access->size;
            if (end > runtime_info.hw.cs.shared_memory_size) {
                return false;
            }
            accesses.push_back(*access);
        }
    }
    if (accesses.empty()) {
        return false;
    }
    for (size_t i = 0; i < accesses.size(); ++i) {
        for (size_t j = i + 1; j < accesses.size(); ++j) {
            const auto& a = accesses[i];
            const auto& b = accesses[j];
            const u64 a_end = a.offset + (a.count - 1) * a.stride + a.size;
            const u64 b_end = b.offset + (b.count - 1) * b.stride + b.size;
            if (a_end <= b.offset || b_end <= a.offset) {
                continue;
            }
            if (a.stride != b.stride || a.offset / a.stride != b.offset / b.stride) {
                return false;
            }
        }
    }
    return true;
}

static constexpr u32 GcnSubgroupSize = 64;

void SharedMemoryBarrierPass(IR::Program& program, const RuntimeInfo& runtime_info,
                             const Profile& profile) {
    if (program.info.hw_stage != HwStage::Compute) {
        return;
    }
    const auto& cs_info = runtime_info.hw.cs;
    const u32 shared_memory_size = cs_info.shared_memory_size;
    const u32 threadgroup_size =
        cs_info.workgroup_size[0] * cs_info.workgroup_size[1] * cs_info.workgroup_size[2];
    if (shared_memory_size == 0) {
        return;
    }
    // The compiler can only omit barriers when the local workgroup size is the same as the HW
    // subgroup.
    if (threadgroup_size != GcnSubgroupSize || !profile.needs_lds_barriers) {
        // GCN waves can use shared memory between invocations without explicit barriers because
        // they run in lockstep. Host compilers can reorder these accesses, so keep their order
        // with subgroup memory barriers.
        for (const IR::AbstractSyntaxNode& node : program.syntax_list) {
            if (node.type == IR::AbstractSyntaxNode::Type::Block) {
                EmitBarrierInBlock(node.data.block, true);
            }
        }
        return;
    }
    if (IsSharedMemoryInvocationPrivate(program, runtime_info)) {
        return;
    }
    using Type = IR::AbstractSyntaxNode::Type;
    u32 divergence_depth{};
    NodeSet divergence_end;
    const NodeSet divergent_loops = FindDivergentLoops(program.syntax_list);
    for (const IR::AbstractSyntaxNode& node : program.syntax_list) {
        if (node.type == Type::EndIf) {
            if (divergence_end.contains(node.data.end_if.merge)) {
                --divergence_depth;
            }
            continue;
        }
        // Check if branch depth is zero, we don't want to insert barrier in potentially divergent
        // code.
        if (node.type == Type::If) {
            EmitBarrierInMergeBlock(node.data, divergence_end, divergence_depth);
            continue;
        }
        if (node.type == Type::Loop && divergent_loops.contains(node.data.loop.merge)) {
            ++divergence_depth;
            continue;
        }
        if (node.type == Type::Repeat && divergent_loops.contains(node.data.repeat.merge)) {
            ASSERT(divergence_depth > 0);
            --divergence_depth;
            if (divergence_depth == 0) {
                EmitBarrierAtBlockStart(node.data.repeat.merge);
            }
            continue;
        }
        if (node.type == Type::Block && divergence_depth == 0) {
            EmitBarrierInBlock(node.data.block);
        }
    }
}

} // namespace Shader::Optimization
