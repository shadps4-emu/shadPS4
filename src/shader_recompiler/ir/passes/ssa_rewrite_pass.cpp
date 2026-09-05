// SPDX-FileCopyrightText: Copyright 2025 Philip Rebohle
// SPDX-License-Identifier: MIT

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

#include <variant>
#include <vector>

#include "shader_recompiler/ir/basic_block.h"
#include "shader_recompiler/ir/opcodes.h"
#include "shader_recompiler/ir/program.h"
#include "shader_recompiler/ir/reg.h"
#include "shader_recompiler/ir/value.h"

namespace Shader::Optimization {
namespace {

using RegType = IR::RegType;
using ValueMap = std::unordered_map<IR::Block*, IR::Value>;

struct DefTable {
    const IR::Value& Def(IR::Block* block, IR::RegTag tag) {
        if (tag.IsIntrusive()) {
            return block->ssa_state.values[tag.Index()];
        }
        switch (tag.type) {
        case RegType::GotoVariable:
            return goto_vars[tag.index][block];
        case RegType::MaskLaneVariable:
            return mask_lane_vars[tag.lane_reg.Key()][block];
        case RegType::VirtualReg:
            return reg_vars[tag.reg.Key()][block];
        default:
            UNREACHABLE();
        }
    }
    void SetDef(IR::Block* block, IR::RegTag tag, const IR::Value& value) {
        if (tag.IsIntrusive()) {
            block->ssa_state.values[tag.Index()] = value;
            return;
        }
        switch (tag.type) {
        case RegType::GotoVariable:
            goto_vars[tag.index].insert_or_assign(block, value);
            return;
        case RegType::MaskLaneVariable:
            mask_lane_vars[tag.lane_reg.Key()].insert_or_assign(block, value);
            return;
        case RegType::VirtualReg:
            reg_vars[tag.reg.Key()].insert_or_assign(block, value);
            return;
        default:
            UNREACHABLE();
        }
    }

    std::unordered_map<u32, ValueMap> goto_vars;
    std::unordered_map<u32, ValueMap> mask_lane_vars;
    std::unordered_map<u64, ValueMap> reg_vars;
};

constexpr IR::Type TypeOf(IR::RegTag tag) noexcept {
    switch (tag.type) {
    case RegType::ScalarReg:
    case RegType::VectorReg:
    case RegType::VccLo:
    case RegType::VccHi:
    case RegType::M0:
        return IR::Type::U32;
    case RegType::ThreadBitReg:
    case RegType::Scc:
    case RegType::Vcc:
    case RegType::Exec:
    case RegType::GotoVariable:
    case RegType::MaskLaneVariable:
        return IR::Type::U1;
    case RegType::VirtualReg:
        return tag.reg.type;
    default:
        UNREACHABLE();
    }
}

constexpr IR::Opcode UndefOpcode(IR::RegTag tag) noexcept {
    switch (tag.type) {
    case RegType::ScalarReg:
    case RegType::VectorReg:
    case RegType::VccLo:
    case RegType::VccHi:
    case RegType::M0:
        return IR::Opcode::UndefU32;
    case RegType::ThreadBitReg:
    case RegType::Scc:
    case RegType::Vcc:
    case RegType::Exec:
    case RegType::GotoVariable:
    case RegType::MaskLaneVariable:
        return IR::Opcode::UndefU1;
    case RegType::VirtualReg:
        switch (tag.reg.type) {
        case IR::Type::U32:
            return IR::Opcode::UndefU32;
        case IR::Type::F32:
            return IR::Opcode::UndefF32;
        case IR::Type::U1:
            return IR::Opcode::UndefU1;
        default:
            UNREACHABLE();
        }
    default:
        UNREACHABLE();
    }
}

class Pass {
public:
    void WriteVariable(IR::RegTag tag, IR::Block* block, const IR::Value& value) {
        current_def.SetDef(block, tag, value);
    }

