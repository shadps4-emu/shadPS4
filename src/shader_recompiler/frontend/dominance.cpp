// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <queue>
#include "shader_recompiler/frontend/dominance.h"

namespace Shader::Gcn {

bool IsCfgBlockDominatedBy(const Shader::Gcn::Block* maybe_dominator,
                           const Shader::Gcn::Block* block, const Shader::Gcn::Block* dest_block) {
    if (block == maybe_dominator) {
        return true;
    }

    boost::container::small_vector<const Shader::Gcn::Block*, 8> visited;
    std::queue<const Shader::Gcn::Block*> queue;
    queue.push(block);

    while (!queue.empty()) {
        const Shader::Gcn::Block* block{queue.front()};
        queue.pop();
        if (block == dest_block) {
            return false;
        }
        if (block == maybe_dominator) {
            continue;
        }
        if (block->branch_false && !std::ranges::contains(visited, block->branch_false)) {
            visited.push_back(block->branch_false);
            queue.push(block->branch_false);
        }
        if (block->branch_true && !std::ranges::contains(visited, block->branch_true)) {
            visited.push_back(block->branch_true);
            queue.push(block->branch_true);
        }
    }

    return true;
}

} // namespace Shader::Gcn