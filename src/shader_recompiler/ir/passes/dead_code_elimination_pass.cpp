// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <unordered_set>
#include <queue>
#include "shader_recompiler/ir/program.h"

namespace Shader::Optimization {

void DeadCodeEliminationPass(IR::Program& program) {
    // We iterate over the instructions in reverse order.
    // This is because removing an instruction reduces the number of uses for earlier instructions.
    std::unordered_set<IR::Inst*> live;
    std::queue<IR::Inst*> worklist;

    for (IR::Block* block : program.post_order_blocks) {
        auto it{block->end()};
        while (it != block->begin()) {
            --it;
            const bool has_side_effects = it->MayHaveSideEffects();
            if (!it->HasUses() && !has_side_effects) {
                it->Invalidate();
                it = block->Instructions().erase(it);
            } else if (has_side_effects) {
                live.insert(&*it);
                worklist.push(&*it);
            }
        }
    }

    while (!worklist.empty()) {
        IR::Inst* inst = worklist.front();
        worklist.pop();

        for (size_t i = 0; i < inst->NumArgs(); i++) {
            if (inst->Arg(i).IsImmediate()) {
                continue;
            }
            IR::Inst* operand = inst->Arg(i).Inst();
            if (live.insert(operand).second) {
                worklist.push(operand);
            }
        }
    }

    for (IR::Block* block : program.post_order_blocks) {
        for (auto it = block->begin(); it != block->end();) {
            if (live.contains(&*it)) {
                ++it;
            } else {
                it->Invalidate();
                it = block->Instructions().erase(it);
            }
        }
    }
}

} // namespace Shader::Optimization
