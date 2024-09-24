// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

// This file implements the SSA rewriting algorithm proposed in
//
//      Simple and Efficient Construction of Static Single Assignment Form.
//      Braun M., Buchwald S., Hack S., Leiba R., Mallon C., Zwinkau A. (2013)
//      In: Jhala R., De Bosschere K. (eds)
//      Compiler Construction. CC 2013.
//      Lecture Notes in Computer Science, vol 7791.
//      Springer, Berlin, Heidelberg
//
//      https://link.springer.com/chapter/10.1007/978-3-642-37051-9_6
//

#include <map>
#include <span>
#include <unordered_map>
#include <variant>

#include "shader_recompiler/ir/basic_block.h"
#include "shader_recompiler/ir/opcodes.h"
#include "shader_recompiler/ir/reg.h"
#include "shader_recompiler/ir/value.h"

namespace Shader::Optimization {
namespace {
struct FlagTag {
    auto operator<=>(const FlagTag&) const noexcept = default;
};
struct SccFlagTag : FlagTag {};
struct ExecFlagTag : FlagTag {};
struct VccFlagTag : FlagTag {};
struct VccLoTag : FlagTag {};
struct VccHiTag : FlagTag {};
struct M0Tag : FlagTag {};

struct GotoVariable : FlagTag {
    GotoVariable() = default;
    explicit GotoVariable(u32 index_) : index{index_} {}

    auto operator<=>(const GotoVariable&) const noexcept = default;

    u32 index;
};

struct ThreadBitScalar : FlagTag {
    ThreadBitScalar() = default;
    explicit ThreadBitScalar(IR::ScalarReg sgpr_) : sgpr{sgpr_} {}

    auto operator<=>(const ThreadBitScalar&) const noexcept = default;

    IR::ScalarReg sgpr;
};

using Variant = std::variant<IR::ScalarReg, IR::VectorReg, GotoVariable, ThreadBitScalar,
                             SccFlagTag, ExecFlagTag, VccFlagTag, VccLoTag, VccHiTag, M0Tag>;
using ValueMap = std::unordered_map<IR::Block*, IR::Value>;

struct DefTable {
    const IR::Value& Def(IR::Block* block, IR::ScalarReg variable) {
        return block->ssa_sreg_values[RegIndex(variable)];
    }
    void SetDef(IR::Block* block, IR::ScalarReg variable, const IR::Value& value) {
        block->ssa_sreg_values[RegIndex(variable)] = value;
    }

    const IR::Value& Def(IR::Block* block, IR::VectorReg variable) {
        return block->ssa_vreg_values[RegIndex(variable)];
    }
    void SetDef(IR::Block* block, IR::VectorReg variable, const IR::Value& value) {
        block->ssa_vreg_values[RegIndex(variable)] = value;
    }

    const IR::Value& Def(IR::Block* block, GotoVariable variable) {
        return goto_vars[variable.index][block];
    }
    void SetDef(IR::Block* block, GotoVariable variable, const IR::Value& value) {
        goto_vars[variable.index].insert_or_assign(block, value);
    }

    const IR::Value& Def(IR::Block* block, ThreadBitScalar variable) {
        return block->ssa_sbit_values[RegIndex(variable.sgpr)];
    }
    void SetDef(IR::Block* block, ThreadBitScalar variable, const IR::Value& value) {
        block->ssa_sbit_values[RegIndex(variable.sgpr)] = value;
    }

    const IR::Value& Def(IR::Block* block, SccFlagTag) {
        return scc_flag[block];
    }
    void SetDef(IR::Block* block, SccFlagTag, const IR::Value& value) {
        scc_flag.insert_or_assign(block, value);
    }

    const IR::Value& Def(IR::Block* block, ExecFlagTag) {
        return exec_flag[block];
    }
    void SetDef(IR::Block* block, ExecFlagTag, const IR::Value& value) {
        exec_flag.insert_or_assign(block, value);
    }

    const IR::Value& Def(IR::Block* block, VccLoTag) {
        return vcc_lo_flag[block];
    }
    void SetDef(IR::Block* block, VccLoTag, const IR::Value& value) {
        vcc_lo_flag.insert_or_assign(block, value);
    }

