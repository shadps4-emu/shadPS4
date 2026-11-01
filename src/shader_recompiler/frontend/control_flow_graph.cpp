// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <limits>
#include "common/assert.h"
#include "common/logging/log.h"
#include "shader_recompiler/frontend/control_flow_graph.h"
#include "shader_recompiler/ir/condition.h"

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
    default:
        return IR::Condition::True;
    }
}

static bool IgnoresExecMask(const GcnInst& inst) {
    // EXEC mask does not affect scalar instructions or branches.
    switch (inst.category) {
    case InstCategory::ScalarALU:
    case InstCategory::ScalarMemory:
    case InstCategory::FlowControl:
        return true;
    default:
        break;
    }
    // Read/Write Lane instructions are not affected either.
    switch (inst.opcode) {
    case Opcode::V_READLANE_B32:
    case Opcode::V_WRITELANE_B32:
    case Opcode::V_READFIRSTLANE_B32:
        return true;
    default:
        break;
    }
    return false;
}

static std::optional<u32> ResolveSetPcTarget(std::span<const GcnInst> list, u32 setpc_index,
                                             std::span<const u32> pc_map) {
    if (setpc_index < 3) {
        return std::nullopt;
    }

    const auto& getpc = list[setpc_index - 3];
    const auto& arith = list[setpc_index - 2];
    const auto& setpc = list[setpc_index];

    if (getpc.opcode != Opcode::S_GETPC_B64 ||
        !(arith.opcode == Opcode::S_ADD_U32 || arith.opcode == Opcode::S_SUB_U32) ||
        setpc.opcode != Opcode::S_SETPC_B64)
        return std::nullopt;

    if (getpc.dst[0].code != setpc.src[0].code || arith.dst[0].code != setpc.src[0].code)
        return std::nullopt;

    if (arith.src_count < 2 || arith.src[1].field != OperandField::LiteralConst)
        return std::nullopt;

    const u32 imm = arith.src[1].code;

    const s32 signed_offset =
        (arith.opcode == Opcode::S_ADD_U32) ? static_cast<s32>(imm) : -static_cast<s32>(imm);

    const u32 base_pc = pc_map[setpc_index - 3] + getpc.length;

    const u32 result_pc = static_cast<u32>(static_cast<s32>(base_pc) + signed_offset);
    LOG_DEBUG(Render_Recompiler, "SetPC target: {} + {} = {}", base_pc, signed_offset, result_pc);
    return result_pc & ~0x3u;
}

static std::optional<u32> GetConstant(const InstOperand& op) {
    switch (op.field) {
    case OperandField::ConstZero:
        return 0U;
    case OperandField::SignedConstIntPos:
        return op.code - SignedConstIntPosMin + 1;
    case OperandField::LiteralConst:
        return op.code;
    default:
        return std::nullopt;
    }
}

static bool WritesSgpr(const GcnInst& inst, u32 reg) {
    return std::ranges::any_of(inst.dst, [&](const InstOperand& dst) {
        if (dst.field != OperandField::ScalarGPR) {
            return false;
        }
        u32 count = 1;
        if (inst.category == InstCategory::ScalarMemory) {
            count = inst.control.smrd.count;
        } else if (dst.type == ScalarType::Uint64 || dst.type == ScalarType::Sint64 ||
                   dst.type == ScalarType::Float64) {
            count = 2;
        }
        return reg >= dst.code && reg < dst.code + count;
    });
}

/// Returns the index of the last instruction before `index` that writes SGPR `reg`.
static std::optional<u32> FindSgprWriter(std::span<const GcnInst> list, u32 index, u32 reg) {
    while (index-- > 0) {
        if (WritesSgpr(list[index], reg)) {
            return index;
        }
    }
    return std::nullopt;
}

/// Returns the constant operand of `inst` if its other operand is SGPR `reg`.
static std::optional<u32> GetConstantWithSgpr(const GcnInst& inst, u32 reg) {
    for (u32 i = 0; i < 2; i++) {
        const auto& other = inst.src[i ^ 1];
        if (other.field == OperandField::ScalarGPR && other.code == reg) {
            if (const auto value = GetConstant(inst.src[i])) {
                return value;
            }
        }
    }
    return std::nullopt;
}

struct JumpTableInfo {
    u32 index_reg;
    u32 load_index;
    boost::container::small_vector<std::pair<u32, u32>, 16> targets; ///< (index value, target pc)
};

