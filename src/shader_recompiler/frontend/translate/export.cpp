// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/frontend/translate/translate.h"
#include "shader_recompiler/ir/reinterpret.h"
#include "shader_recompiler/runtime_info.h"

namespace Shader::Gcn {

u32 SwizzleMrtComponent(const PsColorBuffer& color_buffer, u32 comp) {
    const auto [r, g, b, a] = color_buffer.swizzle;
    const std::array swizzle_array = {r, g, b, a};
    const auto swizzled_comp_type = static_cast<u32>(swizzle_array[comp]);
    constexpr auto min_comp_type = static_cast<u32>(AmdGpu::CompSwizzle::Red);
    return swizzled_comp_type >= min_comp_type ? swizzled_comp_type - min_comp_type : comp;
}

void Translator::ExportMrtValue(IR::Attribute attribute, u32 comp, const IR::F32& value,
                                const PsColorBuffer& color_buffer) {
    auto converted = ApplyWriteNumberConversion(ir, value, color_buffer.num_conversion);
    if (color_buffer.needs_unorm_fixup) {
        // FIXME: Fix-up for GPUs where float-to-unorm rounding is off from expected.
        converted = ir.FPSub(converted, ir.Imm32(1.f / 127500.f));
    }
    ir.SetAttribute(attribute, converted, comp);
}

void Translator::ExportMrtCompressed(IR::Attribute attribute, u32 idx, const IR::U32& value) {
    u32 color_buffer_idx =
        static_cast<u32>(attribute) - static_cast<u32>(IR::Attribute::RenderTarget0);
    if (runtime_info.fs_info.dual_source_blending && attribute == IR::Attribute::RenderTarget1) {
        color_buffer_idx = 0;
    }
    const auto color_buffer = runtime_info.fs_info.color_buffers[color_buffer_idx];

    AmdGpu::NumberFormat num_format;
    switch (color_buffer.export_format) {
    case AmdGpu::Liverpool::ShaderExportFormat::Zero:
        // No export
        return;
    case AmdGpu::Liverpool::ShaderExportFormat::ABGR_FP16:
        num_format = AmdGpu::NumberFormat::Float;
        break;
    case AmdGpu::Liverpool::ShaderExportFormat::ABGR_UNORM16:
        num_format = AmdGpu::NumberFormat::Unorm;
        break;
    case AmdGpu::Liverpool::ShaderExportFormat::ABGR_SNORM16:
        num_format = AmdGpu::NumberFormat::Snorm;
        break;
    case AmdGpu::Liverpool::ShaderExportFormat::ABGR_UINT16:
        num_format = AmdGpu::NumberFormat::Uint;
        break;
    case AmdGpu::Liverpool::ShaderExportFormat::ABGR_SINT16:
        num_format = AmdGpu::NumberFormat::Sint;
        break;
    default:
        UNREACHABLE_MSG("Unimplemented compressed MRT export format {}",
                        static_cast<u32>(color_buffer.export_format));
        break;
    }

    const auto unpacked_value = ir.Unpack2x16(num_format, value);
    const IR::F32 r = IR::F32{ir.CompositeExtract(unpacked_value, 0)};
    const IR::F32 g = IR::F32{ir.CompositeExtract(unpacked_value, 1)};

    const auto swizzled_r = SwizzleMrtComponent(color_buffer, idx * 2);
    const auto swizzled_g = SwizzleMrtComponent(color_buffer, idx * 2 + 1);

    ExportMrtValue(attribute, swizzled_r, r, color_buffer);
    ExportMrtValue(attribute, swizzled_g, g, color_buffer);
}

void Translator::ExportMrtUncompressed(IR::Attribute attribute, u32 comp, const IR::F32& value) {
    u32 color_buffer_idx =
        static_cast<u32>(attribute) - static_cast<u32>(IR::Attribute::RenderTarget0);
    if (runtime_info.fs_info.dual_source_blending && attribute == IR::Attribute::RenderTarget1) {
        color_buffer_idx = 0;
    }
    const auto color_buffer = runtime_info.fs_info.color_buffers[color_buffer_idx];
    const auto swizzled_comp = SwizzleMrtComponent(color_buffer, comp);

    switch (color_buffer.export_format) {
    case AmdGpu::Liverpool::ShaderExportFormat::Zero:
        // No export
        return;
    case AmdGpu::Liverpool::ShaderExportFormat::R_32:
        // Red only
        if (swizzled_comp != 0) {
            return;
        }
        break;
    case AmdGpu::Liverpool::ShaderExportFormat::GR_32:
        // Red and Green only
        if (swizzled_comp != 0 && swizzled_comp != 1) {
            return;
        }
        break;
    case AmdGpu::Liverpool::ShaderExportFormat::AR_32:
        // Red and Alpha only
        if (swizzled_comp != 0 && swizzled_comp != 3) {
            return;
        }
        break;
    case AmdGpu::Liverpool::ShaderExportFormat::ABGR_32:
        // All components
        break;
    default:
        UNREACHABLE_MSG("Unimplemented uncompressed MRT export format {}",
                        static_cast<u32>(color_buffer.export_format));
        break;
    }
    ExportMrtValue(attribute, swizzled_comp, value, color_buffer);
}

void Translator::ExportCompressed(IR::Attribute attribute, u32 idx, const IR::U32& value) {
    if (IsMrt(attribute)) {
        ExportMrtCompressed(attribute, idx, value);
        return;
    }
    const IR::Value unpacked_value = ir.Unpack2x16(AmdGpu::NumberFormat::Float, value);
    const IR::F32 r = IR::F32{ir.CompositeExtract(unpacked_value, 0)};
    const IR::F32 g = IR::F32{ir.CompositeExtract(unpacked_value, 1)};
    ir.SetAttribute(attribute, r, idx * 2);
    ir.SetAttribute(attribute, g, idx * 2 + 1);
}

void Translator::ExportUncompressed(IR::Attribute attribute, u32 comp, const IR::F32& value) {
    if (IsMrt(attribute)) {
        ExportMrtUncompressed(attribute, comp, value);
        return;
    }
    ir.SetAttribute(attribute, value, comp);
}

void Translator::EmitExport(const GcnInst& inst) {
    if (info.stage == Stage::Fragment && inst.control.exp.vm) {
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

    // Components are float16 packed into a VGPR
    if (exp.compr) {
        // Export R, G
        if (exp.en & 1) {
            ExportCompressed(attrib, 0, ir.GetVectorReg<IR::U32>(vsrc[0]));
        }
        // Export B, A
        if ((exp.en >> 2) & 1) {
            ExportCompressed(attrib, 1, ir.GetVectorReg<IR::U32>(vsrc[1]));
        }
    } else {
        // Components are float32 into separate VGPRS
        u32 mask = exp.en;
        for (u32 i = 0; i < 4; i++, mask >>= 1) {
            if ((mask & 1) == 0) {
                continue;
            }
            ExportUncompressed(attrib, i, ir.GetVectorReg<IR::F32>(vsrc[i]));
        }
    }
    if (IR::IsMrt(attrib)) {
        info.mrt_mask |= 1u << u8(attrib);
    }
}

} // namespace Shader::Gcn
