// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <boost/container/flat_set.hpp>
#include <boost/container/small_vector.hpp>
#include "shader_recompiler/ir/post_order.h"

namespace Shader::IR {

BlockList PostOrder(Block* const first_block) {
    boost::container::small_vector<Block*, 16> block_stack;
    boost::container::flat_set<Block*> visited;
    BlockList post_order_blocks;

    visited.insert(first_block);
    block_stack.push_back(first_block);

    while (!block_stack.empty()) {
        Block* const block = block_stack.back();
        const auto visit = [&](Block* branch) {
            if (!visited.insert(branch).second) {
                return false;
            }
            // Calling push_back twice is faster than insert on MSVC
            block_stack.push_back(block);
            block_stack.push_back(branch);
            return true;
        };
        block_stack.pop_back();
        if (std::ranges::none_of(block->ImmSuccessors(), visit)) {
            block->po_index = post_order_blocks.size();
            post_order_blocks.push_back(block);
        }
    }
    return post_order_blocks;
}

} // namespace Shader::IR
