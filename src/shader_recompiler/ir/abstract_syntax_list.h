// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <map>
#include <vector>
#include "shader_recompiler/ir/value.h"

namespace Shader::IR {

class Block;

struct AbstractSyntaxNode {
    enum class Type {
        Block,
        If,
        EndIf,
        Loop,
        Repeat,
        Break,
        Return,
        Unreachable,
    };
    union Data {
        Block* block;
        struct {
            U1 cond;
            Block* body;
            Block* merge;
        } if_node;
        struct {
            Block* merge;
        } end_if;
        struct {
            Block* body;
            Block* continue_block;
            Block* merge;
        } loop;
        struct {
            U1 cond;
            Block* loop_header;
            Block* merge;
        } repeat;
        struct {
            U1 cond;
            Block* merge;
            Block* skip;
        } break_node;
    };

    Data data{};
    Type type{};
};
using AbstractSyntaxList = std::vector<AbstractSyntaxNode>;

std::string DumpASLNode(const AbstractSyntaxNode& node,
                        const std::map<const Block*, size_t>& block_to_index,
                        const std::map<const Inst*, size_t>& inst_to_index);

} // namespace Shader::IR
