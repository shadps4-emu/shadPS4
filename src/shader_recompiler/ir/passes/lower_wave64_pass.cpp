// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

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

/// Moves subgroup ballots to the position of their subgroup inside the wave64 mask and makes the
/// instructions that consume lane masks index them by wave64 lane.
static void LowerLaneMasksToWave64Layout(IR::Program& program, const Profile& profile,
                                         const std::vector<IR::Inst*>& lowered) {
    std::vector<IR::Inst*> ballots;
    std::vector<IR::Inst*> mask_users;
    for (IR::Block* block : program.blocks) {
        for (IR::Inst& inst : block->Instructions()) {
            switch (inst.GetOpcode()) {
            case IR::Opcode::Ballot:
                if (!std::ranges::contains(lowered, &inst)) {
                    ballots.push_back(&inst);
                }
                break;
            case IR::Opcode::InverseBallot:
            case IR::Opcode::BallotFindLsb:
                mask_users.push_back(&inst);
                break;
            case IR::Opcode::ReadLane:
                if (!std::ranges::contains(lowered, &inst)) {
                    mask_users.push_back(&inst);
                }
                break;
            default:
                break;
            }
        }
    }

    const u32 subgroup_size = profile.subgroup_size;
    const u32 subgroups_per_wave = 64 / subgroup_size;
    for (IR::Inst* inst : ballots) {
        IR::IREmitter ir{*inst->GetParent(), IR::Block::InstructionList::s_iterator_to(*inst)};
        const IR::U32 subgroup_in_wave =
            ir.BitwiseAnd(ir.SubgroupId(), ir.Imm32(subgroups_per_wave - 1));
        const IR::U32 shift = ir.IMul(subgroup_in_wave, ir.Imm32(subgroup_size));
        const IR::U64 mask = ir.Ballot(IR::U1{inst->Arg(0)});
        inst->ReplaceUsesWithAndRemove(ir.ShiftLeftLogical(mask, shift));
    }
    for (IR::Inst* inst : mask_users) {
        IR::IREmitter ir{*inst->GetParent(), IR::Block::InstructionList::s_iterator_to(*inst)};
        switch (inst->GetOpcode()) {
        case IR::Opcode::InverseBallot: {
            const IR::U32 lane = ir.BitwiseAnd(
                ir.GetAttributeU32(IR::Attribute::LocalInvocationIndex), ir.Imm32(63));
            const IR::U64 shifted = ir.ShiftRightLogical(IR::U64{inst->Arg(0)}, lane);
            const IR::U32 low{ir.CompositeExtract(ir.UnpackUint2x32(shifted), 0)};
            inst->ReplaceUsesWithAndRemove(
                ir.INotEqual(ir.BitwiseAnd(low, ir.Imm32(1)), ir.Imm32(0)));
            break;
        }
        case IR::Opcode::BallotFindLsb:
            inst->ReplaceUsesWithAndRemove(ir.FindILsb(IR::U64{inst->Arg(0)}));
            break;
        case IR::Opcode::ReadLane: {
            const IR::U32 lane = ir.BitwiseAnd(IR::U32{inst->Arg(1)}, ir.Imm32(subgroup_size - 1));
            inst->ReplaceUsesWithAndRemove(ir.ReadLane(IR::U32{inst->Arg(0)}, lane));
            break;
        }
        default:
            UNREACHABLE();
        }
    }
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
                push_worklist(inst);
            } else if (inst.GetOpcode() == IR::Opcode::Ballot) {
                const auto is_unpack = [](const IR::Use& use) {
                    return use.user->GetOpcode() == IR::Opcode::UnpackUint2x32;
                };
                if (std::ranges::any_of(inst.Uses(), is_unpack)) {
                    push_worklist(inst);
                }
            } else if (inst.GetOpcode() == IR::Opcode::MaskedBitCount32) {
                worklist.push_back(&inst);
            }
        }
    }
    LowerLaneMasksToWave64Layout(program, profile, worklist);
    if (worklist.empty()) {
        return;
    }

    const u32 scratch_size =
        (Common::AlignUp(num_threads, 64) / profile.subgroup_size) * sizeof(u32);
    Info::SharedMemorySegment& shared_scratch_segment = program.info.shared_scratch_segment;

    for (IR::Inst* inst : worklist) {
        LOG_INFO(Render_Recompiler, "Lowering {} instruction for wave64", inst->GetOpcode());
        IR::IREmitter ir{*inst->GetParent(), IR::Block::InstructionList::s_iterator_to(*inst)};
        const IR::U32 invocation_index = ir.GetAttributeU32(IR::Attribute::LocalInvocationIndex);
        const IR::U32 subgroup_id = ir.SubgroupId();
        if (inst->GetOpcode() == IR::Opcode::Ballot) {
            const IR::U32 mask_low =
                IR::U32{ir.CompositeExtract(ir.UnpackUint2x32(ir.Ballot(IR::U1{inst->Arg(0)})), 0)};
            const IR::U32 offset = ir.ShiftLeftLogical(subgroup_id, ir.Imm32(2u));
            ir.WriteSharedU32Scratch(mask_low, offset);
            ir.Barrier();
            const IR::U64 mask{ir.LoadSharedU64Scratch(offset)};
            ir.Barrier();
            inst->ReplaceUsesWithAndRemove(mask);
            shared_scratch_segment.SetMinSize(scratch_size);
        } else if (inst->GetOpcode() == IR::Opcode::ReadLane) {
            const IR::U32 lane32 = ir.BitwiseAnd(IR::U32{inst->Arg(1)}, ir.Imm32(31));
            const IR::U32 half = ir.ShiftRightLogical(IR::U32{inst->Arg(1)}, ir.Imm32(5u));
            const IR::U32 offset = ir.ShiftLeftLogical(subgroup_id, ir.Imm32(2u));
            ir.WriteSharedU32Scratch(ir.ReadLane(IR::U32{inst->Arg(0)}, lane32), offset);
            ir.Barrier();
            const IR::U32 value = ir.LoadSharedU32Scratch(ir.IAdd(
                ir.BitwiseAnd(offset, ir.Imm32(~7u)), ir.ShiftLeftLogical(half, ir.Imm32(2u))));
            ir.Barrier();
            inst->ReplaceUsesWithAndRemove(value);
            shared_scratch_segment.SetMinSize(scratch_size);
        } else if (inst->GetOpcode() == IR::Opcode::MaskedBitCount32) {
            const IR::U32 subgroup_invocation_id = ir.BitwiseAnd(invocation_index, ir.Imm32(63));
            const IR::U64 mask = ir.ISub(
                ir.ShiftLeftLogical(ir.Imm64(u64{1}), subgroup_invocation_id), ir.Imm64(u64{1}));
            const IR::U32 thread_mask{
                ir.CompositeExtract(ir.UnpackUint2x32(mask), inst->Arg(2).U1() ? 1u : 0u)};
            const IR::U32 masked_value{
                ir.BitCount(ir.BitwiseAnd(IR::U32{inst->Arg(0)}, thread_mask))};
            inst->ReplaceUsesWithAndRemove(ir.IAdd(masked_value, IR::U32{inst->Arg(1)}));
        } else if (inst->GetOpcode() == IR::Opcode::WaveId) {
            u32 subgroup_size = profile.subgroup_size;
            ASSERT(subgroup_size < 64);
            ASSERT(std::popcount(subgroup_size) == 1);
            const u32 scale = 64 / subgroup_size;
            const u32 shift = std::countr_zero(scale);
            const IR::U32 scaled_wave_id = ir.ShiftRightLogical(subgroup_id, ir.Imm32(shift));
            inst->ReplaceUsesWithAndRemove(scaled_wave_id);
        }
    }
}

} // namespace Shader::Optimization
