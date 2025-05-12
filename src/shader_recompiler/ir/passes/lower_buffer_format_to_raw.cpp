// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/info.h"
#include "shader_recompiler/ir/basic_block.h"
#include "shader_recompiler/ir/ir_emitter.h"
#include "shader_recompiler/ir/program.h"
#include "shader_recompiler/ir/reinterpret.h"
#include "video_core/amdgpu/resource.h"

namespace Shader::Optimization {

struct FormatInfo {
    AmdGpu::DataFormat data_format;
    AmdGpu::NumberFormat num_format;
    AmdGpu::CompMapping swizzle;
    AmdGpu::NumberConversion num_conversion;
    int num_components;
};

static bool IsBufferFormatLoad(const IR::Inst& inst) {
    return inst.GetOpcode() == IR::Opcode::LoadBufferFormatF32;
}

static bool IsBufferFormatStore(const IR::Inst& inst) {
    return inst.GetOpcode() == IR::Opcode::StoreBufferFormatF32;
}

static IR::Value LoadBufferFormat(IR::IREmitter& ir, const IR::Value handle, const IR::U32 address,
                                  const IR::BufferInstInfo info, const FormatInfo& format_info) {
    IR::Value interpreted;
    switch (format_info.data_format) {
    case AmdGpu::DataFormat::FormatInvalid:
        interpreted = ir.Imm32(0.f);
        break;
    case AmdGpu::DataFormat::Format8: {
        const auto unpacked =
            ir.Unpack4x8(format_info.num_format, ir.LoadBufferU8(handle, address, info));
        interpreted = ir.CompositeExtract(unpacked, 0);
        break;
    }
    case AmdGpu::DataFormat::Format8_8: {
        const auto raw = ir.LoadBufferU16(handle, address, info);
        const auto unpacked = ir.Unpack4x8(format_info.num_format, raw);
        interpreted = ir.CompositeConstruct(ir.CompositeExtract(unpacked, 0),
                                            ir.CompositeExtract(unpacked, 1));
        break;
    }
    case AmdGpu::DataFormat::Format8_8_8_8:
        interpreted = ir.Unpack4x8(format_info.num_format,
                                   IR::U32{ir.LoadBufferU32(1, handle, address, info)});
        break;
    case AmdGpu::DataFormat::Format16: {
        const auto unpacked =
            ir.Unpack2x16(format_info.num_format, ir.LoadBufferU16(handle, address, info));
        interpreted = ir.CompositeExtract(unpacked, 0);
        break;
    }
    case AmdGpu::DataFormat::Format16_16:
        interpreted = ir.Unpack2x16(format_info.num_format,
                                    IR::U32{ir.LoadBufferU32(1, handle, address, info)});
        break;
    case AmdGpu::DataFormat::Format10_11_11:
        interpreted = ir.Unpack10_11_11(format_info.num_format,
                                        IR::U32{ir.LoadBufferU32(1, handle, address, info)});
        break;
    case AmdGpu::DataFormat::Format2_10_10_10:
        interpreted = ir.Unpack2_10_10_10(format_info.num_format,
                                          IR::U32{ir.LoadBufferU32(1, handle, address, info)});
        break;
    case AmdGpu::DataFormat::Format16_16_16_16: {
        const auto raw = ir.LoadBufferU32(2, handle, address, info);
        interpreted = ir.CompositeConstruct(
            ir.Unpack2x16(format_info.num_format, IR::U32{ir.CompositeExtract(raw, 0)}),
            ir.Unpack2x16(format_info.num_format, IR::U32{ir.CompositeExtract(raw, 1)}));
        break;
    }
    case AmdGpu::DataFormat::Format32:
    case AmdGpu::DataFormat::Format32_32:
    case AmdGpu::DataFormat::Format32_32_32:
    case AmdGpu::DataFormat::Format32_32_32_32: {
        ASSERT(format_info.num_format == AmdGpu::NumberFormat::Uint ||
               format_info.num_format == AmdGpu::NumberFormat::Sint ||
               format_info.num_format == AmdGpu::NumberFormat::Float);
        interpreted = ir.LoadBufferF32(format_info.num_components, handle, address, info);
        break;
    }
    default:
        UNREACHABLE_MSG("Unsupported buffer data format: {}", format_info.data_format);
    }

    // Pad to 4 components and apply additional modifications.
    boost::container::static_vector<IR::Value, 4> components;
    for (u32 i = 0; i < 4; i++) {
        if (i < format_info.num_components) {
            const auto component =
                IR::F32{format_info.num_components == 1 ? interpreted
                                                        : ir.CompositeExtract(interpreted, i)};
            components.push_back(
                ApplyReadNumberConversion(ir, component, format_info.num_conversion));
        } else {
            components.push_back(ir.Imm32(0.f));
        }
    }
    const auto swizzled = ApplySwizzle(ir, ir.CompositeConstruct(components), format_info.swizzle);
    return swizzled;
}

static void StoreBufferFormat(IR::IREmitter& ir, const IR::Value handle, const IR::U32 address,
                              const IR::Value& value, const IR::BufferInstInfo info,
                              const FormatInfo& format_info) {
    // Extract actual number of components and apply additional modifications.
    const auto swizzled = ApplySwizzle(ir, value, format_info.swizzle.Inverse());
    boost::container::static_vector<IR::Value, 4> components;
    for (u32 i = 0; i < format_info.num_components; i++) {
        const auto component = IR::F32{ir.CompositeExtract(swizzled, i)};
        components.push_back(ApplyWriteNumberConversion(ir, component, format_info.num_conversion));
    }
    const auto real_value =
        components.size() == 1 ? components[0] : ir.CompositeConstruct(components);

    switch (format_info.data_format) {
    case AmdGpu::DataFormat::FormatInvalid:
        break;
    case AmdGpu::DataFormat::Format8: {
        const auto packed =
            ir.Pack4x8(format_info.num_format, ir.CompositeConstruct(real_value, ir.Imm32(0.f),
                                                                     ir.Imm32(0.f), ir.Imm32(0.f)));
        ir.StoreBufferU8(handle, address, packed, info);
        break;
    }
    case AmdGpu::DataFormat::Format8_8: {
        const auto packed = ir.Pack4x8(format_info.num_format,
                                       ir.CompositeConstruct(ir.CompositeExtract(real_value, 0),
                                                             ir.CompositeExtract(real_value, 1),
                                                             ir.Imm32(0.f), ir.Imm32(0.f)));
        ir.StoreBufferU16(handle, address, packed, info);
        break;
    }
    case AmdGpu::DataFormat::Format8_8_8_8: {
        auto packed = ir.Pack4x8(format_info.num_format, real_value);
        ir.StoreBufferU32(1, handle, address, packed, info);
        break;
    }
    case AmdGpu::DataFormat::Format16: {
        const auto packed =
            ir.Pack2x16(format_info.num_format, ir.CompositeConstruct(real_value, ir.Imm32(0.f)));
        ir.StoreBufferU16(handle, address, packed, info);
        break;
    }
    case AmdGpu::DataFormat::Format16_16: {
        const auto packed = ir.Pack2x16(format_info.num_format, real_value);
        ir.StoreBufferU32(1, handle, address, packed, info);
        break;
    }
    case AmdGpu::DataFormat::Format10_11_11: {
        const auto packed = ir.Pack10_11_11(format_info.num_format, real_value);
        ir.StoreBufferU32(1, handle, address, packed, info);
        break;
    }
    case AmdGpu::DataFormat::Format2_10_10_10: {
        const auto packed = ir.Pack2_10_10_10(format_info.num_format, real_value);
        ir.StoreBufferU32(1, handle, address, packed, info);
        break;
    }
    case AmdGpu::DataFormat::Format16_16_16_16: {
        const auto packed = ir.CompositeConstruct(
            ir.Pack2x16(format_info.num_format,
                        ir.CompositeConstruct(ir.CompositeExtract(real_value, 0),
                                              ir.CompositeExtract(real_value, 1))),
            ir.Pack2x16(format_info.num_format,
                        ir.CompositeConstruct(ir.CompositeExtract(real_value, 2),
                                              ir.CompositeExtract(real_value, 3))));
        ir.StoreBufferU32(2, handle, address, packed, info);
        break;
    }
    case AmdGpu::DataFormat::Format32:
    case AmdGpu::DataFormat::Format32_32:
    case AmdGpu::DataFormat::Format32_32_32:
    case AmdGpu::DataFormat::Format32_32_32_32: {
        ASSERT(format_info.num_format == AmdGpu::NumberFormat::Uint ||
               format_info.num_format == AmdGpu::NumberFormat::Sint ||
               format_info.num_format == AmdGpu::NumberFormat::Float);
        ir.StoreBufferF32(format_info.num_components, handle, address, real_value, info);
        break;
    }
    default:
        UNREACHABLE_MSG("Unsupported buffer data format: {}", format_info.data_format);
    }
}

static void LowerBufferFormatInst(IR::Block& block, IR::Inst& inst, Info& info) {
    IR::IREmitter ir{block, IR::Block::InstructionList::s_iterator_to(inst)};
    const auto flags = inst.Flags<IR::BufferInstInfo>();
    const auto desc{info.buffers[inst.Arg(0).U32()]};
    const auto buffer{desc.GetSharp(info)};

    const auto is_inst_typed = flags.inst_data_fmt != AmdGpu::DataFormat::FormatInvalid;
    const auto data_format =
        is_inst_typed ? AmdGpu::RemapDataFormat(flags.inst_data_fmt.Value()) : buffer.GetDataFmt();
    const auto format_info = FormatInfo{
        .data_format = data_format,
        .num_format = is_inst_typed
                          ? AmdGpu::RemapNumberFormat(flags.inst_num_fmt.Value(), data_format)
                          : buffer.GetNumberFmt(),
        .swizzle = is_inst_typed
                       ? AmdGpu::RemapSwizzle(flags.inst_data_fmt.Value(), AmdGpu::IdentityMapping)
                       : buffer.DstSelect(),
        .num_conversion = is_inst_typed ? AmdGpu::MapNumberConversion(flags.inst_num_fmt.Value(),
                                                                      flags.inst_data_fmt.Value())
                                        : buffer.GetNumberConversion(),
        .num_components = AmdGpu::NumComponents(data_format),
    };

    if (IsBufferFormatLoad(inst)) {
        const auto interpreted =
            LoadBufferFormat(ir, inst.Arg(0), IR::U32{inst.Arg(1)}, flags, format_info);
        inst.ReplaceUsesWithAndRemove(interpreted);
    } else if (IsBufferFormatStore(inst)) {
        StoreBufferFormat(ir, inst.Arg(0), IR::U32{inst.Arg(1)}, inst.Arg(2), flags, format_info);
        inst.Invalidate();
    }
}

void LowerBufferFormatToRaw(IR::Program& program) {
    auto& info = program.info;
    for (IR::Block* const block : program.blocks) {
        for (IR::Inst& inst : block->Instructions()) {
            if (IsBufferFormatLoad(inst) || IsBufferFormatStore(inst)) {
                LowerBufferFormatInst(*block, inst, info);
            }
        }
    }
}

} // namespace Shader::Optimization