    const IR::Value& Def(IR::Block* block, VccHiTag) {
        return vcc_hi_flag[block];
    }
    void SetDef(IR::Block* block, VccHiTag, const IR::Value& value) {
        vcc_hi_flag.insert_or_assign(block, value);
    }

    const IR::Value& Def(IR::Block* block, VccFlagTag) {
        return vcc_flag[block];
    }
    void SetDef(IR::Block* block, VccFlagTag, const IR::Value& value) {
        vcc_flag.insert_or_assign(block, value);
    }
    const IR::Value& Def(IR::Block* block, M0Tag) {
        return m0_flag[block];
    }
    void SetDef(IR::Block* block, M0Tag, const IR::Value& value) {
        m0_flag.insert_or_assign(block, value);
    }

    std::unordered_map<u32, ValueMap> goto_vars;
    ValueMap scc_flag;
    ValueMap exec_flag;
    ValueMap vcc_flag;
    ValueMap scc_lo_flag;
    ValueMap vcc_lo_flag;
    ValueMap vcc_hi_flag;
    ValueMap m0_flag;
};

IR::Opcode UndefOpcode(IR::ScalarReg) noexcept {
    return IR::Opcode::UndefU32;
}

IR::Opcode UndefOpcode(IR::VectorReg) noexcept {
    return IR::Opcode::UndefU32;
}

IR::Opcode UndefOpcode(const VccLoTag) noexcept {
    return IR::Opcode::UndefU32;
}

IR::Opcode UndefOpcode(const VccHiTag) noexcept {
    return IR::Opcode::UndefU32;
}

IR::Opcode UndefOpcode(const M0Tag) noexcept {
    return IR::Opcode::UndefU32;
}

IR::Opcode UndefOpcode(const FlagTag) noexcept {
    return IR::Opcode::UndefU1;
}

enum class Status {
    Start,
    SetValue,
    PreparePhiArgument,
    PushPhiArgument,
};

template <typename Type>
struct ReadState {
    ReadState(IR::Block* block_) : block{block_} {}
    ReadState() = default;

    IR::Block* block{};
    IR::Value result{};
    IR::Inst* phi{};
    IR::Block* const* pred_it{};
    IR::Block* const* pred_end{};
    Status pc{Status::Start};
};

class Pass {
public:
    template <typename Type>
    void WriteVariable(Type variable, IR::Block* block, const IR::Value& value) {
        current_def.SetDef(block, variable, value);
    }

    template <typename Type>
    IR::Value ReadVariable(Type variable, IR::Block* root_block) {
        boost::container::small_vector<ReadState<Type>, 64> stack{
            ReadState<Type>(nullptr),
            ReadState<Type>(root_block),
        };
        const auto prepare_phi_operand = [&] {
            if (stack.back().pred_it == stack.back().pred_end) {
                IR::Inst* const phi{stack.back().phi};
                IR::Block* const block{stack.back().block};
                const IR::Value result{TryRemoveTrivialPhi(*phi, block, UndefOpcode(variable))};
                stack.pop_back();
                stack.back().result = result;
                WriteVariable(variable, block, result);
            } else {
                IR::Block* const imm_pred{*stack.back().pred_it};
                stack.back().pc = Status::PushPhiArgument;
                stack.emplace_back(imm_pred);
            }
        };
        do {
            IR::Block* const block{stack.back().block};
            switch (stack.back().pc) {
            case Status::Start: {
                if (const IR::Value& def = current_def.Def(block, variable); !def.IsEmpty()) {
                    stack.back().result = def;
                } else if (!block->IsSsaSealed()) {
                    // Incomplete CFG
                    IR::Inst* phi{&*block->PrependNewInst(block->begin(), IR::Opcode::Phi)};
                    phi->SetFlags(IR::TypeOf(UndefOpcode(variable)));

                    incomplete_phis[block].insert_or_assign(variable, phi);
                    stack.back().result = IR::Value{&*phi};
                } else if (const std::span imm_preds = block->ImmPredecessors();
                           imm_preds.size() == 1) {
                    // Optimize the common case of one predecessor: no phi needed
                    stack.back().pc = Status::SetValue;
                    stack.emplace_back(imm_preds.front());
                    break;
                } else {
                    // Break potential cycles with operandless phi
                    IR::Inst* const phi{&*block->PrependNewInst(block->begin(), IR::Opcode::Phi)};
                    phi->SetFlags(IR::TypeOf(UndefOpcode(variable)));

                    WriteVariable(variable, block, IR::Value{phi});

                    stack.back().phi = phi;
                    stack.back().pred_it = imm_preds.data();
                    stack.back().pred_end = imm_preds.data() + imm_preds.size();
                    prepare_phi_operand();
                    break;
                }
            }
                [[fallthrough]];
            case Status::SetValue: {
                const IR::Value result{stack.back().result};
                WriteVariable(variable, block, result);
                stack.pop_back();
                stack.back().result = result;
                break;
            }
            case Status::PushPhiArgument: {
                IR::Inst* const phi{stack.back().phi};
                phi->AddPhiOperand(*stack.back().pred_it, stack.back().result);
                ++stack.back().pred_it;
            }
                [[fallthrough]];
            case Status::PreparePhiArgument:
                prepare_phi_operand();
                break;
            }
        } while (stack.size() > 1);
        return stack.back().result;
    }

