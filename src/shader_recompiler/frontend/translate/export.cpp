// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/frontend/translate/translate.h"
#include "shader_recompiler/ir/position.h"
#include "shader_recompiler/ir/reinterpret.h"
#include "shader_recompiler/runtime_info.h"

namespace Shader::Gcn {

static AmdGpu::NumberFormat NumberFormatCompressed(
    AmdGpu::Liverpool::ShaderExportFormat export_format) {
    switch (export_format) {
    case AmdGpu::Liverpool::ShaderExportFormat::ABGR_FP16:
        return AmdGpu::NumberFormat::Float;
    case AmdGpu::Liverpool::ShaderExportFormat::ABGR_UNORM16:
        return AmdGpu::NumberFormat::Unorm;
    case AmdGpu::Liverpool::ShaderExportFormat::ABGR_SNORM16:
        return AmdGpu::NumberFormat::Snorm;
    case AmdGpu::Liverpool::ShaderExportFormat::ABGR_UINT16:
        return AmdGpu::NumberFormat::Uint;
    case AmdGpu::Liverpool::ShaderExportFormat::ABGR_SINT16:
        return AmdGpu::NumberFormat::Sint;
    default:
        UNREACHABLE_MSG("Unimplemented compressed MRT export format {}",
                        static_cast<u32>(export_format));
    }
}

static u32 MaskFromExportFormat(u8 mask, AmdGpu::Liverpool::ShaderExportFormat export_format) {
    switch (export_format) {
    case AmdGpu::Liverpool::ShaderExportFormat::R_32:
        // Red only
        return mask & 1;
    case AmdGpu::Liverpool::ShaderExportFormat::GR_32:
        // Red and Green only
        return mask & 3;
    case AmdGpu::Liverpool::ShaderExportFormat::AR_32:
        // Red and Alpha only
        return mask & 9;
    case AmdGpu::Liverpool::ShaderExportFormat::ABGR_32:
        // All components
        return mask;
    default:
        UNREACHABLE_MSG("Unimplemented uncompressed MRT export format {}",
                        static_cast<u32>(export_format));
    }
}

void Translator::ExportRenderTarget(const GcnInst& inst) {
    const auto& exp = inst.control.exp;
    const IR::Attribute mrt{exp.target};
    info.mrt_mask |= 1u << static_cast<u8>(mrt);

    // Dual source blending uses MRT1 for exporting src1
    u32 color_buffer_idx = static_cast<u32>(mrt) - static_cast<u32>(IR::Attribute::RenderTarget0);
    if (runtime_info.fs_info.dual_source_blending && mrt == IR::Attribute::RenderTarget1) {
        color_buffer_idx = 0;
    }

    const auto color_buffer = runtime_info.fs_info.color_buffers[color_buffer_idx];
    if (color_buffer.export_format == AmdGpu::Liverpool::ShaderExportFormat::Zero || exp.en == 0) {
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

    // Swizzle components and export
    for (u32 i = 0; i < 4; ++i) {
        const u32 comp_swizzle = static_cast<u32>(color_buffer.swizzle.array[i]);
        constexpr u32 min_swizzle = static_cast<u32>(AmdGpu::CompSwizzle::Red);
        const auto swizzled_comp =
            components[comp_swizzle >= min_swizzle ? comp_swizzle - min_swizzle : i];
        if (swizzled_comp.IsEmpty()) {
            continue;
        }
        auto converted = ApplyWriteNumberConversion(ir, swizzled_comp, color_buffer.num_conversion);
        if (color_buffer.needs_unorm_fixup) {
            // FIXME: Fix-up for GPUs where float-to-unorm rounding is off from expected.
            converted = ir.FPSub(converted, ir.Imm32(1.f / 127500.f));
        }
        ir.SetAttribute(mrt, converted, i);
    }
}

void Translator::EmitExport(const GcnInst& inst) {
    if (info.stage == Stage::Fragment && inst.control.exp.vm) {
        ir.Discard(ir.LogicalNot(ir.GetExec()));
    }

    const auto& exp = inst.control.exp;
    const IR::Attribute attrib{exp.target};
    if (IR::IsMrt(attrib)) {
        return ExportRenderTarget(inst);
    }

    ASSERT_MSG(!exp.compr, "Compressed exports only supported for render targets");
    if (attrib == IR::Attribute::Depth && exp.en != 0 && exp.en != 1) {
        LOG_WARNING(Render_Vulkan, "Unsupported depth export");
        return;
    }

    u32 mask = exp.en;
    for (u32 i = 0; i < 4; i++, mask >>= 1) {
        if ((mask & 1) == 0) {
            continue;
        }
        const auto value = ir.GetVectorReg<IR::F32>(IR::VectorReg(inst.src[i].code));
        if (IsPosition(attrib)) {
            IR::ExportPosition(ir, runtime_info.vs_info, attrib, i, value);
        } else {
            ir.SetAttribute(attrib, value, i);
        }
    }
}

} // namespace Shader::Gcn
