// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <unordered_map>
#include <queue>
#include "shader_recompiler/ir/program.h"

namespace Shader::Optimization {

static IR::Inst* SearchChain(IR::Inst* inst, u32 lane) {
    while (inst->GetOpcode() == IR::Opcode::WriteLane) {
        if (inst->Arg(2).U32() == lane) {
            // We found a possible write lane source, return it.
            return inst;
        }
        inst = inst->Arg(0).InstRecursive();
    }
    return inst;
}

static bool IsPossibleToEliminate(IR::Inst* inst, u32 lane) {
    // Breadth-first search visiting the right most arguments first
    boost::container::small_vector<IR::Inst*, 16> visited;
    std::queue<IR::Inst*> queue;
    queue.push(inst);

    while (!queue.empty()) {
        // Pop one instruction from the queue
        IR::Inst* inst{queue.front()};
        queue.pop();

        // If it's a WriteLane search for possible candidates
        if (inst = SearchChain(inst, lane); inst->GetOpcode() == IR::Opcode::WriteLane) {
            // We found a possible write lane source, stop looking here.
            continue;
        }
        // If there are other instructions in-between that use the value we can't eliminate.
        if (inst->GetOpcode() != IR::Opcode::ReadLane && inst->GetOpcode() != IR::Opcode::Phi) {
            return false;
        }
        // Visit the right most arguments first
        for (size_t arg = inst->NumArgs(); arg--;) {
            auto arg_value{inst->Arg(arg)};
            if (arg_value.IsImmediate()) {
                continue;
            }
            // Queue instruction if it hasn't been visited
            IR::Inst* arg_inst{arg_value.InstRecursive()};
            if (std::ranges::find(visited, arg_inst) == visited.end()) {
                visited.push_back(arg_inst);
                queue.push(arg_inst);
            }
        }
    }
    return true;
}

using PhiMap = std::unordered_map<IR::Inst*, IR::Inst*>;

static IR::Value GetRealValue(PhiMap& phi_map, IR::Inst* inst, u32 lane) {
    // If this is a WriteLane op search the chain for a possible candidate.
    if (inst = SearchChain(inst, lane); inst->GetOpcode() == IR::Opcode::WriteLane) {
        return inst->Arg(1);
    }

    // If this is a phi, duplicate it and populate its arguments with real values.
    if (inst->GetOpcode() == IR::Opcode::Phi) {
        // We are in a phi cycle, use the already duplicated phi.
        const auto [it, is_new_phi] = phi_map.try_emplace(inst);
        if (!is_new_phi) {
            return IR::Value{it->second};
        }

        // Create new phi and insert it right before the old one.
        const auto insert_point = IR::Block::InstructionList::s_iterator_to(*inst);
        IR::Block* block = inst->GetParent();
        IR::Inst* new_phi{&*block->PrependNewInst(insert_point, IR::Opcode::Phi)};
        new_phi->SetFlags(IR::Type::U32);
        it->second = new_phi;

        // Gather all arguments.
        boost::container::static_vector<IR::Value, 5> phi_args;
        for (size_t arg_index = 0; arg_index < inst->NumArgs(); arg_index++) {
            IR::Inst* arg_prod = inst->Arg(arg_index).InstRecursive();
            const IR::Value arg = GetRealValue(phi_map, arg_prod, lane);
            phi_args.push_back(arg);
        }
        const IR::Value arg0 = phi_args[0].Resolve();
        if (std::ranges::all_of(phi_args,
                                [&](const IR::Value& arg) { return arg.Resolve() == arg0; })) {
            new_phi->ReplaceUsesWith(arg0);
        } else {
            for (size_t arg_index = 0; arg_index < inst->NumArgs(); arg_index++) {
                new_phi->AddPhiOperand(inst->PhiBlock(arg_index), phi_args[arg_index]);
            }
        }
        return IR::Value{new_phi};
    }
    UNREACHABLE();
}

void ReadLaneEliminationPass(IR::Program& program) {
    PhiMap phi_map;
    for (IR::Block* const block : program.blocks) {
        for (IR::Inst& inst : block->Instructions()) {
            if (inst.GetOpcode() != IR::Opcode::ReadLane) {
                continue;
            }
            if (!inst.Arg(1).IsImmediate()) {
                continue;
            }

            const u32 lane = inst.Arg(1).U32();
            IR::Inst* prod = inst.Arg(0).InstRecursive();

            // Check simple case of no control flow and phis
            if (prod = SearchChain(prod, lane); prod->GetOpcode() == IR::Opcode::WriteLane) {
                inst.ReplaceUsesWith(prod->Arg(1));
                continue;
            }

            // Traverse the phi tree to see if it's possible to eliminate
            if (prod->GetOpcode() == IR::Opcode::Phi && IsPossibleToEliminate(prod, lane)) {
                inst.ReplaceUsesWith(GetRealValue(phi_map, prod, lane));
                phi_map.clear();
            }
        }
    }
}

} // namespace Shader::Optimization
