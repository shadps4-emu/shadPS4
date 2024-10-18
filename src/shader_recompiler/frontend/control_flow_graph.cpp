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

static IR::Condition MakeCondition(const GcnInst& inst) {
    if (inst.IsCmpx()) {
        return IR::Condition::Execnz;
    }

    switch (inst.opcode) {
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
    case Opcode::S_AND_SAVEEXEC_B64:
    case Opcode::S_ANDN2_B64:
        return IR::Condition::Execnz;
    default:
        return IR::Condition::True;
    }
}

static constexpr size_t LabelReserveSize = 32;

CFG::CFG(Common::ObjectPool<Block>& block_pool_, std::span<const GcnInst> inst_list_)
    : block_pool{block_pool_}, inst_list{inst_list_} {
    index_to_pc.resize(inst_list.size() + 1);
    labels.reserve(LabelReserveSize);
    EmitLabels();
    EmitDivergenceLabels();
    EmitBlocks();
    LinkBlocks();
}

void CFG::EmitLabels() {
    // Always set a label at entry point.
    u32 pc = 0;
    AddLabel(pc);

    // Iterate instruction list and add labels to branch targets.
    for (u32 i = 0; i < inst_list.size(); i++) {
        index_to_pc[i] = pc;
        const GcnInst inst = inst_list[i];
        if (inst.IsUnconditionalBranch()) {
            const u32 target = inst.BranchTarget(pc);
            AddLabel(target);
        } else if (inst.IsConditionalBranch()) {
            const u32 true_label = inst.BranchTarget(pc);
            const u32 false_label = pc + inst.length;
            AddLabel(true_label);
            AddLabel(false_label);
        } else if (inst.opcode == Opcode::S_ENDPGM) {
            const u32 next_label = pc + inst.length;
            AddLabel(next_label);
        }
        pc += inst.length;
    }
    index_to_pc[inst_list.size()] = pc;

    // Sort labels to make sure block insertion is correct.
    std::ranges::sort(labels);
}

void CFG::EmitDivergenceLabels() {
    const auto is_open_scope = [](const GcnInst& inst) {
        // An open scope instruction is an instruction that modifies EXEC
        // but also saves the previous value to restore later. This indicates
        // we are entering a scope.
        return inst.opcode == Opcode::S_AND_SAVEEXEC_B64 ||
               // While this instruction does not save EXEC it is often used paired
               // with SAVEEXEC to mask the threads that didn't pass the condition
               // of initial branch.
               (inst.opcode == Opcode::S_ANDN2_B64 && inst.dst[0].field == OperandField::ExecLo) ||
               inst.IsCmpx();
    };
    const auto is_close_scope = [](const GcnInst& inst) {
        // Closing an EXEC scope can be either a branch instruction
        // (typical case when S_AND_SAVEEXEC_B64 is right before a branch)
        // or by a move instruction to EXEC that restores the backup.
        return (inst.opcode == Opcode::S_MOV_B64 && inst.dst[0].field == OperandField::ExecLo) ||
               // Sometimes compiler might insert instructions between the SAVEEXEC and the branch.
               // Those instructions need to be wrapped in the condition as well so allow branch
               // as end scope instruction.
               inst.opcode == Opcode::S_CBRANCH_EXECZ || inst.opcode == Opcode::S_ENDPGM ||
               (inst.opcode == Opcode::S_ANDN2_B64 && inst.dst[0].field == OperandField::ExecLo);
    };

    // Since we will be adding new labels, avoid iterating those as well.
    const size_t end_size = labels.size();
    for (u32 l = 0; l < end_size; l++) {
        const Label start = labels[l];
        // Stop if we reached end of existing labels.
        if (l == end_size - 1) {
            break;
        }
        const Label end = labels[l + 1];
        const size_t end_index = GetIndex(end);

        s32 curr_begin = -1;
        for (size_t index = GetIndex(start); index < end_index; index++) {
            const auto& inst = inst_list[index];
            const bool is_close = is_close_scope(inst);
            if ((is_close || index == end_index - 1) && curr_begin != -1) {
                // If there are no instructions inside scope don't do anything.
                if (index - curr_begin == 1) {
                    curr_begin = -1;
                    continue;
                }
                // Add a label to the instruction right after the open scope call.
                // It is the start of a new basic block.
                const auto& save_inst = inst_list[curr_begin];
                const Label label = index_to_pc[curr_begin] + save_inst.length;
                AddLabel(label);
                // Add a label to the close scope instruction.
                // There are 3 cases where we need to close a scope.
                // * Close scope instruction inside the block
                // * Close scope instruction at the end of the block (cbranch or endpgm)
                // * Normal instruction at the end of the block
                // For the last case we must NOT add a label as that would cause
                // the instruction to be separated into its own basic block.
                if (is_close) {
                    AddLabel(index_to_pc[index]);
                }
                // Reset scope begin.
                curr_begin = -1;
            }
            // Mark a potential start of an exec scope.
            if (is_open_scope(inst)) {
                curr_begin = index;
            }
        }
    }

    // Sort labels to make sure block insertion is correct.
    std::ranges::sort(labels);
}

void CFG::EmitBlocks() {
    for (auto it = labels.cbegin(); it != labels.cend(); ++it) {
        const Label start = *it;
        const auto next_it = std::next(it);
        const bool is_last = (next_it == labels.cend());
        if (is_last) {
            // Last label is special.
            return;
        }
        // The end label is the start instruction of next block.
        // The end instruction of this block is the previous one.
        const Label end = *next_it;
        const size_t end_index = GetIndex(end) - 1;
        const auto& end_inst = inst_list[end_index];

        // Insert block between the labels using the last instruction
        // as an indicator for branching type.
        Block* block = block_pool.Create();
        block->begin = start;
        block->end = end;
        block->begin_index = GetIndex(start);
        block->end_index = end_index;
        block->end_inst = end_inst;
        block->cond = MakeCondition(end_inst);
        blocks.insert(*block);
    }
}

void CFG::LinkBlocks() {
    const auto get_block = [this](u32 address) {
        auto it = blocks.find(address, Compare{});
        ASSERT_MSG(it != blocks.cend() && it->begin == address);
        return &*it;
    };

    for (auto it = blocks.begin(); it != blocks.end(); it++) {
        auto& block = *it;
        const auto end_inst{block.end_inst};
        // Handle divergence block inserted here.
        if (end_inst.opcode == Opcode::S_AND_SAVEEXEC_B64 ||
            end_inst.opcode == Opcode::S_ANDN2_B64 || end_inst.IsCmpx()) {
            // Blocks are stored ordered by address in the set
            auto next_it = std::next(it);
            auto* target_block = &(*next_it);
            ++target_block->num_predecessors;
            block.branch_true = target_block;

            auto merge_it = std::next(next_it);
            auto* merge_block = &(*merge_it);
            ++merge_block->num_predecessors;
            block.branch_false = merge_block;
            block.end_class = EndClass::Branch;
            continue;
        }

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
