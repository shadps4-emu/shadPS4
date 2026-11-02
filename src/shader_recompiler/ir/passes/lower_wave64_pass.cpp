// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/classes.h"
#include "shader_recompiler/info.h"
#include "shader_recompiler/ir/basic_block.h"
#include "shader_recompiler/ir/breadth_first_search.h"
#include "shader_recompiler/ir/ir_emitter.h"
#include "shader_recompiler/ir/passes/ir_passes.h"
#include "shader_recompiler/ir/post_order.h"
#include "shader_recompiler/ir/program.h"
#include "shader_recompiler/profile.h"
#include "shader_recompiler/recompiler.h"

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

static void RebuildControlFlow(IR::Program& program) {
    using Type = IR::AbstractSyntaxNode::Type;
    program.blocks.clear();
    for (const auto& node : program.syntax_list) {
        if (node.type == Type::Block) {
            program.blocks.push_back(node.data.block);
        }
    }
    for (auto* block : program.blocks) {
        block->imm_predecessors.clear();
        block->imm_successors.clear();
        block->ssa_state.Reset();
    }
    IR::Block* current{};
    for (const auto& node : program.syntax_list) {
        switch (node.type) {
        case Type::Block:
            if (current) {
                current->AddBranch(node.data.block);
            }
            current = node.data.block;
            continue;
        case Type::If:
            current->AddBranch(node.data.if_node.body);
            current->AddBranch(node.data.if_node.merge);
            break;
        case Type::EndIf:
            if (current) {
                current->AddBranch(node.data.end_if.merge);
            }
            break;
        case Type::Loop:
            current->AddBranch(node.data.loop.body);
            break;
        case Type::Repeat:
            current->AddBranch(node.data.repeat.loop_header);
            current->AddBranch(node.data.repeat.merge);
            break;
        case Type::Break:
            current->AddBranch(node.data.break_node.merge);
            current->AddBranch(node.data.break_node.skip);
            break;
        default:
            break;
        }
        current = nullptr;
    }
    program.post_order_blocks = IR::PostOrder(program.blocks.front());
}

