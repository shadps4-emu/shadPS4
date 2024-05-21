// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/frontend/translate/translate.h"

namespace Shader::Gcn {

void Load(IR::IREmitter& ir, int num_dwords, const IR::Value& handle, IR::ScalarReg dst_reg,
          const IR::U32U64& address) {
    for (u32 i = 0; i < num_dwords; i++) {
        const IR::U32 value = handle.IsEmpty() ? ir.ReadConst(address, ir.Imm32(i))
                                               : ir.ReadConstBuffer(handle, address, ir.Imm32(i));
        ir.SetScalarReg(dst_reg++, value);
    }
}

void Translator::S_LOAD_DWORD(int num_dwords, const GcnInst& inst) {
    const auto& smrd = inst.control.smrd;
    const IR::ScalarReg sbase = IR::ScalarReg(inst.src[0].code * 2);
    const IR::U32 offset =
        smrd.imm ? ir.Imm32(smrd.offset * 4)
                 : IR::U32{ir.ShiftLeftLogical(ir.GetScalarReg(IR::ScalarReg(smrd.offset)),
                                               ir.Imm32(2))};
    const IR::U64 base =
        ir.PackUint2x32(ir.CompositeConstruct(ir.GetScalarReg(sbase), ir.GetScalarReg(sbase + 1)));
    const IR::U64 address = ir.IAdd(base, offset);
    const IR::ScalarReg dst_reg{inst.dst[0].code};
    Load(ir, num_dwords, {}, dst_reg, address);
}

void Translator::S_BUFFER_LOAD_DWORD(int num_dwords, const GcnInst& inst) {
    const auto& smrd = inst.control.smrd;
    const IR::ScalarReg sbase = IR::ScalarReg(inst.src[0].code * 2);
    const IR::U32 offset =
        smrd.imm ? ir.Imm32(smrd.offset * 4)
                 : IR::U32{ir.ShiftLeftLogical(ir.GetScalarReg(IR::ScalarReg(smrd.offset)),
                                               ir.Imm32(2))};
    const IR::Value vsharp =
        ir.CompositeConstruct(ir.GetScalarReg(sbase), ir.GetScalarReg(sbase + 1),
                              ir.GetScalarReg(sbase + 2), ir.GetScalarReg(sbase + 3));
    const IR::ScalarReg dst_reg{inst.dst[0].code};
    Load(ir, num_dwords, vsharp, dst_reg, offset);
}

} // namespace Shader::Gcn
