// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <unordered_map>
#include <vector>
#include <boost/container/small_vector.hpp>

#include "shader_recompiler/ir/ir_emitter.h"
#include "shader_recompiler/ir/program.h"

namespace Shader::Optimization {

static bool IsIdenticalInst(const IR::Inst* a, const IR::Inst* b) {
    if (a->GetOpcode() != b->GetOpcode()) {
        return false;
    }
    for (size_t i = 0; i < a->NumArgs(); i++) {
        if (a->Arg(i) != b->Arg(i)) {
            return false;
        }
    }
    return true;
}

static bool IsAllowedInst(const IR::Inst& inst) {
    return inst.GetOpcode() == IR::Opcode::Ballot ||
           inst.GetOpcode() == IR::Opcode::UnpackUint2x32;
}

using InstList = boost::container::small_vector<IR::Inst*, 8>;

static bool DeduplicateInst(IR::Inst& inst, InstList& inst_list) {
    if (!IsAllowedInst(inst)) {
        return false;
    }
    const auto it = std::ranges::find_if(
        inst_list, [&](const IR::Inst* b) { return IsIdenticalInst(&inst, b); });
    if (it != inst_list.end()) {
        inst.ReplaceUsesWithAndRemove(IR::Value{*it});
        return true;
    }

    inst_list.emplace_back(&inst);
    return false;
}

static void FoldCompositeConstruct(IR::Inst& inst, IR::Opcode extract) {
    IR::Value result{};
    for (size_t i = 0; i < inst.NumArgs(); ++i) {
        const IR::Value value{inst.Arg(i)};
        if (value.IsImmediate()) {
            return;
        }
        IR::Inst* const inst = value.Inst();
        if (inst->GetOpcode() != extract || inst->Arg(1).U32() != i) {
            return;
        }

        if (result.IsEmpty()) {
            result = inst->Arg(0);
        } else if (result != inst->Arg(0)) {
            return;
        }
    }
    inst.ReplaceUsesWithAndRemove(result);
}

static void FoldInverseFunc(IR::Inst& inst, IR::Opcode reverse) {
    const IR::Value value{inst.Arg(0)};
    if (value.IsImmediate()) {
        return;
    }
    IR::Inst* const arg_inst{value.Inst()};
    if (arg_inst->GetOpcode() == reverse) {
        inst.ReplaceUsesWithAndRemove(arg_inst->Arg(0));
        return;
    }
}

static void TrySimplifyInst(IR::Inst& inst) {
    switch (inst.GetOpcode()) {
    case IR::Opcode::PackUint2x32:
        return FoldInverseFunc(inst, IR::Opcode::UnpackUint2x32);
    case IR::Opcode::CompositeConstructU32x2:
        return FoldCompositeConstruct(inst, IR::Opcode::CompositeExtractU32x2);
    case IR::Opcode::CompositeConstructU32x3:
        return FoldCompositeConstruct(inst, IR::Opcode::CompositeExtractU32x3);
    case IR::Opcode::CompositeConstructU32x4:
        return FoldCompositeConstruct(inst, IR::Opcode::CompositeExtractU32x4);
    default:
        break;
    }
}

void InverseBallotEliminationPass(IR::Program& program) {
    InstList inst_list;
    std::vector<IR::Inst*> worklist;
    for (IR::Block* const block : program.blocks) {
        inst_list.clear();
        for (IR::Inst& inst : block->Instructions()) {
            if (DeduplicateInst(inst, inst_list)) {
                continue;
            }
            TrySimplifyInst(inst);
            if (inst.GetOpcode() != IR::Opcode::InverseBallot) {
                continue;
            }
            worklist.push_back(&inst);
        }
    }
    std::unordered_map<IR::Inst*, IR::Value> inst_map;
    while (!worklist.empty()) {
        IR::Inst* const inst = worklist.back();
        worklist.pop_back();

        if (inst->GetOpcode() == IR::Opcode::Void) {
            continue;
        }

        IR::Value value{inst->Arg(0)};
        if (value.IsImmediate()) {
            if (value.U64() == 0ull) {
                inst->ReplaceUsesWithAndRemove(IR::Value{false});
            } else if (value.U64() == std::numeric_limits<u64>::max()) {
                inst->ReplaceUsesWithAndRemove(IR::Value{true});
            } else {
                UNREACHABLE_MSG("Unexpected immediate argument for InverseBallot {:#x}",
                                value.U64());
            }
            continue;
        }

        IR::Inst* const prod = value.Inst();
        if (prod->GetOpcode() == IR::Opcode::Ballot) {
            inst->ReplaceUsesWithAndRemove(prod->Arg(0));
            continue;
        }

        IR::Block* const block = prod->GetParent();
        if (prod->GetOpcode() == IR::Opcode::UndefU64) {
            auto insert_point = IR::Block::InstructionList::s_iterator_to(*prod);
            IR::Value const undef{&*block->PrependNewInst(insert_point, IR::Opcode::UndefU1)};
            inst->ReplaceUsesWithAndRemove(undef);
            continue;
        }

        if (prod->GetOpcode() != IR::Opcode::BitwiseAnd64 &&
            prod->GetOpcode() != IR::Opcode::BitwiseNot64 &&
            prod->GetOpcode() != IR::Opcode::BitwiseOr64 &&
            prod->GetOpcode() != IR::Opcode::BitwiseXor64 &&
            prod->GetOpcode() != IR::Opcode::SelectU64 && prod->GetOpcode() != IR::Opcode::Phi) {
            continue;
        }

        auto [it, is_new] = inst_map.try_emplace(prod);
        if (is_new) {
            auto insert_point = IR::Block::InstructionList::s_iterator_to(*prod);
            IR::IREmitter ir{*block, insert_point};

            if (prod->GetOpcode() == IR::Opcode::Phi) {
                IR::Inst* const new_phi{&*block->PrependNewInst(insert_point, IR::Opcode::Phi)};
                new_phi->SetFlags(IR::Type::U1);
                for (size_t arg_index = 0; arg_index < prod->NumArgs(); ++arg_index) {
                    IR::Block* const phi_block = prod->PhiBlock(arg_index);
                    IR::IREmitter ir{*phi_block};
                    const IR::U1 new_arg = ir.InverseBallot(IR::U64{prod->Arg(arg_index)});
                    worklist.push_back(new_arg.Inst());
                    new_phi->AddPhiOperand(phi_block, new_arg);
                }
                it->second = IR::Value{new_phi};
            } else if (prod->GetOpcode() == IR::Opcode::SelectU64) {
                const IR::U1 a = ir.InverseBallot(IR::U64{prod->Arg(1)});
                worklist.push_back(a.Inst());
                const IR::U1 b = ir.InverseBallot(IR::U64{prod->Arg(2)});
                worklist.push_back(b.Inst());
                it->second = ir.Select(IR::U1{prod->Arg(0)}, a, b);
            } else {
                const IR::U1 a = ir.InverseBallot(IR::U64{prod->Arg(0)});
                worklist.push_back(a.Inst());

                if (prod->GetOpcode() == IR::Opcode::BitwiseNot64) {
                    it->second = ir.LogicalNot(a);
                } else {
                    const IR::U1 b = ir.InverseBallot(IR::U64{prod->Arg(1)});
                    worklist.push_back(b.Inst());
                    switch (prod->GetOpcode()) {
                    case IR::Opcode::BitwiseAnd64:
                        it->second = ir.LogicalAnd(a, b);
                        break;
                    case IR::Opcode::BitwiseOr64:
                        it->second = ir.LogicalOr(a, b);
                        break;
                    case IR::Opcode::BitwiseXor64:
                        it->second = ir.LogicalXor(a, b);
                        break;
                    default:
                        break;
                    }
                }
            }
        }

        auto uses = prod->Uses();
        for (auto [user, operand] : uses) {
            if (user->GetOpcode() == IR::Opcode::InverseBallot) {
                user->ReplaceUsesWithAndRemove(it->second);
            }
        }
    }
}

} // namespace Shader::Optimization
