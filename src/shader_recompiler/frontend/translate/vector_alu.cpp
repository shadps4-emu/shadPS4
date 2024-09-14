// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/frontend/opcodes.h"
#include "shader_recompiler/frontend/translate/translate.h"

namespace Shader::Gcn {

void Translator::EmitVectorAlu(const GcnInst& inst) {
    switch (inst.opcode) {
    case Opcode::V_LSHLREV_B32:
        return V_LSHLREV_B32(inst);
    case Opcode::V_LSHL_B32:
        return V_LSHL_B32(inst);
    case Opcode::V_LSHL_B64:
        return V_LSHL_B64(inst);
    case Opcode::V_BFREV_B32:
        return V_BFREV_B32(inst);
    case Opcode::V_BFE_U32:
        return V_BFE_U32(false, inst);
    case Opcode::V_BFE_I32:
        return V_BFE_U32(true, inst);
    case Opcode::V_BFI_B32:
        return V_BFI_B32(inst);
    case Opcode::V_LSHR_B32:
        return V_LSHR_B32(inst);
    case Opcode::V_ASHRREV_I32:
        return V_ASHRREV_I32(inst);
    case Opcode::V_ASHR_I32:
        return V_ASHR_I32(inst);
    case Opcode::V_LSHRREV_B32:
        return V_LSHRREV_B32(inst);
    case Opcode::V_NOT_B32:
        return V_NOT_B32(inst);
    case Opcode::V_AND_B32:
        return V_AND_B32(inst);
    case Opcode::V_OR_B32:
        return V_OR_B32(false, inst);
    case Opcode::V_XOR_B32:
        return V_OR_B32(true, inst);
    case Opcode::V_FFBL_B32:
        return V_FFBL_B32(inst);

    case Opcode::V_MOV_B32:
        return V_MOV(inst);
    case Opcode::V_ADD_I32:
        return V_ADD_I32(inst);
    case Opcode::V_ADDC_U32:
        return V_ADDC_U32(inst);
    case Opcode::V_CVT_F32_I32:
        return V_CVT_F32_I32(inst);
    case Opcode::V_CVT_F32_U32:
        return V_CVT_F32_U32(inst);
    case Opcode::V_CVT_PKRTZ_F16_F32:
        return V_CVT_PKRTZ_F16_F32(inst);
    case Opcode::V_CVT_F32_F16:
        return V_CVT_F32_F16(inst);
    case Opcode::V_CVT_F16_F32:
        return V_CVT_F16_F32(inst);
    case Opcode::V_CVT_F32_UBYTE0:
        return V_CVT_F32_UBYTE(0, inst);
    case Opcode::V_CVT_F32_UBYTE1:
        return V_CVT_F32_UBYTE(1, inst);
    case Opcode::V_CVT_F32_UBYTE2:
        return V_CVT_F32_UBYTE(2, inst);
    case Opcode::V_CVT_F32_UBYTE3:
        return V_CVT_F32_UBYTE(3, inst);
    case Opcode::V_CVT_OFF_F32_I4:
        return V_CVT_OFF_F32_I4(inst);
    case Opcode::V_MAD_U64_U32:
        return V_MAD_U64_U32(inst);
    case Opcode::V_CMP_GE_I32:
        return V_CMP_U32(ConditionOp::GE, true, false, inst);
    case Opcode::V_CMP_EQ_I32:
        return V_CMP_U32(ConditionOp::EQ, true, false, inst);
    case Opcode::V_CMP_LE_I32:
        return V_CMP_U32(ConditionOp::LE, true, false, inst);
    case Opcode::V_CMP_NE_I32:
        return V_CMP_U32(ConditionOp::LG, true, false, inst);
    case Opcode::V_CMP_NE_U32:
        return V_CMP_U32(ConditionOp::LG, false, false, inst);
    case Opcode::V_CMP_EQ_U32:
        return V_CMP_U32(ConditionOp::EQ, false, false, inst);
    case Opcode::V_CMP_F_U32:
        return V_CMP_U32(ConditionOp::F, false, false, inst);
    case Opcode::V_CMP_LT_U32:
        return V_CMP_U32(ConditionOp::LT, false, false, inst);
    case Opcode::V_CMP_GT_U32:
        return V_CMP_U32(ConditionOp::GT, false, false, inst);
    case Opcode::V_CMP_GE_U32:
        return V_CMP_U32(ConditionOp::GE, false, false, inst);
    case Opcode::V_CMP_TRU_U32:
        return V_CMP_U32(ConditionOp::TRU, false, false, inst);
    case Opcode::V_CMP_NEQ_F32:
        return V_CMP_F32(ConditionOp::LG, false, inst);
    case Opcode::V_CMP_F_F32:
        return V_CMP_F32(ConditionOp::F, false, inst);
    case Opcode::V_CMP_LT_F32:
        return V_CMP_F32(ConditionOp::LT, false, inst);
    case Opcode::V_CMP_EQ_F32:
        return V_CMP_F32(ConditionOp::EQ, false, inst);
    case Opcode::V_CMP_LE_F32:
        return V_CMP_F32(ConditionOp::LE, false, inst);
    case Opcode::V_CMP_GT_F32:
        return V_CMP_F32(ConditionOp::GT, false, inst);
    case Opcode::V_CMP_LG_F32:
        return V_CMP_F32(ConditionOp::LG, false, inst);
    case Opcode::V_CMP_GE_F32:
        return V_CMP_F32(ConditionOp::GE, false, inst);
    case Opcode::V_CMP_NLE_F32:
        return V_CMP_F32(ConditionOp::GT, false, inst);
    case Opcode::V_CMP_NLT_F32:
        return V_CMP_F32(ConditionOp::GE, false, inst);
    case Opcode::V_CMP_NGT_F32:
        return V_CMP_F32(ConditionOp::LE, false, inst);
    case Opcode::V_CMP_NGE_F32:
        return V_CMP_F32(ConditionOp::LT, false, inst);
    case Opcode::V_CMP_U_F32:
        return V_CMP_F32(ConditionOp::U, false, inst);
    case Opcode::V_CNDMASK_B32:
        return V_CNDMASK_B32(inst);
    case Opcode::V_MAX_I32:
        return V_MAX_U32(true, inst);
    case Opcode::V_MAX_U32:
        return V_MAX_U32(false, inst);
    case Opcode::V_MIN_I32:
        return V_MIN_I32(inst);
    case Opcode::V_CUBEMA_F32:
        return V_CUBEMA_F32(inst);
    case Opcode::V_CUBESC_F32:
        return V_CUBESC_F32(inst);
    case Opcode::V_CUBETC_F32:
        return V_CUBETC_F32(inst);
    case Opcode::V_CUBEID_F32:
        return V_CUBEID_F32(inst);
    case Opcode::V_CVT_U32_F32:
        return V_CVT_U32_F32(inst);
    case Opcode::V_CVT_I32_F32:
        return V_CVT_I32_F32(inst);
    case Opcode::V_CVT_FLR_I32_F32:
        return V_CVT_FLR_I32_F32(inst);
    case Opcode::V_SUBREV_I32:
        return V_SUBREV_I32(inst);
    case Opcode::V_MUL_HI_U32:
        return V_MUL_HI_U32(false, inst);
    case Opcode::V_MUL_LO_I32:
        return V_MUL_LO_U32(inst);
    case Opcode::V_SAD_U32:
        return V_SAD_U32(inst);
    case Opcode::V_SUB_I32:
        return V_SUB_I32(inst);
    case Opcode::V_MAD_I32_I24:
        return V_MAD_I32_I24(inst);
    case Opcode::V_MUL_I32_I24:
    case Opcode::V_MUL_U32_U24:
        return V_MUL_I32_I24(inst);
    case Opcode::V_MAD_U32_U24:
        return V_MAD_U32_U24(inst);
    case Opcode::V_BCNT_U32_B32:
        return V_BCNT_U32_B32(inst);
    case Opcode::V_MUL_LO_U32:
        return V_MUL_LO_U32(inst);
    case Opcode::V_MIN_U32:
        return V_MIN_U32(inst);
    case Opcode::V_CMP_NE_U64:
        return V_CMP_NE_U64(inst);
    case Opcode::V_READFIRSTLANE_B32:
        return V_READFIRSTLANE_B32(inst);
    case Opcode::V_READLANE_B32:
        return V_READLANE_B32(inst);
    case Opcode::V_WRITELANE_B32:
        return V_WRITELANE_B32(inst);

    case Opcode::V_MAD_F32:
        return V_MAD_F32(inst);
    case Opcode::V_MAC_F32:
        return V_MAC_F32(inst);
    case Opcode::V_MUL_F32:
        return V_MUL_F32(inst);
    case Opcode::V_RCP_F32:
        return V_RCP_F32(inst);
    case Opcode::V_LDEXP_F32:
        return V_LDEXP_F32(inst);
    case Opcode::V_FRACT_F32:
        return V_FRACT_F32(inst);
    case Opcode::V_ADD_F32:
        return V_ADD_F32(inst);
    case Opcode::V_MED3_F32:
        return V_MED3_F32(inst);
    case Opcode::V_MED3_I32:
        return V_MED3_I32(inst);
    case Opcode::V_FLOOR_F32:
        return V_FLOOR_F32(inst);
    case Opcode::V_SUB_F32:
        return V_SUB_F32(inst);
    case Opcode::V_FMA_F32:
    case Opcode::V_MADAK_F32:
        return V_FMA_F32(inst);
    case Opcode::V_MAX_F32:
        return V_MAX_F32(inst);
    case Opcode::V_MAX_F64:
        return V_MAX_F64(inst);
    case Opcode::V_RSQ_F32:
        return V_RSQ_F32(inst);
    case Opcode::V_SIN_F32:
        return V_SIN_F32(inst);
    case Opcode::V_COS_F32:
        return V_COS_F32(inst);
    case Opcode::V_LOG_F32:
        return V_LOG_F32(inst);
    case Opcode::V_EXP_F32:
        return V_EXP_F32(inst);
    case Opcode::V_SQRT_F32:
        return V_SQRT_F32(inst);
    case Opcode::V_MIN_F32:
        return V_MIN_F32(inst, false);
    case Opcode::V_MIN3_F32:
        return V_MIN3_F32(inst);
    case Opcode::V_MIN3_I32:
        return V_MIN3_I32(inst);
    case Opcode::V_MIN_LEGACY_F32:
        return V_MIN_F32(inst, true);
    case Opcode::V_MADMK_F32:
        return V_MADMK_F32(inst);
    case Opcode::V_SUBREV_F32:
        return V_SUBREV_F32(inst);
    case Opcode::V_RNDNE_F32:
        return V_RNDNE_F32(inst);
    case Opcode::V_MAX3_F32:
        return V_MAX3_F32(inst);
    case Opcode::V_MAX3_U32:
        return V_MAX3_U32(false, inst);
    case Opcode::V_MAX3_I32:
        return V_MAX_U32(true, inst);
    case Opcode::V_TRUNC_F32:
        return V_TRUNC_F32(inst);
    case Opcode::V_CEIL_F32:
        return V_CEIL_F32(inst);
    case Opcode::V_MUL_LEGACY_F32:
        return V_MUL_F32(inst);
    case Opcode::V_MAC_LEGACY_F32:
        return V_MAC_F32(inst);
    case Opcode::V_MAD_LEGACY_F32:
        return V_MAD_F32(inst);
    case Opcode::V_MAX_LEGACY_F32:
        return V_MAX_F32(inst, true);
    case Opcode::V_RSQ_LEGACY_F32:
    case Opcode::V_RSQ_CLAMP_F32:
        return V_RSQ_F32(inst);
    case Opcode::V_RCP_IFLAG_F32:
        return V_RCP_F32(inst);

    case Opcode::V_CMPX_F_F32:
        return V_CMP_F32(ConditionOp::F, true, inst);
    case Opcode::V_CMPX_LT_F32:
        return V_CMP_F32(ConditionOp::LT, true, inst);
    case Opcode::V_CMPX_EQ_F32:
        return V_CMP_F32(ConditionOp::EQ, true, inst);
    case Opcode::V_CMPX_LE_F32:
        return V_CMP_F32(ConditionOp::LE, true, inst);
    case Opcode::V_CMPX_GT_F32:
        return V_CMP_F32(ConditionOp::GT, true, inst);
    case Opcode::V_CMPX_LG_F32:
        return V_CMP_F32(ConditionOp::LG, true, inst);
    case Opcode::V_CMPX_GE_F32:
        return V_CMP_F32(ConditionOp::GE, true, inst);
    case Opcode::V_CMPX_NGE_F32:
        return V_CMP_F32(ConditionOp::LT, true, inst);
    case Opcode::V_CMPX_NLG_F32:
        return V_CMP_F32(ConditionOp::EQ, true, inst);
    case Opcode::V_CMPX_NGT_F32:
        return V_CMP_F32(ConditionOp::LE, true, inst);
    case Opcode::V_CMPX_NLE_F32:
        return V_CMP_F32(ConditionOp::GT, true, inst);
    case Opcode::V_CMPX_NEQ_F32:
        return V_CMP_F32(ConditionOp::LG, true, inst);
    case Opcode::V_CMPX_NLT_F32:
        return V_CMP_F32(ConditionOp::GE, true, inst);
    case Opcode::V_CMPX_TRU_F32:
        return V_CMP_F32(ConditionOp::TRU, true, inst);
    case Opcode::V_CMP_CLASS_F32:
        return V_CMP_CLASS_F32(inst);

    case Opcode::V_CMP_LE_U32:
        return V_CMP_U32(ConditionOp::LE, false, false, inst);
    case Opcode::V_CMP_GT_I32:
        return V_CMP_U32(ConditionOp::GT, true, false, inst);
    case Opcode::V_CMP_LT_I32:
        return V_CMP_U32(ConditionOp::LT, true, false, inst);
    case Opcode::V_CMPX_GT_I32:
        return V_CMP_U32(ConditionOp::GT, true, true, inst);
    case Opcode::V_CMPX_LT_I32:
        return V_CMP_U32(ConditionOp::LT, true, true, inst);
    case Opcode::V_CMPX_F_U32:
        return V_CMP_U32(ConditionOp::F, false, true, inst);
    case Opcode::V_CMPX_LT_U32:
        return V_CMP_U32(ConditionOp::LT, false, true, inst);
    case Opcode::V_CMPX_EQ_U32:
        return V_CMP_U32(ConditionOp::EQ, false, true, inst);
    case Opcode::V_CMPX_LE_U32:
        return V_CMP_U32(ConditionOp::LE, false, true, inst);
    case Opcode::V_CMPX_GT_U32:
        return V_CMP_U32(ConditionOp::GT, false, true, inst);
    case Opcode::V_CMPX_NE_U32:
        return V_CMP_U32(ConditionOp::LG, false, true, inst);
    case Opcode::V_CMPX_GE_U32:
        return V_CMP_U32(ConditionOp::GE, false, true, inst);
    case Opcode::V_CMPX_TRU_U32:
        return V_CMP_U32(ConditionOp::TRU, false, true, inst);
    case Opcode::V_CMPX_LG_I32:
        return V_CMP_U32(ConditionOp::LG, true, true, inst);

    case Opcode::V_MBCNT_LO_U32_B32:
        return V_MBCNT_U32_B32(true, inst);
    case Opcode::V_MBCNT_HI_U32_B32:
        return V_MBCNT_U32_B32(false, inst);
    case Opcode::V_MOVRELS_B32:
        return V_MOVRELS_B32(inst);
    case Opcode::V_MOVRELD_B32:
        return V_MOVRELD_B32(inst);
    case Opcode::V_MOVRELSD_B32:
        return V_MOVRELSD_B32(inst);
    case Opcode::V_NOP:
        return;

    case Opcode::V_BFM_B32:
        return V_BFM_B32(inst);
    case Opcode::V_FFBH_U32:
        return V_FFBH_U32(inst);
    default:
        LogMissingOpcode(inst);
    }
}

void Translator::V_MOV(const GcnInst& inst) {
    SetDst(inst.dst[0], GetSrc<IR::F32>(inst.src[0]));
}

void Translator::V_SAD(const GcnInst& inst) {
    const IR::U32 abs_diff = ir.IAbs(ir.ISub(GetSrc(inst.src[0]), GetSrc(inst.src[1])));
    SetDst(inst.dst[0], ir.IAdd(abs_diff, GetSrc(inst.src[2])));
}

void Translator::V_MAC_F32(const GcnInst& inst) {
    SetDst(inst.dst[0], ir.FPFma(GetSrc<IR::F32>(inst.src[0]), GetSrc<IR::F32>(inst.src[1]),
                                 GetSrc<IR::F32>(inst.dst[0])));
}

void Translator::V_CVT_PKRTZ_F16_F32(const GcnInst& inst) {
    const IR::VectorReg dst_reg{inst.dst[0].code};
    const IR::Value vec_f32 =
        ir.CompositeConstruct(GetSrc<IR::F32>(inst.src[0]), GetSrc<IR::F32>(inst.src[1]));
    ir.SetVectorReg(dst_reg, ir.PackHalf2x16(vec_f32));
}

void Translator::V_CVT_F32_F16(const GcnInst& inst) {
    const IR::U32 src0 = GetSrc(inst.src[0]);
    const IR::U16 src0l = ir.UConvert(16, src0);
    SetDst(inst.dst[0], ir.FPConvert(32, ir.BitCast<IR::F16>(src0l)));
}

void Translator::V_CVT_F16_F32(const GcnInst& inst) {
    const IR::F32 src0 = GetSrc<IR::F32>(inst.src[0]);
    const IR::F16 src0fp16 = ir.FPConvert(16, src0);
    SetDst(inst.dst[0], ir.UConvert(32, ir.BitCast<IR::U16>(src0fp16)));
}

void Translator::V_MUL_F32(const GcnInst& inst) {
    SetDst(inst.dst[0], ir.FPMul(GetSrc<IR::F32>(inst.src[0]), GetSrc<IR::F32>(inst.src[1])));
}

void Translator::V_CNDMASK_B32(const GcnInst& inst) {
    const IR::VectorReg dst_reg{inst.dst[0].code};
    const IR::ScalarReg flag_reg{inst.src[2].code};
    const IR::U1 flag = inst.src[2].field == OperandField::ScalarGPR
                            ? ir.GetThreadBitScalarReg(flag_reg)
                            : ir.GetVcc();
    const IR::Value result =
        ir.Select(flag, GetSrc<IR::F32>(inst.src[1]), GetSrc<IR::F32>(inst.src[0]));
    ir.SetVectorReg(dst_reg, IR::U32F32{result});
}

void Translator::V_OR_B32(bool is_xor, const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{ir.GetVectorReg(IR::VectorReg(inst.src[1].code))};
    const IR::VectorReg dst_reg{inst.dst[0].code};
    ir.SetVectorReg(dst_reg,
                    is_xor ? ir.BitwiseXor(src0, src1) : IR::U32(ir.BitwiseOr(src0, src1)));
}

void Translator::V_AND_B32(const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{ir.GetVectorReg(IR::VectorReg(inst.src[1].code))};
    const IR::VectorReg dst_reg{inst.dst[0].code};
    ir.SetVectorReg(dst_reg, ir.BitwiseAnd(src0, src1));
}

void Translator::V_LSHLREV_B32(const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{GetSrc(inst.src[1])};
    const IR::VectorReg dst_reg{inst.dst[0].code};
    ir.SetVectorReg(dst_reg, ir.ShiftLeftLogical(src1, ir.BitwiseAnd(src0, ir.Imm32(0x1F))));
}

void Translator::V_LSHL_B32(const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{GetSrc(inst.src[1])};
    SetDst(inst.dst[0], ir.ShiftLeftLogical(src0, ir.BitwiseAnd(src1, ir.Imm32(0x1F))));
}

void Translator::V_LSHL_B64(const GcnInst& inst) {
    const IR::U64 src0{GetSrc64(inst.src[0])};
    const IR::U64 src1{GetSrc64(inst.src[1])};
    const IR::VectorReg dst_reg{inst.dst[0].code};
    ASSERT_MSG(src0.IsImmediate() && src0.U64() == 0 && src1.IsImmediate() && src1.U64() == 0,
               "V_LSHL_B64 with non-zero src0 or src1 is not supported");
    ir.SetVectorReg(dst_reg, ir.Imm32(0));
    ir.SetVectorReg(dst_reg + 1, ir.Imm32(0));
}

void Translator::V_ADD_I32(const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{ir.GetVectorReg(IR::VectorReg(inst.src[1].code))};
    const IR::VectorReg dst_reg{inst.dst[0].code};
    ir.SetVectorReg(dst_reg, ir.IAdd(src0, src1));
    // TODO: Carry
}

void Translator::V_ADDC_U32(const GcnInst& inst) {

    const auto src0 = GetSrc<IR::U32>(inst.src[0]);
    const auto src1 = GetSrc<IR::U32>(inst.src[1]);

    IR::U1 carry;
    if (inst.src_count == 3) { // VOP3
        if (inst.src[2].field == OperandField::VccLo) {
            carry = ir.GetVcc();
        } else if (inst.src[2].field == OperandField::ScalarGPR) {
            carry = ir.GetThreadBitScalarReg(IR::ScalarReg(inst.src[2].code));
        } else {
            UNREACHABLE();
        }
    } else { // VOP2
        carry = ir.GetVcc();
    }

    const IR::U32 scarry = IR::U32{ir.Select(carry, ir.Imm32(1), ir.Imm32(0))};
    const IR::U32 result = ir.IAdd(ir.IAdd(src0, src1), scarry);

    const IR::VectorReg dst_reg{inst.dst[0].code};
    ir.SetVectorReg(dst_reg, result);

    const IR::U1 less_src0 = ir.ILessThan(result, src0, false);
    const IR::U1 less_src1 = ir.ILessThan(result, src1, false);
    const IR::U1 did_overflow = ir.LogicalOr(less_src0, less_src1);
    ir.SetVcc(did_overflow);
}

void Translator::V_CVT_F32_I32(const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::VectorReg dst_reg{inst.dst[0].code};
    ir.SetVectorReg(dst_reg, ir.ConvertSToF(32, 32, src0));
}

void Translator::V_CVT_F32_U32(const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::VectorReg dst_reg{inst.dst[0].code};
    ir.SetVectorReg(dst_reg, ir.ConvertUToF(32, 32, src0));
}

void Translator::V_MAD_F32(const GcnInst& inst) {
    const IR::F32 src0{GetSrc<IR::F32>(inst.src[0])};
    const IR::F32 src1{GetSrc<IR::F32>(inst.src[1])};
    const IR::F32 src2{GetSrc<IR::F32>(inst.src[2])};
    SetDst(inst.dst[0], ir.FPFma(src0, src1, src2));
}

void Translator::V_FRACT_F32(const GcnInst& inst) {
    const IR::F32 src0{GetSrc<IR::F32>(inst.src[0])};
    const IR::VectorReg dst_reg{inst.dst[0].code};
    ir.SetVectorReg(dst_reg, ir.Fract(src0));
}

void Translator::V_ADD_F32(const GcnInst& inst) {
    const IR::F32 src0{GetSrc<IR::F32>(inst.src[0])};
    const IR::F32 src1{GetSrc<IR::F32>(inst.src[1])};
    SetDst(inst.dst[0], ir.FPAdd(src0, src1));
}

void Translator::V_CVT_OFF_F32_I4(const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::VectorReg dst_reg{inst.dst[0].code};
    ASSERT(src0.IsImmediate());
    static constexpr std::array IntToFloat = {
        0.0f,     0.0625f,  0.1250f,  0.1875f,  0.2500f,  0.3125f,  0.3750f,  0.4375f,
        -0.5000f, -0.4375f, -0.3750f, -0.3125f, -0.2500f, -0.1875f, -0.1250f, -0.0625f};
    ir.SetVectorReg(dst_reg, ir.Imm32(IntToFloat[src0.U32() & 0xF]));
}

void Translator::V_MED3_F32(const GcnInst& inst) {
    const IR::F32 src0{GetSrc<IR::F32>(inst.src[0])};
    const IR::F32 src1{GetSrc<IR::F32>(inst.src[1])};
    const IR::F32 src2{GetSrc<IR::F32>(inst.src[2])};
    const IR::F32 mmx = ir.FPMin(ir.FPMax(src0, src1), src2);
    SetDst(inst.dst[0], ir.FPMax(ir.FPMin(src0, src1), mmx));
}

void Translator::V_MED3_I32(const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{GetSrc(inst.src[1])};
    const IR::U32 src2{GetSrc(inst.src[2])};
    const IR::U32 mmx = ir.SMin(ir.SMax(src0, src1), src2);
    SetDst(inst.dst[0], ir.SMax(ir.SMin(src0, src1), mmx));
}

void Translator::V_FLOOR_F32(const GcnInst& inst) {
    const IR::F32 src0{GetSrc<IR::F32>(inst.src[0])};
    const IR::VectorReg dst_reg{inst.dst[0].code};
    ir.SetVectorReg(dst_reg, ir.FPFloor(src0));
}

void Translator::V_SUB_F32(const GcnInst& inst) {
    const IR::F32 src0{GetSrc<IR::F32>(inst.src[0])};
    const IR::F32 src1{GetSrc<IR::F32>(inst.src[1])};
    SetDst(inst.dst[0], ir.FPSub(src0, src1));
}

void Translator::V_RCP_F32(const GcnInst& inst) {
    const IR::F32 src0{GetSrc<IR::F32>(inst.src[0])};
    SetDst(inst.dst[0], ir.FPRecip(src0));
}

void Translator::V_FMA_F32(const GcnInst& inst) {
    const IR::F32 src0{GetSrc<IR::F32>(inst.src[0])};
    const IR::F32 src1{GetSrc<IR::F32>(inst.src[1])};
    const IR::F32 src2{GetSrc<IR::F32>(inst.src[2])};
    SetDst(inst.dst[0], ir.FPFma(src0, src1, src2));
}

void Translator::V_CMP_F32(ConditionOp op, bool set_exec, const GcnInst& inst) {
    const IR::F32 src0{GetSrc<IR::F32>(inst.src[0])};
    const IR::F32 src1{GetSrc<IR::F32>(inst.src[1])};
    const IR::U1 result = [&] {
        switch (op) {
        case ConditionOp::F:
            return ir.Imm1(false);
        case ConditionOp::EQ:
            return ir.FPEqual(src0, src1);
        case ConditionOp::LG:
            return ir.FPNotEqual(src0, src1);
        case ConditionOp::GT:
            return ir.FPGreaterThan(src0, src1);
        case ConditionOp::LT:
            return ir.FPLessThan(src0, src1);
        case ConditionOp::LE:
            return ir.FPLessThanEqual(src0, src1);
        case ConditionOp::GE:
            return ir.FPGreaterThanEqual(src0, src1);
        case ConditionOp::U:
            return ir.LogicalNot(ir.LogicalAnd(ir.FPIsNan(src0), ir.FPIsNan(src1)));
        default:
            UNREACHABLE();
        }
    }();
    if (set_exec) {
        ir.SetExec(result);
    }

    switch (inst.dst[1].field) {
    case OperandField::VccLo:
        ir.SetVcc(result);
        break;
    case OperandField::ScalarGPR:
        ir.SetThreadBitScalarReg(IR::ScalarReg(inst.dst[1].code), result);
        break;
    default:
        UNREACHABLE();
    }
}

void Translator::V_MAX_F32(const GcnInst& inst, bool is_legacy) {
    const IR::F32 src0{GetSrc<IR::F32>(inst.src[0])};
    const IR::F32 src1{GetSrc<IR::F32>(inst.src[1])};
    SetDst(inst.dst[0], ir.FPMax(src0, src1, is_legacy));
}

void Translator::V_MAX_F64(const GcnInst& inst) {
    const IR::F64 src0{GetSrc64<IR::F64>(inst.src[0])};
    const IR::F64 src1{GetSrc64<IR::F64>(inst.src[1])};
    SetDst64(inst.dst[0], ir.FPMax(src0, src1));
}

void Translator::V_MAX_U32(bool is_signed, const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{GetSrc(inst.src[1])};
    SetDst(inst.dst[0], ir.IMax(src0, src1, is_signed));
}

void Translator::V_RSQ_F32(const GcnInst& inst) {
    const IR::F32 src0{GetSrc<IR::F32>(inst.src[0])};
    SetDst(inst.dst[0], ir.FPRecipSqrt(src0));
}

void Translator::V_SIN_F32(const GcnInst& inst) {
    const IR::F32 src0{GetSrc<IR::F32>(inst.src[0])};
    SetDst(inst.dst[0], ir.FPSin(src0));
}

void Translator::V_LOG_F32(const GcnInst& inst) {
    const IR::F32 src0{GetSrc<IR::F32>(inst.src[0])};
    SetDst(inst.dst[0], ir.FPLog2(src0));
}

void Translator::V_EXP_F32(const GcnInst& inst) {
    const IR::F32 src0{GetSrc<IR::F32>(inst.src[0])};
    SetDst(inst.dst[0], ir.FPExp2(src0));
}

void Translator::V_SQRT_F32(const GcnInst& inst) {
    const IR::F32 src0{GetSrc<IR::F32>(inst.src[0])};
    SetDst(inst.dst[0], ir.FPSqrt(src0));
}

void Translator::V_MIN_F32(const GcnInst& inst, bool is_legacy) {
    const IR::F32 src0{GetSrc<IR::F32>(inst.src[0])};
    const IR::F32 src1{GetSrc<IR::F32>(inst.src[1])};
    SetDst(inst.dst[0], ir.FPMin(src0, src1, is_legacy));
}

void Translator::V_MIN3_F32(const GcnInst& inst) {
    const IR::F32 src0{GetSrc<IR::F32>(inst.src[0])};
    const IR::F32 src1{GetSrc<IR::F32>(inst.src[1])};
    const IR::F32 src2{GetSrc<IR::F32>(inst.src[2])};
    SetDst(inst.dst[0], ir.FPMin(src0, ir.FPMin(src1, src2)));
}

void Translator::V_MIN3_I32(const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{GetSrc(inst.src[1])};
    const IR::U32 src2{GetSrc(inst.src[2])};
    SetDst(inst.dst[0], ir.SMin(src0, ir.SMin(src1, src2)));
}

void Translator::V_MADMK_F32(const GcnInst& inst) {
    const IR::F32 src0{GetSrc<IR::F32>(inst.src[0])};
    const IR::F32 src1{GetSrc<IR::F32>(inst.src[1])};
    const IR::F32 k{GetSrc<IR::F32>(inst.src[2])};
    SetDst(inst.dst[0], ir.FPFma(src0, k, src1));
}

void Translator::V_CUBEMA_F32(const GcnInst& inst) {
    SetDst(inst.dst[0], ir.Imm32(1.f));
}

void Translator::V_CUBESC_F32(const GcnInst& inst) {
    SetDst(inst.dst[0], GetSrc<IR::F32>(inst.src[0]));
}

void Translator::V_CUBETC_F32(const GcnInst& inst) {
    SetDst(inst.dst[0], GetSrc<IR::F32>(inst.src[1]));
}

void Translator::V_CUBEID_F32(const GcnInst& inst) {
    SetDst(inst.dst[0], GetSrc<IR::F32>(inst.src[2]));
}

void Translator::V_CVT_U32_F32(const GcnInst& inst) {
    const IR::F32 src0{GetSrc<IR::F32>(inst.src[0])};
    SetDst(inst.dst[0], ir.ConvertFToU(32, src0));
}

void Translator::V_SUBREV_F32(const GcnInst& inst) {
    const IR::F32 src0{GetSrc<IR::F32>(inst.src[0])};
    const IR::F32 src1{GetSrc<IR::F32>(inst.src[1])};
    SetDst(inst.dst[0], ir.FPSub(src1, src0));
}

void Translator::V_SUBREV_I32(const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{GetSrc(inst.src[1])};
    SetDst(inst.dst[0], ir.ISub(src1, src0));
    // TODO: Carry-out
}

void Translator::V_MAD_U64_U32(const GcnInst& inst) {
    const auto src0 = GetSrc<IR::U32>(inst.src[0]);
    const auto src1 = GetSrc<IR::U32>(inst.src[1]);
    const auto src2 = GetSrc64<IR::U64>(inst.src[2]);

    // const IR::U64 mul_result = ir.UConvert(64, ir.IMul(src0, src1));
    const IR::U64 mul_result =
        ir.PackUint2x32(ir.CompositeConstruct(ir.IMul(src0, src1), ir.Imm32(0U)));
    const IR::U64 sum_result = ir.IAdd(mul_result, src2);

    SetDst64(inst.dst[0], sum_result);

    const IR::U1 less_src0 = ir.ILessThan(sum_result, mul_result, false);
    const IR::U1 less_src1 = ir.ILessThan(sum_result, src2, false);
    const IR::U1 did_overflow = ir.LogicalOr(less_src0, less_src1);
    ir.SetVcc(did_overflow);
}

void Translator::V_CMP_U32(ConditionOp op, bool is_signed, bool set_exec, const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{GetSrc(inst.src[1])};
    const IR::U1 result = [&] {
        switch (op) {
        case ConditionOp::F:
            return ir.Imm1(false);
        case ConditionOp::TRU:
            return ir.Imm1(true);
        case ConditionOp::EQ:
            return ir.IEqual(src0, src1);
        case ConditionOp::LG:
            return ir.INotEqual(src0, src1);
        case ConditionOp::GT:
            return ir.IGreaterThan(src0, src1, is_signed);
        case ConditionOp::LT:
            return ir.ILessThan(src0, src1, is_signed);
        case ConditionOp::LE:
            return ir.ILessThanEqual(src0, src1, is_signed);
        case ConditionOp::GE:
            return ir.IGreaterThanEqual(src0, src1, is_signed);
        default:
            UNREACHABLE();
        }
    }();
    if (set_exec) {
        ir.SetExec(result);
    }
    switch (inst.dst[1].field) {
    case OperandField::VccLo:
        return ir.SetVcc(result);
    case OperandField::ScalarGPR:
        return ir.SetThreadBitScalarReg(IR::ScalarReg(inst.dst[0].code), result);
    default:
        UNREACHABLE();
    }
}

void Translator::V_LSHRREV_B32(const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{GetSrc(inst.src[1])};
    SetDst(inst.dst[0], ir.ShiftRightLogical(src1, ir.BitwiseAnd(src0, ir.Imm32(0x1F))));
}

void Translator::V_MUL_HI_U32(bool is_signed, const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{GetSrc(inst.src[1])};
    const IR::U32 hi{ir.CompositeExtract(ir.IMulExt(src0, src1, is_signed), 1)};
    SetDst(inst.dst[0], hi);
}

void Translator::V_SAD_U32(const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{GetSrc(inst.src[1])};
    const IR::U32 src2{GetSrc(inst.src[2])};
    IR::U32 result;
    if (src0.IsImmediate() && src0.U32() == 0U) {
        result = src1;
    } else if (src1.IsImmediate() && src1.U32() == 0U) {
        result = src0;
    } else {
        const IR::U32 max{ir.IMax(src0, src1, false)};
        const IR::U32 min{ir.IMin(src0, src1, false)};
        result = ir.ISub(max, min);
    }
    SetDst(inst.dst[0], ir.IAdd(result, src2));
}

void Translator::V_BFE_U32(bool is_signed, const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{ir.BitwiseAnd(GetSrc(inst.src[1]), ir.Imm32(0x1F))};
    const IR::U32 src2{ir.BitwiseAnd(GetSrc(inst.src[2]), ir.Imm32(0x1F))};
    SetDst(inst.dst[0], ir.BitFieldExtract(src0, src1, src2, is_signed));
}

void Translator::V_MAD_I32_I24(const GcnInst& inst, bool is_signed) {
    const IR::U32 src0{
        ir.BitFieldExtract(GetSrc(inst.src[0]), ir.Imm32(0), ir.Imm32(24), is_signed)};
    const IR::U32 src1{
        ir.BitFieldExtract(GetSrc(inst.src[1]), ir.Imm32(0), ir.Imm32(24), is_signed)};
    const IR::U32 src2{GetSrc(inst.src[2])};
    SetDst(inst.dst[0], ir.IAdd(ir.IMul(src0, src1), src2));
}

void Translator::V_MUL_I32_I24(const GcnInst& inst) {
    const IR::U32 src0{ir.BitFieldExtract(GetSrc(inst.src[0]), ir.Imm32(0), ir.Imm32(24), true)};
    const IR::U32 src1{ir.BitFieldExtract(GetSrc(inst.src[1]), ir.Imm32(0), ir.Imm32(24), true)};
    SetDst(inst.dst[0], ir.IMul(src0, src1));
}

void Translator::V_SUB_I32(const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{GetSrc(inst.src[1])};
    SetDst(inst.dst[0], ir.ISub(src0, src1));
}

void Translator::V_LSHR_B32(const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{GetSrc(inst.src[1])};
    SetDst(inst.dst[0], ir.ShiftRightLogical(src0, ir.BitwiseAnd(src1, ir.Imm32(0x1F))));
}

void Translator::V_ASHRREV_I32(const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{GetSrc(inst.src[1])};
    SetDst(inst.dst[0], ir.ShiftRightArithmetic(src1, ir.BitwiseAnd(src0, ir.Imm32(0x1F))));
}

void Translator::V_ASHR_I32(const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{GetSrc(inst.src[1])};
    SetDst(inst.dst[0], ir.ShiftRightArithmetic(src0, ir.BitwiseAnd(src1, ir.Imm32(0x1F))));
}

void Translator::V_MAD_U32_U24(const GcnInst& inst) {
    V_MAD_I32_I24(inst, false);
}

void Translator::V_RNDNE_F32(const GcnInst& inst) {
    const IR::F32 src0{GetSrc<IR::F32>(inst.src[0])};
    SetDst(inst.dst[0], ir.FPRoundEven(src0));
}

void Translator::V_BCNT_U32_B32(const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{GetSrc(inst.src[1])};
    SetDst(inst.dst[0], ir.IAdd(ir.BitCount(src0), src1));
}

void Translator::V_COS_F32(const GcnInst& inst) {
    const IR::F32 src0{GetSrc<IR::F32>(inst.src[0])};
    SetDst(inst.dst[0], ir.FPCos(src0));
}

void Translator::V_MAX3_F32(const GcnInst& inst) {
    const IR::F32 src0{GetSrc<IR::F32>(inst.src[0])};
    const IR::F32 src1{GetSrc<IR::F32>(inst.src[1])};
    const IR::F32 src2{GetSrc<IR::F32>(inst.src[2])};
    SetDst(inst.dst[0], ir.FPMax(src0, ir.FPMax(src1, src2)));
}

void Translator::V_MAX3_U32(bool is_signed, const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{GetSrc(inst.src[1])};
    const IR::U32 src2{GetSrc(inst.src[2])};
    SetDst(inst.dst[0], ir.IMax(src0, ir.IMax(src1, src2, is_signed), is_signed));
}

void Translator::V_CVT_I32_F32(const GcnInst& inst) {
    const IR::F32 src0{GetSrc<IR::F32>(inst.src[0])};
    SetDst(inst.dst[0], ir.ConvertFToS(32, src0));
}

void Translator::V_MIN_I32(const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{GetSrc(inst.src[1])};
    SetDst(inst.dst[0], ir.SMin(src0, src1));
}

void Translator::V_MUL_LO_U32(const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{GetSrc(inst.src[1])};
    SetDst(inst.dst[0], ir.IMul(src0, src1));
}

void Translator::V_TRUNC_F32(const GcnInst& inst) {
    const IR::F32 src0{GetSrc<IR::F32>(inst.src[0])};
    SetDst(inst.dst[0], ir.FPTrunc(src0));
}

void Translator::V_CEIL_F32(const GcnInst& inst) {
    const IR::F32 src0{GetSrc<IR::F32>(inst.src[0])};
    SetDst(inst.dst[0], ir.FPCeil(src0));
}

void Translator::V_MIN_U32(const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{GetSrc(inst.src[1])};
    SetDst(inst.dst[0], ir.IMin(src0, src1, false));
}

void Translator::V_CMP_NE_U64(const GcnInst& inst) {
    const auto get_src = [&](const InstOperand& operand) {
        switch (operand.field) {
        case OperandField::VccLo:
            return ir.GetVcc();
        case OperandField::ExecLo:
            return ir.GetExec();
        case OperandField::ScalarGPR:
            return ir.GetThreadBitScalarReg(IR::ScalarReg(operand.code));
        case OperandField::ConstZero:
            return ir.Imm1(false);
        default:
            UNREACHABLE();
        }
    };
    const IR::U1 src0{get_src(inst.src[0])};
    ASSERT(inst.src[1].field == OperandField::ConstZero); // src0 != 0
    switch (inst.dst[1].field) {
    case OperandField::VccLo:
        ir.SetVcc(src0);
        break;
    case OperandField::ScalarGPR:
        ir.SetThreadBitScalarReg(IR::ScalarReg(inst.dst[1].code), src0);
        break;
    default:
        UNREACHABLE();
    }
}

void Translator::V_BFI_B32(const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{GetSrc(inst.src[1])};
    const IR::U32 src2{GetSrc(inst.src[2])};
    SetDst(inst.dst[0],
           ir.BitwiseOr(ir.BitwiseAnd(src0, src1), ir.BitwiseAnd(ir.BitwiseNot(src0), src2)));
}

void Translator::V_NOT_B32(const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    SetDst(inst.dst[0], ir.BitwiseNot(src0));
}

void Translator::V_CVT_F32_UBYTE(u32 index, const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 byte = ir.BitFieldExtract(src0, ir.Imm32(8 * index), ir.Imm32(8));
    SetDst(inst.dst[0], ir.ConvertUToF(32, 32, byte));
}

void Translator::V_BFREV_B32(const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    SetDst(inst.dst[0], ir.BitReverse(src0));
}

void Translator::V_LDEXP_F32(const GcnInst& inst) {
    const IR::F32 src0{GetSrc<IR::F32>(inst.src[0])};
    const IR::U32 src1{GetSrc(inst.src[1])};
    SetDst(inst.dst[0], ir.FPLdexp(src0, src1));
}

void Translator::V_CVT_FLR_I32_F32(const GcnInst& inst) {
    const IR::F32 src0{GetSrc<IR::F32>(inst.src[0])};
    SetDst(inst.dst[0], ir.ConvertFToI(32, true, ir.FPFloor(src0)));
}

void Translator::V_CMP_CLASS_F32(const GcnInst& inst) {
    const IR::F32 src0{GetSrc<IR::F32>(inst.src[0])};
    const IR::U32 src1{GetSrc(inst.src[1])};
    IR::U1 value;
    if (src1.IsImmediate()) {
        const auto class_mask = static_cast<IR::FloatClassFunc>(src1.U32());
        if ((class_mask & IR::FloatClassFunc::NaN) == IR::FloatClassFunc::NaN) {
            value = ir.FPIsNan(src0);
        } else if ((class_mask & IR::FloatClassFunc::Infinity) == IR::FloatClassFunc::Infinity) {
            value = ir.FPIsInf(src0);
        } else {
            UNREACHABLE();
        }
    } else {
        // We don't know the type yet, delay its resolution.
        value = ir.FPCmpClass32(src0, src1);
    }

    switch (inst.dst[1].field) {
    case OperandField::VccLo:
        return ir.SetVcc(value);
    default:
        UNREACHABLE();
    }
}

void Translator::V_FFBL_B32(const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    SetDst(inst.dst[0], ir.FindILsb(src0));
}

void Translator::V_MBCNT_U32_B32(bool is_low, const GcnInst& inst) {
    if (!is_low) {
        // v_mbcnt_hi_u32_b32 v2, -1, 0
        if (inst.src[0].field == OperandField::SignedConstIntNeg && inst.src[0].code == 193 &&
            inst.src[1].field == OperandField::ConstZero) {
            return;
        }
        // v_mbcnt_hi_u32_b32 vX, exec_hi, 0
        if (inst.src[0].field == OperandField::ExecHi &&
            inst.src[1].field == OperandField::ConstZero) {
            return;
        }
    } else {
        // v_mbcnt_lo_u32_b32 v2, -1, vX
        // used combined with above to fetch lane id in non-compute stages
        if (inst.src[0].field == OperandField::SignedConstIntNeg && inst.src[0].code == 193) {
            SetDst(inst.dst[0], ir.LaneId());
        }
        // v_mbcnt_lo_u32_b32 v20, exec_lo, vX
        // used combined in above for append buffer indexing.
        if (inst.src[0].field == OperandField::ExecLo) {
            SetDst(inst.dst[0], ir.Imm32(0));
        }
    }
}

void Translator::V_BFM_B32(const GcnInst& inst) {
    // bitmask width
    const IR::U32 src0{ir.BitFieldExtract(GetSrc(inst.src[0]), ir.Imm32(0), ir.Imm32(4))};
    // bitmask offset
    const IR::U32 src1{ir.BitFieldExtract(GetSrc(inst.src[1]), ir.Imm32(0), ir.Imm32(4))};
    const IR::U32 ones = ir.ISub(ir.ShiftLeftLogical(ir.Imm32(1), src0), ir.Imm32(1));
    SetDst(inst.dst[0], ir.ShiftLeftLogical(ones, src1));
}

void Translator::V_FFBH_U32(const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    // Gcn wants the MSB position counting from the left, but SPIR-V counts from the rightmost (LSB)
    // position
    const IR::U32 msb_pos = ir.FindUMsb(src0);
    const IR::U32 pos_from_left = ir.ISub(ir.Imm32(31), msb_pos);
    // Select 0xFFFFFFFF if src0 was 0
    const IR::U1 cond = ir.INotEqual(src0, ir.Imm32(0));
    SetDst(inst.dst[0], IR::U32{ir.Select(cond, pos_from_left, ir.Imm32(~0U))});
}

// TODO: add range analysis pass to hopefully put an upper bound on m0, and only select one of
// [src_vgprno, src_vgprno + max_m0]. Same for dst regs we may write back to

IR::U32 Translator::VMovRelSHelper(u32 src_vgprno, const IR::U32 m0) {
    // Read from VGPR0 by default when src_vgprno + m0 > num_allocated_vgprs
    IR::U32 src_val = ir.GetVectorReg<IR::U32>(IR::VectorReg::V0);
    for (u32 i = src_vgprno; i < runtime_info.num_allocated_vgprs; i++) {
        const IR::U1 cond = ir.IEqual(m0, ir.Imm32(i - src_vgprno));
        src_val =
            IR::U32{ir.Select(cond, ir.GetVectorReg<IR::U32>(IR::VectorReg::V0 + i), src_val)};
    }
    return src_val;
}

void Translator::VMovRelDHelper(u32 dst_vgprno, const IR::U32 src_val, const IR::U32 m0) {
    for (u32 i = dst_vgprno; i < runtime_info.num_allocated_vgprs; i++) {
        const IR::U1 cond = ir.IEqual(m0, ir.Imm32(i - dst_vgprno));
        const IR::U32 dst_val =
            IR::U32{ir.Select(cond, src_val, ir.GetVectorReg<IR::U32>(IR::VectorReg::V0 + i))};
        ir.SetVectorReg(IR::VectorReg::V0 + i, dst_val);
    }
}

void Translator::V_MOVRELS_B32(const GcnInst& inst) {
    u32 src_vgprno = inst.src[0].code - static_cast<u32>(IR::VectorReg::V0);
    const IR::U32 m0 = ir.GetM0();

    const IR::U32 src_val = VMovRelSHelper(src_vgprno, m0);
    SetDst(inst.dst[0], src_val);
}

void Translator::V_MOVRELD_B32(const GcnInst& inst) {
    const IR::U32 src_val{GetSrc(inst.src[0])};
    u32 dst_vgprno = inst.dst[0].code - static_cast<u32>(IR::VectorReg::V0);
    IR::U32 m0 = ir.GetM0();

    VMovRelDHelper(dst_vgprno, src_val, m0);
}

void Translator::V_MOVRELSD_B32(const GcnInst& inst) {
    u32 src_vgprno = inst.src[0].code - static_cast<u32>(IR::VectorReg::V0);
    u32 dst_vgprno = inst.dst[0].code - static_cast<u32>(IR::VectorReg::V0);
    IR::U32 m0 = ir.GetM0();

    const IR::U32 src_val = VMovRelSHelper(src_vgprno, m0);
    VMovRelDHelper(dst_vgprno, src_val, m0);
}

} // namespace Shader::Gcn
