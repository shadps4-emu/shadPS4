// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/frontend/opcodes.h"
#include "shader_recompiler/frontend/translate/translate.h"

namespace Shader::Gcn {

void Translator::EmitFlowControl(const GcnInst& inst) {
    switch (inst.opcode) {
    case Opcode::S_BARRIER:
        return S_BARRIER();
    case Opcode::S_TTRACEDATA:
        LOG_WARNING(Render_Vulkan, "S_TTRACEDATA instruction!");
        return;
    case Opcode::S_SETPRIO:
        LOG_WARNING(Render_Vulkan, "S_SETPRIO instruction!");
        return;
    case Opcode::S_SETVSKIP:
        return S_SETVSKIP(inst);
    case Opcode::S_TRAP:
        LOG_WARNING(Render_Vulkan, "S_TRAP instruction!");
        return;
    case Opcode::S_GETPC_B64:
        return S_GETPC_B64(inst);
    case Opcode::S_SETPC_B64:
    case Opcode::S_WAITCNT:
    case Opcode::S_NOP:
    case Opcode::S_ENDPGM:
    case Opcode::S_CBRANCH_EXECZ:
    case Opcode::S_CBRANCH_SCC0:
    case Opcode::S_CBRANCH_SCC1:
    case Opcode::S_CBRANCH_VCCNZ:
    case Opcode::S_CBRANCH_VCCZ:
    case Opcode::S_CBRANCH_EXECNZ:
    case Opcode::S_BRANCH:
        return;
    case Opcode::S_SENDMSG:
        S_SENDMSG(inst);
        return;
    default:
        UNREACHABLE();
    }
}

void Translator::S_BARRIER() {
    ir.Barrier();
}

void Translator::S_GETPC_B64(const GcnInst& inst) {
    const IR::ScalarReg dst{inst.dst[0].code};
    ir.SetScalarReg(dst, ir.GetPcLo(ir.Imm32(pc)));
    ir.SetScalarReg(dst + 1, ir.Imm32(0));
}

void Translator::S_SETVSKIP(const GcnInst& inst) {
    // VSKIP = (SSRC0 & (1 << (SSRC1 & 31))) != 0 (AMD GCN3/Vega ISA, SOPC
    // instructions). When set, real hardware skips vector ALU/memory/export/LDS/GDS
    // instructions for the whole wave until the next S_SETVSKIP, as a fast
    // alternative to branching. CFG::SplitDivergenceScopes recognizes the
    // open/close S_SETVSKIP pair around such guarded code and wraps it in a
    // divergence block keyed on Condition::Vskipz, so here we only need to keep
    // the tracked VSKIP value itself up to date; we don't need to (and can't,
    // since it's not a real per-lane mask) skip anything directly.
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{GetSrc(inst.src[1])};
    const IR::U32 bitpos{ir.BitwiseAnd(src1, ir.Imm32(0x1FU))};
    const IR::U32 bit{ir.BitwiseAnd(ir.ShiftRightLogical(src0, bitpos), ir.Imm32(1U))};
    ir.SetVskip(ir.INotEqual(bit, ir.Imm32(0U)));
}

void Translator::S_SENDMSG(const GcnInst& inst) {
    const auto& simm = reinterpret_cast<const SendMsgSimm&>(inst.control.sopp.simm);
    switch (simm.msg) {
    case SendMsgSimm::Message::Gs: {
        switch (simm.op) {
        case SendMsgSimm::GsOp::Nop:
            break;
        case SendMsgSimm::GsOp::Cut:
            ir.EmitPrimitive();
            break;
        case SendMsgSimm::GsOp::Emit:
            ir.EmitVertex();
            break;
        default:
            UNREACHABLE();
        }
        break;
    }
    case SendMsgSimm::Message::GsDone: {
        break;
    }
    default:
        UNREACHABLE();
    }
}

} // namespace Shader::Gcn
