// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>

#include <boost/container/small_vector.hpp>

#include "shader_recompiler/ir/ir_emitter.h"
#include "shader_recompiler/ir/program.h"

namespace Shader::Optimization {

static bool IsStepOf(const IR::Value& incoming, const IR::Inst* phi, u32 step_imm) {
    if (incoming.IsImmediate()) {
        return false;
    }
    const IR::Inst* const producer = incoming.Inst();
    if (producer->GetOpcode() == IR::Opcode::IAdd32) {
        const IR::Value lhs = producer->Arg(0);
        const IR::Value rhs = producer->Arg(1);
        return !lhs.IsImmediate() && lhs.Inst() == phi && rhs.IsImmediate() &&
               rhs.U32() == step_imm;
    }
    if (step_imm == 0xFFFFFFFFu && producer->GetOpcode() == IR::Opcode::ISub32) {
        const IR::Value lhs = producer->Arg(0);
        const IR::Value rhs = producer->Arg(1);
        return !lhs.IsImmediate() && lhs.Inst() == phi && rhs.IsImmediate() && rhs.U32() == 1u;
    }
    return false;
}

static bool OnlyFeedsBranchConditions(IR::Inst& root) {
    boost::container::small_vector<IR::Inst*, 8> worklist{&root};
    boost::container::small_vector<const IR::Inst*, 16> visited;
    while (!worklist.empty()) {
        IR::Inst* const inst = worklist.back();
        worklist.pop_back();
        if (std::ranges::find(visited, inst) != visited.end()) {
            continue;
        }
        visited.push_back(inst);
        if (visited.size() > 64) {
            return false;
        }
        if (!inst->HasUses()) {
            return false;
        }
        for (const auto& [user, arg_index] : inst->Uses()) {
            switch (user->GetOpcode()) {
            case IR::Opcode::ConditionRef:
                continue;
            case IR::Opcode::LogicalNot:
            case IR::Opcode::LogicalAnd:
            case IR::Opcode::LogicalOr:
            case IR::Opcode::Phi:
                worklist.push_back(user);
                continue;
            default:
                return false;
            }
        }
    }
    return true;
}

static const IR::Inst* AsSteppingPhi(const IR::Value& value, u32 step_imm) {
    if (value.IsImmediate()) {
        return nullptr;
    }
    const IR::Inst* const inst = value.Inst();
    if (inst->GetOpcode() != IR::Opcode::Phi) {
        return nullptr;
    }
    for (size_t i = 0; i < inst->NumArgs(); ++i) {
        if (IsStepOf(inst->Arg(i), inst, step_imm)) {
            return inst;
        }
    }
    return nullptr;
}

void LoopWrapGuardPass(IR::Program& program) {
    constexpr u32 bound_cap = 16384;
    constexpr u32 MaxExitConst = 4;
    for (IR::Block* const block : program.blocks) {
        for (IR::Inst& inst : block->Instructions()) {
            const IR::Opcode op = inst.GetOpcode();
            const bool is_wrap_shape = op == IR::Opcode::INotEqual32;
            const bool is_cap_shape =
                bound_cap != 0 &&
                (op == IR::Opcode::ULessThan32 || op == IR::Opcode::SLessThan32 ||
                 op == IR::Opcode::UGreaterThan32 || op == IR::Opcode::SGreaterThan32);
            if (!is_wrap_shape && !is_cap_shape) {
                continue;
            }
            if (!OnlyFeedsBranchConditions(inst)) {
                continue;
            }
            if (is_cap_shape) {
                const bool phi_first =
                    op == IR::Opcode::ULessThan32 || op == IR::Opcode::SLessThan32;
                const IR::Value phi_side = inst.Arg(phi_first ? 0 : 1);
                const u32 bound_idx = phi_first ? 1 : 0;
                const IR::Value bound = inst.Arg(bound_idx);
                if (bound.IsImmediate() || AsSteppingPhi(phi_side, 1u) == nullptr) {
                    continue;
                }
                const bool is_signed =
                    op == IR::Opcode::SLessThan32 || op == IR::Opcode::SGreaterThan32;
                IR::IREmitter ir{*block, IR::Block::InstructionList::s_iterator_to(inst)};
                const IR::U32 capped = ir.IMin(IR::U32{bound}, ir.Imm32(bound_cap), is_signed);
                inst.SetArg(bound_idx, capped);
                continue;
            }
            const IR::Value a = inst.Arg(0);
            const IR::Value b = inst.Arg(1);
            const IR::Inst* phi = nullptr;
            u32 exit_const = 0;
            if (b.IsImmediate() && (phi = AsSteppingPhi(a, 0xFFFFFFFFu)) != nullptr) {
                exit_const = b.U32();
            } else if (a.IsImmediate() && (phi = AsSteppingPhi(b, 0xFFFFFFFFu)) != nullptr) {
                exit_const = a.U32();
                inst.SetArg(0, b);
                inst.SetArg(1, a);
            } else {
                phi = nullptr;
            }
            if (phi != nullptr && exit_const <= MaxExitConst) {
                inst.ReplaceOpcode(IR::Opcode::UGreaterThan32);
                continue;
            }
            bool rewrote_shape2 = false;
            if ((phi = AsSteppingPhi(a, 1u)) != nullptr && (b.IsImmediate() || b.Inst() != phi)) {
                inst.ReplaceOpcode(IR::Opcode::ULessThan32);
                rewrote_shape2 = true;
            } else if ((phi = AsSteppingPhi(b, 1u)) != nullptr &&
                       (a.IsImmediate() || a.Inst() != phi)) {
                inst.SetArg(0, b);
                inst.SetArg(1, a);
                inst.ReplaceOpcode(IR::Opcode::ULessThan32);
                rewrote_shape2 = true;
            }
            if (rewrote_shape2 && bound_cap != 0 && !inst.Arg(1).IsImmediate()) {
                IR::IREmitter ir{*block, IR::Block::InstructionList::s_iterator_to(inst)};
                inst.SetArg(1, ir.IMin(IR::U32{inst.Arg(1)}, ir.Imm32(bound_cap), false));
            }
        }
    }
}

} // namespace Shader::Optimization
