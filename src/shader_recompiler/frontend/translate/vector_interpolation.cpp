// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/frontend/translate/translate.h"

namespace Shader::Gcn {

void Translator::V_INTERP_P2_F32(const GcnInst& inst) {
    const IR::VectorReg dst_reg{inst.dst[0].code};
    const IR::Attribute attrib{IR::Attribute::Param0 + inst.control.vintrp.attr};
    ir.SetVectorReg(dst_reg, ir.GetAttribute(attrib, inst.control.vintrp.chan));
}

} // namespace Shader::Gcn