    void SealBlock(IR::Block* block) {
        const auto it{incomplete_phis.find(block)};
        if (it != incomplete_phis.end()) {
            for (auto& pair : it->second) {
                auto& variant{pair.first};
                auto& phi{pair.second};
                std::visit([&](auto& variable) { AddPhiOperands(variable, *phi, block); }, variant);
            }
        }
        block->SsaSeal();
    }

private:
    template <typename Type>
    IR::Value AddPhiOperands(Type variable, IR::Inst& phi, IR::Block* block) {
        for (IR::Block* const imm_pred : block->ImmPredecessors()) {
            phi.AddPhiOperand(imm_pred, ReadVariable(variable, imm_pred));
        }
        return TryRemoveTrivialPhi(phi, block, UndefOpcode(variable));
    }

    IR::Value TryRemoveTrivialPhi(IR::Inst& phi, IR::Block* block, IR::Opcode undef_opcode) {
        IR::Value same;
        const size_t num_args{phi.NumArgs()};
        for (size_t arg_index = 0; arg_index < num_args; ++arg_index) {
            const IR::Value& op{phi.Arg(arg_index)};
            if (op.Resolve() == same.Resolve() || op == IR::Value{&phi}) {
                // Unique value or self-reference
                continue;
            }
            if (!same.IsEmpty()) {
                // The phi merges at least two values: not trivial
                return IR::Value{&phi};
            }
            same = op;
        }
        // Remove the phi node from the block, it will be reinserted
        IR::Block::InstructionList& list{block->Instructions()};
        list.erase(IR::Block::InstructionList::s_iterator_to(phi));

        // Find the first non-phi instruction and use it as an insertion point
        IR::Block::iterator reinsert_point{std::ranges::find_if_not(list, IR::IsPhi)};
        if (same.IsEmpty()) {
            // The phi is unreachable or in the start block
            // Insert an undefined instruction and make it the phi node replacement
            // The "phi" node reinsertion point is specified after this instruction
            reinsert_point = block->PrependNewInst(reinsert_point, undef_opcode);
            same = IR::Value{&*reinsert_point};
            ++reinsert_point;
        }
        // Reinsert the phi node and reroute all its uses to the "same" value
        list.insert(reinsert_point, phi);
        phi.ReplaceUsesWith(same);
        // TODO: Try to recursively remove all phi users, which might have become trivial
        return same;
    }

