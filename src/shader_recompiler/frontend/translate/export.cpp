// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/frontend/translate/translate.h"
#include "shader_recompiler/ir/position.h"
#include "shader_recompiler/ir/reinterpret.h"
#include "shader_recompiler/profile.h"
#include "shader_recompiler/runtime_info.h"
#include "video_core/amdgpu/regs_color.h"

namespace Shader::Gcn {

static AmdGpu::NumberFormat NumberFormatCompressed(AmdGpu::ShaderExportFormat export_format) {
    switch (export_format) {
    case AmdGpu::ShaderExportFormat::ABGR_FP16:
        return AmdGpu::NumberFormat::Float;
    case AmdGpu::ShaderExportFormat::ABGR_UNORM16:
        return AmdGpu::NumberFormat::Unorm;
    case AmdGpu::ShaderExportFormat::ABGR_SNORM16:
        return AmdGpu::NumberFormat::Snorm;
    case AmdGpu::ShaderExportFormat::ABGR_UINT16:
        return AmdGpu::NumberFormat::Uint;
    case AmdGpu::ShaderExportFormat::ABGR_SINT16:
        return AmdGpu::NumberFormat::Sint;
    default:
        UNREACHABLE_MSG("Unimplemented compressed export format {}",
                        static_cast<u32>(export_format));
    }
}

static u32 MaskFromExportFormat(u8 mask, AmdGpu::ShaderExportFormat export_format) {
    switch (export_format) {
    case AmdGpu::ShaderExportFormat::R_32:
        // Red only
        return mask & 1;
    case AmdGpu::ShaderExportFormat::GR_32:
        // Red and Green only
        return mask & 3;
    case AmdGpu::ShaderExportFormat::AR_32:
        // Red and Alpha only
        return mask & 9;
    case AmdGpu::ShaderExportFormat::ABGR_32:
        // All components
        return mask;
    default:
        UNREACHABLE_MSG("Unimplemented uncompressed export format {}",
                        static_cast<u32>(export_format));
    }
}

IR::U32 Translator::AlphaToMaskLevel(const IR::F32& alpha) {
    const u32 max_level = 8u << runtime_info.hw.fs.alpha_to_mask_num_samples;
    const IR::U32 scaled{
        ir.ConvertFToU(32, ir.FPMul(alpha, ir.Imm32(static_cast<f32>(max_level))))};
    const IR::U1 positive{ir.FPGreaterThan(alpha, ir.Imm32(0.f))};
    const IR::U1 full{ir.FPGreaterThanEqual(alpha, ir.Imm32(1.f))};
    return IR::U32{ir.Select(positive, ir.Select(full, ir.Imm32(max_level), scaled), ir.Imm32(0u))};
}

IR::U32 Translator::AlphaToMaskLevel(const IR::U32& packed, AmdGpu::ShaderExportFormat format) {
    const u32 max_level = 8u << runtime_info.hw.fs.alpha_to_mask_num_samples;
    switch (format) {
    case AmdGpu::ShaderExportFormat::ABGR_FP16:
        return AlphaToMaskLevel(
            IR::F32{ir.CompositeExtract(ir.Unpack2x16(AmdGpu::NumberFormat::Float, packed), 1)});
    case AmdGpu::ShaderExportFormat::ABGR_UNORM16:
    case AmdGpu::ShaderExportFormat::ABGR_UINT16: {
        const IR::U32 alpha{ir.ShiftRightLogical(packed, ir.Imm32(16u))};
        return ir.IDiv(ir.IMul(alpha, ir.Imm32(max_level)), ir.Imm32(65535u));
    }
    case AmdGpu::ShaderExportFormat::ABGR_SNORM16:
    case AmdGpu::ShaderExportFormat::ABGR_SINT16: {
        const IR::U32 alpha{ir.IMax(ir.BitFieldExtract(packed, ir.Imm32(16u), ir.Imm32(16u), true),
                                    ir.Imm32(0u), true)};
        return ir.IDiv(ir.IMul(alpha, ir.Imm32(max_level)), ir.Imm32(32767u));
    }
    default:
        UNREACHABLE_MSG("Unsupported compressed export format {}", static_cast<u32>(format));
    }
}

void Translator::ExportAlphaToMask(const IR::U32& level) {
    const auto& fs = runtime_info.hw.fs;
    const u32 log2_samples = fs.alpha_to_mask_num_samples;
    const IR::U32 x{ir.ConvertFToU(32, ir.GetAttribute(IR::Attribute::FragCoord, 0))};
    const IR::U32 y{ir.ConvertFToU(32, ir.GetAttribute(IR::Attribute::FragCoord, 1))};
    const IR::U32 pixel{
        ir.BitwiseOr(ir.BitwiseAnd(x, ir.Imm32(1u)),
                     ir.ShiftLeftLogical(ir.BitwiseAnd(y, ir.Imm32(1u)), ir.Imm32(1u)))};
    const IR::U32 offset{ir.BitFieldExtract(ir.Imm32(static_cast<u32>(fs.alpha_to_mask_offsets)),
                                            ir.IMul(pixel, ir.Imm32(3u)), ir.Imm32(3u))};
    const auto covered = [&](u32 log2_k) {
        const IR::U32 level_k{ir.ShiftRightLogical(level, ir.Imm32(log2_samples - log2_k))};
        return ir.UMin(ir.ShiftRightLogical(ir.IAdd(level_k, offset), ir.Imm32(3u)),
                       ir.Imm32(1u << log2_k));
    };
    const auto low_bits = [&](const IR::U32& count) {
        return IR::U32{ir.ISub(ir.ShiftLeftLogical(ir.Imm32(1u), count), ir.Imm32(1u))};
    };
    IR::U32 mask;
    if (fs.alpha_to_mask_eqaa_disable) {
        mask = low_bits(covered(log2_samples));
    } else {
        mask = ir.Imm32(0u);
        IR::U32 covered_k = covered(log2_samples);
        for (u32 log2_k = log2_samples; log2_k > 0; --log2_k) {
            const IR::U32 covered_half = covered(log2_k - 1);
            const IR::U32 bits = low_bits(IR::U32{ir.ISub(covered_k, covered_half)});
            mask = ir.BitwiseOr(mask, ir.ShiftLeftLogical(bits, ir.Imm32(1u << (log2_k - 1))));
            covered_k = covered_half;
        }
        mask = ir.BitwiseOr(mask, covered_k);
    }
    const IR::U32 sample_mask{ReplicateSampleMask(mask, log2_samples)};
    ir.SetAttribute(IR::Attribute::SampleMask, ir.BitCast<IR::F32>(sample_mask));
}

void Translator::ExportRenderTarget(const GcnInst& inst) {
    const auto& exp = inst.control.exp;
    const IR::Attribute export_target{exp.target};
    IR::Attribute mrt = export_target;

    // Dual source blending uses MRT1 for exporting src1
    u32 color_buffer_idx = static_cast<u32>(mrt) - static_cast<u32>(IR::Attribute::RenderTarget0);
    if (runtime_info.hw.fs.dual_source_blending && mrt == IR::Attribute::RenderTarget1) {
        color_buffer_idx = 0;
    } else if (const AmdGpu::ColorBufferMask shader_mask{runtime_info.hw.fs.cb_shader_mask};
               shader_mask.raw != 0) {
        color_buffer_idx = shader_mask.ExportTarget(color_buffer_idx);
        if (color_buffer_idx >= AmdGpu::NUM_COLOR_BUFFERS) {
            return;
        }
        mrt = static_cast<IR::Attribute>(static_cast<u32>(IR::Attribute::RenderTarget0) +
                                         color_buffer_idx);
    }
    info.mrt_mask |= 1u << static_cast<u8>(mrt);

    if (export_target == IR::Attribute::RenderTarget0 && runtime_info.hw.fs.alpha_to_mask_enable) {
        IR::U32 level = ir.Imm32(0u);
        if (exp.compr ? (exp.en & 4) : (exp.en & 8)) {
            level =
                exp.compr
                    ? AlphaToMaskLevel(ir.GetVectorReg(IR::VectorReg(inst.src[1].code)),
                                       runtime_info.hw.fs.mrt0_export_format)
                    : AlphaToMaskLevel(ir.GetVectorReg<IR::F32>(IR::VectorReg(inst.src[3].code)));
        }
        ExportAlphaToMask(level);
    }

    const auto color_buffer = runtime_info.hw.fs.color_buffers[color_buffer_idx];
    if (color_buffer.export_format == AmdGpu::ShaderExportFormat::Zero || exp.en == 0) {
        // No export
        return;
    }

    std::array<IR::F32, 4> components{};
    if (exp.compr) {
        // Components are float16 packed into a VGPR
        const auto num_format = NumberFormatCompressed(color_buffer.export_format);
        // Export R, G
        if (exp.en & 1) {
            const IR::Value unpacked_value =
                ir.Unpack2x16(num_format, ir.GetVectorReg(IR::VectorReg(inst.src[0].code)));
            components[0] = IR::F32{ir.CompositeExtract(unpacked_value, 0)};
            components[1] = IR::F32{ir.CompositeExtract(unpacked_value, 1)};
        }
        // Export B, A
        if ((exp.en >> 2) & 1) {
            const IR::Value unpacked_value =
                ir.Unpack2x16(num_format, ir.GetVectorReg(IR::VectorReg(inst.src[1].code)));
            components[2] = IR::F32{ir.CompositeExtract(unpacked_value, 0)};
            components[3] = IR::F32{ir.CompositeExtract(unpacked_value, 1)};
        }
    } else {
        // Components are float32 into separate VGPRS
        u32 mask = MaskFromExportFormat(exp.en, color_buffer.export_format);
        for (u32 i = 0; i < 4; i++, mask >>= 1) {
            if ((mask & 1) == 0) {
                continue;
            }
            components[i] = ir.GetVectorReg<IR::F32>(IR::VectorReg(inst.src[i].code));
        }
    }

    // Metal seems to have an issue where 8-bit unorm/snorm/sRGB outputs to render target
    // need a bias applied to round correctly; detect and set the flag for that here.
    const auto needs_unorm_fixup = profile.needs_unorm_fixup &&
                                   (color_buffer.num_format == AmdGpu::NumberFormat::Unorm ||
                                    color_buffer.num_format == AmdGpu::NumberFormat::Snorm ||
                                    color_buffer.num_format == AmdGpu::NumberFormat::Srgb) &&
                                   (color_buffer.data_format == AmdGpu::DataFormat::Format8 ||
                                    color_buffer.data_format == AmdGpu::DataFormat::Format8_8 ||
                                    color_buffer.data_format == AmdGpu::DataFormat::Format8_8_8_8);

    // Swizzle components and export
    for (u32 i = 0; i < 4; ++i) {
        const auto swizzled_comp = components[color_buffer.swizzle.Map(i)];
        if (swizzled_comp.IsEmpty()) {
            continue;
        }
        auto converted = ApplyWriteNumberConversion(ir, swizzled_comp, color_buffer.num_conversion);
        if (needs_unorm_fixup) {
            // FIXME: Fix-up for GPUs where float-to-unorm rounding is off from expected.
            converted = ir.FPSub(converted, ir.Imm32(1.f / 127500.f));
        }
        ir.SetAttribute(mrt, converted, i);
    }
}

IR::U32 Translator::ReplicateSampleMask(const IR::U32& mask, u32 log2_samples) {
    const u32 log2_msaa_samples = runtime_info.hw.fs.msaa_num_samples;
    if (log2_samples >= log2_msaa_samples) {
        return mask;
    }
    const u32 num_samples = 1u << log2_samples;
    u32 repeat = 0;
    for (u32 sample = 0; sample < (1u << log2_msaa_samples); sample += num_samples) {
        repeat |= 1u << sample;
    }
    return ir.IMul(ir.BitwiseAnd(mask, ir.Imm32((1u << num_samples) - 1)), ir.Imm32(repeat));
}

void Translator::ExportDepth(const GcnInst& inst) {
    const auto& exp = inst.control.exp;
    if (exp.en == 0) {
        // No export
        return;
    }

    std::array<IR::F32, 4> components{};
    if (exp.compr) {
        // Components are float16 packed into a VGPR
        const auto num_format = NumberFormatCompressed(runtime_info.hw.fs.z_export_format);
        // Export R, G
        if (exp.en & 1) {
            const IR::Value unpacked_value =
                ir.Unpack2x16(num_format, ir.GetVectorReg(IR::VectorReg(inst.src[0].code)));
            components[0] = IR::F32{ir.CompositeExtract(unpacked_value, 0)};
            components[1] = IR::F32{ir.CompositeExtract(unpacked_value, 1)};
        }
        // Export B, A
        if ((exp.en >> 2) & 1) {
            const IR::Value unpacked_value =
                ir.Unpack2x16(num_format, ir.GetVectorReg(IR::VectorReg(inst.src[1].code)));
            components[2] = IR::F32{ir.CompositeExtract(unpacked_value, 0)};
            // components[3] = IR::F32{ir.CompositeExtract(unpacked_value, 1)};
        }
    } else {
        // Components are float32 into separate VGPRS
        u32 mask = MaskFromExportFormat(exp.en & runtime_info.hw.fs.mrtz_mask,
                                        runtime_info.hw.fs.z_export_format);
        for (u32 i = 0; i < 4; i++, mask >>= 1) {
            if ((mask & 1) == 0) {
                continue;
            }
            components[i] = ir.GetVectorReg<IR::F32>(IR::VectorReg(inst.src[i].code));
        }
    }

    if (!components[0].IsEmpty()) {
        ir.SetAttribute(IR::Attribute::Depth, components[0]);
    }
    if (!components[1].IsEmpty()) {
        ir.SetAttribute(IR::Attribute::StencilRef, components[1]);
    }
    if (!components[2].IsEmpty()) {
        const IR::U32 mask{ReplicateSampleMask(ir.BitCast<IR::U32>(components[2]),
                                               runtime_info.hw.fs.mask_export_num_samples)};
        ir.SetAttribute(IR::Attribute::SampleMask, ir.BitCast<IR::F32>(mask));
    }
    if (!components[3].IsEmpty() && runtime_info.hw.fs.coverage_to_mask_enable) {
        ExportAlphaToMask(AlphaToMaskLevel(components[3]));
    }
}

void Translator::EmitExport(const GcnInst& inst) {
    if (info.hw_stage == HwStage::Fragment && inst.control.exp.vm) {
        valid_mask_exported = true;
    }

    const IR::Attribute attrib{inst.control.exp.target};
    if (IR::IsMrt(attrib)) {
        return ExportRenderTarget(inst);
    }
    if (attrib == IR::Attribute::Depth) {
        return ExportDepth(inst);
    }

    ASSERT_MSG(!inst.control.exp.compr, "Compressed exports only supported for render targets");

    const bool tess_emulated_primitive =
        info.sw_stage == SwStage::Vertex && runtime_info.sw.vs.tess_emulated_primitive;

    u32 mask = inst.control.exp.en;
    for (u32 i = 0; i < 4; i++, mask >>= 1) {
        if ((mask & 1) == 0) {
            continue;
        }
        const auto value = ir.GetVectorReg<IR::F32>(IR::VectorReg(inst.src[i].code));
        if (IsPosition(attrib)) {
            IR::ExportPosition(ir, runtime_info.hw.vs, tess_emulated_primitive, attrib, i, value);
        } else {
            ir.SetAttribute(attrib, value, i);
        }
    }
}

} // namespace Shader::Gcn
