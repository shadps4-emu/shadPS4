// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/frontend/translate/translate.h"

namespace Shader::Gcn {

void Translator::EmitVectorInterpolation(const GcnInst& inst) {
    switch (inst.opcode) {
        // VINTRP
    case Opcode::V_INTERP_P1_F32:
        return;
    case Opcode::V_INTERP_P2_F32:
        return V_INTERP_P2_F32(inst);
    case Opcode::V_INTERP_MOV_F32:
        return V_INTERP_MOV_F32(inst);
    default:
        LogMissingOpcode(inst);
    }
}

// VINTRP

void Translator::V_INTERP_P2_F32(const GcnInst& inst) {
    const u32 attr_index = inst.control.vintrp.attr;
    const auto& attr = runtime_info.fs_info.inputs.at(attr_index);
    info.interp_qualifiers[attr_index] = vgpr_to_interp[inst.src[0].code];
    const IR::Attribute attrib{IR::Attribute::Param0 + attr_index};
    SetDst(inst.dst[0], ir.GetAttribute(attrib, inst.control.vintrp.chan));
}

void Translator::V_INTERP_MOV_F32(const GcnInst& inst) {
    const u32 attr_index = inst.control.vintrp.attr;
    const auto& attr = runtime_info.fs_info.inputs.at(attr_index);
    const IR::Attribute attrib{IR::Attribute::Param0 + attr_index};
    SetDst(inst.dst[0], ir.GetAttribute(attrib, inst.control.vintrp.chan));
}

} // namespace Shader::Gcn
