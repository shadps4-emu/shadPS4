// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/ir/basic_block.h"
#include "shader_recompiler/ir/ir_emitter.h"
#include "shader_recompiler/ir/opcodes.h"
#include "shader_recompiler/ir/program.h"
#include "shader_recompiler/ir/reg.h"
#include "shader_recompiler/ir/value.h"

namespace Shader::Optimization {

static IR::Value LoadForRegType(IR::IREmitter& ir, IR::RegTag tag) {
    switch (tag.type) {
    case IR::RegType::ScalarReg:
        return ir.GetScalarReg(tag.sreg);
    case IR::RegType::VectorReg:
        return ir.GetVectorReg(tag.vreg);
    case IR::RegType::ThreadBitReg:
        return ir.GetThreadBitScalarReg(tag.sreg);
    case IR::RegType::Scc:
        return ir.GetScc();
    case IR::RegType::Exec:
        return ir.GetExec();
    case IR::RegType::Vcc:
        return ir.GetVcc();
    case IR::RegType::VccLo:
        return ir.GetVccLo();
    case IR::RegType::VccHi:
        return ir.GetVccHi();
    case IR::RegType::M0:
        return ir.GetM0();
    case IR::RegType::MaskLaneVariable:
        return ir.GetMaskLaneVariable(tag.lane_reg.vreg, tag.lane_reg.lane);
    case IR::RegType::VirtualReg:
        return ir.GetVirtualReg(tag.reg);
    default:
        UNREACHABLE_MSG("Unknown reg type {}", u32(tag.type));
    }
}

void LowerPhisToRegsPass(IR::Program& program) {
    const auto end = program.post_order_blocks.rend();
    for (auto it1 = program.post_order_blocks.rbegin(); it1 != end; ++it1) {
        IR::Block* block{*it1};
        for (auto it = block->begin(); it != block->end(); ++it) {
            IR::Inst& inst{*it};
            if (inst.GetOpcode() == IR::Opcode::Void) {
                continue;
            }
            if (inst.GetOpcode() != IR::Opcode::Phi) {
                // Phis are always first in a block
                break;
            }

            IR::IREmitter ir{*block, it};
            if (const auto tag = inst.GetRegTag()) {
                inst.ReplaceUsesWithAndRemove(LoadForRegType(ir, tag));
                continue;
            }

            const IR::VirtualReg reg{program.next_reg_index++, inst.Flags<IR::Type>()};
            for (size_t arg_index = 0; arg_index < inst.NumArgs(); arg_index++) {
                IR::IREmitter ir{*inst.PhiBlock(arg_index)};
                ir.SetVirtualReg(reg, inst.Arg(arg_index));
            }
            inst.ReplaceUsesWithAndRemove(ir.GetVirtualReg(reg));
        }
    }
}

} // namespace Shader::Optimization
