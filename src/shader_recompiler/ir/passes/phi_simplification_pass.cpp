// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <vector>
#include "shader_recompiler/ir/program.h"

namespace Shader::Optimization {

static bool IsCompositeExtract(IR::Inst* const inst) {
    switch (inst->GetOpcode()) {
    case IR::Opcode::CompositeExtractU32x2:
    case IR::Opcode::CompositeExtractU32x3:
    case IR::Opcode::CompositeExtractU32x4:
        return true;
    default:
        return false;
    }
}

static IR::Inst* FoldPhi(IR::Inst& phi, IR::Opcode opcode, IR::Type type, auto&&... args) {
    auto insert_point = IR::Block::InstructionList::s_iterator_to(phi);
    IR::Block* block = phi.GetParent();
    IR::Inst* const new_phi{&*block->PrependNewInst(insert_point, IR::Opcode::Phi)};
    new_phi->SetFlags(type);

    for (size_t arg_index = 0; arg_index < phi.NumArgs(); ++arg_index) {
        new_phi->AddPhiOperand(phi.PhiBlock(arg_index), phi.Arg(arg_index).Inst()->Arg(0));
    }

    auto it = std::ranges::find_if_not(block->Instructions(), IR::IsPhi);
    IR::Value const replacement{
        &*block->PrependNewInst(it, opcode, {IR::Value{new_phi}, IR::Value{args}...})};
    phi.ReplaceUsesWithAndRemove(replacement);
    block->Instructions().erase(insert_point);
    return new_phi;
}

static IR::Inst* FoldPhiArgOpIntoPhi(IR::Inst& phi) {
    IR::Inst* const first_arg = phi.Arg(0).Inst();
    const IR::Opcode opcode = first_arg->GetOpcode();
    const IR::Type phi_type = first_arg->Arg(0).Type();

    if (IsCompositeExtract(first_arg)) {
        const u32 index = first_arg->Arg(1).U32();
        for (size_t arg_index = 1; arg_index < phi.NumArgs(); ++arg_index) {
            const IR::Inst* arg = phi.Arg(arg_index).Inst();
            if (arg->Arg(1).U32() != index) {
                return nullptr;
            }
        }
        return FoldPhi(phi, opcode, phi_type, index);
    } else if (first_arg->NumArgs() == 1) {
        return FoldPhi(phi, opcode, phi_type);
    }

    return nullptr;
}

static bool AllPhiArgsHaveSameOp(IR::Inst& phi) {
    IR::Inst* const first_arg = phi.Arg(0).Inst();
    for (size_t arg_index = 1; arg_index < phi.NumArgs(); ++arg_index) {
        if (phi.Arg(arg_index).IsImmediate()) {
            return false;
        }
        IR::Inst* arg = phi.Arg(arg_index).Inst();
        if (first_arg->GetOpcode() != arg->GetOpcode()) {
            return false;
        }
    }
    return true;
}

static IR::Inst* VisitPhiNode(IR::Inst& phi) {
    if (phi.Arg(0).IsImmediate()) {
        return nullptr;
    }

    // If all phi operands are the same operation, pull them through the phi
    if (AllPhiArgsHaveSameOp(phi)) {
        if (IR::Inst* inst = FoldPhiArgOpIntoPhi(phi)) {
            return inst;
        }
    }

    // If there are identical phi nodes in the current block, deduplicate them
    IR::Block* block = phi.GetParent();
    for (IR::Inst& inst : block->Instructions()) {
        if (inst.GetOpcode() != IR::Opcode::Phi) {
            break;
        }
        if (&inst == &phi) {
            continue;
        }
        bool identical = true;
        for (size_t i = 0; i < inst.NumArgs(); i++) {
            if (phi.PhiBlock(i) != inst.PhiBlock(i) || phi.Arg(i) != inst.Arg(i)) {
                identical = false;
                break;
            }
        }
        if (identical) {
            phi.ReplaceUsesWithAndRemove(IR::Value{&inst});
            auto it = IR::Block::InstructionList::s_iterator_to(phi);
            block->Instructions().erase(it);
            return &inst;
        }
    }

    return nullptr;
}

void PhiSimplificationPass(IR::Program& program) {
    std::vector<IR::Inst*> worklist;
    for (IR::Block* const block : program.blocks) {
        for (IR::Inst& inst : block->Instructions()) {
            if (inst.GetOpcode() != IR::Opcode::Phi) {
                break;
            }
            worklist.push_back(&inst);
        }
    }
    while (!worklist.empty()) {
        IR::Inst* phi = worklist.back();
        worklist.pop_back();
        if (auto* new_phi = VisitPhiNode(*phi)) {
            worklist.push_back(new_phi);
        }
    }
}

} // namespace Shader::Optimization