    std::unordered_map<IR::Block*, std::map<Variant, IR::Inst*>> incomplete_phis;
    DefTable current_def;
};

void VisitInst(Pass& pass, IR::Block* block, IR::Inst& inst) {
    const IR::Opcode opcode{inst.GetOpcode()};
    switch (opcode) {
    case IR::Opcode::SetThreadBitScalarReg: {
        const IR::ScalarReg reg{inst.Arg(0).ScalarReg()};
        pass.WriteVariable(ThreadBitScalar{reg}, block, inst.Arg(1));
        break;
    }
    case IR::Opcode::SetScalarRegister: {
        const IR::ScalarReg reg{inst.Arg(0).ScalarReg()};
        pass.WriteVariable(reg, block, inst.Arg(1));
        break;
    }
    case IR::Opcode::SetVectorRegister: {
        const IR::VectorReg reg{inst.Arg(0).VectorReg()};
        pass.WriteVariable(reg, block, inst.Arg(1));
        break;
    }
    case IR::Opcode::SetGotoVariable:
        pass.WriteVariable(GotoVariable{inst.Arg(0).U32()}, block, inst.Arg(1));
        break;
    case IR::Opcode::SetExec:
        pass.WriteVariable(ExecFlagTag{}, block, inst.Arg(0));
        break;
    case IR::Opcode::SetScc:
        pass.WriteVariable(SccFlagTag{}, block, inst.Arg(0));
        break;
    case IR::Opcode::SetVcc:
        pass.WriteVariable(VccFlagTag{}, block, inst.Arg(0));
        break;
    case IR::Opcode::SetVccLo:
        pass.WriteVariable(VccLoTag{}, block, inst.Arg(0));
        break;
    case IR::Opcode::SetVccHi:
        pass.WriteVariable(VccHiTag{}, block, inst.Arg(0));
        break;
    case IR::Opcode::SetM0:
        pass.WriteVariable(M0Tag{}, block, inst.Arg(0));
        break;
    case IR::Opcode::GetThreadBitScalarReg: {
        const IR::ScalarReg reg{inst.Arg(0).ScalarReg()};
        const IR::Value value = pass.ReadVariable(ThreadBitScalar{reg}, block);
        inst.ReplaceUsesWith(value);
        break;
    }
    case IR::Opcode::GetScalarRegister: {
        const IR::ScalarReg reg{inst.Arg(0).ScalarReg()};
        const IR::Value value = pass.ReadVariable(reg, block);
        inst.ReplaceUsesWith(value);
        break;
    }
    case IR::Opcode::GetVectorRegister: {
        const IR::VectorReg reg{inst.Arg(0).VectorReg()};
        const IR::Value value = pass.ReadVariable(reg, block);
        inst.ReplaceUsesWith(value);
        break;
    }
    case IR::Opcode::GetGotoVariable:
        inst.ReplaceUsesWith(pass.ReadVariable(GotoVariable{inst.Arg(0).U32()}, block));
        break;
    case IR::Opcode::GetExec:
        inst.ReplaceUsesWith(pass.ReadVariable(ExecFlagTag{}, block));
        break;
    case IR::Opcode::GetScc:
        inst.ReplaceUsesWith(pass.ReadVariable(SccFlagTag{}, block));
        break;
    case IR::Opcode::GetVcc:
        inst.ReplaceUsesWith(pass.ReadVariable(VccFlagTag{}, block));
        break;
    case IR::Opcode::GetVccLo:
        inst.ReplaceUsesWith(pass.ReadVariable(VccLoTag{}, block));
        break;
    case IR::Opcode::GetVccHi:
        inst.ReplaceUsesWith(pass.ReadVariable(VccHiTag{}, block));
        break;
    case IR::Opcode::GetM0:
        inst.ReplaceUsesWith(pass.ReadVariable(M0Tag{}, block));
        break;
    default:
        break;
    }
}

void VisitBlock(Pass& pass, IR::Block* block) {
    for (IR::Inst& inst : block->Instructions()) {
        VisitInst(pass, block, inst);
    }
    pass.SealBlock(block);
}

} // Anonymous namespace

void SsaRewritePass(IR::BlockList& program) {
    Pass pass;
    const auto end{program.rend()};
    for (auto block = program.rbegin(); block != end; ++block) {
        VisitBlock(pass, *block);
    }
}

} // namespace Shader::Optimization
