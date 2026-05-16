// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/frontend/translate/translate.h"
#include "shader_recompiler/profile.h"

namespace Shader::Gcn {

using Interpolation = Info::Interpolation;

static Interpolation GetInterpolation(IR::Attribute attribute) {
    switch (attribute) {
    case IR::Attribute::BaryCoordNoPersp:
        return {Qualifier::NoPerspective, Qualifier::None};
    case IR::Attribute::BaryCoordNoPerspCentroid:
        return {Qualifier::NoPerspective, Qualifier::Centroid};
    case IR::Attribute::BaryCoordNoPerspSample:
        return {Qualifier::NoPerspective, Qualifier::Sample};
    case IR::Attribute::BaryCoordSmooth:
        return {Qualifier::Smooth, Qualifier::None};
    case IR::Attribute::BaryCoordSmoothCentroid:
        return {Qualifier::Smooth, Qualifier::Centroid};
    case IR::Attribute::BaryCoordSmoothSample:
        return {Qualifier::Smooth, Qualifier::Sample};
    case IR::Attribute::BaryCoordPullModel:
        return {Qualifier::Smooth, Qualifier::None};
    default:
        UNREACHABLE_MSG("Unhandled barycentric attribute {}", NameOf(attribute));
    }
}

static constexpr std::array DefaultValTable = {
    std::array{0.0f, 0.0f, 0.0f, 0.0f},
    std::array{0.0f, 0.0f, 0.0f, 1.0f},
    std::array{1.0f, 1.0f, 1.0f, 0.0f},
    std::array{1.0f, 1.0f, 1.0f, 1.0f},
};

void Translator::EmitVectorInterpolation(const GcnInst& inst) {
    switch (inst.opcode) {
        // VINTRP
    case Opcode::V_INTERP_P1_F32:
        return V_INTERP_P1_F32(inst);
    case Opcode::V_INTERP_P2_F32:
        return V_INTERP_P2_F32(inst);
    case Opcode::V_INTERP_MOV_F32:
        return V_INTERP_MOV_F32(inst);
    default:
        LogMissingOpcode(inst);
    }
}

// VINTRP

void Translator::V_INTERP_P1_F32(const GcnInst& inst) {
    if (!profile.needs_manual_interpolation) {
        return;
    }
    const u32 attr_index = inst.control.vintrp.attr;
    const auto& attr = runtime_info.fs_info.inputs[attr_index];
    if (attr.IsDefault()) {
        return;
    }
    // VDST = P10 * VSRC + P0
    const IR::Attribute attrib = IR::Attribute::Param0 + attr_index;
    const IR::F32 p0 = ir.GetAttribute(attrib, inst.control.vintrp.chan, 0);
    const IR::F32 p1 = ir.GetAttribute(attrib, inst.control.vintrp.chan, 1);
    const IR::F32 i = GetSrc<IR::F32>(inst.src[0]);
    const IR::F32 result = ir.FPFma(ir.FPSub(p1, p0), i, p0);
    SetDst(inst.dst[0], result);
}

void Translator::V_INTERP_P2_F32(const GcnInst& inst) {
    const u32 attr_index = inst.control.vintrp.attr;
    const IR::Attribute attrib = IR::Attribute::Param0 + attr_index;
    const auto& attr = runtime_info.fs_info.inputs[attr_index];
    auto& interp = info.fs_interpolation[attr_index];
    if (attr.IsDefault()) {
        SetDst(inst.dst[0],
               ir.Imm32(DefaultValTable[attr.default_value][inst.control.vintrp.chan]));
        return;
    }
    ASSERT(!attr.is_flat);
    if (!profile.needs_manual_interpolation) {
        interp = GetInterpolation(vgpr_to_interp[inst.src[0].code]);
        SetDst(inst.dst[0], ir.GetAttribute(attrib, inst.control.vintrp.chan));
        return;
    }
    // VDST = P20 * VSRC + VDST
    const IR::F32 p0 = ir.GetAttribute(attrib, inst.control.vintrp.chan, 0);
    const IR::F32 p2 = ir.GetAttribute(attrib, inst.control.vintrp.chan, 2);
    const IR::F32 j = GetSrc<IR::F32>(inst.src[0]);
    const IR::F32 result = ir.FPFma(ir.FPSub(p2, p0), j, GetSrc<IR::F32>(inst.dst[0]));
    interp.primary = Qualifier::PerVertex;
    SetDst(inst.dst[0], result);
}

void Translator::V_INTERP_MOV_F32(const GcnInst& inst) {
    const u32 attr_index = inst.control.vintrp.attr;
    const IR::Attribute attrib = IR::Attribute::Param0 + attr_index;
    const auto& attr = runtime_info.fs_info.inputs[attr_index];
    auto& interp = info.fs_interpolation[attr_index];
    const u32 src_select = inst.src[0].code & 0x3;
    ASSERT_MSG(src_select != 3, "Invalid V_INTERP_MOV_F32 selector VSRC[1:0]=3");

    if (attr.IsDefault()) {
        SetDst(inst.dst[0],
               ir.Imm32(DefaultValTable[attr.default_value][inst.control.vintrp.chan]));
        return;
    }

    // Flat inputs are constant over the primitive: P0 is that constant and P10/P20 are zero.
    if (attr.is_flat) {
        interp.primary = Qualifier::Flat;
        if (src_select == 2) {
            SetDst(inst.dst[0], ir.GetAttribute(attrib, inst.control.vintrp.chan));
        } else {
            SetDst(inst.dst[0], ir.Imm32(0.0f));
        }
        return;
    }

    const bool supports_per_vertex_params = profile.supports_amd_shader_explicit_vertex_parameter ||
                                            (profile.supports_fragment_shader_barycentric &&
                                             !profile.has_incomplete_fragment_shader_barycentric);
    if (supports_per_vertex_params) {
        // VSRC 0=P10, 1=P20, 2=P0
        interp.primary = Qualifier::PerVertex;
        SetDst(inst.dst[0],
               ir.GetAttribute(attrib, inst.control.vintrp.chan, (src_select + 1) % 3));
        return;
    }

    // Without explicit per-vertex parameter access we can still represent P0 via flat input.
    // P10/P20 cannot be reconstructed exactly here; return a zero slope to keep execution stable.
    interp.primary = Qualifier::Flat;
    if (src_select == 2) {
        SetDst(inst.dst[0], ir.GetAttribute(attrib, inst.control.vintrp.chan));
    } else {
        SetDst(inst.dst[0], ir.Imm32(0.0f));
    }
}

} // namespace Shader::Gcn
