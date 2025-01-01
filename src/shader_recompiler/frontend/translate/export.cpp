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
    if (attrib == IR::Attribute::Depth && exp.en != 0 && exp.en != 1) {
        LOG_WARNING(Render_Vulkan, "Unsupported depth export");
        return;
    }

    const std::array vsrc = {
        IR::VectorReg(inst.src[0].code),
        IR::VectorReg(inst.src[1].code),
        IR::VectorReg(inst.src[2].code),
        IR::VectorReg(inst.src[3].code),
    };

    const auto set_attribute = [&](u32 comp, IR::F32 value) {
        if (!IR::IsMrt(attrib)) {
            ir.SetAttribute(attrib, value, comp);
            return;
        }
        const u32 index = u32(attrib) - u32(IR::Attribute::RenderTarget0);
        const auto [r, g, b, a] = runtime_info.fs_info.color_buffers[index].swizzle;
        const std::array swizzle_array = {r, g, b, a};
        const auto swizzled_comp = swizzle_array[comp];
        if (u32(swizzled_comp) < u32(AmdGpu::CompSwizzle::Red)) {
            ir.SetAttribute(attrib, value, comp);
            return;
        }
        ir.SetAttribute(attrib, value, u32(swizzled_comp) - u32(AmdGpu::CompSwizzle::Red));
    };

    const auto unpack = [&](u32 idx) {
        const IR::Value value = ir.UnpackHalf2x16(ir.GetVectorReg(vsrc[idx]));
        const IR::F32 r = IR::F32{ir.CompositeExtract(value, 0)};
        const IR::F32 g = IR::F32{ir.CompositeExtract(value, 1)};
        set_attribute(idx * 2, r);
        set_attribute(idx * 2 + 1, g);
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
            set_attribute(i, comp);
        }
    }
    if (IR::IsMrt(attrib)) {
        info.mrt_mask |= 1u << u8(attrib);
    }
}

} // namespace Shader::Gcn
