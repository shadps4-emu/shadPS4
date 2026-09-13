// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/ir/dominance.h"

namespace Shader::IR {

// Those methods are an implementation of the algorithms on "A Simple, Fast Dominance Algorithm".
// https://hexrat.cc/data/pdf/optimization/opt3.pdf

void ComputeDominators(const IR::BlockList& post_order) {
    IR::Block* const entry = post_order.back();
    entry->immediate_dominator = entry;

    const auto intersect = [&](IR::Block* block1, IR::Block* block2) -> IR::Block* {
        while (block1 != block2) {
            while (block1->po_index < block2->po_index) {
                block1 = block1->immediate_dominator;
            }
            while (block2->po_index < block1->po_index) {
                block2 = block2->immediate_dominator;
            }
        }
        return block1;
    };

    bool modified;
    do {
        modified = false;
        for (auto it = post_order.rbegin() + 1; it != post_order.rend(); ++it) {
            IR::Block* block{*it};
            IR::Block* new_idom{};

            for (IR::Block* const pred : block->ImmPredecessors()) {
                if (pred->immediate_dominator) {
                    new_idom = new_idom ? intersect(new_idom, pred) : pred;
                }
            }

            if (block->immediate_dominator != new_idom) {
                block->immediate_dominator = new_idom;
                modified = true;
            }
        }
    } while (modified);
}

void ComputeDominanceFrontiers(const IR::BlockList& post_order) {
    for (IR::Block* block : post_order) {
        if (block->ImmPredecessors().size() < 2) {
            continue;
        }
        for (IR::Block* const pred : block->ImmPredecessors()) {
            IR::Block* current = pred;
            while (current != block->immediate_dominator) {
                current->dominance_frontiers.push_back(block);
                current = current->immediate_dominator;
            }
        }
    }
}

} // namespace Shader::IR