static void ReconvergeWave64(IR::Program& program, Pools& pools) {
    using Type = IR::AbstractSyntaxNode::Type;
    if (std::ranges::count_if(program.syntax_list, [](const auto& node) {
            return node.type == Type::Return || node.type == Type::Unreachable;
        }) != 1) {
        return;
    }
    struct Scope {
        size_t header_index;
        IR::Block* header;
        IR::Block* merge;
        IR::VirtualReg condition;
    };
    std::vector<IR::Inst*> phis;
    for (auto* block : program.blocks) {
        for (auto& inst : block->Instructions()) {
            if (IR::IsPhi(inst)) {
                phis.push_back(&inst);
            }
        }
    }
    for (auto* inst : phis) {
        const IR::VirtualReg reg{program.next_reg_index++, inst->Type()};
        for (size_t i = 0; i < inst->NumArgs(); ++i) {
            IR::IREmitter ir{*inst->PhiBlock(i)};
            ir.SetVirtualReg(reg, inst->Arg(i));
        }
        IR::IREmitter ir{*inst->GetParent(), IR::Block::InstructionList::s_iterator_to(*inst)};
        inst->ReplaceUsesWithAndRemove(ir.GetVirtualReg(reg));
    }
    IR::AbstractSyntaxList syntax;
    std::vector<Scope> scopes;
    IR::Block* current{};
    u32 loop_depth{};
    const auto append_block = [&](IR::Block* block) {
        syntax.push_back({.data = {.block = block}, .type = Type::Block});
    };
    for (const auto& node : program.syntax_list) {
        if (node.type == Type::If) {
            const IR::VirtualReg condition{program.next_reg_index++, IR::Type::U1};
            IR::IREmitter{*current}.SetVirtualReg(condition, node.data.if_node.cond);
            scopes.push_back({syntax.size(), current, node.data.if_node.merge, condition});
        } else if (node.type == Type::EndIf) {
            scopes.pop_back();
        } else if (node.type == Type::Loop) {
            ++loop_depth;
        } else if (node.type == Type::Repeat) {
            --loop_depth;
        }
        syntax.push_back(node);
        if (node.type != Type::Block) {
            continue;
        }
        current = node.data.block;
        for (auto it = current->begin(); it != current->end();) {
            IR::Inst& inst = *it++;
            const auto opcode = inst.GetOpcode();
            const bool shared =
                (opcode >= IR::Opcode::SharedAtomicIAdd32 &&
                 opcode <= IR::Opcode::SharedAtomicCmpSwap64) ||
                opcode == IR::Opcode::LoadSharedU16 || opcode == IR::Opcode::LoadSharedU32 ||
                opcode == IR::Opcode::LoadSharedU64 || opcode == IR::Opcode::WriteSharedU16 ||
                opcode == IR::Opcode::WriteSharedU32 || opcode == IR::Opcode::WriteSharedU64;
            if (loop_depth != 0 ||
                (!shared && opcode != IR::Opcode::Ballot && opcode != IR::Opcode::ReadFirstLane)) {
                continue;
            }
            if (scopes.empty()) {
                if (opcode == IR::Opcode::ReadFirstLane) {
                    IR::IREmitter ir{*current, IR::Block::InstructionList::s_iterator_to(inst)};
                    inst.ReplaceUsesWithAndRemove(ir.ReadLane(IR::U32{inst.Arg(0)}, ir.Imm32(0)));
                }
                continue;
            }
            const IR::VirtualReg active{program.next_reg_index++, IR::Type::U1};
            const IR::VirtualReg input{program.next_reg_index++,
                                       opcode == IR::Opcode::Ballot ? IR::Type::U1 : IR::Type::U32};
            if (!shared) {
                IR::IREmitter init{*scopes.front().header};
                init.SetVirtualReg(active, init.Imm1(false));
                init.SetVirtualReg(input, opcode == IR::Opcode::Ballot ? IR::Value{false}
                                                                       : IR::Value{u32{0}});
            }
            auto* continuation = pools.block_pool.Create(pools.inst_pool);
            auto first = IR::Block::InstructionList::s_iterator_to(inst);
            continuation->Instructions().splice(continuation->end(), current->Instructions(), first,
                                                current->end());
            for (auto& moved : continuation->Instructions()) {
                moved.SetParent(continuation);
            }
            if (!shared) {
                IR::IREmitter save{*current};
                save.SetVirtualReg(active, save.Imm1(true));
                save.SetVirtualReg(input, inst.Arg(0));
            }
            for (auto scope = scopes.rbegin(); scope != scopes.rend(); ++scope) {
                auto* merge = pools.block_pool.Create(pools.inst_pool);
                syntax[scope->header_index].data.if_node.merge = merge;
                IR::AbstractSyntaxNode end_if{.type = Type::EndIf};
                end_if.data.end_if.merge = merge;
                syntax.push_back(end_if);
                append_block(merge);
                current = merge;
            }
            IR::IREmitter collective{*current};
            if (shared) {
                collective.Barrier();
            } else {
                const IR::U1 enabled{collective.GetVirtualReg(active)};
                const IR::U64 mask = collective.Ballot(
                    opcode == IR::Opcode::Ballot
                        ? collective.LogicalAnd(enabled, IR::U1{collective.GetVirtualReg(input)})
                        : enabled);
                if (opcode == IR::Opcode::Ballot) {
                    inst.ReplaceUsesWithAndRemove(mask);
                } else {
                    const IR::U32 lane{
                        collective.Select(collective.IEqual(mask, collective.Imm64(u64{0})),
                                          collective.Imm32(0), collective.FindILsb(mask))};
                    inst.ReplaceUsesWithAndRemove(
                        collective.ReadLane(IR::U32{collective.GetVirtualReg(input)}, lane));
                }
            }
            for (size_t i = 0; i < scopes.size(); ++i) {
                auto& scope = scopes[i];
                IR::IREmitter reopen{*current};
                auto* body = i + 1 == scopes.size() ? continuation
                                                    : pools.block_pool.Create(pools.inst_pool);
                IR::AbstractSyntaxNode if_node{.type = Type::If};
                if_node.data.if_node = {
                    reopen.ConditionRef(IR::U1{reopen.GetVirtualReg(scope.condition)}), body,
                    scope.merge};
                scope.header = current;
                scope.header_index = syntax.size();
                syntax.push_back(if_node);
                append_block(body);
                current = body;
            }
            it = current->begin();
            if (shared) {
                ++it;
            }
        }
    }
    program.syntax_list = std::move(syntax);
    RebuildControlFlow(program);
    SsaRepairPass(program);
    SsaRewritePass(program);
    DeadCodeEliminationPass(program);
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
                           const Profile& profile, Pools& pools) {
    if (program.info.hw_stage != HwStage::Compute || profile.subgroup_size == 64) {
        return;
    }

    const auto [size_x, size_y, size_z] = runtime_info.hw.cs.workgroup_size;
    const u32 num_threads = size_x * size_y * size_z;
    if (num_threads <= 32) {
        return;
    }

    if (num_threads == 64) {
        ReconvergeWave64(program, pools);
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
            if (inst.GetOpcode() == IR::Opcode::ReadLane) {
                push_worklist(inst);
            } else if (inst.GetOpcode() == IR::Opcode::Ballot) {
                push_worklist(inst);
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
        (Common::AlignUp(num_threads, 64) / profile.subgroup_size) * sizeof(u32);
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
