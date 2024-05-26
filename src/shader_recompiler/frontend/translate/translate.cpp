// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/exception.h"
#include "shader_recompiler/frontend/fetch_shader.h"
#include "shader_recompiler/frontend/translate/translate.h"
#include "shader_recompiler/runtime_info.h"
#include "video_core/amdgpu/resource.h"

namespace Shader::Gcn {

Translator::Translator(IR::Block* block_, Info& info_) : block{block_}, ir{*block}, info{info_} {
    IR::VectorReg dst_vreg = IR::VectorReg::V0;
    switch (info.stage) {
    case Stage::Vertex:
        // https://github.com/chaotic-cx/mesa-mirror/blob/72326e15/src/amd/vulkan/radv_shader_args.c#L146C1-L146C23
        ir.SetVectorReg(dst_vreg++, ir.GetAttributeU32(IR::Attribute::VertexId));
        ir.SetVectorReg(dst_vreg++, ir.GetAttributeU32(IR::Attribute::InstanceId));
        ir.SetVectorReg(dst_vreg++, ir.GetAttributeU32(IR::Attribute::PrimitiveId));
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
    default:
        throw NotImplementedException("Unknown shader stage");
    }

    // Initialize user data.
    IR::ScalarReg dst_sreg = IR::ScalarReg::S0;
    for (u32 i = 0; i < 16; i++) {
        ir.SetScalarReg(dst_sreg++, ir.GetUserData(dst_sreg));
    }
}

IR::U32F32 Translator::GetSrc(const InstOperand& operand, bool force_flt) {
    switch (operand.field) {
    case OperandField::ScalarGPR:
        if (operand.type == ScalarType::Float32 || force_flt) {
            return ir.GetScalarReg<IR::F32>(IR::ScalarReg(operand.code));
        } else {
            return ir.GetScalarReg<IR::U32>(IR::ScalarReg(operand.code));
        }
    case OperandField::VectorGPR:
        if (operand.type == ScalarType::Float32 || force_flt) {
            return ir.GetVectorReg<IR::F32>(IR::VectorReg(operand.code));
        } else {
            return ir.GetVectorReg<IR::U32>(IR::VectorReg(operand.code));
        }
    case OperandField::ConstZero:
        if (force_flt) {
            return ir.Imm32(0.f);
        } else {
            return ir.Imm32(0U);
        }
    case OperandField::SignedConstIntPos:
        ASSERT(!force_flt);
        return ir.Imm32(operand.code - SignedConstIntPosMin + 1);
    case OperandField::SignedConstIntNeg:
        ASSERT(!force_flt);
        return ir.Imm32(-s32(operand.code) + SignedConstIntNegMin - 1);
    case OperandField::LiteralConst:
        ASSERT(!force_flt);
        return ir.Imm32(operand.code);
    case OperandField::ConstFloatPos_1_0:
        return ir.Imm32(1.f);
    case OperandField::ConstFloatPos_0_5:
        return ir.Imm32(0.5f);
    case OperandField::ConstFloatNeg_0_5:
        return ir.Imm32(-0.5f);
    default:
        UNREACHABLE();
    }
}

void Translator::SetDst(const InstOperand& operand, const IR::U32F32& value) {
    switch (operand.field) {
    case OperandField::ScalarGPR:
        return ir.SetScalarReg(IR::ScalarReg(operand.code), value);
    case OperandField::VectorGPR:
        return ir.SetVectorReg(IR::VectorReg(operand.code), value);
    case OperandField::VccHi:
    case OperandField::M0:
        break; // Ignore for now
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
    const auto attribs = ParseFetchShader(code);
    for (const auto& attrib : attribs) {
        const IR::Attribute attr{IR::Attribute::Param0 + attrib.semantic};
        IR::VectorReg dst_reg{attrib.dest_vgpr};
        for (u32 i = 0; i < attrib.num_elements; i++) {
            ir.SetVectorReg(dst_reg++, ir.GetAttribute(attr, i));
        }

        // Read the V# of the attribute to figure out component number and type.
        const auto buffer = info.ReadUd<AmdGpu::Buffer>(attrib.sgpr_base, attrib.dword_offset);
        const u32 num_components = AmdGpu::NumComponents(buffer.data_format);
        info.vs_inputs.push_back({
            .fmt = buffer.num_format,
            .binding = attrib.semantic,
            .num_components = std::min<u16>(attrib.num_elements, num_components),
            .sgpr_base = attrib.sgpr_base,
            .dword_offset = attrib.dword_offset,
        });
    }
}

void Translate(IR::Block* block, std::span<const GcnInst> inst_list, Info& info) {
    if (inst_list.empty()) {
        return;
    }
    Translator translator{block, info};
    for (const auto& inst : inst_list) {
        switch (inst.opcode) {
        case Opcode::S_MOV_B32:
            translator.S_MOV(inst);
            break;
        case Opcode::S_MUL_I32:
            translator.S_MUL_I32(inst);
            break;
        case Opcode::V_MOV_B32:
            translator.V_MOV(inst);
            break;
        case Opcode::V_MAC_F32:
            translator.V_MAC_F32(inst);
            break;
        case Opcode::V_MUL_F32:
            translator.V_MUL_F32(inst);
            break;
        case Opcode::S_SWAPPC_B64:
            ASSERT(info.stage == Stage::Vertex);
            translator.EmitFetch(inst);
            break;
        case Opcode::S_WAITCNT:
            break; // Ignore for now.
        case Opcode::S_BUFFER_LOAD_DWORDX16:
            translator.S_BUFFER_LOAD_DWORD(16, inst);
            break;
        case Opcode::EXP:
            translator.EXP(inst);
            break;
        case Opcode::V_INTERP_P2_F32:
            translator.V_INTERP_P2_F32(inst);
            break;
        case Opcode::V_CVT_PKRTZ_F16_F32:
            translator.V_CVT_PKRTZ_F16_F32(inst);
            break;
        case Opcode::IMAGE_SAMPLE:
            translator.IMAGE_SAMPLE(inst);
            break;
        case Opcode::V_CMP_EQ_U32:
            translator.V_CMP_EQ_U32(inst);
            break;
        case Opcode::V_CNDMASK_B32:
            translator.V_CNDMASK_B32(inst);
            break;
        case Opcode::TBUFFER_LOAD_FORMAT_XYZW:
            translator.TBUFFER_LOAD_FORMAT_XYZW(inst);
            break;
        case Opcode::S_MOV_B64:
        case Opcode::S_WQM_B64:
        case Opcode::V_INTERP_P1_F32:
        case Opcode::S_ENDPGM:
            break;
        default:
            UNREACHABLE_MSG("Unknown opcode {}", u32(inst.opcode));
        }
    }
}

} // namespace Shader::Gcn
