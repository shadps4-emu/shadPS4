// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/config.h"
#include "common/io_file.h"
#include "common/path_util.h"
#include "shader_recompiler/frontend/decode.h"
#include "shader_recompiler/frontend/fetch_shader.h"
#include "shader_recompiler/frontend/translate/translate.h"
#include "shader_recompiler/info.h"
#include "shader_recompiler/ir/attribute.h"
#include "shader_recompiler/ir/reg.h"
#include "shader_recompiler/ir/reinterpret.h"
#include "shader_recompiler/runtime_info.h"
#include "video_core/amdgpu/resource.h"
#include "video_core/amdgpu/types.h"

#define MAGIC_ENUM_RANGE_MIN 0
#define MAGIC_ENUM_RANGE_MAX 1515
#include <magic_enum/magic_enum.hpp>

namespace Shader::Gcn {

static IR::VectorReg IterateBarycentrics(const RuntimeInfo& runtime_info, auto&& set_attribute) {
    if (runtime_info.stage != Stage::Fragment) {
        return IR::VectorReg::V0;
    }
    u32 dst_vreg{};
    if (runtime_info.fs_info.addr_flags.persp_sample_ena) {
        set_attribute(dst_vreg++, IR::Attribute::BaryCoordSmoothSample, 0); // I
        set_attribute(dst_vreg++, IR::Attribute::BaryCoordSmoothSample, 1); // J
    }
    if (runtime_info.fs_info.addr_flags.persp_center_ena) {
        set_attribute(dst_vreg++, IR::Attribute::BaryCoordSmooth, 0); // I
        set_attribute(dst_vreg++, IR::Attribute::BaryCoordSmooth, 1); // J
    }
    if (runtime_info.fs_info.addr_flags.persp_centroid_ena) {
        set_attribute(dst_vreg++, IR::Attribute::BaryCoordSmoothCentroid, 0); // I
        set_attribute(dst_vreg++, IR::Attribute::BaryCoordSmoothCentroid, 1); // J
    }
    if (runtime_info.fs_info.addr_flags.persp_pull_model_ena) {
        set_attribute(dst_vreg++, IR::Attribute::BaryCoordPullModel, 0); // I/W
        set_attribute(dst_vreg++, IR::Attribute::BaryCoordPullModel, 1); // J/W
        set_attribute(dst_vreg++, IR::Attribute::BaryCoordPullModel, 2); // 1/W
    }
    if (runtime_info.fs_info.addr_flags.linear_sample_ena) {
        set_attribute(dst_vreg++, IR::Attribute::BaryCoordNoPerspSample, 0); // I
        set_attribute(dst_vreg++, IR::Attribute::BaryCoordNoPerspSample, 1); // J
    }
    if (runtime_info.fs_info.addr_flags.linear_center_ena) {
        set_attribute(dst_vreg++, IR::Attribute::BaryCoordNoPersp, 0); // I
        set_attribute(dst_vreg++, IR::Attribute::BaryCoordNoPersp, 1); // J
    }
    if (runtime_info.fs_info.addr_flags.linear_centroid_ena) {
        set_attribute(dst_vreg++, IR::Attribute::BaryCoordNoPerspCentroid, 0); // I
        set_attribute(dst_vreg++, IR::Attribute::BaryCoordNoPerspCentroid, 1); // J
    }
    if (runtime_info.fs_info.addr_flags.line_stipple_tex_ena) {
        ++dst_vreg;
    }
    return IR::VectorReg(dst_vreg);
}

Translator::Translator(Info& info_, const RuntimeInfo& runtime_info_, const Profile& profile_)
    : info{info_}, runtime_info{runtime_info_}, profile{profile_},
      next_vgpr_num{runtime_info.num_allocated_vgprs} {
    IterateBarycentrics(runtime_info, [this](u32 vreg, IR::Attribute attrib, u32) {
        vgpr_to_interp[vreg] = attrib;
    });
}

void Translator::EmitPrologue(IR::Block* first_block) {
    ir = IR::IREmitter(*first_block, first_block->begin());

    ir.Prologue();
    ir.SetExec(ir.Imm1(true));

    // Initialize user data.
    IR::ScalarReg dst_sreg = IR::ScalarReg::S0;
    for (u32 i = 0; i < runtime_info.num_user_data; i++) {
        ir.SetScalarReg(dst_sreg, ir.GetUserData(dst_sreg));
        ++dst_sreg;
    }

    IR::VectorReg dst_vreg = IR::VectorReg::V0;
    switch (info.l_stage) {
    case LogicalStage::Vertex:
        // v0: vertex ID, always present
        ir.SetVectorReg(dst_vreg++, ir.GetAttributeU32(IR::Attribute::VertexId));
        if (info.stage == Stage::Local) {
            // v1: rel patch ID
            if (runtime_info.num_input_vgprs > 0) {
                ir.SetVectorReg(dst_vreg++, ir.Imm32(0));
            }
            // v2: unknown
            if (runtime_info.num_input_vgprs > 1) {
                ++dst_vreg;
            }
            // v3: instance ID, plain
            if (runtime_info.num_input_vgprs > 2) {
                ir.SetVectorReg(dst_vreg++, ir.GetAttributeU32(IR::Attribute::InstanceId));
            }
        } else {
            // v1: instance ID, step rate 0
            if (runtime_info.num_input_vgprs > 0) {
                if (runtime_info.vs_info.step_rate_0 != 0) {
                    ir.SetVectorReg(dst_vreg++,
                                    ir.IDiv(ir.GetAttributeU32(IR::Attribute::InstanceId),
                                            ir.Imm32(runtime_info.vs_info.step_rate_0)));
                } else {
                    ir.SetVectorReg(dst_vreg++, ir.Imm32(0));
                }
            }
            // v2: instance ID, step rate 1
            if (runtime_info.num_input_vgprs > 1) {
                if (runtime_info.vs_info.step_rate_1 != 0) {
                    ir.SetVectorReg(dst_vreg++,
                                    ir.IDiv(ir.GetAttributeU32(IR::Attribute::InstanceId),
                                            ir.Imm32(runtime_info.vs_info.step_rate_1)));
                } else {
                    ir.SetVectorReg(dst_vreg++, ir.Imm32(0));
                }
            }
            // v3: instance ID, plain
            if (runtime_info.num_input_vgprs > 2) {
                ir.SetVectorReg(dst_vreg++, ir.GetAttributeU32(IR::Attribute::InstanceId));
            }
        }
        break;
    case LogicalStage::Fragment:
        dst_vreg =
            IterateBarycentrics(runtime_info, [this](u32 vreg, IR::Attribute attrib, u32 comp) {
                if (profile.supports_amd_shader_explicit_vertex_parameter ||
                    profile.supports_fragment_shader_barycentric) {
                    ir.SetVectorReg(IR::VectorReg(vreg), ir.GetAttribute(attrib, comp));
                }
            });
        if (runtime_info.fs_info.addr_flags.pos_x_float_ena) {
            if (runtime_info.fs_info.en_flags.pos_x_float_ena) {
                ir.SetVectorReg(dst_vreg++, ir.GetAttribute(IR::Attribute::FragCoord, 0));
            } else {
                ir.SetVectorReg(dst_vreg++, ir.Imm32(0.0f));
            }
        }
        if (runtime_info.fs_info.addr_flags.pos_y_float_ena) {
            if (runtime_info.fs_info.en_flags.pos_y_float_ena) {
                ir.SetVectorReg(dst_vreg++, ir.GetAttribute(IR::Attribute::FragCoord, 1));
            } else {
                ir.SetVectorReg(dst_vreg++, ir.Imm32(0.0f));
            }
        }
        if (runtime_info.fs_info.addr_flags.pos_z_float_ena) {
            if (runtime_info.fs_info.en_flags.pos_z_float_ena) {
                ir.SetVectorReg(dst_vreg++, ir.GetAttribute(IR::Attribute::FragCoord, 2));
            } else {
                ir.SetVectorReg(dst_vreg++, ir.Imm32(0.0f));
            }
        }
        if (runtime_info.fs_info.addr_flags.pos_w_float_ena) {
            if (runtime_info.fs_info.en_flags.pos_w_float_ena) {
                ir.SetVectorReg(dst_vreg++,
                                ir.FPRecip(ir.GetAttribute(IR::Attribute::FragCoord, 3)));
            } else {
                ir.SetVectorReg(dst_vreg++, ir.Imm32(0.0f));
            }
        }
        if (runtime_info.fs_info.addr_flags.front_face_ena) {
            if (runtime_info.fs_info.en_flags.front_face_ena) {
                ir.SetVectorReg(dst_vreg++, ir.GetAttributeU32(IR::Attribute::IsFrontFace));
            } else {
                ir.SetVectorReg(dst_vreg++, ir.Imm32(0));
            }
        }
        break;
    case LogicalStage::TessellationControl: {
        ir.SetVectorReg(IR::VectorReg::V0, ir.GetAttributeU32(IR::Attribute::PrimitiveId));
        // Should be laid out like:
        // [0:8]: patch id within VGT
        // [8:12]: output control point id
        ir.SetVectorReg(IR::VectorReg::V1,
                        ir.GetAttributeU32(IR::Attribute::PackedHullInvocationInfo));

        if (runtime_info.hs_info.offchip_lds_enable) {
            // No off-chip tessellation has been observed yet. If this survives dead code elim,
            // revisit
            ir.SetScalarReg(dst_sreg++, ir.GetAttributeU32(IR::Attribute::OffChipLdsBase));
        }
        ir.SetScalarReg(dst_sreg++, ir.GetAttributeU32(IR::Attribute::TessFactorsBufferBase));

        break;
    }
    case LogicalStage::TessellationEval:
        ir.SetVectorReg(IR::VectorReg::V0,
                        ir.GetAttribute(IR::Attribute::TessellationEvaluationPointU));
        ir.SetVectorReg(IR::VectorReg::V1,
                        ir.GetAttribute(IR::Attribute::TessellationEvaluationPointV));
        // V2 is similar to PrimitiveID but not the same. It seems to only be used in
        // compiler-generated address calculations. Its probably the patch id within the
        // patches running locally on a given VGT (or CU, whichever is the granularity of LDS
        // memory)
        // Set to 0. See explanation in comment describing hull/domain passes
        ir.SetVectorReg(IR::VectorReg::V2, ir.Imm32(0u));
        // V3 is the actual PrimitiveID as intended by the shader author.
        ir.SetVectorReg(IR::VectorReg::V3, ir.GetAttributeU32(IR::Attribute::PrimitiveId));
        break;
    case LogicalStage::Compute:
        ir.SetVectorReg(dst_vreg++, ir.GetAttributeU32(IR::Attribute::LocalInvocationId, 0));
        ir.SetVectorReg(dst_vreg++, ir.GetAttributeU32(IR::Attribute::LocalInvocationId, 1));
        ir.SetVectorReg(dst_vreg++, ir.GetAttributeU32(IR::Attribute::LocalInvocationId, 2));

        if (runtime_info.cs_info.tgid_enable[0]) {
            ir.SetScalarReg(dst_sreg++, ir.GetAttributeU32(IR::Attribute::WorkgroupId, 0));
        }
        if (runtime_info.cs_info.tgid_enable[1]) {
            ir.SetScalarReg(dst_sreg++, ir.GetAttributeU32(IR::Attribute::WorkgroupId, 1));
        }
        if (runtime_info.cs_info.tgid_enable[2]) {
            ir.SetScalarReg(dst_sreg++, ir.GetAttributeU32(IR::Attribute::WorkgroupId, 2));
        }
        break;
    case LogicalStage::Geometry:
        switch (runtime_info.gs_info.out_primitive[0]) {
        case AmdGpu::GsOutputPrimitiveType::TriangleStrip:
            ir.SetVectorReg(IR::VectorReg::V3, ir.Imm32(2u)); // vertex 2
        case AmdGpu::GsOutputPrimitiveType::LineStrip:
            ir.SetVectorReg(IR::VectorReg::V1, ir.Imm32(1u)); // vertex 1
        default:
            ir.SetVectorReg(IR::VectorReg::V0, ir.Imm32(0u)); // vertex 0
            break;
        }
        ir.SetVectorReg(IR::VectorReg::V2, ir.GetAttributeU32(IR::Attribute::PrimitiveId));
        break;
    default:
        UNREACHABLE_MSG("Unknown shader stage");
    }

    // Clear any scratch vgpr mappings for next shader.
    vgpr_map.clear();
}

IR::VectorReg Translator::GetScratchVgpr(u32 offset) {
    const auto [it, is_new] = vgpr_map.try_emplace(offset);
    if (is_new) {
        ASSERT_MSG(next_vgpr_num < 256, "Out of VGPRs");
        const auto new_vgpr = static_cast<IR::VectorReg>(next_vgpr_num++);
        it->second = new_vgpr;
    }
    return it->second;
};

template <typename T>
T Translator::GetSrc(const InstOperand& operand) {
    constexpr bool is_float = std::is_same_v<T, IR::F32>;

    const auto get_imm = [&](auto value) -> T {
        if constexpr (is_float) {
            return ir.Imm32(std::bit_cast<float>(value));
        } else {
            return ir.Imm32(std::bit_cast<u32>(value));
        }
    };

    T value{};
    switch (operand.field) {
    case OperandField::ScalarGPR:
        value = ir.GetScalarReg<T>(IR::ScalarReg(operand.code));
        break;
    case OperandField::VectorGPR:
        value = ir.GetVectorReg<T>(IR::VectorReg(operand.code));
        break;
    case OperandField::ConstZero:
        value = get_imm(0U);
        break;
    case OperandField::SignedConstIntPos:
        value = get_imm(operand.code - SignedConstIntPosMin + 1);
        break;
    case OperandField::SignedConstIntNeg:
        value = get_imm(-s32(operand.code) + SignedConstIntNegMin - 1);
        break;
    case OperandField::LiteralConst:
        value = get_imm(operand.code);
        break;
    case OperandField::ConstFloatPos_1_0:
        value = get_imm(1.f);
        break;
    case OperandField::ConstFloatPos_0_5:
        value = get_imm(0.5f);
        break;
    case OperandField::ConstFloatPos_2_0:
        value = get_imm(2.0f);
        break;
    case OperandField::ConstFloatPos_4_0:
        value = get_imm(4.0f);
        break;
    case OperandField::ConstFloatNeg_0_5:
        value = get_imm(-0.5f);
        break;
    case OperandField::ConstFloatNeg_1_0:
        value = get_imm(-1.0f);
        break;
    case OperandField::ConstFloatNeg_2_0:
        value = get_imm(-2.0f);
        break;
    case OperandField::ConstFloatNeg_4_0:
        value = get_imm(-4.0f);
        break;
    case OperandField::VccLo:
        if constexpr (is_float) {
            value = ir.BitCast<IR::F32>(ir.GetVccLo());
        } else {
            value = ir.GetVccLo();
        }
        break;
    case OperandField::VccHi:
        if constexpr (is_float) {
            value = ir.BitCast<IR::F32>(ir.GetVccHi());
        } else {
            value = ir.GetVccHi();
        }
        break;
    case OperandField::M0:
        if constexpr (is_float) {
            value = ir.BitCast<IR::F32>(ir.GetM0());
        } else {
            value = ir.GetM0();
        }
        break;
    case OperandField::Scc:
        if constexpr (is_float) {
            UNREACHABLE();
        } else {
            value = ir.BitCast<IR::U32>(ir.GetScc());
        }
        break;
    default:
        UNREACHABLE();
    }

    if constexpr (is_float) {
        if (operand.input_modifier.abs) {
            value = ir.FPAbs(value);
        }
        if (operand.input_modifier.neg) {
            value = ir.FPNeg(value);
        }
    } else {
        if (operand.input_modifier.abs) {
            value = ir.IAbs(value);
        }
        if (operand.input_modifier.neg) {
            value = ir.INeg(value);
        }
    }
    return value;
}

template IR::U32 Translator::GetSrc<IR::U32>(const InstOperand&);
template IR::F32 Translator::GetSrc<IR::F32>(const InstOperand&);

template <typename T>
T Translator::GetSrc64(const InstOperand& operand) {
    constexpr bool is_float = std::is_same_v<T, IR::F64>;

    const auto get_imm = [&](auto value) -> T {
        if constexpr (is_float) {
            return ir.Imm64(std::bit_cast<double>(value));
        } else {
            return ir.Imm64(std::bit_cast<u64>(value));
        }
    };

    T value{};
    switch (operand.field) {
    case OperandField::ScalarGPR: {
        const auto value_lo = ir.GetScalarReg(IR::ScalarReg(operand.code));
        const auto value_hi = ir.GetScalarReg(IR::ScalarReg(operand.code + 1));
        if constexpr (is_float) {
            value = ir.PackDouble2x32(ir.CompositeConstruct(value_lo, value_hi));
        } else {
            value = ir.PackUint2x32(ir.CompositeConstruct(value_lo, value_hi));
        }
        break;
    }
    case OperandField::VectorGPR: {
        const auto value_lo = ir.GetVectorReg(IR::VectorReg(operand.code));
        const auto value_hi = ir.GetVectorReg(IR::VectorReg(operand.code + 1));
        if constexpr (is_float) {
            value = ir.PackDouble2x32(ir.CompositeConstruct(value_lo, value_hi));
        } else {
            value = ir.PackUint2x32(ir.CompositeConstruct(value_lo, value_hi));
        }
        break;
    }
    case OperandField::ConstZero:
        value = get_imm(0ULL);
        break;
    case OperandField::SignedConstIntPos:
        value = get_imm(s64(operand.code) - SignedConstIntPosMin + 1);
        break;
    case OperandField::SignedConstIntNeg:
        value = get_imm(-s64(operand.code) + SignedConstIntNegMin - 1);
        break;
    case OperandField::LiteralConst:
        value = get_imm(u64(operand.code));
        break;
    case OperandField::ConstFloatPos_1_0:
        value = get_imm(1.0);
        break;
    case OperandField::ConstFloatPos_0_5:
        value = get_imm(0.5);
        break;
    case OperandField::ConstFloatPos_2_0:
        value = get_imm(2.0);
        break;
    case OperandField::ConstFloatPos_4_0:
        value = get_imm(4.0);
        break;
    case OperandField::ConstFloatNeg_0_5:
        value = get_imm(-0.5);
        break;
    case OperandField::ConstFloatNeg_1_0:
        value = get_imm(-1.0);
        break;
    case OperandField::ConstFloatNeg_2_0:
        value = get_imm(-2.0);
        break;
    case OperandField::ConstFloatNeg_4_0:
        value = get_imm(-4.0);
        break;
    case OperandField::VccLo:
        if constexpr (is_float) {
            value = ir.PackDouble2x32(ir.CompositeConstruct(ir.GetVccLo(), ir.GetVccHi()));
        } else {
            value = ir.PackUint2x32(ir.CompositeConstruct(ir.GetVccLo(), ir.GetVccHi()));
        }
        break;
    case OperandField::VccHi:
    default:
        UNREACHABLE();
    }

    if constexpr (is_float) {
        if (operand.input_modifier.abs) {
            value = ir.FPAbs(value);
        }
        if (operand.input_modifier.neg) {
            value = ir.FPNeg(value);
        }
    }
    return value;
}

template IR::U64 Translator::GetSrc64<IR::U64>(const InstOperand&);
template IR::F64 Translator::GetSrc64<IR::F64>(const InstOperand&);

void Translator::SetDst(const InstOperand& operand, const IR::U32F32& value) {
    IR::U32F32 result = value;
    if (value.Type() == IR::Type::F32) {
        if (operand.output_modifier.multiplier != 0.f) {
            result = ir.FPMul(result, ir.Imm32(operand.output_modifier.multiplier));
        }
        if (operand.output_modifier.clamp) {
            result = ir.FPSaturate(value);
        }
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
        return ir.SetM0(result);
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

    const IR::Value unpacked{is_float ? ir.UnpackDouble2x32(IR::F64{value_untyped})
                                      : ir.UnpackUint2x32(IR::U64{value_untyped})};
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
        ir.SetVccLo(lo);
        return ir.SetVccHi(hi);
    case OperandField::VccHi:
        UNREACHABLE();
    case OperandField::M0:
        break;
    default:
        UNREACHABLE();
    }
}

void Translator::EmitFetch(const GcnInst& inst) {
    const auto code_sgpr_base = inst.src[0].code;

    // The fetch shader must be inlined to access as regular buffers, so that
    // bounds checks can be emitted to emulate robust buffer access.
    if (!profile.supports_robust_buffer_access) {
        const auto* code = GetFetchShaderCode(info, code_sgpr_base);
        GcnCodeSlice slice(code, code + std::numeric_limits<u32>::max());
        GcnDecodeContext decoder;

        // Decode and save instructions
        while (!slice.atEnd()) {
            const auto sub_inst = decoder.decodeInstruction(slice);
            if (sub_inst.opcode == Opcode::S_SETPC_B64) {
                // Assume we're swapping back to the main shader.
                break;
            }
            TranslateInstruction(sub_inst);
        }
        return;
    }

    info.has_fetch_shader = true;
    info.fetch_shader_sgpr_base = code_sgpr_base;

    const auto fetch_data = ParseFetchShader(info);
    ASSERT(fetch_data.has_value());

    if (Config::dumpShaders()) {
        using namespace Common::FS;
        const auto dump_dir = GetUserPath(PathType::ShaderDir) / "dumps";
        if (!std::filesystem::exists(dump_dir)) {
            std::filesystem::create_directories(dump_dir);
        }
        const auto filename = fmt::format("vs_{:#018x}.fetch.bin", info.pgm_hash);
        const auto file = IOFile{dump_dir / filename, FileAccessMode::Write};
        file.WriteRaw<u8>(fetch_data->code, fetch_data->size);
    }

    for (const auto& attrib : fetch_data->attributes) {
        const IR::Attribute attr{IR::Attribute::Param0 + attrib.semantic};
        IR::VectorReg dst_reg{attrib.dest_vgpr};

        // Read the V# of the attribute to figure out component number and type.
        const auto buffer = attrib.GetSharp(info);
        const auto values =
            ir.CompositeConstruct(ir.GetAttribute(attr, 0), ir.GetAttribute(attr, 1),
                                  ir.GetAttribute(attr, 2), ir.GetAttribute(attr, 3));
        const auto converted =
            IR::ApplyReadNumberConversionVec4(ir, values, buffer.GetNumberConversion());
        const auto swizzled = ApplySwizzle(ir, converted, buffer.DstSelect());
        for (u32 i = 0; i < 4; i++) {
            ir.SetVectorReg(dst_reg++, IR::F32{ir.CompositeExtract(swizzled, i)});
        }
    }
}

void Translator::LogMissingOpcode(const GcnInst& inst) {
    LOG_ERROR(Render_Recompiler, "Unknown opcode {} ({}, category = {})",
              magic_enum::enum_name(inst.opcode), u32(inst.opcode),
              magic_enum::enum_name(inst.category));
    info.translation_failed = true;
}

void Translator::Translate(IR::Block* block, u32 start_pc, std::span<const GcnInst> inst_list) {
    if (inst_list.empty()) {
        return;
    }
    ir = IR::IREmitter{*block, block->begin()};
    pc = start_pc;
    for (const auto& inst : inst_list) {
        pc += inst.length;

        // Special case for emitting fetch shader.
        if (inst.opcode == Opcode::S_SWAPPC_B64) {
            ASSERT(info.stage == Stage::Vertex || info.stage == Stage::Export ||
                   info.stage == Stage::Local);
            EmitFetch(inst);
            continue;
        }

        TranslateInstruction(inst);
    }
}

void Translator::TranslateInstruction(const GcnInst& inst) {
    // Emit instructions for each category.
    switch (inst.category) {
    case InstCategory::DataShare:
        EmitDataShare(inst);
        break;
    case InstCategory::VectorInterpolation:
        EmitVectorInterpolation(inst);
        break;
    case InstCategory::ScalarMemory:
        EmitScalarMemory(inst);
        break;
    case InstCategory::VectorMemory:
        EmitVectorMemory(inst);
        break;
    case InstCategory::Export:
        EmitExport(inst);
        break;
    case InstCategory::FlowControl:
        EmitFlowControl(inst);
        break;
    case InstCategory::ScalarALU:
        EmitScalarAlu(inst);
        break;
    case InstCategory::VectorALU:
        EmitVectorAlu(inst);
        break;
    case InstCategory::DebugProfile:
        break;
    default:
        UNREACHABLE();
    }
}

} // namespace Shader::Gcn
