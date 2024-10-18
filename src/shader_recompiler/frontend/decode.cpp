// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include "common/assert.h"
#include "shader_recompiler/frontend/decode.h"

#include "magic_enum.hpp"

namespace Shader::Gcn {

namespace bit {
template <typename T>
T extract(T value, u32 lst, u32 fst) {
    return (value >> fst) & ~(~T(0) << (lst - fst + 1));
}
} // namespace bit

InstEncoding GetInstructionEncoding(u32 token) {
    auto encoding = static_cast<InstEncoding>(token & (u32)EncodingMask::MASK_9bit);
    switch (encoding) {
    case InstEncoding::SOP1:
    case InstEncoding::SOPP:
    case InstEncoding::SOPC:
        return encoding;
    default:
        break;
    }

    encoding = static_cast<InstEncoding>(token & (u32)EncodingMask::MASK_7bit);
    switch (encoding) {
    case InstEncoding::VOP1:
    case InstEncoding::VOPC:
        return encoding;
    default:
        break;
    }

    encoding = static_cast<InstEncoding>(token & (u32)EncodingMask::MASK_6bit);
    switch (encoding) {
    case InstEncoding::VOP3:
    case InstEncoding::EXP:
    case InstEncoding::VINTRP:
    case InstEncoding::DS:
    case InstEncoding::MUBUF:
    case InstEncoding::MTBUF:
    case InstEncoding::MIMG:
        return encoding;
    default:
        break;
    }

    encoding = static_cast<InstEncoding>(token & (u32)EncodingMask::MASK_5bit);
    switch (encoding) {
    case InstEncoding::SMRD:
        return encoding;
    default:
        break;
    }

    encoding = static_cast<InstEncoding>(token & (u32)EncodingMask::MASK_4bit);
    switch (encoding) {
    case InstEncoding::SOPK:
        return encoding;
    default:
        break;
    }

    encoding = static_cast<InstEncoding>(token & (u32)EncodingMask::MASK_2bit);
    switch (encoding) {
    case InstEncoding::SOP2:
        return encoding;
    default:
        break;
    }

    encoding = static_cast<InstEncoding>(token & (u32)EncodingMask::MASK_1bit);
    switch (encoding) {
    case InstEncoding::VOP2:
        return encoding;
    default:
        break;
    }

    UNREACHABLE();
    return InstEncoding::ILLEGAL;
}

bool HasAdditionalLiteral(InstEncoding encoding, Opcode opcode) {
    switch (encoding) {
    case InstEncoding::SOPK: {
        return opcode == Opcode::S_SETREG_IMM32_B32;
    }
    case InstEncoding::VOP2: {
        return opcode == Opcode::V_MADMK_F32 || opcode == Opcode::V_MADAK_F32;
    }
    default:
        return false;
    }
}

bool IsVop3BEncoding(Opcode opcode) {
    return opcode == Opcode::V_ADD_I32 || opcode == Opcode::V_ADDC_U32 ||
           opcode == Opcode::V_SUB_I32 || opcode == Opcode::V_SUBB_U32 ||
           opcode == Opcode::V_SUBREV_I32 || opcode == Opcode::V_SUBBREV_U32 ||
           opcode == Opcode::V_DIV_SCALE_F32 || opcode == Opcode::V_DIV_SCALE_F64 ||
           opcode == Opcode::V_MAD_U64_U32 || opcode == Opcode::V_MAD_I64_I32;
}

GcnInst GcnDecodeContext::decodeInstruction(GcnCodeSlice& code) {
    const uint32_t token = code.at(0);

    InstEncoding encoding = GetInstructionEncoding(token);
    ASSERT_MSG(encoding != InstEncoding::ILLEGAL, "illegal encoding");
    uint32_t encodingLen = getEncodingLength(encoding);

    // Clear the instruction
    m_instruction = GcnInst();

    // Decode
    if (encodingLen == sizeof(uint32_t)) {
        decodeInstruction32(encoding, code);
    } else {
        decodeInstruction64(encoding, code);
    }

    // Update instruction meta info.
    updateInstructionMeta(encoding);

    // Detect literal constant. Only 32 bits instructions may have literal constant.
    // Note: Literal constant decode must be performed after meta info updated.
    if (encodingLen == sizeof(u32)) {
        decodeLiteralConstant(encoding, code);
    }

    repairOperandType();
    return m_instruction;
}

uint32_t GcnDecodeContext::getEncodingLength(InstEncoding encoding) {
    uint32_t instLength = 0;

    switch (encoding) {
    case InstEncoding::SOP1:
    case InstEncoding::SOPP:
    case InstEncoding::SOPC:
    case InstEncoding::SOPK:
    case InstEncoding::SOP2:
    case InstEncoding::VOP1:
    case InstEncoding::VOPC:
    case InstEncoding::VOP2:
    case InstEncoding::SMRD:
    case InstEncoding::VINTRP:
        instLength = sizeof(uint32_t);
        break;

    case InstEncoding::VOP3:
    case InstEncoding::MUBUF:
    case InstEncoding::MTBUF:
    case InstEncoding::MIMG:
    case InstEncoding::DS:
    case InstEncoding::EXP:
        instLength = sizeof(uint64_t);
        break;
    default:
        break;
    }
    return instLength;
}

uint32_t GcnDecodeContext::getOpMapOffset(InstEncoding encoding) {
    uint32_t offset = 0;
    switch (encoding) {
    case InstEncoding::SOP1:
        offset = (uint32_t)OpcodeMap::OP_MAP_SOP1;
        break;
    case InstEncoding::SOPP:
        offset = (uint32_t)OpcodeMap::OP_MAP_SOPP;
        break;
    case InstEncoding::SOPC:
        offset = (uint32_t)OpcodeMap::OP_MAP_SOPC;
        break;
    case InstEncoding::VOP1:
        offset = (uint32_t)OpcodeMap::OP_MAP_VOP1;
        break;
    case InstEncoding::VOPC:
        offset = (uint32_t)OpcodeMap::OP_MAP_VOPC;
        break;
    case InstEncoding::VOP3:
        offset = (uint32_t)OpcodeMap::OP_MAP_VOP3;
        break;
    case InstEncoding::EXP:
        offset = (uint32_t)OpcodeMap::OP_MAP_EXP;
        break;
    case InstEncoding::VINTRP:
        offset = (uint32_t)OpcodeMap::OP_MAP_VINTRP;
        break;
    case InstEncoding::DS:
        offset = (uint32_t)OpcodeMap::OP_MAP_DS;
        break;
    case InstEncoding::MUBUF:
        offset = (uint32_t)OpcodeMap::OP_MAP_MUBUF;
        break;
    case InstEncoding::MTBUF:
        offset = (uint32_t)OpcodeMap::OP_MAP_MTBUF;
        break;
    case InstEncoding::MIMG:
        offset = (uint32_t)OpcodeMap::OP_MAP_MIMG;
        break;
    case InstEncoding::SMRD:
        offset = (uint32_t)OpcodeMap::OP_MAP_SMRD;
        break;
    case InstEncoding::SOPK:
        offset = (uint32_t)OpcodeMap::OP_MAP_SOPK;
        break;
    case InstEncoding::SOP2:
        offset = (uint32_t)OpcodeMap::OP_MAP_SOP2;
        break;
    case InstEncoding::VOP2:
        offset = (uint32_t)OpcodeMap::OP_MAP_VOP2;
        break;
    default:
        break;
    }
    return offset;
}

uint32_t GcnDecodeContext::mapEncodingOp(InstEncoding encoding, Opcode opcode) {
    // Map from uniform opcode to encoding specific opcode.
    uint32_t encodingOp = 0;
    if (encoding == InstEncoding::VOP3) {
        if (opcode >= Opcode::V_CMP_F_F32 && opcode <= Opcode::V_CMPX_T_U64) {
            uint32_t op =
                static_cast<uint32_t>(opcode) - static_cast<uint32_t>(OpcodeMap::OP_MAP_VOPC);
            encodingOp = op + static_cast<uint32_t>(OpMapVOP3VOPX::VOP3_TO_VOPC);
        } else if (opcode >= Opcode::V_CNDMASK_B32 && opcode <= Opcode::V_CVT_PK_I16_I32) {
            uint32_t op =
                static_cast<uint32_t>(opcode) - static_cast<uint32_t>(OpcodeMap::OP_MAP_VOP2);
            encodingOp = op + static_cast<uint32_t>(OpMapVOP3VOPX::VOP3_TO_VOP2);
        } else if (opcode >= Opcode::V_NOP && opcode <= Opcode::V_MOVRELSD_B32) {
            uint32_t op =
                static_cast<uint32_t>(opcode) - static_cast<uint32_t>(OpcodeMap::OP_MAP_VOP1);
            encodingOp = op + static_cast<uint32_t>(OpMapVOP3VOPX::VOP3_TO_VOP1);
        } else {
            encodingOp =
                static_cast<uint32_t>(opcode) - static_cast<uint32_t>(OpcodeMap::OP_MAP_VOP3);
        }
    } else {
        uint32_t mapOffset = getOpMapOffset(encoding);
        encodingOp = static_cast<uint32_t>(opcode) - mapOffset;
    }

    return encodingOp;
}

void GcnDecodeContext::updateInstructionMeta(InstEncoding encoding) {
    uint32_t encodingOp = mapEncodingOp(encoding, m_instruction.opcode);
    InstFormat instFormat = InstructionFormat(encoding, encodingOp);

    ASSERT_MSG(instFormat.src_type != ScalarType::Undefined &&
                   instFormat.dst_type != ScalarType::Undefined,
               "Instruction format table incomplete for opcode {} ({}, encoding = {})",
               magic_enum::enum_name(m_instruction.opcode), u32(m_instruction.opcode),
               magic_enum::enum_name(encoding));

    m_instruction.inst_class = instFormat.inst_class;
    m_instruction.category = instFormat.inst_category;
    m_instruction.encoding = encoding;
    m_instruction.src_count = instFormat.src_count;
    m_instruction.length = getEncodingLength(encoding);

    // Update src operand scalar type.
    auto setOperandType = [&instFormat](InstOperand& src) {
        // Only update uninitialized numeric type.
        if (src.type == ScalarType::Undefined) {
            src.type = instFormat.src_type;
        }
    };

    std::for_each_n(m_instruction.src.begin(), m_instruction.src_count, setOperandType);

    // Update dst operand scalar type.
    switch (m_instruction.dst_count) {
    case 2: {
        if (m_instruction.dst[1].type == ScalarType::Undefined) {
            // Only VOP3B has an additional sdst operand,
            // and it must be Uint64
            m_instruction.dst[1].type = ScalarType::Uint64;
        }
    }
        [[fallthrough]];
    case 1: {
        if (m_instruction.dst[0].type == ScalarType::Undefined) {
            m_instruction.dst[0].type = instFormat.dst_type;
        }
    }
    }
}

void GcnDecodeContext::repairOperandType() {
    // Some instructions' operand type is not uniform,
    // it's best to change the instruction table's format and fix them there,
    // but it's a hard work.
    // We fix them here.
    switch (m_instruction.opcode) {
    case Opcode::V_MAD_U64_U32:
        m_instruction.src[2].type = ScalarType::Uint64;
        break;
    case Opcode::V_MAD_I64_I32:
        m_instruction.src[2].type = ScalarType::Sint64;
        break;
    case Opcode::V_ADDC_U32:
        m_instruction.src[2].type = ScalarType::Uint64;
        break;
    case Opcode::IMAGE_GATHER4_C:
    case Opcode::IMAGE_GATHER4_C_O:
        m_instruction.src[0].type = ScalarType::Any;
        break;
    default:
        break;
    }
}

OperandField GcnDecodeContext::getOperandField(uint32_t code) {
    OperandField field = {};
    if (code >= ScalarGPRMin && code <= ScalarGPRMax) {
        field = OperandField::ScalarGPR;
    } else if (code >= SignedConstIntPosMin && code <= SignedConstIntPosMax) {
        field = OperandField::SignedConstIntPos;
    } else if (code >= SignedConstIntNegMin && code <= SignedConstIntNegMax) {
        field = OperandField::SignedConstIntNeg;
    } else if (code >= VectorGPRMin && code <= VectorGPRMax) {
        field = OperandField::VectorGPR;
    } else {
        field = static_cast<OperandField>(code);
    }
    return field;
}

void GcnDecodeContext::decodeInstruction32(InstEncoding encoding, GcnCodeSlice& code) {
    u32 hexInstruction = code.readu32();
    switch (encoding) {
    case InstEncoding::SOP1:
        decodeInstructionSOP1(hexInstruction);
        break;
    case InstEncoding::SOPP:
        decodeInstructionSOPP(hexInstruction);
        break;
    case InstEncoding::SOPC:
        decodeInstructionSOPC(hexInstruction);
        break;
    case InstEncoding::SOPK:
        decodeInstructionSOPK(hexInstruction);
        break;
    case InstEncoding::SOP2:
        decodeInstructionSOP2(hexInstruction);
        break;
    case InstEncoding::VOP1:
        decodeInstructionVOP1(hexInstruction);
        break;
    case InstEncoding::VOPC:
        decodeInstructionVOPC(hexInstruction);
        break;
    case InstEncoding::VOP2:
        decodeInstructionVOP2(hexInstruction);
        break;
    case InstEncoding::SMRD:
        decodeInstructionSMRD(hexInstruction);
        break;
    case InstEncoding::VINTRP:
        decodeInstructionVINTRP(hexInstruction);
        break;
    default:
        break;
    }
}

void GcnDecodeContext::decodeInstruction64(InstEncoding encoding, GcnCodeSlice& code) {
    uint64_t hexInstruction = code.readu64();
    switch (encoding) {
    case InstEncoding::VOP3:
        decodeInstructionVOP3(hexInstruction);
        break;
    case InstEncoding::MUBUF:
        decodeInstructionMUBUF(hexInstruction);
        break;
    case InstEncoding::MTBUF:
        decodeInstructionMTBUF(hexInstruction);
        break;
    case InstEncoding::MIMG:
        decodeInstructionMIMG(hexInstruction);
        break;
    case InstEncoding::DS:
        decodeInstructionDS(hexInstruction);
        break;
    case InstEncoding::EXP:
        decodeInstructionEXP(hexInstruction);
        break;
    default:
        break;
    }
}

void GcnDecodeContext::decodeLiteralConstant(InstEncoding encoding, GcnCodeSlice& code) {
    if (HasAdditionalLiteral(encoding, m_instruction.opcode)) {
        u32 encoding_op = mapEncodingOp(encoding, m_instruction.opcode);
        InstFormat instFormat = InstructionFormat(encoding, encoding_op);
        m_instruction.src[m_instruction.src_count].field = OperandField::LiteralConst;
        m_instruction.src[m_instruction.src_count].type = instFormat.src_type;
        m_instruction.src[m_instruction.src_count].code = code.readu32();
        ++m_instruction.src_count;
        m_instruction.length += sizeof(u32);
        return;
    }

    // Find if the instruction contains a literal constant
    const auto it = std::ranges::find_if(m_instruction.src, [](InstOperand& src) {
        return src.field == OperandField::LiteralConst;
    });
    if (it != m_instruction.src.end()) {
        it->code = code.readu32();
        m_instruction.length += sizeof(u32);
    }
}

void GcnDecodeContext::decodeInstructionSOP1(u32 hexInstruction) {
    u32 ssrc0 = bit::extract(hexInstruction, 7, 0);
    u32 op = bit::extract(hexInstruction, 15, 8);
    u32 sdst = bit::extract(hexInstruction, 22, 16);

    m_instruction.opcode = static_cast<Opcode>(op + static_cast<u32>(OpcodeMap::OP_MAP_SOP1));

    m_instruction.src[0].field = getOperandField(ssrc0);
    m_instruction.src[0].code = ssrc0;
    m_instruction.dst[0].field = getOperandField(sdst);
    m_instruction.dst[0].code = sdst;
    m_instruction.dst_count = 1;
}

void GcnDecodeContext::decodeInstructionSOPP(u32 hexInstruction) {
    u32 op = bit::extract(hexInstruction, 22, 16);

    m_instruction.opcode = static_cast<Opcode>(op + static_cast<u32>(OpcodeMap::OP_MAP_SOPP));

    m_instruction.control.sopp = *reinterpret_cast<InstControlSOPP*>(&hexInstruction);
}

void GcnDecodeContext::decodeInstructionSOPC(u32 hexInstruction) {
    u32 ssrc0 = bit::extract(hexInstruction, 7, 0);
    u32 ssrc1 = bit::extract(hexInstruction, 15, 8);
    u32 op = bit::extract(hexInstruction, 22, 16);

    m_instruction.opcode = static_cast<Opcode>(op + static_cast<u32>(OpcodeMap::OP_MAP_SOPC));

    m_instruction.src[0].field = getOperandField(ssrc0);
    m_instruction.src[0].code = ssrc0;
    m_instruction.src[1].field = getOperandField(ssrc1);
    m_instruction.src[1].code = ssrc1;
}

void GcnDecodeContext::decodeInstructionSOPK(u32 hexInstruction) {
    u32 sdst = bit::extract(hexInstruction, 22, 16);
    u32 op = bit::extract(hexInstruction, 27, 23);

    m_instruction.opcode = static_cast<Opcode>(op + static_cast<u32>(OpcodeMap::OP_MAP_SOPK));

    m_instruction.dst[0].field = getOperandField(sdst);
    m_instruction.dst[0].code = sdst;
    m_instruction.dst_count = 1;

    m_instruction.control.sopk = *reinterpret_cast<InstControlSOPK*>(&hexInstruction);
}

void GcnDecodeContext::decodeInstructionSOP2(u32 hexInstruction) {
    u32 ssrc0 = bit::extract(hexInstruction, 7, 0);
    u32 ssrc1 = bit::extract(hexInstruction, 15, 8);
    u32 sdst = bit::extract(hexInstruction, 22, 16);
    u32 op = bit::extract(hexInstruction, 29, 23);

    m_instruction.opcode = static_cast<Opcode>(op + static_cast<u32>(OpcodeMap::OP_MAP_SOP2));

    m_instruction.src[0].field = getOperandField(ssrc0);
    m_instruction.src[0].code = ssrc0;
    m_instruction.src[1].field = getOperandField(ssrc1);
    m_instruction.src[1].code = ssrc1;
    m_instruction.dst[0].field = getOperandField(sdst);
    m_instruction.dst[0].code = sdst;
    m_instruction.dst_count = 1;
}

void GcnDecodeContext::decodeInstructionVOP1(u32 hexInstruction) {
    u32 src0 = bit::extract(hexInstruction, 8, 0);
    u32 op = bit::extract(hexInstruction, 16, 9);
    u32 vdst = bit::extract(hexInstruction, 24, 17);

    m_instruction.opcode = static_cast<Opcode>(op + static_cast<u32>(OpcodeMap::OP_MAP_VOP1));

    m_instruction.src[0].field = getOperandField(src0);
    m_instruction.src[0].code =
        m_instruction.src[0].field == OperandField::VectorGPR ? src0 - VectorGPRMin : src0;
    m_instruction.dst[0].field = OperandField::VectorGPR;
    m_instruction.dst[0].code = vdst;
    m_instruction.dst_count = 1;

    OpcodeVOP1 vop1Op = static_cast<OpcodeVOP1>(op);
    if (vop1Op == OpcodeVOP1::V_READFIRSTLANE_B32) {
        m_instruction.dst[0].field = getOperandField(vdst);
        m_instruction.dst[0].type = ScalarType::Uint32;
    }
}

void GcnDecodeContext::decodeInstructionVOPC(u32 hexInstruction) {
    u32 src0 = bit::extract(hexInstruction, 8, 0);
    u32 vsrc1 = bit::extract(hexInstruction, 16, 9);
    u32 op = bit::extract(hexInstruction, 24, 17);

    m_instruction.opcode = static_cast<Opcode>(op + static_cast<u32>(OpcodeMap::OP_MAP_VOPC));

    m_instruction.src[0].field = getOperandField(src0);
    m_instruction.src[0].code =
        m_instruction.src[0].field == OperandField::VectorGPR ? src0 - VectorGPRMin : src0;
    m_instruction.src[1].field = OperandField::VectorGPR;
    m_instruction.src[1].code = vsrc1;
    // VOPC dst is forced to VCC.
    // In order to be unified with VOP3 encoding,
    // we store it to dst[1]
    m_instruction.dst[1].field = OperandField::VccLo;
    m_instruction.dst[1].type = ScalarType::Uint64;
    m_instruction.dst[1].code = static_cast<u32>(OperandField::VccLo);
}

void GcnDecodeContext::decodeInstructionVOP2(u32 hexInstruction) {
    u32 src0 = bit::extract(hexInstruction, 8, 0);
    u32 vsrc1 = bit::extract(hexInstruction, 16, 9);
    u32 vdst = bit::extract(hexInstruction, 24, 17);
    u32 op = bit::extract(hexInstruction, 30, 25);

    m_instruction.opcode = static_cast<Opcode>(op + static_cast<u32>(OpcodeMap::OP_MAP_VOP2));

    m_instruction.src[0].field = getOperandField(src0);
    m_instruction.src[0].code =
        m_instruction.src[0].field == OperandField::VectorGPR ? src0 - VectorGPRMin : src0;
    m_instruction.src[1].field = OperandField::VectorGPR;
    m_instruction.src[1].code = vsrc1;
    m_instruction.dst[0].field = OperandField::VectorGPR;
    m_instruction.dst[0].code = vdst;
    m_instruction.dst_count = 1;

    OpcodeVOP2 vop2Op = static_cast<OpcodeVOP2>(op);
    if (vop2Op == OpcodeVOP2::V_READLANE_B32) {
        // vsrc1 is scalar for lane instructions
        m_instruction.src[1].field = getOperandField(vsrc1);
        // dst is sgpr
        m_instruction.dst[0].field = getOperandField(vdst);
        m_instruction.dst[0].type = ScalarType::Uint32;
    } else if (vop2Op == OpcodeVOP2::V_WRITELANE_B32) {
        m_instruction.src[1].field = getOperandField(vsrc1);
        // dst is vgpr, as normal
    } else if (IsVop3BEncoding(m_instruction.opcode)) {
        m_instruction.dst[1].field = OperandField::VccLo;
        m_instruction.dst[1].type = ScalarType::Uint64;
        m_instruction.dst[1].code = static_cast<u32>(OperandField::VccLo);
    }
}

void GcnDecodeContext::decodeInstructionSMRD(u32 hexInstruction) {
    u32 sbase = bit::extract(hexInstruction, 14, 9);
    u32 sdst = bit::extract(hexInstruction, 21, 15);
    u32 op = bit::extract(hexInstruction, 26, 22);

    m_instruction.opcode = static_cast<Opcode>(op + static_cast<u32>(OpcodeMap::OP_MAP_SMRD));

    m_instruction.src[0].field = OperandField::ScalarGPR;
    m_instruction.src[0].code = sbase;
    m_instruction.dst[0].field = OperandField::ScalarGPR;
    m_instruction.dst[0].code = sdst;
    m_instruction.dst_count = 1;

    m_instruction.control.smrd = *reinterpret_cast<InstControlSMRD*>(&hexInstruction);

    if (op <= static_cast<u32>(OpcodeSMRD::S_LOAD_DWORDX16)) {
        m_instruction.control.smrd.count = 1 << op;
    } else if (op >= static_cast<u32>(OpcodeSMRD::S_BUFFER_LOAD_DWORD) &&
               op <= static_cast<u32>(OpcodeSMRD::S_BUFFER_LOAD_DWORDX16)) {
        m_instruction.control.smrd.count = 1 << (op - 8);
    }

    if (m_instruction.control.smrd.imm == 0) {
        u32 code = m_instruction.control.smrd.offset;
        m_instruction.src[1].field = getOperandField(code);
        m_instruction.src[1].type = ScalarType::Uint32;
        m_instruction.src[1].code = code;
    }
}

void GcnDecodeContext::decodeInstructionVINTRP(u32 hexInstruction) {
    u32 vsrc = bit::extract(hexInstruction, 7, 0);
    u32 op = bit::extract(hexInstruction, 17, 16);
    u32 vdst = bit::extract(hexInstruction, 25, 18);

    m_instruction.opcode = static_cast<Opcode>(op + static_cast<u32>(OpcodeMap::OP_MAP_VINTRP));

    m_instruction.src[0].field = OperandField::VectorGPR;
    m_instruction.src[0].code = vsrc;
    m_instruction.dst[0].field = OperandField::VectorGPR;
    m_instruction.dst[0].code = vdst;
    m_instruction.dst_count = 1;

    m_instruction.control.vintrp = *reinterpret_cast<InstControlVINTRP*>(&hexInstruction);
}

void GcnDecodeContext::decodeInstructionVOP3(uint64_t hexInstruction) {
    u32 vdst = bit::extract(hexInstruction, 7, 0);
    u32 sdst = bit::extract(hexInstruction, 14, 8); // For VOP3B
    u32 op = bit::extract(hexInstruction, 25, 17);
    u32 src0 = bit::extract(hexInstruction, 40, 32);
    u32 src1 = bit::extract(hexInstruction, 49, 41);
    u32 src2 = bit::extract(hexInstruction, 58, 50);

    if (op >= static_cast<u32>(OpcodeVOP3::V_CMP_F_F32) &&
        op <= static_cast<u32>(OpcodeVOP3::V_CMPX_T_U64)) {
        // Map from VOP3 to VOPC
        u32 vopcOp = op - static_cast<u32>(OpMapVOP3VOPX::VOP3_TO_VOPC);
        m_instruction.opcode =
            static_cast<Opcode>(vopcOp + static_cast<u32>(OpcodeMap::OP_MAP_VOPC));
    } else if (op >= static_cast<u32>(OpcodeVOP3::V_CNDMASK_B32) &&
               op <= static_cast<u32>(OpcodeVOP3::V_CVT_PK_I16_I32)) {
        // Map from VOP3 to VOP2
        u32 vop2Op = op - static_cast<u32>(OpMapVOP3VOPX::VOP3_TO_VOP2);
        m_instruction.opcode =
            static_cast<Opcode>(vop2Op + static_cast<u32>(OpcodeMap::OP_MAP_VOP2));
    } else if (op >= static_cast<u32>(OpcodeVOP3::V_NOP) &&
               op <= static_cast<u32>(OpcodeVOP3::V_MOVRELSD_B32)) {
        // Map from VOP3 to VOP1
        u32 vop1Op = op - static_cast<u32>(OpMapVOP3VOPX::VOP3_TO_VOP1);
        m_instruction.opcode =
            static_cast<Opcode>(vop1Op + static_cast<u32>(OpcodeMap::OP_MAP_VOP1));
    } else {
        // VOP3 encoding, do not map.
        m_instruction.opcode = static_cast<Opcode>(op + static_cast<u32>(OpcodeMap::OP_MAP_VOP3));
    }

    m_instruction.src[0].field = getOperandField(src0);
    m_instruction.src[0].code =
        m_instruction.src[0].field == OperandField::VectorGPR ? src0 - VectorGPRMin : src0;
    m_instruction.src[1].field = getOperandField(src1);
    m_instruction.src[1].code =
        m_instruction.src[1].field == OperandField::VectorGPR ? src1 - VectorGPRMin : src1;
    m_instruction.src[2].field = getOperandField(src2);
    m_instruction.src[2].code =
        m_instruction.src[2].field == OperandField::VectorGPR ? src2 - VectorGPRMin : src2;
    m_instruction.dst[0].field = OperandField::VectorGPR;
    m_instruction.dst[0].code = vdst;

    OpcodeVOP3 vop3Op = static_cast<OpcodeVOP3>(op);
    if (IsVop3BEncoding(m_instruction.opcode)) {
        m_instruction.dst[1].field = getOperandField(sdst);
        m_instruction.dst[1].type = ScalarType::Uint64;
        m_instruction.dst[1].code = sdst;
    } else {
        if (vop3Op >= OpcodeVOP3::V_CMP_F_F32 && vop3Op <= OpcodeVOP3::V_CMPX_T_U64) {
            m_instruction.dst[1].field = getOperandField(vdst);
            m_instruction.dst[1].type = ScalarType::Uint64;
            m_instruction.dst[1].code = vdst;
        } else if (vop3Op == OpcodeVOP3::V_READLANE_B32 ||
                   vop3Op == OpcodeVOP3::V_READFIRSTLANE_B32) {
            m_instruction.dst[0].field = getOperandField(vdst);
            m_instruction.dst[0].type = ScalarType::Uint32;
            // WRITELANE can be decoded like other VOP3's
        }
    }

    if (op >= static_cast<u32>(OpcodeVOP3::V_ADD_I32) &&
        op <= static_cast<u32>(OpcodeVOP3::V_DIV_SCALE_F64)) {
        // VOP3B has a sdst operand.
        m_instruction.dst_count = 2;
    } else {
        m_instruction.dst_count = 1;
    }

    m_instruction.control.vop3 = *reinterpret_cast<InstControlVOP3*>(&hexInstruction);

    // update input modifier
    auto& control = m_instruction.control.vop3;
    for (u32 i = 0; i != 3; ++i) {
        if (control.abs & (1u << i)) {
            m_instruction.src[i].input_modifier.abs = true;
        }

        if (control.neg & (1u << i)) {
            m_instruction.src[i].input_modifier.neg = true;
        }
    }

    // update output modifier
    auto& outputMod = m_instruction.dst[0].output_modifier;

    outputMod.clamp = static_cast<bool>(control.clmp);
    switch (control.omod) {
    case 0:
        outputMod.multiplier = 0.f;
        break;
    case 1:
        outputMod.multiplier = 2.0f;
        break;
    case 2:
        outputMod.multiplier = 4.0f;
        break;
    case 3:
        outputMod.multiplier = 0.5f;
        break;
    }
}

void GcnDecodeContext::decodeInstructionMUBUF(uint64_t hexInstruction) {
    u32 op = bit::extract(hexInstruction, 24, 18);
    u32 vaddr = bit::extract(hexInstruction, 39, 32);
    u32 vdata = bit::extract(hexInstruction, 47, 40);
    u32 srsrc = bit::extract(hexInstruction, 52, 48);
    u32 soffset = bit::extract(hexInstruction, 63, 56);

    m_instruction.opcode = static_cast<Opcode>(op + static_cast<u32>(OpcodeMap::OP_MAP_MUBUF));

    m_instruction.src[0].field = OperandField::VectorGPR;
    m_instruction.src[0].code = vaddr;
    m_instruction.src[1].field = OperandField::VectorGPR;
    m_instruction.src[1].code = vdata;
    m_instruction.src[2].field = OperandField::ScalarGPR;
    m_instruction.src[2].code = srsrc;
    m_instruction.src[3].field = getOperandField(soffset);
    m_instruction.src[3].code = soffset;

    m_instruction.control.mubuf = *reinterpret_cast<InstControlMUBUF*>(&hexInstruction);

    if (op >= static_cast<u32>(OpcodeMUBUF::BUFFER_LOAD_FORMAT_X) &&
        op <= static_cast<u32>(OpcodeMUBUF::BUFFER_LOAD_FORMAT_XYZW)) {
        m_instruction.control.mubuf.count = op + 1;
        m_instruction.control.mubuf.size = (op + 1) * sizeof(u32);
    } else if (op >= static_cast<u32>(OpcodeMUBUF::BUFFER_STORE_FORMAT_X) &&
               op <= static_cast<u32>(OpcodeMUBUF::BUFFER_STORE_FORMAT_XYZW)) {
        m_instruction.control.mubuf.count = op - 3;
        m_instruction.control.mubuf.size = (op - 3) * sizeof(u32);
    } else if (op >= static_cast<u32>(OpcodeMUBUF::BUFFER_LOAD_DWORD) &&
               op <= static_cast<u32>(OpcodeMUBUF::BUFFER_LOAD_DWORDX3)) {
        m_instruction.control.mubuf.count =
            op == static_cast<u32>(OpcodeMUBUF::BUFFER_LOAD_DWORDX3) ? 3 : 1 << (op - 12);
        m_instruction.control.mubuf.size = m_instruction.control.mubuf.count * sizeof(u32);
    } else if (op >= static_cast<u32>(OpcodeMUBUF::BUFFER_STORE_DWORD) &&
               op <= static_cast<u32>(OpcodeMUBUF::BUFFER_STORE_DWORDX3)) {
        m_instruction.control.mubuf.count =
            op == static_cast<u32>(OpcodeMUBUF::BUFFER_STORE_DWORDX3) ? 3 : 1 << (op - 28);
        m_instruction.control.mubuf.size = m_instruction.control.mubuf.count * sizeof(u32);
    } else if (op >= static_cast<u32>(OpcodeMUBUF::BUFFER_LOAD_UBYTE) &&
               op <= static_cast<u32>(OpcodeMUBUF::BUFFER_LOAD_SSHORT)) {
        m_instruction.control.mubuf.count = 1;
        if (op >= static_cast<u32>(OpcodeMUBUF::BUFFER_LOAD_UBYTE) &&
            op <= static_cast<u32>(OpcodeMUBUF::BUFFER_LOAD_SBYTE)) {
            m_instruction.control.mubuf.size = 1;
        } else {
            m_instruction.control.mubuf.size = 2;
        }
    } else if (op >= static_cast<u32>(OpcodeMUBUF::BUFFER_STORE_BYTE) &&
               op <= static_cast<u32>(OpcodeMUBUF::BUFFER_STORE_SHORT)) {
        m_instruction.control.mubuf.count = 1;
        if (op == static_cast<u32>(OpcodeMUBUF::BUFFER_STORE_BYTE)) {
            m_instruction.control.mubuf.size = 1;
        } else {
            m_instruction.control.mubuf.size = 2;
        }
    } else if (op >= static_cast<u32>(OpcodeMUBUF::BUFFER_ATOMIC_SWAP) &&
               op <= static_cast<u32>(OpcodeMUBUF::BUFFER_ATOMIC_FMAX)) {
        m_instruction.control.mubuf.count = 1;
        m_instruction.control.mubuf.size = sizeof(u32);
    } else if (op >= static_cast<u32>(OpcodeMUBUF::BUFFER_ATOMIC_SWAP_X2) &&
               op <= static_cast<u32>(OpcodeMUBUF::BUFFER_ATOMIC_FMAX_X2)) {
        m_instruction.control.mubuf.count = 2;
        m_instruction.control.mubuf.size = sizeof(u32) * 2;
    }
}

void GcnDecodeContext::decodeInstructionMTBUF(uint64_t hexInstruction) {
    u32 op = bit::extract(hexInstruction, 18, 16);
    u32 vaddr = bit::extract(hexInstruction, 39, 32);
    u32 vdata = bit::extract(hexInstruction, 47, 40);
    u32 srsrc = bit::extract(hexInstruction, 52, 48);
    u32 soffset = bit::extract(hexInstruction, 63, 56);

    m_instruction.opcode = static_cast<Opcode>(op + static_cast<u32>(OpcodeMap::OP_MAP_MTBUF));

    m_instruction.src[0].field = OperandField::VectorGPR;
    m_instruction.src[0].code = vaddr;
    m_instruction.src[1].field = OperandField::VectorGPR;
    m_instruction.src[1].code = vdata;
    m_instruction.src[2].field = OperandField::ScalarGPR;
    m_instruction.src[2].code = srsrc;
    m_instruction.src[3].field = getOperandField(soffset);
    m_instruction.src[3].code = soffset;

    m_instruction.control.mtbuf = *reinterpret_cast<InstControlMTBUF*>(&hexInstruction);

    if (op >= static_cast<u32>(OpcodeMTBUF::TBUFFER_LOAD_FORMAT_X) &&
        op <= static_cast<u32>(OpcodeMTBUF::TBUFFER_LOAD_FORMAT_XYZW)) {
        m_instruction.control.mtbuf.count = op + 1;
    } else if (op >= static_cast<u32>(OpcodeMTBUF::TBUFFER_STORE_FORMAT_X) &&
               op <= static_cast<u32>(OpcodeMTBUF::TBUFFER_STORE_FORMAT_XYZW)) {
        m_instruction.control.mtbuf.count = op - 3;
    }
}

u32 GcnDecodeContext::getMimgModifier(Opcode opcode) {
    MimgModifierFlags flags = {};

    switch (opcode) {
    case Opcode::IMAGE_SAMPLE:
        break;
    case Opcode::IMAGE_SAMPLE_CL:
        flags.set(MimgModifier::LodClamp);
        break;
    case Opcode::IMAGE_SAMPLE_D:
        flags.set(MimgModifier::Derivative);
        break;
    case Opcode::IMAGE_SAMPLE_D_CL:
        flags.set(MimgModifier::Derivative, MimgModifier::LodClamp);
        break;
    case Opcode::IMAGE_SAMPLE_L:
        flags.set(MimgModifier::Lod);
        break;
    case Opcode::IMAGE_SAMPLE_B:
        flags.set(MimgModifier::LodBias);
        break;
    case Opcode::IMAGE_SAMPLE_B_CL:
        flags.set(MimgModifier::LodBias, MimgModifier::LodClamp);
        break;
    case Opcode::IMAGE_SAMPLE_LZ:
        flags.set(MimgModifier::Level0);
        break;
    case Opcode::IMAGE_SAMPLE_C:
        flags.set(MimgModifier::Pcf);
        break;
    case Opcode::IMAGE_SAMPLE_C_CL:
        flags.set(MimgModifier::Pcf, MimgModifier::LodClamp);
        break;
    case Opcode::IMAGE_SAMPLE_C_D:
        flags.set(MimgModifier::Pcf, MimgModifier::Derivative);
        break;
    case Opcode::IMAGE_SAMPLE_C_D_CL:
        flags.set(MimgModifier::Pcf, MimgModifier::Derivative, MimgModifier::LodClamp);
        break;
    case Opcode::IMAGE_SAMPLE_C_L:
        flags.set(MimgModifier::Pcf, MimgModifier::Lod);
        break;
    case Opcode::IMAGE_SAMPLE_C_B:
        flags.set(MimgModifier::Pcf, MimgModifier::LodBias);
        break;
    case Opcode::IMAGE_SAMPLE_C_B_CL:
        flags.set(MimgModifier::Pcf, MimgModifier::LodBias, MimgModifier::LodClamp);
        break;
    case Opcode::IMAGE_SAMPLE_C_LZ:
        flags.set(MimgModifier::Pcf, MimgModifier::Level0);
        break;
    case Opcode::IMAGE_SAMPLE_O:
        flags.set(MimgModifier::Offset);
        break;
    case Opcode::IMAGE_SAMPLE_CL_O:
        flags.set(MimgModifier::LodClamp, MimgModifier::Offset);
        break;
    case Opcode::IMAGE_SAMPLE_D_O:
        flags.set(MimgModifier::Derivative, MimgModifier::Offset);
        break;
    case Opcode::IMAGE_SAMPLE_D_CL_O:
        flags.set(MimgModifier::Derivative, MimgModifier::LodClamp, MimgModifier::Offset);
        break;
    case Opcode::IMAGE_SAMPLE_L_O:
        flags.set(MimgModifier::Lod, MimgModifier::Offset);
        break;
    case Opcode::IMAGE_SAMPLE_B_O:
        flags.set(MimgModifier::LodBias, MimgModifier::Offset);
        break;
    case Opcode::IMAGE_SAMPLE_B_CL_O:
        flags.set(MimgModifier::LodBias, MimgModifier::LodClamp, MimgModifier::Offset);
        break;
    case Opcode::IMAGE_SAMPLE_LZ_O:
        flags.set(MimgModifier::Level0, MimgModifier::Offset);
        break;
    case Opcode::IMAGE_SAMPLE_C_O:
        flags.set(MimgModifier::Pcf, MimgModifier::Offset);
        break;
    case Opcode::IMAGE_SAMPLE_C_CL_O:
        flags.set(MimgModifier::Pcf, MimgModifier::LodClamp, MimgModifier::Offset);
        break;
    case Opcode::IMAGE_SAMPLE_C_D_O:
        flags.set(MimgModifier::Pcf, MimgModifier::Derivative, MimgModifier::Offset);
        break;
    case Opcode::IMAGE_SAMPLE_C_D_CL_O:
        flags.set(MimgModifier::Pcf, MimgModifier::Derivative, MimgModifier::LodClamp,
                  MimgModifier::Offset);
        break;
    case Opcode::IMAGE_SAMPLE_C_L_O:
        flags.set(MimgModifier::Pcf, MimgModifier::Lod, MimgModifier::Offset);
        break;
    case Opcode::IMAGE_SAMPLE_C_B_O:
        flags.set(MimgModifier::Pcf, MimgModifier::LodBias, MimgModifier::Offset);
        break;
    case Opcode::IMAGE_SAMPLE_C_B_CL_O:
        flags.set(MimgModifier::Pcf, MimgModifier::LodBias, MimgModifier::LodClamp,
                  MimgModifier::Offset);
        break;
    case Opcode::IMAGE_SAMPLE_C_LZ_O:
        flags.set(MimgModifier::Pcf, MimgModifier::Level0, MimgModifier::Offset);
        break;
    case Opcode::IMAGE_GATHER4:
        break;
    case Opcode::IMAGE_GATHER4_CL:
        flags.set(MimgModifier::LodClamp);
        break;
    case Opcode::IMAGE_GATHER4_L:
        flags.set(MimgModifier::Lod);
        break;
    case Opcode::IMAGE_GATHER4_B:
        flags.set(MimgModifier::LodBias);
        break;
    case Opcode::IMAGE_GATHER4_B_CL:
        flags.set(MimgModifier::LodBias, MimgModifier::LodClamp);
        break;
    case Opcode::IMAGE_GATHER4_LZ:
        flags.set(MimgModifier::Level0);
        break;
    case Opcode::IMAGE_GATHER4_C:
        flags.set(MimgModifier::Pcf);
        break;
    case Opcode::IMAGE_GATHER4_C_CL:
        flags.set(MimgModifier::Pcf, MimgModifier::LodClamp);
        break;
    case Opcode::IMAGE_GATHER4_C_L:
        flags.set(MimgModifier::Pcf, MimgModifier::Lod);
        break;
    case Opcode::IMAGE_GATHER4_C_B:
        flags.set(MimgModifier::Pcf, MimgModifier::LodBias);
        break;
    case Opcode::IMAGE_GATHER4_C_B_CL:
        flags.set(MimgModifier::Pcf, MimgModifier::LodBias, MimgModifier::LodClamp);
        break;
    case Opcode::IMAGE_GATHER4_C_LZ:
        flags.set(MimgModifier::Pcf, MimgModifier::Level0);
        break;
    case Opcode::IMAGE_GATHER4_O:
        flags.set(MimgModifier::Offset);
        break;
    case Opcode::IMAGE_GATHER4_CL_O:
        flags.set(MimgModifier::LodClamp, MimgModifier::Offset);
        break;
    case Opcode::IMAGE_GATHER4_L_O:
        flags.set(MimgModifier::Lod, MimgModifier::Offset);
        break;
    case Opcode::IMAGE_GATHER4_B_O:
        flags.set(MimgModifier::LodBias, MimgModifier::Offset);
        break;
    case Opcode::IMAGE_GATHER4_B_CL_O:
        flags.set(MimgModifier::LodBias, MimgModifier::LodClamp, MimgModifier::Offset);
        break;
    case Opcode::IMAGE_GATHER4_LZ_O:
        flags.set(MimgModifier::Level0, MimgModifier::Offset);
        break;
    case Opcode::IMAGE_GATHER4_C_O:
        flags.set(MimgModifier::Pcf, MimgModifier::Offset);
        break;
    case Opcode::IMAGE_GATHER4_C_CL_O:
        flags.set(MimgModifier::Pcf, MimgModifier::LodClamp, MimgModifier::Offset);
        break;
    case Opcode::IMAGE_GATHER4_C_L_O:
        flags.set(MimgModifier::Pcf, MimgModifier::Lod, MimgModifier::Offset);
        break;
    case Opcode::IMAGE_GATHER4_C_B_O:
        flags.set(MimgModifier::Pcf, MimgModifier::LodBias, MimgModifier::Offset);
        break;
    case Opcode::IMAGE_GATHER4_C_B_CL_O:
        flags.set(MimgModifier::Pcf, MimgModifier::LodBias, MimgModifier::LodClamp,
                  MimgModifier::Offset);
        break;
    case Opcode::IMAGE_GATHER4_C_LZ_O:
        flags.set(MimgModifier::Pcf, MimgModifier::Level0, MimgModifier::Offset);
        break;
    case Opcode::IMAGE_SAMPLE_CD:
        flags.set(MimgModifier::CoarseDerivative);
        break;
    case Opcode::IMAGE_SAMPLE_CD_CL:
        flags.set(MimgModifier::CoarseDerivative, MimgModifier::LodClamp);
        break;
    case Opcode::IMAGE_SAMPLE_C_CD:
        flags.set(MimgModifier::Pcf, MimgModifier::CoarseDerivative);
        break;
    case Opcode::IMAGE_SAMPLE_C_CD_CL:
        flags.set(MimgModifier::Pcf, MimgModifier::CoarseDerivative, MimgModifier::LodClamp);
        break;
    case Opcode::IMAGE_SAMPLE_CD_O:
        flags.set(MimgModifier::CoarseDerivative, MimgModifier::Offset);
        break;
    case Opcode::IMAGE_SAMPLE_CD_CL_O:
        flags.set(MimgModifier::CoarseDerivative, MimgModifier::LodClamp, MimgModifier::Offset);
        break;
    case Opcode::IMAGE_SAMPLE_C_CD_O:
        flags.set(MimgModifier::Pcf, MimgModifier::CoarseDerivative, MimgModifier::Offset);
        break;
    case Opcode::IMAGE_SAMPLE_C_CD_CL_O:
        flags.set(MimgModifier::Pcf, MimgModifier::CoarseDerivative, MimgModifier::LodClamp,
                  MimgModifier::Offset);
        break;
    default:
        break;
    }

    return flags.raw();
}

void GcnDecodeContext::decodeInstructionMIMG(uint64_t hexInstruction) {
    u32 op = bit::extract(hexInstruction, 24, 18);
    u32 vaddr = bit::extract(hexInstruction, 39, 32);
    u32 vdata = bit::extract(hexInstruction, 47, 40);
    u32 srsrc = bit::extract(hexInstruction, 52, 48);
    u32 ssamp = bit::extract(hexInstruction, 57, 53);

    m_instruction.opcode = static_cast<Opcode>(op + static_cast<u32>(OpcodeMap::OP_MAP_MIMG));

    m_instruction.src[0].field = OperandField::VectorGPR;
    m_instruction.src[0].code = vaddr;
    m_instruction.src[2].field = OperandField::ScalarGPR;
    m_instruction.src[2].code = srsrc;
    m_instruction.src[3].field = OperandField::ScalarGPR;
    m_instruction.src[3].code = ssamp;
    m_instruction.dst[0].field = OperandField::VectorGPR;
    m_instruction.dst[0].code = vdata;

    m_instruction.control.mimg = *reinterpret_cast<InstControlMIMG*>(&hexInstruction);
    m_instruction.control.mimg.mod = getMimgModifier(m_instruction.opcode);
    ASSERT(m_instruction.control.mimg.r128 == 0);
}

void GcnDecodeContext::decodeInstructionDS(uint64_t hexInstruction) {
    OpcodeDS op = (OpcodeDS)bit::extract(hexInstruction, 25, 18);
    u32 addr = bit::extract(hexInstruction, 39, 32);
    u32 data0 = bit::extract(hexInstruction, 47, 40);
    u32 data1 = bit::extract(hexInstruction, 55, 48);
    u32 vdst = bit::extract(hexInstruction, 63, 56);

    m_instruction.opcode = static_cast<Opcode>(u32(op) + static_cast<u32>(OpcodeMap::OP_MAP_DS));

    m_instruction.src[0].field = OperandField::VectorGPR;
    m_instruction.src[0].code = addr;
    m_instruction.src[1].field = OperandField::VectorGPR;
    m_instruction.src[1].code = data0;
    m_instruction.src[2].field = OperandField::VectorGPR;
    m_instruction.src[2].code = data1;
    m_instruction.dst[0].field = OperandField::VectorGPR;
    m_instruction.dst[0].code = vdst;
    m_instruction.dst_count = 1;

    m_instruction.control.ds = *reinterpret_cast<InstControlDS*>(&hexInstruction);

    auto instFormat = InstructionFormat(InstEncoding::DS, (u32)op);

    m_instruction.control.ds.dual =
        op == OpcodeDS::DS_WRITE2_B32 || op == OpcodeDS::DS_WRXCHG2_RTN_B32 ||
        op == OpcodeDS::DS_READ2_B32 || op == OpcodeDS::DS_WRITE2_B64 ||
        op == OpcodeDS::DS_WRXCHG2_RTN_B64 || op == OpcodeDS::DS_READ2_B64;

    m_instruction.control.ds.sign = instFormat.src_type == ScalarType::Sint32;

    m_instruction.control.ds.relative =
        op >= OpcodeDS::DS_ADD_SRC2_U32 && op <= OpcodeDS::DS_MAX_SRC2_F64;

    m_instruction.control.ds.stride =
        op == OpcodeDS::DS_WRITE2ST64_B32 || op == OpcodeDS::DS_WRXCHG2ST64_RTN_B32 ||
        op == OpcodeDS::DS_READ2ST64_B32 || op == OpcodeDS::DS_WRITE2ST64_B64 ||
        op == OpcodeDS::DS_WRXCHG2ST64_RTN_B64 || op == OpcodeDS::DS_READ2ST64_B64;

    if (op == OpcodeDS::DS_WRITE_B8 || op == OpcodeDS::DS_READ_I8 || op == OpcodeDS::DS_READ_U8) {
        m_instruction.control.ds.size = 1;
    } else if (op == OpcodeDS::DS_WRITE_B16 || op == OpcodeDS::DS_READ_I16 ||
               op == OpcodeDS::DS_READ_U16) {
        m_instruction.control.ds.size = 2;
    } else {
        if (instFormat.src_type == ScalarType::Sint32 ||
            instFormat.src_type == ScalarType::Uint32) {
            m_instruction.control.ds.size = 4;
        } else if (instFormat.src_type == ScalarType::Sint64 ||
                   instFormat.src_type == ScalarType::Uint64) {
            m_instruction.control.ds.size = 8;
        } else {
            m_instruction.control.ds.size = 0;
        }
    }
}

void GcnDecodeContext::decodeInstructionEXP(uint64_t hexInstruction) {
    u32 vsrc0 = bit::extract(hexInstruction, 39, 32);
    u32 vsrc1 = bit::extract(hexInstruction, 47, 40);
    u32 vsrc2 = bit::extract(hexInstruction, 55, 48);
    u32 vsrc3 = bit::extract(hexInstruction, 63, 56);

    m_instruction.opcode = Opcode::EXP;

    m_instruction.src[0].field = OperandField::VectorGPR;
    m_instruction.src[0].code = vsrc0;
    m_instruction.src[1].field = OperandField::VectorGPR;
    m_instruction.src[1].code = vsrc1;
    m_instruction.src[2].field = OperandField::VectorGPR;
    m_instruction.src[2].code = vsrc2;
    m_instruction.src[3].field = OperandField::VectorGPR;
    m_instruction.src[3].code = vsrc3;

    m_instruction.control.exp = *reinterpret_cast<InstControlEXP*>(&hexInstruction);
}

} // namespace Shader::Gcn
