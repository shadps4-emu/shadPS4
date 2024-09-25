// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/frontend/translate/translate.h"

namespace Shader::Gcn {

static constexpr u32 SQ_SRC_LITERAL = 0xFF;

void Translator::EmitScalarMemory(const GcnInst& inst) {
    switch (inst.opcode) {
        // SMRD
    case Opcode::S_LOAD_DWORD:
        return S_LOAD_DWORD(1, inst);
    case Opcode::S_LOAD_DWORDX2:
        return S_LOAD_DWORD(2, inst);
    case Opcode::S_LOAD_DWORDX4:
        return S_LOAD_DWORD(4, inst);
    case Opcode::S_LOAD_DWORDX8:
        return S_LOAD_DWORD(8, inst);
    case Opcode::S_LOAD_DWORDX16:
        return S_LOAD_DWORD(16, inst);
    case Opcode::S_BUFFER_LOAD_DWORD:
        return S_BUFFER_LOAD_DWORD(1, inst);
    case Opcode::S_BUFFER_LOAD_DWORDX2:
        return S_BUFFER_LOAD_DWORD(2, inst);
    case Opcode::S_BUFFER_LOAD_DWORDX4:
        return S_BUFFER_LOAD_DWORD(4, inst);
    case Opcode::S_BUFFER_LOAD_DWORDX8:
        return S_BUFFER_LOAD_DWORD(8, inst);
    case Opcode::S_BUFFER_LOAD_DWORDX16:
        return S_BUFFER_LOAD_DWORD(16, inst);
    default:
        LogMissingOpcode(inst);
    }
}

// SMRD

void Translator::S_LOAD_DWORD(int num_dwords, const GcnInst& inst) {
    const auto& smrd = inst.control.smrd;
    const u32 dword_offset = [&] -> u32 {
        if (smrd.imm) {
            return smrd.offset;
        }
        if (smrd.offset == SQ_SRC_LITERAL) {
            return inst.src[1].code;
        }
        UNREACHABLE();
    }();
    const IR::ScalarReg sbase{inst.src[0].code * 2};
    const IR::Value base =
        ir.CompositeConstruct(ir.GetScalarReg(sbase), ir.GetScalarReg(sbase + 1));
    IR::ScalarReg dst_reg{inst.dst[0].code};
    for (u32 i = 0; i < num_dwords; i++) {
        ir.SetScalarReg(dst_reg++, ir.ReadConst(base, ir.Imm32(dword_offset + i)));
    }
}

void Translator::S_BUFFER_LOAD_DWORD(int num_dwords, const GcnInst& inst) {
    const auto& smrd = inst.control.smrd;
    const IR::ScalarReg sbase{inst.src[0].code * 2};
    const IR::U32 dword_offset = [&] -> IR::U32 {
        if (smrd.imm) {
            return ir.Imm32(smrd.offset);
        }
        if (smrd.offset == SQ_SRC_LITERAL) {
            return ir.Imm32(inst.src[1].code);
        }
        return ir.ShiftRightLogical(ir.GetScalarReg(IR::ScalarReg(smrd.offset)), ir.Imm32(2));
    }();
    const IR::Value vsharp =
        ir.CompositeConstruct(ir.GetScalarReg(sbase), ir.GetScalarReg(sbase + 1),
                              ir.GetScalarReg(sbase + 2), ir.GetScalarReg(sbase + 3));
    IR::ScalarReg dst_reg{inst.dst[0].code};
    for (u32 i = 0; i < num_dwords; i++) {
        const IR::U32 index = ir.IAdd(dword_offset, ir.Imm32(i));
        ir.SetScalarReg(dst_reg++, ir.ReadConstBuffer(vsharp, index));
    }
}

} // namespace Shader::Gcn
