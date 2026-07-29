// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <queue>
#include "shader_recompiler/frontend/dominance.h"
#include "shader_recompiler/ir/basic_block.h"

namespace Shader::IR {

template <typename Instruction, typename Pred>
auto DominanceSearch(Instruction* inst, const IR::Block& current_parent, bool deep, Pred&& pred)
    -> std::invoke_result_t<Pred, Instruction*> {
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
            IR::Inst* arg_inst = arg_value.Inst();
            if (std::ranges::find(visited, arg_inst) == visited.end()) {
                visited.push_back(arg_inst);
                queue.push(arg_inst);
            }
        }
    }
    if (findings.empty()) {
        return std::nullopt;
    }

    Gcn::EliminateNonDominantInstructions(findings, current_parent);
    ASSERT_MSG(findings.size() == 1, "Unable to deduce correct finding");
    return findings[0];
}

template <typename Pred>
auto DominanceSearch(const Value& value, const IR::Block& current_parent, bool deep, Pred&& pred)
    -> std::invoke_result_t<Pred, const Inst*> {
    if (value.IsImmediate()) {
        // Nothing to do with immediates
        return std::nullopt;
    }
    return DominanceSearch(value.Inst(), current_parent, deep, pred);
}

template <typename Pred>
auto DominanceSearch(Value value, const IR::Block& current_parent, bool deep, Pred&& pred)
    -> std::invoke_result_t<Pred, Inst*> {
    if (value.IsImmediate()) {
        // Nothing to do with immediates
        return std::nullopt;
    }
    return DominanceSearch(value.Inst(), current_parent, deep, pred);
}

} // namespace Shader::IR