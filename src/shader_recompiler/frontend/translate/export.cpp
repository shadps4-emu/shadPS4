// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/frontend/translate/translate.h"

namespace Shader::Gcn {

void Translator::EXP(const GcnInst& inst) {
    const auto& exp = inst.control.exp;
    const IR::Attribute attrib{exp.target};
    const std::array vsrc = {
        IR::VectorReg(inst.src[0].code),
        IR::VectorReg(inst.src[1].code),
        IR::VectorReg(inst.src[2].code),
        IR::VectorReg(inst.src[3].code),
    };

    const auto unpack = [&](u32 idx) {
        const IR::Value value = ir.UnpackHalf2x16(ir.GetVectorReg(vsrc[idx]));
        const IR::F32 r = IR::F32{ir.CompositeExtract(value, 0)};
        const IR::F32 g = IR::F32{ir.CompositeExtract(value, 1)};
        ir.SetAttribute(attrib, r, idx * 2);
        ir.SetAttribute(attrib, g, idx * 2 + 1);
    };

    // Components are float16 packed into a VGPR
    if (exp.compr) {
        // Export R, G
        if (exp.en & 1) {
            unpack(0);
        }
        // Export B, A
        if ((exp.en >> 2) & 1) {
            unpack(1);
        }
    } else {
        // Components are float32 into separate VGPRS
        u32 mask = exp.en;
        for (u32 i = 0; i < 4; i++, mask >>= 1) {
            if ((mask & 1) == 0) {
                continue;
            }
            const IR::F32 comp = ir.GetVectorReg<IR::F32>(vsrc[i]);
            ir.SetAttribute(attrib, comp, i);
        }
    }
}

} // namespace Shader::Gcn
