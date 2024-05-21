// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/frontend/translate/translate.h"

namespace Shader::Gcn {

void Translator::IMAGE_GET_RESINFO(const GcnInst& inst) {
    IR::VectorReg dst_reg{inst.src[1].code};
    const IR::ScalarReg tsharp_reg{inst.src[2].code};
    const auto flags = ImageResFlags(inst.control.mimg.dmask);
    const IR::U32 lod = ir.GetVectorReg(IR::VectorReg(inst.src[0].code));
    const IR::Value tsharp =
        ir.CompositeConstruct(ir.GetScalarReg(tsharp_reg), ir.GetScalarReg(tsharp_reg + 1),
                              ir.GetScalarReg(tsharp_reg + 2), ir.GetScalarReg(tsharp_reg + 3));
    const IR::Value size = ir.ImageQueryDimension(tsharp, lod, ir.Imm1(false));

    if (flags.test(ImageResComponent::Width)) {
        ir.SetVectorReg(dst_reg++, IR::U32{ir.CompositeExtract(size, 0)});
    }
    if (flags.test(ImageResComponent::Height)) {
        ir.SetVectorReg(dst_reg++, IR::U32{ir.CompositeExtract(size, 1)});
    }
    if (flags.test(ImageResComponent::Depth)) {
        ir.SetVectorReg(dst_reg++, IR::U32{ir.CompositeExtract(size, 2)});
    }
    if (flags.test(ImageResComponent::MipCount)) {
        ir.SetVectorReg(dst_reg++, IR::U32{ir.CompositeExtract(size, 3)});
    }
}

void Translator::IMAGE_SAMPLE(const GcnInst& inst) {
    const auto& mimg = inst.control.mimg;
    ASSERT(!mimg.da);

    IR::VectorReg addr_reg{inst.src[0].code};
    IR::VectorReg dest_reg{inst.dst[0].code};
    const IR::ScalarReg tsharp_reg{inst.src[2].code * 4};
    const IR::ScalarReg sampler_reg{inst.src[3].code * 4};
    const auto flags = MimgModifierFlags(mimg.mod);

    // Load first dword of T# and S#. We will use them as the handle that will guide resource
    // tracking pass where to read the sharps. This will later also get patched to the SPIRV texture
    // binding index.
    const IR::Value handle =
        ir.CompositeConstruct(ir.GetScalarReg(tsharp_reg), ir.GetScalarReg(sampler_reg));

    // Load first address components as denoted in 8.2.4 VGPR Usage Sea Islands Series Instruction
    // Set Architecture
    const IR::Value offset =
        flags.test(MimgModifier::Offset) ? ir.GetVectorReg(addr_reg++) : IR::Value{};
    const IR::F32 bias =
        flags.test(MimgModifier::LodBias) ? ir.GetVectorReg<IR::F32>(addr_reg++) : IR::F32{};
    const IR::F32 dref =
        flags.test(MimgModifier::Pcf) ? ir.GetVectorReg<IR::F32>(addr_reg++) : IR::F32{};

    // Derivatives are tricky because their number depends on the texture type which is located in
    // T#. We don't have access to T# though until resource tracking pass. For now assume no
    // derivatives are present, otherwise we don't know where coordinates are placed in the address
    // stream.
    ASSERT_MSG(!flags.test(MimgModifier::Derivative), "Derivative image instruction");

    // Now we can load body components as noted in Table 8.9 Image Opcodes with Sampler
    // Since these are at most 4 dwords, we load them into a single uvec4 and place them
    // in coords field of the instruction. Then the resource tracking pass will patch the
    // IR instruction to fill in lod_clamp field. The vector can also be used
    // as coords directly as SPIR-V will ignore any extra parameters.
    const IR::Value body =
        ir.CompositeConstruct(ir.GetVectorReg(addr_reg++), ir.GetVectorReg(addr_reg++),
                              ir.GetVectorReg(addr_reg++), ir.GetVectorReg(addr_reg++));

    // Issue IR instruction, leaving unknown fields blank to patch later.
    const IR::Value texel = [&]() -> IR::Value {
        const IR::F32 lod = flags.test(MimgModifier::Level0) ? ir.Imm32(0.f) : IR::F32{};
        const bool explicit_lod = flags.any(MimgModifier::Level0, MimgModifier::Lod);
        if (!flags.test(MimgModifier::Pcf)) {
            if (explicit_lod) {
                return ir.ImageSampleExplicitLod(handle, body, lod, offset, {});
            } else {
                return ir.ImageSampleImplicitLod(handle, body, bias, offset, {}, {});
            }
        }
        if (explicit_lod) {
            return ir.ImageSampleDrefExplicitLod(handle, body, dref, lod, offset, {});
        }
        return ir.ImageSampleDrefImplicitLod(handle, body, dref, bias, offset, {}, {});
    }();

    for (u32 i = 0; i < 4; i++) {
        if (((mimg.dmask >> i) & 1) == 0) {
            continue;
        }
        IR::F32 value;
        if (flags.test(MimgModifier::Pcf)) {
            value = i < 3 ? IR::F32{texel} : ir.Imm32(1.0f);
        } else {
            value = IR::F32{ir.CompositeExtract(texel, i)};
        }
        ir.SetVectorReg(dest_reg++, value);
    }
}

} // namespace Shader::Gcn
