// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/frontend/translate/translate.h"

namespace Shader::Gcn {

void Translator::S_LOAD_DWORD(int num_dwords, const GcnInst& inst) {
    const auto& smrd = inst.control.smrd;
    ASSERT_MSG(smrd.imm, "Bindless texture loads unsupported");
    const IR::ScalarReg sbase{inst.src[0].code * 2};
    const IR::Value base =
        ir.CompositeConstruct(ir.GetScalarReg(sbase), ir.GetScalarReg(sbase + 1));
    IR::ScalarReg dst_reg{inst.dst[0].code};
    for (u32 i = 0; i < num_dwords; i++) {
        ir.SetScalarReg(dst_reg++, ir.ReadConst(base, ir.Imm32(smrd.offset + i)));
    }
}

void Translator::S_BUFFER_LOAD_DWORD(int num_dwords, const GcnInst& inst) {
    const auto& smrd = inst.control.smrd;
    const IR::ScalarReg sbase{inst.src[0].code * 2};
    const IR::U32 dword_offset =
        smrd.imm ? ir.Imm32(smrd.offset) : ir.GetScalarReg(IR::ScalarReg(smrd.offset));
    const IR::Value vsharp = ir.GetScalarReg(sbase);
    IR::ScalarReg dst_reg{inst.dst[0].code};
    for (u32 i = 0; i < num_dwords; i++) {
        const IR::U32 index = ir.IAdd(dword_offset, ir.Imm32(i));
        ir.SetScalarReg(dst_reg++, ir.ReadConstBuffer(vsharp, index));
    }
}

} // namespace Shader::Gcn
