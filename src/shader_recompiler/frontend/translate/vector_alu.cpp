// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/frontend/translate/translate.h"

namespace Shader::Gcn {

void Translator::V_MOV(const GcnInst& inst) {
    SetDst(inst.dst[0], GetSrc(inst.src[0]));
}

void Translator::V_SAD(const GcnInst& inst) {
    const IR::U32 abs_diff = ir.IAbs(ir.ISub(GetSrc(inst.src[0]), GetSrc(inst.src[1])));
    SetDst(inst.dst[0], ir.IAdd(abs_diff, GetSrc(inst.src[2])));
}

void Translator::V_MAC_F32(const GcnInst& inst) {
    SetDst(inst.dst[0], ir.FPFma(GetSrc(inst.src[0], true), GetSrc(inst.src[1], true),
                                 GetSrc(inst.dst[0], true)));
}

void Translator::V_CVT_PKRTZ_F16_F32(const GcnInst& inst) {
    const IR::VectorReg dst_reg{inst.dst[0].code};
    const IR::Value vec_f32 =
        ir.CompositeConstruct(GetSrc(inst.src[0], true), GetSrc(inst.src[1], true));
    ir.SetVectorReg(dst_reg, ir.PackHalf2x16(vec_f32));
}

void Translator::V_CVT_F32_F16(const GcnInst& inst) {
    const IR::U32 src0 = GetSrc(inst.src[0]);
    const IR::U16 src0l = ir.UConvert(16, src0);
    SetDst(inst.dst[0], ir.FPConvert(32, ir.BitCast<IR::F16>(src0l)));
}

void Translator::V_MUL_F32(const GcnInst& inst) {
    SetDst(inst.dst[0], ir.FPMul(GetSrc(inst.src[0], true), GetSrc(inst.src[1], true)));
}

void Translator::V_CNDMASK_B32(const GcnInst& inst) {
    const IR::VectorReg dst_reg{inst.dst[0].code};
    const IR::ScalarReg flag_reg{inst.src[2].code};
    const IR::U1 flag = inst.src[2].field == OperandField::ScalarGPR
                            ? ir.GetThreadBitScalarReg(flag_reg)
                            : ir.GetVcc();

    // We can treat the instruction as integer most of the time, but when a source is
    // a floating point constant we will force the other as float for better readability
    // The other operand is also higly likely to be float as well.
    const auto is_float_const = [](OperandField field) {
        return field >= OperandField::ConstFloatPos_0_5 && field <= OperandField::ConstFloatNeg_4_0;
    };
    const bool has_flt_source =
        is_float_const(inst.src[0].field) || is_float_const(inst.src[1].field);
    IR::U32F32 src0 = GetSrc(inst.src[0], has_flt_source);
    IR::U32F32 src1 = GetSrc(inst.src[1], has_flt_source);
    if (src0.Type() == IR::Type::F32 && src1.Type() == IR::Type::U32) {
        src1 = ir.BitCast<IR::F32, IR::U32>(src1);
    }
    if (src1.Type() == IR::Type::F32 && src0.Type() == IR::Type::U32) {
        src0 = ir.BitCast<IR::F32, IR::U32>(src0);
    }
    const IR::Value result = ir.Select(flag, src1, src0);
    ir.SetVectorReg(dst_reg, IR::U32F32{result});
}

void Translator::V_OR_B32(bool is_xor, const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{ir.GetVectorReg(IR::VectorReg(inst.src[1].code))};
    const IR::VectorReg dst_reg{inst.dst[0].code};
    ir.SetVectorReg(dst_reg, is_xor ? ir.BitwiseXor(src0, src1) : ir.BitwiseOr(src0, src1));
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

void Translator::V_ADD_I32(const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{ir.GetVectorReg(IR::VectorReg(inst.src[1].code))};
    const IR::VectorReg dst_reg{inst.dst[0].code};
    ir.SetVectorReg(dst_reg, ir.IAdd(src0, src1));
    // TODO: Carry
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
    const IR::F32 src0{GetSrc(inst.src[0], true)};
    const IR::F32 src1{GetSrc(inst.src[1], true)};
    const IR::F32 src2{GetSrc(inst.src[2], true)};
    SetDst(inst.dst[0], ir.FPFma(src0, src1, src2));
}

void Translator::V_FRACT_F32(const GcnInst& inst) {
    const IR::F32 src0{GetSrc(inst.src[0], true)};
    const IR::VectorReg dst_reg{inst.dst[0].code};
    ir.SetVectorReg(dst_reg, ir.Fract(src0));
}

void Translator::V_ADD_F32(const GcnInst& inst) {
    const IR::F32 src0{GetSrc(inst.src[0], true)};
    const IR::F32 src1{GetSrc(inst.src[1], true)};
    SetDst(inst.dst[0], ir.FPAdd(src0, src1));
}

void Translator::V_CVT_OFF_F32_I4(const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::VectorReg dst_reg{inst.dst[0].code};
    ir.SetVectorReg(
        dst_reg,
        ir.FPMul(ir.ConvertUToF(32, 32, ir.ISub(ir.BitwiseAnd(src0, ir.Imm32(0xF)), ir.Imm32(8))),
                 ir.Imm32(1.f / 16.f)));
}

void Translator::V_MED3_F32(const GcnInst& inst) {
    const IR::F32 src0{GetSrc(inst.src[0], true)};
    const IR::F32 src1{GetSrc(inst.src[1], true)};
    const IR::F32 src2{GetSrc(inst.src[2], true)};
    const IR::F32 mmx = ir.FPMin(ir.FPMax(src0, src1), src2);
    SetDst(inst.dst[0], ir.FPMax(ir.FPMin(src0, src1), mmx));
}

void Translator::V_FLOOR_F32(const GcnInst& inst) {
    const IR::F32 src0{GetSrc(inst.src[0], true)};
    const IR::VectorReg dst_reg{inst.dst[0].code};
    ir.SetVectorReg(dst_reg, ir.FPFloor(src0));
}

void Translator::V_SUB_F32(const GcnInst& inst) {
    const IR::F32 src0{GetSrc(inst.src[0], true)};
    const IR::F32 src1{GetSrc(inst.src[1], true)};
    SetDst(inst.dst[0], ir.FPSub(src0, src1));
}

void Translator::V_RCP_F32(const GcnInst& inst) {
    const IR::F32 src0{GetSrc(inst.src[0], true)};
    SetDst(inst.dst[0], ir.FPRecip(src0));
}

void Translator::V_FMA_F32(const GcnInst& inst) {
    const IR::F32 src0{GetSrc(inst.src[0], true)};
    const IR::F32 src1{GetSrc(inst.src[1], true)};
    const IR::F32 src2{GetSrc(inst.src[2], true)};
    SetDst(inst.dst[0], ir.FPFma(src0, src1, src2));
}

void Translator::V_CMP_F32(ConditionOp op, bool set_exec, const GcnInst& inst) {
    const IR::F32 src0{GetSrc(inst.src[0], true)};
    const IR::F32 src1{GetSrc(inst.src[1], true)};
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
    const IR::F32 src0{GetSrc(inst.src[0], true)};
    const IR::F32 src1{GetSrc(inst.src[1], true)};
    SetDst(inst.dst[0], ir.FPMax(src0, src1, is_legacy));
}

void Translator::V_MAX_U32(bool is_signed, const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{GetSrc(inst.src[1])};
    SetDst(inst.dst[0], ir.IMax(src0, src1, is_signed));
}

void Translator::V_RSQ_F32(const GcnInst& inst) {
    const IR::F32 src0{GetSrc(inst.src[0], true)};
    SetDst(inst.dst[0], ir.FPRecipSqrt(src0));
}

void Translator::V_SIN_F32(const GcnInst& inst) {
    const IR::F32 src0{GetSrc(inst.src[0], true)};
    SetDst(inst.dst[0], ir.FPSin(src0));
}

void Translator::V_LOG_F32(const GcnInst& inst) {
    const IR::F32 src0{GetSrc(inst.src[0], true)};
    SetDst(inst.dst[0], ir.FPLog2(src0));
}

void Translator::V_EXP_F32(const GcnInst& inst) {
    const IR::F32 src0{GetSrc(inst.src[0], true)};
    SetDst(inst.dst[0], ir.FPExp2(src0));
}

void Translator::V_SQRT_F32(const GcnInst& inst) {
    const IR::F32 src0{GetSrc(inst.src[0], true)};
    SetDst(inst.dst[0], ir.FPSqrt(src0));
}

void Translator::V_MIN_F32(const GcnInst& inst, bool is_legacy) {
    const IR::F32 src0{GetSrc(inst.src[0], true)};
    const IR::F32 src1{GetSrc(inst.src[1], true)};
    SetDst(inst.dst[0], ir.FPMin(src0, src1, is_legacy));
}

void Translator::V_MIN3_F32(const GcnInst& inst) {
    const IR::F32 src0{GetSrc(inst.src[0], true)};
    const IR::F32 src1{GetSrc(inst.src[1], true)};
    const IR::F32 src2{GetSrc(inst.src[2], true)};
    SetDst(inst.dst[0], ir.FPMin(src0, ir.FPMin(src1, src2)));
}

void Translator::V_MADMK_F32(const GcnInst& inst) {
    const IR::F32 src0{GetSrc(inst.src[0], true)};
    const IR::F32 src1{GetSrc(inst.src[1], true)};
    const IR::F32 k{GetSrc(inst.src[2], true)};
    SetDst(inst.dst[0], ir.FPFma(src0, k, src1));
}

void Translator::V_CUBEMA_F32(const GcnInst& inst) {
    SetDst(inst.dst[0], ir.Imm32(1.f));
}

void Translator::V_CUBESC_F32(const GcnInst& inst) {
    SetDst(inst.dst[0], GetSrc(inst.src[0], true));
}

void Translator::V_CUBETC_F32(const GcnInst& inst) {
    SetDst(inst.dst[0], GetSrc(inst.src[1], true));
}

void Translator::V_CUBEID_F32(const GcnInst& inst) {
    SetDst(inst.dst[0], GetSrc(inst.src[2], true));
}

void Translator::V_CVT_U32_F32(const GcnInst& inst) {
    const IR::F32 src0{GetSrc(inst.src[0], true)};
    SetDst(inst.dst[0], ir.ConvertFToU(32, src0));
}

void Translator::V_SUBREV_F32(const GcnInst& inst) {
    const IR::F32 src0{GetSrc(inst.src[0], true)};
    const IR::F32 src1{GetSrc(inst.src[1], true)};
    SetDst(inst.dst[0], ir.FPSub(src1, src0));
}

void Translator::V_SUBREV_I32(const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{GetSrc(inst.src[1])};
    SetDst(inst.dst[0], ir.ISub(src1, src0));
    // TODO: Carry-out
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
    const IR::U32 max{ir.IMax(src0, src1, false)};
    const IR::U32 min{ir.IMin(src0, src1, false)};
    SetDst(inst.dst[0], ir.IAdd(ir.ISub(max, min), src2));
}

void Translator::V_BFE_U32(bool is_signed, const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{ir.BitwiseAnd(GetSrc(inst.src[1]), ir.Imm32(0x1F))};
    const IR::U32 src2{ir.BitwiseAnd(GetSrc(inst.src[2]), ir.Imm32(0x1F))};
    SetDst(inst.dst[0], ir.BitFieldExtract(src0, src1, src2, is_signed));
}

void Translator::V_MAD_I32_I24(const GcnInst& inst) {
    const IR::U32 src0{ir.BitFieldExtract(GetSrc(inst.src[0]), ir.Imm32(0), ir.Imm32(24), true)};
    const IR::U32 src1{ir.BitFieldExtract(GetSrc(inst.src[1]), ir.Imm32(0), ir.Imm32(24), true)};
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

void Translator::V_MAD_U32_U24(const GcnInst& inst) {
    // TODO:
    V_MAD_I32_I24(inst);
}

void Translator::V_RNDNE_F32(const GcnInst& inst) {
    const IR::F32 src0{GetSrc(inst.src[0], true)};
    SetDst(inst.dst[0], ir.FPRoundEven(src0));
}

void Translator::V_BCNT_U32_B32(const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{GetSrc(inst.src[1])};
    SetDst(inst.dst[0], ir.IAdd(ir.BitCount(src0), src1));
}

void Translator::V_COS_F32(const GcnInst& inst) {
    const IR::F32 src0{GetSrc(inst.src[0], true)};
    SetDst(inst.dst[0], ir.FPCos(src0));
}

void Translator::V_MAX3_F32(const GcnInst& inst) {
    const IR::F32 src0{GetSrc(inst.src[0], true)};
    const IR::F32 src1{GetSrc(inst.src[1], true)};
    const IR::F32 src2{GetSrc(inst.src[2], true)};
    SetDst(inst.dst[0], ir.FPMax(src0, ir.FPMax(src1, src2)));
}

void Translator::V_CVT_I32_F32(const GcnInst& inst) {
    const IR::F32 src0{GetSrc(inst.src[0], true)};
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
    const IR::F32 src0{GetSrc(inst.src[0], true)};
    SetDst(inst.dst[0], ir.FPTrunc(src0));
}

void Translator::V_CEIL_F32(const GcnInst& inst) {
    const IR::F32 src0{GetSrc(inst.src[0], true)};
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
    const IR::F32 src0{GetSrc(inst.src[0], true)};
    const IR::U32 src1{GetSrc(inst.src[1])};
    SetDst(inst.dst[0], ir.FPLdexp(src0, src1));
}

void Translator::V_CVT_FLR_I32_F32(const GcnInst& inst) {
    const IR::F32 src0{GetSrc(inst.src[0], true)};
    SetDst(inst.dst[0], ir.ConvertFToI(32, true, ir.FPFloor(src0)));
}

void Translator::V_CMP_CLASS_F32(const GcnInst& inst) {
    UNREACHABLE();
}

} // namespace Shader::Gcn
