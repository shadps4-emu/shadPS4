// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/frontend/translate/translate.h"

namespace Shader::Gcn {

void Load(IR::IREmitter& ir, int num_dwords, const IR::Value& handle, IR::ScalarReg dst_reg,
          const IR::U32U64& address) {
    for (u32 i = 0; i < num_dwords; i++) {
        if (handle.IsEmpty()) {
            ir.SetScalarReg(dst_reg++, ir.ReadConst(address, ir.Imm32(i)));
        } else {
            const IR::U32 index = ir.IAdd(address, ir.Imm32(i));
            ir.SetScalarReg(dst_reg++, ir.ReadConstBuffer(handle, index));
        }
    }
}

void Translator::S_LOAD_DWORD(int num_dwords, const GcnInst& inst) {
    const auto& smrd = inst.control.smrd;
    const IR::ScalarReg sbase{inst.src[0].code * 2};
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
    const IR::ScalarReg sbase{inst.src[0].code * 2};
    const IR::U32 dword_offset =
        smrd.imm ? ir.Imm32(smrd.offset) : ir.GetScalarReg(IR::ScalarReg(smrd.offset));
    const IR::Value vsharp = ir.GetScalarReg(sbase);
    const IR::ScalarReg dst_reg{inst.dst[0].code};
    Load(ir, num_dwords, vsharp, dst_reg, dword_offset);
}

} // namespace Shader::Gcn
