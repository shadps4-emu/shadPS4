// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <optional>
#include <unordered_map>
#include <unordered_set>

#include "shader_recompiler/ir/basic_block.h"
#include "shader_recompiler/ir/dominance.h"
#include "shader_recompiler/ir/ir_emitter.h"
#include "shader_recompiler/ir/opcodes.h"
#include "shader_recompiler/ir/passes/ir_passes.h"
#include "shader_recompiler/ir/program.h"
#include "shader_recompiler/runtime_info.h"

namespace Shader::Optimization {

namespace {

std::optional<u32> AnalyzeLoopInduction(const IR::Inst* phi) {
    const IR::Block* header = phi->GetParent();
    if (!header || phi->NumArgs() != 2) {
        return std::nullopt;
    }

    const IR::Block* latch = nullptr;
    size_t latch_idx = 0;
    size_t back_edges = 0;

    for (size_t i = 0; i < 2; ++i) {
        const IR::Block* pred = phi->PhiBlock(i);
        if (IR::Dominates(const_cast<IR::Block*>(header), const_cast<IR::Block*>(pred))) {
            latch = pred;
            latch_idx = i;
            back_edges++;
        }
    }

    if (back_edges != 1 || !latch) {
        return std::nullopt;
    }

    // Step must be an increment by a positive constant along the loop back-edge
    const IR::Value& step_val = phi->Arg(latch_idx);
    const IR::Inst* step_inst = step_val.TryInst();
    if (!step_inst || step_inst->GetOpcode() != IR::Opcode::IAdd32) {
        return std::nullopt;
    }

    u32 step_k = 0;
    if (step_inst->Arg(0).TryInst() == phi && step_inst->Arg(1).IsImmediate()) {
        step_k = step_inst->Arg(1).U32();
    } else if (step_inst->Arg(1).TryInst() == phi && step_inst->Arg(0).IsImmediate()) {
        step_k = step_inst->Arg(0).U32();
    } else {
        return std::nullopt;
    }

    if (step_k == 0) {
        return std::nullopt;
    }

    auto is_target = [&](const IR::Value& v) {
        return v.TryInst() == phi || v.TryInst() == step_inst;
    };

    auto check_inst_bound = [&](const IR::Inst* cond_inst) -> std::optional<u32> {
        while (cond_inst) {
            if (cond_inst->GetOpcode() == IR::Opcode::ConditionRef ||
                cond_inst->GetOpcode() == IR::Opcode::LogicalNot) {
                cond_inst = cond_inst->Arg(0).TryInst();
            } else {
                break;
            }
        }
        if (!cond_inst) {
            return std::nullopt;
        }

        const auto op = cond_inst->GetOpcode();
        if (op == IR::Opcode::ULessThan32 || op == IR::Opcode::SLessThan32) {
            if (is_target(cond_inst->Arg(0)) && cond_inst->Arg(1).IsImmediate()) {
                const u32 lim = cond_inst->Arg(1).U32();
                if (lim > 0) {
                    return lim - 1;
                }
            }
        } else if (op == IR::Opcode::ULessThanEqual32 || op == IR::Opcode::SLessThanEqual32) {
            if (is_target(cond_inst->Arg(0)) && cond_inst->Arg(1).IsImmediate()) {
                return cond_inst->Arg(1).U32();
            }
        } else if (op == IR::Opcode::UGreaterThan32 || op == IR::Opcode::SGreaterThan32) {
            if (is_target(cond_inst->Arg(1)) && cond_inst->Arg(0).IsImmediate()) {
                const u32 lim = cond_inst->Arg(0).U32();
                if (lim > 0) {
                    return lim - 1;
                }
            }
        } else if (op == IR::Opcode::UGreaterThanEqual32 || op == IR::Opcode::SGreaterThanEqual32) {
            if (is_target(cond_inst->Arg(1)) && cond_inst->Arg(0).IsImmediate()) {
                return cond_inst->Arg(0).U32();
            }
        }
        return std::nullopt;
    };

    auto check_block_bound = [&](const IR::Block* b) -> std::optional<u32> {
        if (!b) {
            return std::nullopt;
        }
        return check_inst_bound(b->branch_cond.TryInst());
    };

    if (auto bound = check_block_bound(header)) {
        return bound;
    }
    if (auto bound = check_block_bound(latch)) {
        return bound;
    }
    if (auto bound = check_block_bound(step_inst->GetParent())) {
        return bound;
    }
    for (const IR::Block* pred : latch->imm_predecessors) {
        if (auto bound = check_block_bound(pred)) {
            return bound;
        }
    }

    auto check_uses = [&](const IR::Inst* target) -> std::optional<u32> {
        for (const auto& use : target->Uses()) {
            if (const IR::Inst* user = use.user) {
                if (auto bound = check_inst_bound(user)) {
                    return bound;
                }
            }
        }
        return std::nullopt;
    };

    if (auto bound = check_uses(step_inst)) {
        return bound;
    }
    if (auto bound = check_uses(phi)) {
        return bound;
    }

    return std::nullopt;
}

std::vector<const IR::Inst*> FindReachingVectorDefs(const IR::Block* start_block,
                                                    const IR::Inst* before_inst, IR::VectorReg reg,
                                                    bool& all_paths_defined) {
    std::vector<const IR::Inst*> defs;
    all_paths_defined = true;

    if (before_inst) {
        const IR::Inst* last_def = nullptr;
        for (const IR::Inst& cur : start_block->Instructions()) {
            if (&cur == before_inst) {
                break;
            }
            if (cur.GetOpcode() == IR::Opcode::SetVectorRegister && cur.Arg(0).VectorReg() == reg) {
                last_def = &cur;
            }
        }
        if (last_def) {
            defs.push_back(last_def);
            return defs;
        }
    }

    std::unordered_set<const IR::Block*> visited;
    visited.insert(start_block);
    std::vector<const IR::Block*> worklist(start_block->imm_predecessors.begin(),
                                           start_block->imm_predecessors.end());

    if (worklist.empty()) {
        all_paths_defined = false;
        return defs;
    }

    while (!worklist.empty()) {
        const IR::Block* b = worklist.back();
        worklist.pop_back();

        if (!visited.insert(b).second) {
            continue;
        }

        const IR::Inst* def = nullptr;
        for (const IR::Inst& cur : b->Instructions()) {
            if (cur.GetOpcode() == IR::Opcode::SetVectorRegister && cur.Arg(0).VectorReg() == reg) {
                def = &cur;
            }
        }

        if (def) {
            defs.push_back(def);
        } else {
            if (b->imm_predecessors.empty()) {
                all_paths_defined = false;
                break;
            }
            for (const IR::Block* pred : b->imm_predecessors) {
                if (!visited.contains(pred)) {
                    worklist.push_back(pred);
                }
            }
        }
    }
    return defs;
}

u32 FindUpperBound(const IR::Value& val, u32 unbounded,
                   std::unordered_set<const IR::Inst*>& active_stack,
                   std::unordered_map<const IR::Inst*, u32>& memo, u32 depth = 0) {
    if (depth > 24 || val.IsEmpty()) {
        return unbounded;
    }
    if (val.IsImmediate()) {
        return val.U32();
    }
    const IR::Inst* inst = val.TryInst();
    if (!inst) {
        return unbounded;
    }
    if (active_stack.contains(inst)) {
        return unbounded;
    }
    if (const auto it = memo.find(inst); it != memo.end()) {
        return it->second;
    }

    active_stack.insert(inst);

    auto compute = [&]() -> u32 {
        switch (inst->GetOpcode()) {
        case IR::Opcode::BitCastU32F32:
        case IR::Opcode::ReadFirstLane:
            return FindUpperBound(inst->Arg(0), unbounded, active_stack, memo, depth + 1);
        case IR::Opcode::GetVectorRegister: {
            const auto reg = inst->Arg(0).VectorReg();
            const IR::Block* block = inst->GetParent();
            if (!block) {
                return unbounded;
            }
            bool all_paths_defined = true;
            const auto defs = FindReachingVectorDefs(block, inst, reg, all_paths_defined);
            if (!all_paths_defined || defs.empty()) {
                return unbounded;
            }
            u32 max_val = 0;
            for (const IR::Inst* def : defs) {
                const u32 b = FindUpperBound(def->Arg(1), unbounded, active_stack, memo, depth + 1);
                if (b >= unbounded) {
                    return unbounded;
                }
                max_val = std::max(max_val, b);
            }
            return max_val;
        }
        case IR::Opcode::BitwiseAnd32: {
            const u32 b0 = FindUpperBound(inst->Arg(0), unbounded, active_stack, memo, depth + 1);
            const u32 b1 = FindUpperBound(inst->Arg(1), unbounded, active_stack, memo, depth + 1);
            return std::min(b0, b1);
        }
        case IR::Opcode::UMin32: {
            const u32 b0 = FindUpperBound(inst->Arg(0), unbounded, active_stack, memo, depth + 1);
            const u32 b1 = FindUpperBound(inst->Arg(1), unbounded, active_stack, memo, depth + 1);
            return std::min(b0, b1);
        }
        case IR::Opcode::UMax32: {
            const u32 b0 = FindUpperBound(inst->Arg(0), unbounded, active_stack, memo, depth + 1);
            const u32 b1 = FindUpperBound(inst->Arg(1), unbounded, active_stack, memo, depth + 1);
            return (b0 < unbounded && b1 < unbounded) ? std::max(b0, b1) : unbounded;
        }
        case IR::Opcode::SelectU32: {
            const u32 b0 = FindUpperBound(inst->Arg(1), unbounded, active_stack, memo, depth + 1);
            const u32 b1 = FindUpperBound(inst->Arg(2), unbounded, active_stack, memo, depth + 1);
            return (b0 < unbounded && b1 < unbounded) ? std::max(b0, b1) : unbounded;
        }
        case IR::Opcode::ShiftLeftLogical32: {
            if (inst->Arg(1).IsImmediate()) {
                const u32 shift = inst->Arg(1).U32();
                if (shift < 32) {
                    const u64 b = static_cast<u64>(FindUpperBound(inst->Arg(0), unbounded,
                                                                  active_stack, memo, depth + 1))
                                  << shift;
                    return b < unbounded ? static_cast<u32>(b) : unbounded;
                }
            }
            return unbounded;
        }
        case IR::Opcode::ShiftRightLogical32: {
            if (inst->Arg(1).IsImmediate()) {
                const u32 shift = inst->Arg(1).U32();
                if (shift < 32) {
                    const u32 b =
                        FindUpperBound(inst->Arg(0), unbounded, active_stack, memo, depth + 1);
                    return b < unbounded ? (b >> shift) : unbounded;
                }
            }
            return unbounded;
        }
        case IR::Opcode::IMul32: {
            const u32 b0 = FindUpperBound(inst->Arg(0), unbounded, active_stack, memo, depth + 1);
            const u32 b1 = FindUpperBound(inst->Arg(1), unbounded, active_stack, memo, depth + 1);
            if (b0 < unbounded && b1 < unbounded) {
                const u64 p = static_cast<u64>(b0) * b1;
                return p < unbounded ? static_cast<u32>(p) : unbounded;
            }
            return unbounded;
        }
        case IR::Opcode::IAdd32: {
            const u32 b0 = FindUpperBound(inst->Arg(0), unbounded, active_stack, memo, depth + 1);
            const u32 b1 = FindUpperBound(inst->Arg(1), unbounded, active_stack, memo, depth + 1);
            if (b0 < unbounded && b1 < unbounded) {
                const u64 s = static_cast<u64>(b0) + b1;
                return s < unbounded ? static_cast<u32>(s) : unbounded;
            }
            return unbounded;
        }
        case IR::Opcode::Phi: {
            if (const auto bound = AnalyzeLoopInduction(inst)) {
                return *bound;
            }

            u32 max_arg = 0;
            for (size_t i = 0; i < inst->NumArgs(); ++i) {
                const auto& arg = inst->Arg(i);
                if (const auto* arg_inst = arg.TryInst()) {
                    if (active_stack.contains(arg_inst)) {
                        return unbounded;
                    }
                }
                const u32 arg_b = FindUpperBound(arg, unbounded, active_stack, memo, depth + 1);
                if (arg_b >= unbounded) {
                    return unbounded;
                }
                max_arg = std::max(max_arg, arg_b);
            }
            return max_arg;
        }
        default:
            return unbounded;
        }
    };

    const u32 result = compute();
    active_stack.erase(inst);
    memo[inst] = result;
    return result;
}

u32 FindUpperBound(const IR::Value& val, u32 unbounded) {
    std::unordered_set<const IR::Inst*> active_stack;
    std::unordered_map<const IR::Inst*, u32> memo;
    return FindUpperBound(val, unbounded, active_stack, memo, 0);
}

} // Anonymous namespace

void LowerMovRelPass(IR::Program& program, const RuntimeInfo& runtime_info) {
    IR::ComputeDominators(program.post_order_blocks);
    const u32 num_allocated_vgprs = runtime_info.props.num_allocated_vgprs;

    for (auto* block : program.blocks) {
        for (auto it = block->begin(); it != block->end();) {
            IR::Inst& inst = *it;
            const auto opcode = inst.GetOpcode();
            if (opcode != IR::Opcode::VMovRelS && opcode != IR::Opcode::VMovRelD) {
                ++it;
                continue;
            }

            if (opcode == IR::Opcode::VMovRelS) {
                const auto src_reg = inst.Arg(0).VectorReg();
                const IR::U32 m0{inst.Arg(1)};
                const u32 src_vgprno = static_cast<u32>(src_reg);
                const u32 unbounded = num_allocated_vgprs - src_vgprno - 1;

                const u32 max_m0 = FindUpperBound(m0, unbounded);
                const u32 max_vgprno = std::min(src_vgprno + max_m0 + 1, num_allocated_vgprs);

                IR::IREmitter ir{*block, it};
                IR::U32 result = ir.GetVectorReg(IR::VectorReg::V0);
                for (u32 i = src_vgprno; i < max_vgprno; ++i) {
                    const IR::U1 cond = ir.IEqual(m0, ir.Imm32(i - src_vgprno));
                    result =
                        IR::U32{ir.Select(cond, ir.GetVectorReg(IR::VectorReg::V0 + i), result)};
                }

                auto next_it = std::next(it);
                inst.ReplaceUsesWithAndRemove(result);
                it = next_it;
            } else if (opcode == IR::Opcode::VMovRelD) {
                const auto dst_reg = inst.Arg(0).VectorReg();
                const IR::U32 src_val{inst.Arg(1)};
                const IR::U32 m0{inst.Arg(2)};
                const u32 dst_vgprno = static_cast<u32>(dst_reg);
                const u32 unbounded = num_allocated_vgprs - dst_vgprno - 1;

                const u32 max_m0 = FindUpperBound(m0, unbounded);
                const u32 max_vgprno = std::min(dst_vgprno + max_m0 + 1, num_allocated_vgprs);

                IR::IREmitter ir{*block, it};
                for (u32 i = dst_vgprno; i < max_vgprno; ++i) {
                    const IR::U1 cond = ir.IEqual(m0, ir.Imm32(i - dst_vgprno));
                    const IR::U32 old_val = ir.GetVectorReg(IR::VectorReg::V0 + i);
                    const IR::U32 dst_val{ir.Select(cond, src_val, old_val)};
                    ir.SetVectorReg(IR::VectorReg::V0 + i, dst_val);
                }

                auto next_it = std::next(it);
                inst.Invalidate();
                it = next_it;
            }
        }
    }
}

} // namespace Shader::Optimization
