// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/ir/basic_block.h"
#include "shader_recompiler/ir/conditional_tree.h"

#include <span>

// This can be used to get, for a given block, the list of conditions that
// must be true for the block to be executed. Can be also useful for
// for determining the maximum number of thimes a block is executed.

namespace Shader::IR {

static void AddConditionalTree(std::span<AbstractSyntaxNode> asl_span,
                               Block::ConditionalData* parent) {
    const auto get_span = [&asl_span](AbstractSyntaxNode& node,
                                      Block* merge_block) -> std::span<AbstractSyntaxNode> {
        auto it = std::find_if(asl_span.begin(), asl_span.end(),
                               [&node, &merge_block](const AbstractSyntaxNode& n) {
                                   return n.data.block == merge_block;
                               });
        ASSERT(it != asl_span.end());
        std::ptrdiff_t merge_index = std::distance(asl_span.begin(), it);
        return std::span<AbstractSyntaxNode>(&node + 1, asl_span.data() + merge_index);
    };
    const Block::ConditionalData* copied_parent = nullptr;
    for (auto it = asl_span.begin(); it < asl_span.end(); ++it) {
        AbstractSyntaxNode& node = *it;
        if (node.type == AbstractSyntaxNode::Type::If ||
            node.type == AbstractSyntaxNode::Type::Loop) {
            ASSERT(copied_parent);
            Block* merge_block;
            switch (node.type) {
            case AbstractSyntaxNode::Type::If:
                merge_block = node.data.if_node.merge;
                break;
            case AbstractSyntaxNode::Type::Loop:
                merge_block = node.data.loop.merge;
                break;
            default:
                UNREACHABLE();
            }
            auto subspan = get_span(node, merge_block);
            Block::ConditionalData cond{copied_parent->depth + 1, copied_parent, &node};
            AddConditionalTree(subspan, &cond);
            it += subspan.size();
        } else if (node.type == AbstractSyntaxNode::Type::Block) {
            Block* block = node.data.block;
            if (!copied_parent) {
                block->SetConditionalData(*parent);
                copied_parent = &block->CondData();
            } else {
                block->SetConditionalData(*copied_parent);
            }
        }
    }
}

void AddConditionalTreeFromASL(AbstractSyntaxList& syntax_list) {
    Block::ConditionalData cond{0, nullptr, nullptr};
    AddConditionalTree(syntax_list, &cond);
}

} // namespace Shader::IR
