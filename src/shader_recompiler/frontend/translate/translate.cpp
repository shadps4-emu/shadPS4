// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/config.h"
#include "common/io_file.h"
#include "common/path_util.h"
#include "shader_recompiler/exception.h"
#include "shader_recompiler/frontend/fetch_shader.h"
#include "shader_recompiler/frontend/translate/translate.h"
#include "shader_recompiler/runtime_info.h"
#include "video_core/amdgpu/resource.h"

#define MAGIC_ENUM_RANGE_MIN 0
#define MAGIC_ENUM_RANGE_MAX 1515
#include "magic_enum.hpp"

namespace Shader::Gcn {

Translator::Translator(IR::Block* block_, Info& info_, const Profile& profile_)
    : ir{*block_, block_->begin()}, info{info_}, profile{profile_} {}

void Translator::EmitPrologue() {
    ir.Prologue();
    ir.SetExec(ir.Imm1(true));

    // Initialize user data.
    IR::ScalarReg dst_sreg = IR::ScalarReg::S0;
    for (u32 i = 0; i < info.num_user_data; i++) {
        ir.SetScalarReg(dst_sreg, ir.GetUserData(dst_sreg));
        ++dst_sreg;
    }

    IR::VectorReg dst_vreg = IR::VectorReg::V0;
    switch (info.stage) {
    case Stage::Vertex:
        // v0: vertex ID, always present
        ir.SetVectorReg(dst_vreg++, ir.GetAttributeU32(IR::Attribute::VertexId));
        // v1: instance ID, step rate 0
        if (info.num_input_vgprs > 0) {
            ir.SetVectorReg(dst_vreg++, ir.GetAttributeU32(IR::Attribute::InstanceId0));
        }
        // v2: instance ID, step rate 1
        if (info.num_input_vgprs > 1) {
            ir.SetVectorReg(dst_vreg++, ir.GetAttributeU32(IR::Attribute::InstanceId1));
        }
        // v3: instance ID, plain
        if (info.num_input_vgprs > 2) {
            ir.SetVectorReg(dst_vreg++, ir.GetAttributeU32(IR::Attribute::InstanceId));
        }
        break;
    case Stage::Fragment:
        // https://github.com/chaotic-cx/mesa-mirror/blob/72326e15/src/amd/vulkan/radv_shader_args.c#L258
        // The first two VGPRs are used for i/j barycentric coordinates. In the vast majority of
        // cases it will be only those two, but if shader is using both e.g linear and perspective
        // inputs it can be more For now assume that this isn't the case.
        dst_vreg = IR::VectorReg::V2;
        for (u32 i = 0; i < 4; i++) {
            ir.SetVectorReg(dst_vreg++, ir.GetAttribute(IR::Attribute::FragCoord, i));
        }
        ir.SetVectorReg(dst_vreg++, ir.GetAttributeU32(IR::Attribute::IsFrontFace));
        break;
    case Stage::Compute:
        ir.SetVectorReg(dst_vreg++, ir.GetAttributeU32(IR::Attribute::LocalInvocationId, 0));
        ir.SetVectorReg(dst_vreg++, ir.GetAttributeU32(IR::Attribute::LocalInvocationId, 1));
        ir.SetVectorReg(dst_vreg++, ir.GetAttributeU32(IR::Attribute::LocalInvocationId, 2));

        ir.SetScalarReg(dst_sreg++, ir.GetAttributeU32(IR::Attribute::WorkgroupId, 0));
        ir.SetScalarReg(dst_sreg++, ir.GetAttributeU32(IR::Attribute::WorkgroupId, 1));
        ir.SetScalarReg(dst_sreg++, ir.GetAttributeU32(IR::Attribute::WorkgroupId, 2));
        break;
    default:
        throw NotImplementedException("Unknown shader stage");
    }
}

template <>
IR::U32F32 Translator::GetSrc(const InstOperand& operand, bool force_flt) {
    IR::U32F32 value{};

    const bool is_float = operand.type == ScalarType::Float32 || force_flt;
    switch (operand.field) {
    case OperandField::ScalarGPR:
        if (is_float) {
            value = ir.GetScalarReg<IR::F32>(IR::ScalarReg(operand.code));
        } else {
            value = ir.GetScalarReg<IR::U32>(IR::ScalarReg(operand.code));
        }
        break;
    case OperandField::VectorGPR:
        if (is_float) {
            value = ir.GetVectorReg<IR::F32>(IR::VectorReg(operand.code));
        } else {
            value = ir.GetVectorReg<IR::U32>(IR::VectorReg(operand.code));
        }
        break;
    case OperandField::ConstZero:
        if (is_float) {
            value = ir.Imm32(0.f);
        } else {
            value = ir.Imm32(0U);
        }
        break;
    case OperandField::SignedConstIntPos:
        ASSERT(!force_flt);
        value = ir.Imm32(operand.code - SignedConstIntPosMin + 1);
        break;
    case OperandField::SignedConstIntNeg:
        ASSERT(!force_flt);
        value = ir.Imm32(-s32(operand.code) + SignedConstIntNegMin - 1);
        break;
    case OperandField::LiteralConst:
        if (is_float) {
            value = ir.Imm32(std::bit_cast<float>(operand.code));
        } else {
            value = ir.Imm32(operand.code);
        }
        break;
    case OperandField::ConstFloatPos_1_0:
        if (is_float) {
            value = ir.Imm32(1.f);
        } else {
            value = ir.Imm32(std::bit_cast<u32>(1.f));
        }
        break;
    case OperandField::ConstFloatPos_0_5:
        value = ir.Imm32(0.5f);
        break;
    case OperandField::ConstFloatPos_2_0:
        value = ir.Imm32(2.0f);
        break;
    case OperandField::ConstFloatPos_4_0:
        value = ir.Imm32(4.0f);
        break;
    case OperandField::ConstFloatNeg_0_5:
        value = ir.Imm32(-0.5f);
        break;
    case OperandField::ConstFloatNeg_1_0:
        if (is_float) {
            value = ir.Imm32(-1.0f);
        } else {
            value = ir.Imm32(std::bit_cast<u32>(-1.0f));
        }
        break;
    case OperandField::ConstFloatNeg_2_0:
        value = ir.Imm32(-2.0f);
        break;
    case OperandField::ConstFloatNeg_4_0:
        value = ir.Imm32(-4.0f);
        break;
    case OperandField::VccLo:
        if (force_flt) {
            value = ir.BitCast<IR::F32>(ir.GetVccLo());
        } else {
            value = ir.GetVccLo();
        }
        break;
    case OperandField::VccHi:
        if (force_flt) {
            value = ir.BitCast<IR::F32>(ir.GetVccHi());
        } else {
            value = ir.GetVccHi();
        }
        break;
    case OperandField::M0:
        return m0_value;
    default:
        UNREACHABLE();
    }

    if (is_float) {
        if (operand.input_modifier.abs) {
            value = ir.FPAbs(value);
        }
        if (operand.input_modifier.neg) {
            value = ir.FPNeg(value);
        }
    }
    return value;
}

template <>
IR::U32 Translator::GetSrc(const InstOperand& operand, bool force_flt) {
    return GetSrc<IR::U32F32>(operand, force_flt);
}

template <>
IR::F32 Translator::GetSrc(const InstOperand& operand, bool) {
    return GetSrc<IR::U32F32>(operand, true);
}

template <>
IR::U64F64 Translator::GetSrc64(const InstOperand& operand, bool force_flt) {
    IR::Value value_hi{};
    IR::Value value_lo{};

    bool immediate = false;
    const bool is_float = operand.type == ScalarType::Float64 || force_flt;
    switch (operand.field) {
    case OperandField::ScalarGPR:
        if (is_float) {
            value_lo = ir.GetScalarReg<IR::F32>(IR::ScalarReg(operand.code));
            value_hi = ir.GetScalarReg<IR::F32>(IR::ScalarReg(operand.code + 1));
        } else if (operand.type == ScalarType::Uint64 || operand.type == ScalarType::Sint64) {
            value_lo = ir.GetScalarReg<IR::U32>(IR::ScalarReg(operand.code));
            value_hi = ir.GetScalarReg<IR::U32>(IR::ScalarReg(operand.code + 1));
        } else {
            UNREACHABLE();
        }
        break;
    case OperandField::VectorGPR:
        if (is_float) {
            value_lo = ir.GetVectorReg<IR::F32>(IR::VectorReg(operand.code));
            value_hi = ir.GetVectorReg<IR::F32>(IR::VectorReg(operand.code + 1));
        } else if (operand.type == ScalarType::Uint64 || operand.type == ScalarType::Sint64) {
            value_lo = ir.GetVectorReg<IR::U32>(IR::VectorReg(operand.code));
            value_hi = ir.GetVectorReg<IR::U32>(IR::VectorReg(operand.code + 1));
        } else {
            UNREACHABLE();
        }
        break;
    case OperandField::ConstZero:
        immediate = true;
        if (force_flt) {
            value_lo = ir.Imm64(0.0);
        } else {
            value_lo = ir.Imm64(u64(0U));
        }
        break;
    case OperandField::SignedConstIntPos:
        ASSERT(!force_flt);
        immediate = true;
        value_lo = ir.Imm64(s64(operand.code) - SignedConstIntPosMin + 1);
        break;
    case OperandField::SignedConstIntNeg:
        ASSERT(!force_flt);
        immediate = true;
        value_lo = ir.Imm64(-s64(operand.code) + SignedConstIntNegMin - 1);
        break;
    case OperandField::LiteralConst:
        immediate = true;
        if (force_flt) {
            UNREACHABLE(); // There is a literal double?
        } else {
            value_lo = ir.Imm64(u64(operand.code));
        }
        break;
    case OperandField::ConstFloatPos_1_0:
        immediate = true;
        if (force_flt) {
            value_lo = ir.Imm64(1.0);
        } else {
            value_lo = ir.Imm64(std::bit_cast<u64>(f64(1.0)));
        }
        break;
    case OperandField::ConstFloatPos_0_5:
        immediate = true;
        value_lo = ir.Imm64(0.5);
        break;
    case OperandField::ConstFloatPos_2_0:
        immediate = true;
        value_lo = ir.Imm64(2.0);
        break;
    case OperandField::ConstFloatPos_4_0:
        immediate = true;
        value_lo = ir.Imm64(4.0);
        break;
    case OperandField::ConstFloatNeg_0_5:
        immediate = true;
        value_lo = ir.Imm64(-0.5);
        break;
    case OperandField::ConstFloatNeg_1_0:
        immediate = true;
        value_lo = ir.Imm64(-1.0);
        break;
    case OperandField::ConstFloatNeg_2_0:
        immediate = true;
        value_lo = ir.Imm64(-2.0);
        break;
    case OperandField::ConstFloatNeg_4_0:
        immediate = true;
        value_lo = ir.Imm64(-4.0);
        break;
    case OperandField::VccLo: {
        value_lo = ir.GetVccLo();
        value_hi = ir.GetVccHi();
    } break;
    case OperandField::VccHi:
        UNREACHABLE();
    default:
        UNREACHABLE();
    }

    IR::Value value;

    if (immediate) {
        value = value_lo;
    } else if (is_float) {
        throw NotImplementedException("required OpPackDouble2x32 implementation");
    } else {
        IR::Value packed = ir.CompositeConstruct(value_lo, value_hi);
        value = ir.PackUint2x32(packed);
    }

    if (is_float) {
        if (operand.input_modifier.abs) {
            value = ir.FPAbs(IR::F32F64(value));
        }
        if (operand.input_modifier.neg) {
            value = ir.FPNeg(IR::F32F64(value));
        }
    }
    return IR::U64F64(value);
}

template <>
IR::U64 Translator::GetSrc64(const InstOperand& operand, bool force_flt) {
    return GetSrc64<IR::U64F64>(operand, force_flt);
}
template <>
IR::F64 Translator::GetSrc64(const InstOperand& operand, bool) {
    return GetSrc64<IR::U64F64>(operand, true);
}

void Translator::SetDst(const InstOperand& operand, const IR::U32F32& value) {
    IR::U32F32 result = value;
    if (operand.output_modifier.multiplier != 0.f) {
        result = ir.FPMul(result, ir.Imm32(operand.output_modifier.multiplier));
    }
    if (operand.output_modifier.clamp) {
        result = ir.FPSaturate(value);
    }
    switch (operand.field) {
    case OperandField::ScalarGPR:
        return ir.SetScalarReg(IR::ScalarReg(operand.code), result);
    case OperandField::VectorGPR:
        return ir.SetVectorReg(IR::VectorReg(operand.code), result);
    case OperandField::VccLo:
        return ir.SetVccLo(result);
    case OperandField::VccHi:
        return ir.SetVccHi(result);
    case OperandField::M0:
        m0_value = result;
        break;
    default:
        UNREACHABLE();
    }
}

void Translator::SetDst64(const InstOperand& operand, const IR::U64F64& value_raw) {
    IR::U64F64 value_untyped = value_raw;

    const bool is_float = value_raw.Type() == IR::Type::F64 || value_raw.Type() == IR::Type::F32;
    if (is_float) {
        if (operand.output_modifier.multiplier != 0.f) {
            value_untyped =
                ir.FPMul(value_untyped, ir.Imm64(f64(operand.output_modifier.multiplier)));
        }
        if (operand.output_modifier.clamp) {
            value_untyped = ir.FPSaturate(value_raw);
        }
    }
    const IR::U64 value =
        is_float ? ir.BitCast<IR::U64>(IR::F64{value_untyped}) : IR::U64{value_untyped};

    const IR::Value unpacked{ir.UnpackUint2x32(value)};
    const IR::U32 lo{ir.CompositeExtract(unpacked, 0U)};
    const IR::U32 hi{ir.CompositeExtract(unpacked, 1U)};
    switch (operand.field) {
    case OperandField::ScalarGPR:
        ir.SetScalarReg(IR::ScalarReg(operand.code + 1), hi);
        return ir.SetScalarReg(IR::ScalarReg(operand.code), lo);
    case OperandField::VectorGPR:
        ir.SetVectorReg(IR::VectorReg(operand.code + 1), hi);
        return ir.SetVectorReg(IR::VectorReg(operand.code), lo);
    case OperandField::VccLo:
        UNREACHABLE();
    case OperandField::VccHi:
        UNREACHABLE();
    case OperandField::M0:
        break;
    default:
        UNREACHABLE();
    }
}

void Translator::EmitFetch(const GcnInst& inst) {
    // Read the pointer to the fetch shader assembly.
    const u32 sgpr_base = inst.src[0].code;
    const u32* code;
    std::memcpy(&code, &info.user_data[sgpr_base], sizeof(code));

    // Parse the assembly to generate a list of attributes.
    u32 fetch_size{};
    const auto attribs = ParseFetchShader(code, &fetch_size);

    if (Config::dumpShaders()) {
        using namespace Common::FS;
        const auto dump_dir = GetUserPath(PathType::ShaderDir) / "dumps";
        if (!std::filesystem::exists(dump_dir)) {
            std::filesystem::create_directories(dump_dir);
        }
        const auto filename = fmt::format("vs_fetch_{:#018x}.bin", info.pgm_hash);
        const auto file = IOFile{dump_dir / filename, FileAccessMode::Write};
        file.WriteRaw<u8>(code, fetch_size);
    }

    for (const auto& attrib : attribs) {
        const IR::Attribute attr{IR::Attribute::Param0 + attrib.semantic};
        IR::VectorReg dst_reg{attrib.dest_vgpr};

        // Read the V# of the attribute to figure out component number and type.
        const auto buffer = info.ReadUd<AmdGpu::Buffer>(attrib.sgpr_base, attrib.dword_offset);
        for (u32 i = 0; i < 4; i++) {
            const IR::F32 comp = [&] {
                switch (buffer.GetSwizzle(i)) {
                case AmdGpu::CompSwizzle::One:
                    return ir.Imm32(1.f);
                case AmdGpu::CompSwizzle::Zero:
                    return ir.Imm32(0.f);
                case AmdGpu::CompSwizzle::Red:
                    return ir.GetAttribute(attr, 0);
                case AmdGpu::CompSwizzle::Green:
                    return ir.GetAttribute(attr, 1);
                case AmdGpu::CompSwizzle::Blue:
                    return ir.GetAttribute(attr, 2);
                case AmdGpu::CompSwizzle::Alpha:
                    return ir.GetAttribute(attr, 3);
                default:
                    UNREACHABLE();
                }
            }();
            ir.SetVectorReg(dst_reg++, comp);
        }

        // In case of programmable step rates we need to fallback to instance data pulling in
        // shader, so VBs should be bound as regular data buffers
        s32 instance_buf_handle = -1;
        const auto step_rate = static_cast<Info::VsInput::InstanceIdType>(attrib.instance_data);
        if (step_rate == Info::VsInput::OverStepRate0 ||
            step_rate == Info::VsInput::OverStepRate1) {
            info.buffers.push_back({
                .sgpr_base = attrib.sgpr_base,
                .dword_offset = attrib.dword_offset,
                .length = buffer.num_records,
                .used_types = IR::Type::F32,
                .is_storage = true, // we may not fit into UBO with large meshes
                .is_instance_data = true,
            });
            instance_buf_handle = s32(info.buffers.size() - 1);
        }

        const u32 num_components = AmdGpu::NumComponents(buffer.GetDataFmt());
        info.vs_inputs.push_back({
            .fmt = buffer.GetNumberFmt(),
            .binding = attrib.semantic,
            .num_components = std::min<u16>(attrib.num_elements, num_components),
            .sgpr_base = attrib.sgpr_base,
            .dword_offset = attrib.dword_offset,
            .instance_step_rate = step_rate,
            .instance_data_buf = instance_buf_handle,
        });
    }
}

void Translator::EmitFlowControl(u32 pc, const GcnInst& inst) {
    switch (inst.opcode) {
    case Opcode::S_BARRIER:
        return S_BARRIER();
    case Opcode::S_TTRACEDATA:
        LOG_WARNING(Render_Vulkan, "S_TTRACEDATA instruction!");
        return;
    case Opcode::S_GETPC_B64:
        return S_GETPC_B64(pc, inst);
    case Opcode::S_WAITCNT:
    case Opcode::S_NOP:
    case Opcode::S_ENDPGM:
    case Opcode::S_CBRANCH_EXECZ:
    case Opcode::S_CBRANCH_SCC0:
    case Opcode::S_CBRANCH_SCC1:
    case Opcode::S_CBRANCH_VCCNZ:
    case Opcode::S_CBRANCH_VCCZ:
    case Opcode::S_BRANCH:
        return;
    default:
        UNREACHABLE();
    }
}

void Translator::LogMissingOpcode(const GcnInst& inst) {
    const u32 opcode = u32(inst.opcode);
    LOG_ERROR(Render_Recompiler, "Unknown opcode {} ({}, category = {})",
              magic_enum::enum_name(inst.opcode), u32(inst.opcode),
              magic_enum::enum_name(inst.category));
    info.translation_failed = true;
}

void Translate(IR::Block* block, u32 pc, std::span<const GcnInst> inst_list, Info& info,
               const Profile& profile) {
    if (inst_list.empty()) {
        return;
    }
    Translator translator{block, info, profile};
    for (const auto& inst : inst_list) {
        pc += inst.length;

        // Special case for emitting fetch shader.
        if (inst.opcode == Opcode::S_SWAPPC_B64) {
            ASSERT(info.stage == Stage::Vertex);
            translator.EmitFetch(inst);
            continue;
        }

        // Emit instructions for each category.
        switch (inst.category) {
        case InstCategory::DataShare:
            translator.EmitDataShare(inst);
            break;
        case InstCategory::VectorInterpolation:
            translator.EmitVectorInterpolation(inst);
            break;
        case InstCategory::ScalarMemory:
            translator.EmitScalarMemory(inst);
            break;
        case InstCategory::VectorMemory:
            translator.EmitVectorMemory(inst);
            break;
        case InstCategory::Export:
            translator.EmitExport(inst);
            break;
        case InstCategory::FlowControl:
            translator.EmitFlowControl(pc, inst);
            break;
        case InstCategory::ScalarALU:
            translator.EmitScalarAlu(inst);
            break;
        case InstCategory::VectorALU:
            translator.EmitVectorAlu(inst);
            break;
        case InstCategory::DebugProfile:
            break;
        default:
            UNREACHABLE();
        }
    }
}

} // namespace Shader::Gcn