// Resolves an S_SETPC_B64 through a jump table, as emitted for switch statements:
//   s_min_u32      s20, s20, N           ; clamp case index
//   s_lshl_b32     s20, s20, 3           ; 8 byte entries
//   s_add_u32      s20, s20, bias        ; optional
//   s_getpc_b64    s[12:13]
//   s_add_u32      s12, s12, table
//   s_addc_u32     s13, s13, 0
//   s_load_dwordx2 s[12:13], s[12:13], s20 ; offset relative to the second s_getpc_b64
//   s_getpc_b64    s[36:37]
//   s_add_u32      s36, s36, s12
//   s_addc_u32     s37, s37, s13
//   s_setpc_b64    s[36:37]
// shortcut: only the s_min_u32 clamped form is recognized, extend if a game bounds the index
// with a compare and branch instead.
static std::optional<JumpTableInfo> ResolveJumpTable(std::span<const GcnInst> list, u32 setpc_index,
                                                     std::span<const u32> pc_map, const u32* code) {
    if (setpc_index < 3) {
        return std::nullopt;
    }
    const auto& setpc = list[setpc_index];
    const auto& getpc = list[setpc_index - 3];
    const auto& add = list[setpc_index - 2];
    const u32 target_reg = setpc.src[0].code;
    if (getpc.opcode != Opcode::S_GETPC_B64 || getpc.dst[0].code != target_reg ||
        add.opcode != Opcode::S_ADD_U32 || add.dst[0].code != target_reg ||
        list[setpc_index - 1].opcode != Opcode::S_ADDC_U32) {
        return std::nullopt;
    }
    const auto& offset_src = add.src[0].code == target_reg ? add.src[1] : add.src[0];
    if (offset_src.field != OperandField::ScalarGPR) {
        return std::nullopt;
    }
    const u32 offset_reg = offset_src.code;

    // Table entry load.
    const auto load_index = FindSgprWriter(list, setpc_index - 3, offset_reg);
    if (!load_index) {
        return std::nullopt;
    }
    const auto& load = list[*load_index];
    if (load.opcode != Opcode::S_LOAD_DWORDX2 || load.dst[0].code != offset_reg ||
        load.control.smrd.imm || load.src[1].field != OperandField::ScalarGPR) {
        return std::nullopt;
    }
    const u32 base_reg = load.src[0].code * 2;
    const u32 index_reg = load.src[1].code;

    // Table base address: s_getpc_b64 + constant.
    const auto base_add_index = FindSgprWriter(list, *load_index, base_reg);
    if (!base_add_index || list[*base_add_index].opcode != Opcode::S_ADD_U32) {
        return std::nullopt;
    }
    const auto table_offset = GetConstantWithSgpr(list[*base_add_index], base_reg);
    const auto base_getpc_index = FindSgprWriter(list, *base_add_index, base_reg);
    if (!table_offset || !base_getpc_index ||
        list[*base_getpc_index].opcode != Opcode::S_GETPC_B64) {
        return std::nullopt;
    }
    const u32 table_pc = pc_map[*base_getpc_index] + list[*base_getpc_index].length + *table_offset;

    // Index computation: (min(x, N) << 3) + bias. It must still be live at the S_SETPC_B64.
    auto index_writer = FindSgprWriter(list, *load_index, index_reg);
    if (!index_writer || FindSgprWriter(list, setpc_index, index_reg) != index_writer) {
        return std::nullopt;
    }
    u32 bias = 0;
    const auto& bias_add = list[*index_writer];
    if (bias_add.opcode == Opcode::S_ADD_U32 || bias_add.opcode == Opcode::S_ADD_I32) {
        const auto bias_value = GetConstantWithSgpr(bias_add, index_reg);
        if (!bias_value) {
            return std::nullopt;
        }
        bias = *bias_value;
        index_writer = FindSgprWriter(list, *index_writer, index_reg);
    }
    if (!index_writer || list[*index_writer].opcode != Opcode::S_LSHL_B32 ||
        GetConstantWithSgpr(list[*index_writer], index_reg) != 3U) {
        return std::nullopt;
    }
    const auto min_index = FindSgprWriter(list, *index_writer, index_reg);
    if (!min_index || list[*min_index].opcode != Opcode::S_MIN_U32) {
        return std::nullopt;
    }
    const auto& min = list[*min_index];
    const auto max_case = GetConstant(min.src[1]).or_else([&] { return GetConstant(min.src[0]); });
    // Bound reads of guest memory in case the pattern matched something unrelated.
    constexpr u32 MaxJumpTableCases = 256;
    if (!max_case || *max_case >= MaxJumpTableCases || ((table_pc + bias) & 3) != 0) {
        return std::nullopt;
    }

    // Entries are 64-bit offsets relative to the value of the final s_getpc_b64.
    const u32 base_pc = pc_map[setpc_index - 3] + getpc.length;
    JumpTableInfo info{index_reg, *load_index, {}};
    for (u32 i = 0; i <= *max_case; i++) {
        const u32 value = bias + (i << 3);
        const u32 entry_dw = (table_pc + value) / sizeof(u32);
        const s64 offset = static_cast<s64>(code[entry_dw] | u64(code[entry_dw + 1]) << 32);
        const s64 target = s64(base_pc) + offset;
        if (target < 0 || target > std::numeric_limits<u32>::max() || (target & 3) != 0) {
            return std::nullopt;
        }
        info.targets.emplace_back(value, static_cast<u32>(target));
    }
    return info;
}