    IR::Value ReadVariable(IR::RegTag tag, IR::Block* block) {
        boost::container::small_vector<IR::Block*, 16> chain;
        IR::Value result{};

        while (block) {
            if (const IR::Value& def = current_def.Def(block, tag); !def.IsEmpty()) {
                result = def;
                break;
            }

            const auto preds = block->ImmPredecessors();
            if (preds.size() == 1) {
                // Optimize the common case of one predecessor: no phi needed
                chain.push_back(std::exchange(block, preds.front()));
                continue;
            } else if (preds.empty()) {
                result = IR::Value{&*block->PrependNewInst(block->begin(), UndefOpcode(tag))};
                WriteVariable(tag, block, result);
                break;
            }

            // This is a join block which may require a phi.
            // That will act as the variables current definition to break potential cycles.
            IR::Inst* const phi{&*block->PrependNewInst(block->begin(), IR::Opcode::Phi)};
            phi->SetFlags(TypeOf(tag));
            phi->SetRegTag(tag);

            result = IR::Value{phi};
            WriteVariable(tag, block, result);
            m_pending_phis.push_back(phi);
            break;
        }

        for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
            WriteVariable(tag, *it, result);
        }

        return result;
    }

    void EvaluatePendingPhis() {
        for (size_t i = 0; i < m_pending_phis.size(); i++) {
            IR::Inst* phi = m_pending_phis[i];
            for (IR::Block* pred : phi->GetParent()->ImmPredecessors()) {
                phi->AddPhiOperand(pred, ReadVariable(phi->GetRegTag(), pred));
            }
        }
    }

