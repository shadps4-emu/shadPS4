// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/frontend/translate/translate.h"
#include "shader_recompiler/runtime_info.h"

namespace Shader::Gcn {

void Translator::EmitExport(const GcnInst& inst) {
    if (ir.block->has_multiple_predecessors && info.stage == Stage::Fragment) {
        ir.Discard(ir.LogicalNot(ir.GetExec()));
    }

    const auto& exp = inst.control.exp;
    const IR::Attribute attrib{exp.target};
    const std::array vsrc = {
        IR::VectorReg(inst.src[0].code),
        IR::VectorReg(inst.src[1].code),
        IR::VectorReg(inst.src[2].code),
        IR::VectorReg(inst.src[3].code),
    };

    const auto swizzle = [&](u32 comp) {
        if (!IR::IsMrt(attrib)) {
            return comp;
        }
        const u32 index = u32(attrib) - u32(IR::Attribute::RenderTarget0);
        switch (runtime_info.fs_info.color_buffers[index].mrt_swizzle) {
        case MrtSwizzle::Identity:
            return comp;
        case MrtSwizzle::Alt:
            static constexpr std::array<u32, 4> AltSwizzle = {2, 1, 0, 3};
            return AltSwizzle[comp];
        case MrtSwizzle::Reverse:
            static constexpr std::array<u32, 4> RevSwizzle = {3, 2, 1, 0};
            return RevSwizzle[comp];
        case MrtSwizzle::ReverseAlt:
            static constexpr std::array<u32, 4> AltRevSwizzle = {3, 0, 1, 2};
            return AltRevSwizzle[comp];
        default:
            UNREACHABLE();
        }
    };

    const auto unpack = [&](u32 idx) {
        const IR::Value value = ir.UnpackHalf2x16(ir.GetVectorReg(vsrc[idx]));
        const IR::F32 r = IR::F32{ir.CompositeExtract(value, 0)};
        const IR::F32 g = IR::F32{ir.CompositeExtract(value, 1)};
        ir.SetAttribute(attrib, r, swizzle(idx * 2));
        ir.SetAttribute(attrib, g, swizzle(idx * 2 + 1));
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
            ir.SetAttribute(attrib, comp, swizzle(i));
        }
    }
    if (IR::IsMrt(attrib)) {
        info.mrt_mask |= 1u << u8(attrib);
    }
}

} // namespace Shader::Gcn