static constexpr size_t LabelReserveSize = 32;

CFG::CFG(Common::ObjectPool<Block>& block_pool_, std::span<const GcnInst> inst_list_,
         const u32* code_)
    : block_pool{block_pool_}, inst_list{inst_list_}, code{code_} {
    index_to_pc.resize(inst_list.size() + 1);
    labels.reserve(LabelReserveSize);
    EmitLabels();
    EmitBlocks();
    LinkBlocks();
    SplitDivergenceScopes();
    RemoveUnreachableBlocks();
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
            u32 target = inst.BranchTarget(pc);
            if (inst.opcode == Opcode::S_SETPC_B64) {
                if (auto t = ResolveSetPcTarget(inst_list, i, index_to_pc)) {
                    target = *t;
                } else if (const auto table = ResolveJumpTable(inst_list, i, index_to_pc, code)) {
                    for (const auto& [_, table_target] : table->targets) {
                        AddLabel(table_target);
                    }
                    AddLabel(pc + inst.length);
                    pc += inst.length;
                    continue;
                } else {
                    ASSERT_MSG(
                        false,
                        "S_SETPC_B64 without a resolvable offset at PC {:#x} (Index {}): Involved "
                        "instructions not recognized or invalid pattern",
                        pc, i);
                }
            }
            AddLabel(target);
            // Emit this label so that the block ends with the branching instruction
            AddLabel(pc + inst.length);
        } else if (inst.IsConditionalBranch()) {
            const u32 true_label = inst.BranchTarget(pc);
            const u32 false_label = pc + inst.length;
            if (true_label != false_label) {
                AddLabel(true_label);
                AddLabel(false_label);
            }
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

void CFG::SplitDivergenceScopes() {
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

    for (auto blk = blocks.begin(); blk != blocks.end(); blk++) {
        auto next_blk = std::next(blk);
        s32 curr_begin = -1;
        for (size_t index = blk->begin_index; index <= blk->end_index; index++) {
            const auto& inst = inst_list[index];
            const bool is_close = is_close_scope(inst);
            if ((is_close || index == blk->end_index) && curr_begin != -1) {
                // If there are no instructions inside scope don't do anything.
                if (index - curr_begin == 1 && is_close) {
                    curr_begin = is_open_scope(inst) ? static_cast<s32>(index) : -1;
                    continue;
                }
                // If all instructions in the scope ignore exec masking, we shouldn't insert a
                // scope.
                const auto start = inst_list.begin() + curr_begin + 1;
                if (!std::ranges::all_of(start, inst_list.begin() + index + !is_close,
                                         IgnoresExecMask)) {
                    // Determine the first instruction affected by the exec mask.
                    do {
                        ++curr_begin;
                    } while (IgnoresExecMask(inst_list[curr_begin]));

                    // Determine the last instruction affected by the exec mask.
                    s32 curr_end = index;
                    while (IgnoresExecMask(inst_list[curr_end])) {
                        --curr_end;
                    }

                    // Create a new block for the divergence scope.
                    Block* block = block_pool.Create();
                    block->begin = index_to_pc[curr_begin];
                    block->end = index_to_pc[curr_end];
                    block->begin_index = curr_begin;
                    block->end_index = curr_end;
                    block->end_inst = inst_list[curr_end];
                    block->num_predecessors = 1;
                    blocks.insert_before(next_blk, *block);

                    // If we are inside the parent block, make an epilogue block and jump to it.
                    if (curr_end != blk->end_index) {
                        Block* epi_block = block_pool.Create();
                        epi_block->begin = index_to_pc[curr_end + 1];
                        epi_block->end = blk->end;
                        epi_block->begin_index = curr_end + 1;
                        epi_block->end_index = blk->end_index;
                        epi_block->end_inst = blk->end_inst;
                        epi_block->cond = blk->cond;
                        epi_block->end_class = blk->end_class;
                        epi_block->branch_true = blk->branch_true;
                        epi_block->branch_false = blk->branch_false;
                        epi_block->jump_table = std::exchange(blk->jump_table, {});
                        epi_block->num_predecessors = 2;
                        blocks.insert_before(next_blk, *epi_block);

                        // Have divergence block always jump to epilogue block.
                        block->cond = IR::Condition::True;
                        block->branch_true = epi_block;
                        block->branch_false = nullptr;

                        // If the parent block fails to enter divergence block make it jump to
                        // epilogue too
                        blk->branch_false = epi_block;
                    } else {
                        // No epilogue block is needed since the divergence block
                        // also ends the parent block. Inherit the end condition.
                        auto& parent_blk = *blk;
                        ASSERT(blk->cond == IR::Condition::True && blk->branch_true);
                        block->cond = IR::Condition::True;
                        block->branch_true = blk->branch_true;
                        block->branch_false = nullptr;

                        // If the parent block didn't enter the divergence scope
                        // have it jump directly to the next one
                        blk->branch_false = blk->branch_true;
                        blk->branch_true->num_predecessors++;
                    }

                    // Shrink parent block to end right before curr_begin
                    // and make it jump to divergence block
                    --curr_begin;
                    blk->end = index_to_pc[curr_begin];
                    blk->end_index = curr_begin;
                    blk->end_inst = inst_list[curr_begin];
                    blk->cond = IR::Condition::Execnz;
                    blk->end_class = EndClass::Branch;
                    blk->branch_true = block;
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

        // If the block doesn't end with a branch we simply
        // need to link with the next block.
        if (!end_inst.IsTerminateInstruction()) {
            auto* next_block = get_block(block.end);
            block.cond = IR::Condition::True;
            block.branch_true = next_block;
            block.end_class = EndClass::Branch;
            next_block->num_predecessors++;
            continue;
        }

        // Find the branch targets from the instruction and link the blocks.
        // Note: Block end address is one instruction after end_inst.
        const u32 branch_pc = block.end - end_inst.length;
        u32 target_pc = 0;
        if (end_inst.opcode == Opcode::S_SETPC_B64) {
            if (auto tgt = ResolveSetPcTarget(inst_list, block.end_index, index_to_pc)) {
                target_pc = *tgt;
            } else {
                // EmitLabels already asserted that the target resolves to one or the other.
                const auto table =
                    ResolveJumpTable(inst_list, block.end_index, index_to_pc, code).value();
                ASSERT_MSG(table.load_index >= block.begin_index,
                           "Jump table load at index {} is outside of S_SETPC_B64 block",
                           table.load_index);
                block.end_class = EndClass::IndirectBranch;
                block.jump_table.index_reg = table.index_reg;
                block.jump_table.load_index = table.load_index;
                for (const auto& [value, pc] : table.targets) {
                    Block* const target_block = get_block(pc);
                    if (std::ranges::none_of(block.jump_table.targets, [&](const auto& target) {
                            return target.second == target_block;
                        })) {
                        target_block->num_predecessors++;
                    }
                    block.jump_table.targets.emplace_back(value, target_block);
                }
                continue;
            }
        } else {
            target_pc = end_inst.BranchTarget(branch_pc);
        }

        if (end_inst.IsUnconditionalBranch()) {
            auto* target_block = get_block(target_pc);
            block.branch_true = target_block;
            block.end_class = EndClass::Branch;
            target_block->num_predecessors++;
        } else if (end_inst.IsConditionalBranch()) {
            auto* end_block = get_block(block.end);
            block.end_class = EndClass::Branch;
            if (target_pc != block.end) {
                auto* target_block = get_block(target_pc);
                block.branch_true = target_block;
                block.branch_false = end_block;
                target_block->num_predecessors++;
            } else {
                block.branch_true = end_block;
                block.cond = IR::Condition::True;
            }
            end_block->num_predecessors++;
        } else if (end_inst.opcode == Opcode::S_ENDPGM) {
            block.end_class = EndClass::Exit;
        } else {
            UNREACHABLE();
        }
    }
}

void CFG::RemoveUnreachableBlocks() {
    for (auto it = std::next(blocks.begin()); it != blocks.end();) {
        if (it->num_predecessors == 0) {
            LOG_WARNING(Render_Recompiler, "Removing unreachable block begin={:#x}", it->begin);
            it = blocks.erase(it);
        } else {
            it++;
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
        case EndClass::IndirectBranch:
            for (const auto& [value, target] : block.jump_table.targets) {
                dot += fmt::format("\t\t{}->{} [label=\"s{} == {:#x}\"]\n", name, name_of(*target),
                                   block.jump_table.index_reg, value);
            }
            break;
        case EndClass::Exit:
            dot += fmt::format("\t\t{}->N{};\n", name, node_uid);
            dot +=
                fmt::format("\t\tN{} [label=\"Exit\"][shape=square][style=stripped];\n", node_uid);
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
