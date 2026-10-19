// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <unordered_set>
#include "common/logging/classes.h"
#include "shader_recompiler/info.h"
#include "shader_recompiler/ir/basic_block.h"
#include "shader_recompiler/ir/breadth_first_search.h"
#include "shader_recompiler/ir/ir_emitter.h"
#include "shader_recompiler/ir/program.h"
#include "shader_recompiler/profile.h"

namespace Shader::Optimization {

static bool IsDivergentCondition(const IR::U1& condition) {
    if (condition.IsImmediate()) {
        return false;
    }
    const IR::Inst* const condition_inst = condition.Inst();
    return IR::BreadthFirstSearch(condition_inst,
                                  [](const IR::Inst* inst) -> std::optional<bool> {
                                      switch (inst->GetOpcode()) {
                                      case IR::Opcode::LaneId:
                                          return true;
                                      case IR::Opcode::GetAttributeU32:
                                          if (inst->Arg(0).Attribute() ==
                                              IR::Attribute::LocalInvocationId) {
                                              return true;
                                          }
                                          break;
                                      default:
                                          break;
                                      }
                                      return std::nullopt;
                                  })
        .value_or(false);
}

static std::vector<IR::Block*> FindUniformBlocks(const IR::Program& program) {
    using Type = IR::AbstractSyntaxNode::Type;

    struct ConditionalScope {
        const IR::Block* merge;
        bool divergent;
    };

    std::vector<IR::Block*> blocks;
    std::vector<ConditionalScope> conditionals;
    std::vector<const IR::Block*> loops;
    u32 divergence_depth{};
    for (const IR::AbstractSyntaxNode& node : program.syntax_list) {
        switch (node.type) {
        case Type::If: {
            const bool divergent = IsDivergentCondition(node.data.if_node.cond);
            conditionals.push_back({node.data.if_node.merge, divergent});
            divergence_depth += static_cast<u32>(divergent);
            break;
        }
        case Type::EndIf:
            ASSERT(!conditionals.empty() && conditionals.back().merge == node.data.end_if.merge);
            divergence_depth -= static_cast<u32>(conditionals.back().divergent);
            conditionals.pop_back();
            break;
        case Type::Loop:
            loops.push_back(node.data.loop.merge);
            break;
        case Type::Repeat:
            if (loops.empty() || loops.back() != node.data.repeat.merge) {
                return {};
            }
            loops.pop_back();
            break;
        case Type::Block:
            if (divergence_depth == 0 && loops.empty()) {
                blocks.push_back(node.data.block);
            }
            break;
        default:
            break;
        }
    }
    if (!conditionals.empty() || !loops.empty()) {
        return {};
    }
    return blocks;
}

static bool IsWaveUniform(IR::Value value, const std::unordered_set<IR::Inst*>& reads) {
    if (value.IsImmediate()) {
        return true;
    }
    const IR::Inst* inst = value.Inst();
    if (reads.contains(value.Inst())) {
        return true;
    }
    switch (inst->GetOpcode()) {
    case IR::Opcode::GetAttributeU32:
        return inst->Arg(0).Attribute() == IR::Attribute::WorkgroupId;
    case IR::Opcode::ReadConstBuffer:
    case IR::Opcode::ConditionRef:
    case IR::Opcode::LogicalNot:
    case IR::Opcode::LogicalAnd:
    case IR::Opcode::LogicalOr:
    case IR::Opcode::IEqual32:
    case IR::Opcode::INotEqual32:
    case IR::Opcode::SLessThan32:
    case IR::Opcode::ULessThan32:
    case IR::Opcode::SLessThanEqual32:
    case IR::Opcode::ULessThanEqual32:
    case IR::Opcode::SGreaterThan32:
    case IR::Opcode::UGreaterThan32:
    case IR::Opcode::IAdd32:
    case IR::Opcode::ISub32:
    case IR::Opcode::IMul32:
    case IR::Opcode::ShiftRightLogical32:
    case IR::Opcode::ShiftLeftLogical32:
    case IR::Opcode::BitwiseAnd32:
    case IR::Opcode::BitwiseOr32:
        for (size_t i = 0; i < inst->NumArgs(); ++i) {
            if (!IsWaveUniform(inst->Arg(i), reads)) {
                return false;
            }
        }
        return true;
    default:
        return false;
    }
}

static std::unordered_set<IR::Inst*> FindUniformWaveReads(const IR::Program& program) {
    using Type = IR::AbstractSyntaxNode::Type;
    std::unordered_set<IR::Inst*> reads;
    for (auto* block : program.blocks) {
        for (IR::Inst& inst : block->Instructions()) {
            if (inst.GetOpcode() == IR::Opcode::ReadLane && inst.Arg(1).IsImmediate() &&
                inst.Arg(1).U32() < 64) {
                reads.insert(&inst);
            }
        }
    }
    bool changed;
    do {
        changed = false;
        std::unordered_set<const IR::Block*> divergent_loops;
        std::vector<const IR::Block*> loops;
        std::vector<bool> conditionals;
        u32 divergence_depth = 0;
        for (const auto& node : program.syntax_list) {
            switch (node.type) {
            case Type::If: {
                const bool divergent = !IsWaveUniform(node.data.if_node.cond, reads);
                conditionals.push_back(divergent);
                divergence_depth += divergent;
                break;
            }
            case Type::EndIf:
                divergence_depth -= conditionals.back();
                conditionals.pop_back();
                break;
            case Type::Loop:
                loops.push_back(node.data.loop.merge);
                break;
            case Type::Repeat:
                if (!IsWaveUniform(node.data.repeat.cond, reads)) {
                    divergent_loops.insert(loops.back());
                }
                loops.pop_back();
                break;
            case Type::Break:
                if (divergence_depth || !IsWaveUniform(node.data.break_node.cond, reads)) {
                    divergent_loops.insert(loops.back());
                }
                break;
            case Type::Return:
                if (divergence_depth || !loops.empty()) {
                    return {};
                }
                break;
            default:
                break;
            }
        }
        std::unordered_set<IR::Block*> uniform_blocks;
        loops.clear();
        conditionals.clear();
        divergence_depth = 0;
        for (const auto& node : program.syntax_list) {
            switch (node.type) {
            case Type::If: {
                const bool divergent = !IsWaveUniform(node.data.if_node.cond, reads);
                conditionals.push_back(divergent);
                divergence_depth += divergent;
                break;
            }
            case Type::EndIf:
                divergence_depth -= conditionals.back();
                conditionals.pop_back();
                break;
            case Type::Loop:
                loops.push_back(node.data.loop.merge);
                break;
            case Type::Repeat:
                loops.pop_back();
                break;
            case Type::Block:
                if (divergence_depth == 0 && std::ranges::none_of(loops, [&](auto* loop) {
                        return divergent_loops.contains(loop);
                    })) {
                    uniform_blocks.insert(node.data.block);
                }
                break;
            default:
                break;
            }
        }
        for (auto it = reads.begin(); it != reads.end();) {
            if (!uniform_blocks.contains((*it)->GetParent())) {
                it = reads.erase(it);
                changed = true;
            } else {
                ++it;
            }
        }
    } while (changed);
    return reads;
}

static IR::Inst* FindBallotForMaskedBitCount(const IR::Inst& mbcnt) {
    IR::Value value = mbcnt.Arg(0);
    if (value.IsImmediate()) {
        return nullptr;
    }
    IR::Inst* inst = value.Inst();
    if (inst->GetOpcode() != IR::Opcode::CompositeExtractU32x2 || inst->Arg(0).IsImmediate()) {
        return nullptr;
    }
    inst = inst->Arg(0).Inst();
    if (inst->GetOpcode() != IR::Opcode::UnpackUint2x32 || inst->Arg(0).IsImmediate()) {
        return nullptr;
    }
    inst = inst->Arg(0).Inst();
    return inst->GetOpcode() == IR::Opcode::Ballot ? inst : nullptr;
}

void LowerWave64BallotPass(IR::Program& program, const RuntimeInfo& runtime_info,
                           const Profile& profile) {
    if (program.info.hw_stage != HwStage::Compute || profile.subgroup_size == 64) {
        return;
    }

    const auto [size_x, size_y, size_z] = runtime_info.hw.cs.workgroup_size;
    const u32 num_threads = size_x * size_y * size_z;
    if (num_threads <= 32) {
        return;
    }

    std::vector<IR::Inst*> worklist;
    const auto uniform_blocks = FindUniformBlocks(program);
    const auto uniform_reads = num_threads == 64 && profile.subgroup_size == 32
                                   ? FindUniformWaveReads(program)
                                   : std::unordered_set<IR::Inst*>{};
    for (IR::Block* block : program.blocks) {
        const bool is_uniform = std::ranges::contains(uniform_blocks, block);
        const auto push_worklist = [&](IR::Inst& inst) {
            if (is_uniform) {
                worklist.push_back(&inst);
            } else {
                LOG_WARNING(Render_Recompiler, "{} instruction in non uniform control flow",
                            inst.GetOpcode());
            }
        };
        for (IR::Inst& inst : block->Instructions()) {
            if (inst.GetOpcode() == IR::Opcode::ReadLane && inst.Arg(1).IsImmediate()) {
                if (uniform_reads.contains(&inst)) {
                    worklist.push_back(&inst);
                } else {
                    push_worklist(inst);
                }
            } else if (inst.GetOpcode() == IR::Opcode::Ballot) {
                const auto is_unpack = [](const IR::Use& use) {
                    return use.user->GetOpcode() == IR::Opcode::UnpackUint2x32;
                };
                if (std::ranges::any_of(inst.Uses(), is_unpack)) {
                    push_worklist(inst);
                }
            } else if (inst.GetOpcode() == IR::Opcode::MaskedBitCount32) {
                IR::Inst* const ballot = FindBallotForMaskedBitCount(inst);
                if (ballot == nullptr ||
                    std::ranges::contains(uniform_blocks, ballot->GetParent())) {
                    worklist.push_back(&inst);
                }
            }
        }
    }
    if (worklist.empty()) {
        return;
    }

    const u32 scratch_base = Common::AlignUp(runtime_info.hw.cs.shared_memory_size, sizeof(u64));
    const u32 scratch_size =
        uniform_reads.empty()
            ? (Common::AlignUp(num_threads, 64) / profile.subgroup_size) * sizeof(u32)
            : num_threads * sizeof(u32);
    program.info.shared_memory_scratch_size =
        scratch_base + scratch_size - runtime_info.hw.cs.shared_memory_size;

    for (IR::Inst* inst : worklist) {
        LOG_INFO(Render_Recompiler, "Lowering {} instruction for wave64", inst->GetOpcode());
        IR::IREmitter ir{*inst->GetParent(), IR::Block::InstructionList::s_iterator_to(*inst)};
        const IR::U32 invocation_index = ir.GetAttributeU32(IR::Attribute::LocalInvocationIndex);
        const IR::U32 subgroup_id = ir.ShiftRightLogical(invocation_index, ir.Imm32(5));
        if (inst->GetOpcode() == IR::Opcode::Ballot) {
            const IR::U32 mask_low =
                IR::U32{ir.CompositeExtract(ir.UnpackUint2x32(ir.Ballot(IR::U1{inst->Arg(0)})), 0)};
            const IR::U32 offset =
                ir.IAdd(ir.Imm32(scratch_base), ir.ShiftLeftLogical(subgroup_id, ir.Imm32(2u)));
            ir.WriteShared(32, mask_low, offset);
            ir.Barrier();
            const IR::U64 mask = IR::U64{ir.LoadShared(64, false, offset)};
            ir.Barrier();
            inst->ReplaceUsesWithAndRemove(mask);
        } else if (inst->GetOpcode() == IR::Opcode::ReadLane && uniform_reads.contains(inst)) {
            const IR::U32 offset = ir.IAdd(ir.Imm32(scratch_base),
                                           ir.ShiftLeftLogical(invocation_index, ir.Imm32(2u)));
            ir.WriteShared(32, IR::U32{inst->Arg(0)}, offset);
            ir.Barrier();
            const IR::U32 value =
                IR::U32{ir.LoadShared(32, false, ir.Imm32(scratch_base + inst->Arg(1).U32() * 4u))};
            ir.Barrier();
            inst->ReplaceUsesWithAndRemove(value);
        } else if (inst->GetOpcode() == IR::Opcode::ReadLane) {
            const IR::U32 lane32 = ir.BitwiseAnd(IR::U32{inst->Arg(1)}, ir.Imm32(31));
            const IR::U32 half = ir.ShiftRightLogical(IR::U32{inst->Arg(1)}, ir.Imm32(5u));
            const IR::U32 offset =
                ir.IAdd(ir.Imm32(scratch_base), ir.ShiftLeftLogical(subgroup_id, ir.Imm32(2u)));
            ir.WriteShared(32, ir.ReadLane(IR::U32{inst->Arg(0)}, lane32), offset);
            ir.Barrier();
            const IR::U32 value =
                IR::U32{ir.LoadShared(32, false,
                                      ir.IAdd(ir.BitwiseAnd(offset, ir.Imm32(~7u)),
                                              ir.ShiftLeftLogical(half, ir.Imm32(2u))))};
            ir.Barrier();
            inst->ReplaceUsesWithAndRemove(value);
        } else if (inst->GetOpcode() == IR::Opcode::MaskedBitCount32) {
            const IR::U32 subgroup_invocation_id = ir.BitwiseAnd(invocation_index, ir.Imm32(63));
            const IR::U64 mask = ir.ISub(
                ir.ShiftLeftLogical(ir.Imm64(u64{1}), subgroup_invocation_id), ir.Imm64(u64{1}));
            const IR::U32 thread_mask{
                ir.CompositeExtract(ir.UnpackUint2x32(mask), inst->Arg(2).U1() ? 1u : 0u)};
            const IR::U32 masked_value{
                ir.BitCount(ir.BitwiseAnd(IR::U32{inst->Arg(0)}, thread_mask))};
            inst->ReplaceUsesWithAndRemove(ir.IAdd(masked_value, IR::U32{inst->Arg(1)}));
        }
    }
}

} // namespace Shader::Optimization
