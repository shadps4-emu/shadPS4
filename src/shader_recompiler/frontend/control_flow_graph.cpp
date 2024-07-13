// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include "common/assert.h"
#include "shader_recompiler/frontend/control_flow_graph.h"

namespace Shader::Gcn {

struct Compare {
    bool operator()(const Block& lhs, u32 rhs) const noexcept {
        return lhs.begin < rhs;
    }

    bool operator()(u32 lhs, const Block& rhs) const noexcept {
        return lhs < rhs.begin;
    }

    bool operator()(const Block& lhs, const Block& rhs) const noexcept {
        return lhs.begin < rhs.begin;
    }
};

static IR::Condition MakeCondition(Opcode opcode) {
    switch (opcode) {
    case Opcode::S_CBRANCH_SCC0:
        return IR::Condition::Scc0;
    case Opcode::S_CBRANCH_SCC1:
        return IR::Condition::Scc1;
    case Opcode::S_CBRANCH_VCCZ:
        return IR::Condition::Vccz;
    case Opcode::S_CBRANCH_VCCNZ:
        return IR::Condition::Vccnz;
    case Opcode::S_CBRANCH_EXECZ:
        return IR::Condition::Execz;
    case Opcode::S_CBRANCH_EXECNZ:
        return IR::Condition::Execnz;
    default:
        return IR::Condition::True;
    }
}

CFG::CFG(ObjectPool<Block>& block_pool_, std::span<const GcnInst> inst_list_)
    : block_pool{block_pool_}, inst_list{inst_list_} {
    index_to_pc.resize(inst_list.size() + 1);
    EmitLabels();
    EmitBlocks();
    LinkBlocks();
}

void CFG::EmitLabels() {
    // Always set a label at entry point.
    u32 pc = 0;
    labels.push_back(pc);

    const auto add_label = [this](u32 address) {
        const auto it = std::ranges::find(labels, address);
        if (it == labels.end()) {
            labels.push_back(address);
        }
    };

    // Iterate instruction list and add labels to branch targets.
    for (u32 i = 0; i < inst_list.size(); i++) {
        index_to_pc[i] = pc;
        const GcnInst inst = inst_list[i];
        if (inst.IsUnconditionalBranch()) {
            const u32 target = inst.BranchTarget(pc);
            add_label(target);
        } else if (inst.IsConditionalBranch()) {
            const u32 true_label = inst.BranchTarget(pc);
            const u32 false_label = pc + inst.length;
            add_label(true_label);
            add_label(false_label);
        } else if (inst.opcode == Opcode::S_ENDPGM) {
            const u32 next_label = pc + inst.length;
            add_label(next_label);
        }
        pc += inst.length;
    }
    index_to_pc[inst_list.size()] = pc;

    // Sort labels to make sure block insertion is correct.
    std::ranges::sort(labels);
}

void CFG::EmitBlocks() {
    const auto get_index = [this](Label label) -> size_t {
        if (label == 0) {
            return 0ULL;
        }
        const auto it_index = std::ranges::lower_bound(index_to_pc, label);
        ASSERT(it_index != index_to_pc.end() || label > index_to_pc.back());
        return std::distance(index_to_pc.begin(), it_index);
    };

    for (auto it = labels.begin(); it != labels.end(); it++) {
        const Label start = *it;
        const auto next_it = std::next(it);
        const bool is_last = next_it == labels.end();
        if (is_last) {
            // Last label is special.
            return;
        }
        const Label end = *next_it;
        const size_t end_index = get_index(end) - 1;
        const auto& end_inst = inst_list[end_index];

        // Insert block between the labels using the last instruction
        // as an indicator for branching type.
        Block* block = block_pool.Create();
        block->begin = start;
        block->end = end;
        block->begin_index = get_index(start);
        block->end_index = end_index;
        block->end_inst = end_inst;
        block->cond = MakeCondition(end_inst.opcode);
        blocks.insert(*block);
    }
}

void CFG::LinkBlocks() {
    const auto get_block = [this](u32 address) {
        auto it = blocks.find(address, Compare{});
        ASSERT_MSG(it != blocks.end() && it->begin == address);
        return &*it;
    };

    for (auto& block : blocks) {
        const auto end_inst{block.end_inst};
        // If the block doesn't end with a branch we simply
        // need to link with the next block.
        if (!end_inst.IsTerminateInstruction()) {
            auto* next_block = get_block(block.end);
            ++next_block->num_predecessors;

            block.branch_true = next_block;
            block.end_class = EndClass::Branch;
            continue;
        }

        // Find the branch targets from the instruction and link the blocks.
        // Note: Block end address is one instruction after end_inst.
        const u32 branch_pc = block.end - end_inst.length;
        const u32 target_pc = end_inst.BranchTarget(branch_pc);
        if (end_inst.IsUnconditionalBranch()) {
            auto* target_block = get_block(target_pc);
            ++target_block->num_predecessors;

            block.branch_true = target_block;
            block.end_class = EndClass::Branch;
        } else if (end_inst.IsConditionalBranch()) {
            auto* target_block = get_block(target_pc);
            ++target_block->num_predecessors;

            auto* end_block = get_block(block.end);
            ++end_block->num_predecessors;

            block.branch_true = target_block;
            block.branch_false = end_block;
            block.end_class = EndClass::Branch;
        } else if (end_inst.opcode == Opcode::S_ENDPGM) {
            const auto& prev_inst = inst_list[block.end_index - 1];
            if (prev_inst.opcode == Opcode::EXP && prev_inst.control.exp.en == 0) {
                if (prev_inst.control.exp.target != 9) {
                    block.end_class = EndClass::Kill;
                } else if (const auto& exec_mask = inst_list[block.end_index - 2];
                           exec_mask.src[0].field == OperandField::ConstZero) {
                    block.end_class = EndClass::Kill;
                } else {
                    block.end_class = EndClass::Exit;
                }
            } else {
                block.end_class = EndClass::Exit;
            }
        } else {
            UNREACHABLE();
        }
    }
}

std::string CFG::Dot() const {
    int node_uid{0};

    const auto name_of = [](const Block& block) { return fmt::format("\"{:#x}\"", block.begin); };

    std::string dot{"digraph shader {\n"};
    dot += fmt::format("\tsubgraph cluster_{} {{\n", 0);
    dot += fmt::format("\t\tnode [style=filled];\n");
    for (const Block& block : blocks) {
        const std::string name{name_of(block)};
        const auto add_branch = [&](Block* branch, bool add_label) {
            dot += fmt::format("\t\t{}->{}", name, name_of(*branch));
            if (add_label && block.cond != IR::Condition::True &&
                block.cond != IR::Condition::False) {
                dot += fmt::format(" [label=\"{}\"]", block.cond);
            }
            dot += '\n';
        };
        dot += fmt::format("\t\t{};\n", name);
        switch (block.end_class) {
        case EndClass::Branch:
            if (block.cond != IR::Condition::False) {
                add_branch(block.branch_true, true);
            }
            if (block.cond != IR::Condition::True) {
                add_branch(block.branch_false, false);
            }
            break;
        case EndClass::Exit:
            dot += fmt::format("\t\t{}->N{};\n", name, node_uid);
            dot +=
                fmt::format("\t\tN{} [label=\"Exit\"][shape=square][style=stripped];\n", node_uid);
            ++node_uid;
            break;
        case EndClass::Kill:
            dot += fmt::format("\t\t{}->N{};\n", name, node_uid);
            dot +=
                fmt::format("\t\tN{} [label=\"Kill\"][shape=square][style=stripped];\n", node_uid);
            ++node_uid;
            break;
        }
    }
    dot += "\t\tlabel = \"main\";\n\t}\n";
    if (blocks.empty()) {
        dot += "Start;\n";
    } else {
        dot += fmt::format("\tStart -> {};\n", name_of(*blocks.begin()));
    }
    dot += fmt::format("\tStart [shape=diamond];\n");
    dot += "}\n";
    return dot;
}

} // namespace Shader::Gcn
