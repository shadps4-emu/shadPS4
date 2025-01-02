// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/ir/breadth_first_search.h"
#include "shader_recompiler/ir/ir_emitter.h"
#include "shader_recompiler/ir/program.h"
#include "shader_recompiler/profile.h"

namespace Shader::Optimization {

static void EmitBarrierInBlock(IR::Block* block) {
    // This is inteded to insert a barrier when shared memory write and read
    // occur in the same basic block. Also checks if branch depth is zero as
    // we don't want to insert barrier in potentially divergent code.
    bool emit_barrier_on_write = false;
    bool emit_barrier_on_read = false;
    const auto emit_barrier = [block](bool& emit_cond, IR::Inst& inst) {
        if (emit_cond) {
            IR::IREmitter ir{*block, IR::Block::InstructionList::s_iterator_to(inst)};
            ir.Barrier();
            emit_cond = false;
        }
    };
    for (IR::Inst& inst : block->Instructions()) {
        if (inst.GetOpcode() == IR::Opcode::LoadSharedU32 ||
            inst.GetOpcode() == IR::Opcode::LoadSharedU64) {
            emit_barrier(emit_barrier_on_read, inst);
            emit_barrier_on_write = true;
        }
        if (inst.GetOpcode() == IR::Opcode::WriteSharedU32 ||
            inst.GetOpcode() == IR::Opcode::WriteSharedU64) {
            emit_barrier(emit_barrier_on_write, inst);
            emit_barrier_on_read = true;
        }
    }
}

static void EmitBarrierInMergeBlock(const IR::AbstractSyntaxNode::Data& data) {
    // Insert a barrier after divergent conditional blocks.
    // This avoids potential softlocks and crashes when some threads
    // initialize shared memory and others read from it.
    const IR::U1 cond = data.if_node.cond;
    const auto insert_barrier =
        IR::BreadthFirstSearch(cond, [](IR::Inst* inst) -> std::optional<bool> {
            if (inst->GetOpcode() == IR::Opcode::GetAttributeU32 &&
                inst->Arg(0).Attribute() == IR::Attribute::LocalInvocationId) {
                return true;
            }
            return std::nullopt;
        });
    if (insert_barrier) {
        IR::Block* const merge = data.if_node.merge;
        auto insert_point = std::ranges::find_if_not(merge->Instructions(), IR::IsPhi);
        IR::IREmitter ir{*merge, insert_point};
        ir.Barrier();
    }
}

void SharedMemoryBarrierPass(IR::Program& program, const Profile& profile) {
    if (!program.info.uses_shared || !profile.needs_lds_barriers) {
        return;
    }
    using Type = IR::AbstractSyntaxNode::Type;
    u32 branch_depth{};
    for (const IR::AbstractSyntaxNode& node : program.syntax_list) {
        if (node.type == Type::EndIf) {
            --branch_depth;
            continue;
        }
        if (node.type == Type::If && branch_depth++ == 0) {
            EmitBarrierInMergeBlock(node.data);
            continue;
        }
        if (node.type == Type::Block && branch_depth == 0) {
            EmitBarrierInBlock(node.data.block);
        }
    }
}

} // namespace Shader::Optimization
