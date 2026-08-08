// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <optional>
#include <type_traits>
#include <boost/container/small_vector.hpp>
#include <queue>
#include "shader_recompiler/frontend/dominance.h"
#include "shader_recompiler/ir/basic_block.h"
#include "shader_recompiler/ir/value.h"

namespace Shader::IR {

// Use typename Instruction so the function can be used to return either const or mutable
// Insts depending on the context.
template <typename Instruction, typename Pred>
auto BreadthFirstSearch(Instruction* inst, Pred&& pred)
    -> std::invoke_result_t<Pred, Instruction*> {
    // Most often case the instruction is the desired already.
    if (std::optional result = pred(inst)) {
        return result;
    }

    // Breadth-first search visiting the right most arguments first
    boost::container::small_vector<Instruction*, 2> visited;
    std::queue<Instruction*> queue;
    queue.push(inst);

    while (!queue.empty()) {
        // Pop one instruction from the queue
        Instruction* inst{queue.front()};
        queue.pop();
        if (std::optional result = pred(inst)) {
            // This is the instruction we were looking for
            return result;
        }
        // Visit the right most arguments first
        for (size_t arg = inst->NumArgs(); arg--;) {
            Value arg_value{inst->Arg(arg)};
            if (arg_value.IsImmediate()) {
                continue;
            }
            // Queue instruction if it hasn't been visited
            Instruction* arg_inst{arg_value.InstRecursive()};
            if (std::ranges::find(visited, arg_inst) == visited.end()) {
                visited.push_back(arg_inst);
                queue.push(arg_inst);
            }
        }
    }
    // SSA tree has been traversed and the result hasn't been found
    return std::nullopt;
}

template <typename Pred>
auto BreadthFirstSearch(const Value& value, Pred&& pred)
    -> std::invoke_result_t<Pred, const Inst*> {
    if (value.IsImmediate()) {
        // Nothing to do with immediates
        return std::nullopt;
    }
    return BreadthFirstSearch(value.InstRecursive(), pred);
}

template <typename Pred>
auto BreadthFirstSearch(Value value, Pred&& pred) -> std::invoke_result_t<Pred, Inst*> {
    if (value.IsImmediate()) {
        // Nothing to do with immediates
        return std::nullopt;
    }
    return BreadthFirstSearch(value.InstRecursive(), pred);
}

template <typename Instruction, typename Pred>
auto DominatingBreadthFirstSearch(Instruction* inst, const IR::Block& current_parent, bool deep,
                                  Pred&& pred) -> std::invoke_result_t<Pred, Instruction*> {
    if (std::optional result = pred(inst)) {
        return result;
    }

    boost::container::small_vector<IR::Inst*, 8> visited, findings;
    std::queue<IR::Inst*> queue;
    queue.push(inst);

    while (!queue.empty()) {
        IR::Inst* inst{queue.front()};
        queue.pop();
        if (std::optional result = pred(inst)) {
            findings.push_back(result.value());
            continue;
        }
        if (inst->GetOpcode() != IR::Opcode::Phi && !deep) {
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
    if (findings.empty()) {
        return std::nullopt;
    }

    // Perform dominance analysis on findings and eliminate ones that don't pass
    // If a finding is dominated by another, the former can be eliminated.
    size_t num_findings = findings.size();
    for (s32 i = 0; i < num_findings;) {
        const IR::Block* block = findings[i]->GetParent();
        ASSERT(block->cfg_block);
        bool was_removed = false;
        for (s32 j = 0; j < num_findings;) {
            const IR::Block* dominator = findings[j]->GetParent();
            ASSERT(dominator->cfg_block);
            if (i != j && Gcn::IsCfgBlockDominatedBy(dominator->cfg_block, block->cfg_block,
                                                     current_parent.cfg_block)) {
                std::swap(findings[i], findings[num_findings - 1]);
                --num_findings;
                findings.pop_back();
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

    ASSERT_MSG(findings.size() == 1, "Unable to deduce correct finding");
    return findings[0];
}

template <typename Pred>
auto DominatingBreadthFirstSearch(const Value& value, const IR::Block& current_parent, bool deep,
                                  Pred&& pred) -> std::invoke_result_t<Pred, const Inst*> {
    if (value.IsImmediate()) {
        // Nothing to do with immediates
        return std::nullopt;
    }
    return DominatingBreadthFirstSearch(value.InstRecursive(), current_parent, deep, pred);
}

template <typename Pred>
auto DominatingBreadthFirstSearch(Value value, const IR::Block& current_parent, bool deep,
                                  Pred&& pred) -> std::invoke_result_t<Pred, Inst*> {
    if (value.IsImmediate()) {
        // Nothing to do with immediates
        return std::nullopt;
    }
    return DominatingBreadthFirstSearch(value.InstRecursive(), current_parent, deep, pred);
}

} // namespace Shader::IR