    void ResolveTrivialPhis() {
        auto worklist = std::move(m_pending_phis);
        while (!worklist.empty()) {
            IR::Inst* phi = worklist.back();
            worklist.pop_back();

            if (phi->GetOpcode() == IR::Opcode::Void) {
                continue;
            }

            IR::Value same;
            bool non_trivial = false;
            for (size_t i = 0; i < phi->NumArgs(); ++i) {
                const IR::Value op{phi->Arg(i)};
                if (op == same || op == IR::Value{phi}) {
                    // Unique value or self-reference
                    continue;
                }
                if (!same.IsEmpty()) {
                    // The phi merges at least two values: not trivial
                    non_trivial = true;
                    break;
                }
                same = op;
            }
            if (non_trivial) {
                continue;
            }

            IR::Block* block = phi->GetParent();
            if (same.IsEmpty()) {
                // All operands are self-references or phi has no operands
                auto& list = block->Instructions();
                auto reinsert_point = std::ranges::find_if_not(list, IR::IsPhi);
                same = IR::Value{
                    &*block->PrependNewInst(reinsert_point, UndefOpcode(phi->GetRegTag()))};
            }

            // Add phi users to worklist since they may have become trivial
            for (const auto& [user, operand] : phi->Uses()) {
                if (user->GetOpcode() == IR::Opcode::Phi && user != phi) {
                    worklist.push_back(user);
                }
            }
            phi->ReplaceUsesWithAndRemove(same);
            auto it = IR::Block::InstructionList::s_iterator_to(*phi);
            block->Instructions().erase(it);
        }
    }

private:
    DefTable current_def;
    std::vector<IR::Inst*> m_pending_phis;
};

void VisitInst(Pass& pass, IR::Block* block, IR::Inst& inst) {
    const IR::Opcode opcode{inst.GetOpcode()};
    switch (opcode) {
    case IR::Opcode::SetThreadBitScalarReg:
        pass.WriteVariable(IR::RegTag{inst.Arg(0).ScalarReg(), true}, block, inst.Arg(1));
        break;
    case IR::Opcode::SetScalarRegister:
        pass.WriteVariable(IR::RegTag{inst.Arg(0).ScalarReg()}, block, inst.Arg(1));
        break;
    case IR::Opcode::SetVectorRegister:
        pass.WriteVariable(IR::RegTag{inst.Arg(0).VectorReg()}, block, inst.Arg(1));
        break;
    case IR::Opcode::SetVirtualRegister:
        pass.WriteVariable(IR::RegTag{inst.Arg(0).VirtualReg()}, block, inst.Arg(1));
        break;
    case IR::Opcode::SetGotoVariable:
        pass.WriteVariable(IR::RegTag{RegType::GotoVariable, inst.Arg(0).U32()}, block,
                           inst.Arg(1));
        break;
    case IR::Opcode::SetMaskLaneVariable:
        pass.WriteVariable(IR::RegTag{inst.Arg(0).VectorReg(), inst.Arg(1).U32()}, block,
                           inst.Arg(2));
        break;
    case IR::Opcode::SetExec:
        pass.WriteVariable(IR::RegTag{RegType::Exec}, block, inst.Arg(0));
        break;
    case IR::Opcode::SetScc:
        pass.WriteVariable(IR::RegTag{RegType::Scc}, block, inst.Arg(0));
        break;
    case IR::Opcode::SetVcc:
        pass.WriteVariable(IR::RegTag{RegType::Vcc}, block, inst.Arg(0));
        break;
    case IR::Opcode::SetVccLo:
        pass.WriteVariable(IR::RegTag{RegType::VccLo}, block, inst.Arg(0));
        break;
    case IR::Opcode::SetVccHi:
        pass.WriteVariable(IR::RegTag{RegType::VccHi}, block, inst.Arg(0));
        break;
    case IR::Opcode::SetM0:
        pass.WriteVariable(IR::RegTag{RegType::M0}, block, inst.Arg(0));
        break;
    case IR::Opcode::GetThreadBitScalarReg:
        inst.ReplaceUsesWithAndRemove(
            pass.ReadVariable(IR::RegTag{inst.Arg(0).ScalarReg(), true}, block));
        break;
    case IR::Opcode::GetScalarRegister:
        inst.ReplaceUsesWithAndRemove(
            pass.ReadVariable(IR::RegTag{inst.Arg(0).ScalarReg()}, block));
        break;
    case IR::Opcode::GetVectorRegister:
        inst.ReplaceUsesWithAndRemove(
            pass.ReadVariable(IR::RegTag{inst.Arg(0).VectorReg()}, block));
        break;
    case IR::Opcode::GetVirtualRegister:
        inst.ReplaceUsesWithAndRemove(
            pass.ReadVariable(IR::RegTag{inst.Arg(0).VirtualReg()}, block));
        break;
    case IR::Opcode::GetGotoVariable:
        inst.ReplaceUsesWithAndRemove(
            pass.ReadVariable(IR::RegTag{RegType::GotoVariable, inst.Arg(0).U32()}, block));
        break;
    case IR::Opcode::GetMaskLaneVariable:
        inst.ReplaceUsesWithAndRemove(
            pass.ReadVariable(IR::RegTag{inst.Arg(0).VectorReg(), inst.Arg(1).U32()}, block));
        break;
    case IR::Opcode::GetExec:
        inst.ReplaceUsesWithAndRemove(pass.ReadVariable(IR::RegTag{RegType::Exec}, block));
        break;
    case IR::Opcode::GetScc:
        inst.ReplaceUsesWithAndRemove(pass.ReadVariable(IR::RegTag{RegType::Scc}, block));
        break;
    case IR::Opcode::GetVcc:
        inst.ReplaceUsesWithAndRemove(pass.ReadVariable(IR::RegTag{RegType::Vcc}, block));
        break;
    case IR::Opcode::GetVccLo:
        inst.ReplaceUsesWithAndRemove(pass.ReadVariable(IR::RegTag{RegType::VccLo}, block));
        break;
    case IR::Opcode::GetVccHi:
        inst.ReplaceUsesWithAndRemove(pass.ReadVariable(IR::RegTag{RegType::VccHi}, block));
        break;
    case IR::Opcode::GetM0:
        inst.ReplaceUsesWithAndRemove(pass.ReadVariable(IR::RegTag{RegType::M0}, block));
        break;
    default:
        break;
    }
}

} // Anonymous namespace

void SsaRewritePass(IR::Program& program) {
    Pass pass;
    const auto end = program.post_order_blocks.rend();
    for (auto it = program.post_order_blocks.rbegin(); it != end; ++it) {
        IR::Block* block{*it};
        for (IR::Inst& inst : block->Instructions()) {
            VisitInst(pass, block, inst);
        }
    }
    pass.EvaluatePendingPhis();
    pass.ResolveTrivialPhis();
}

} // namespace Shader::Optimization
