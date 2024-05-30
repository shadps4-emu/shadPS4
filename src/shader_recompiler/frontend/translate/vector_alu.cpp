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
    SetDst(inst.dst[0], ir.FPFma(GetSrc(inst.src[0]), GetSrc(inst.src[1]), GetSrc(inst.dst[0])));
}

void Translator::V_CVT_PKRTZ_F16_F32(const GcnInst& inst) {
    const IR::VectorReg dst_reg{inst.dst[0].code};
    const IR::Value vec_f32 = ir.CompositeConstruct(GetSrc(inst.src[0]), GetSrc(inst.src[1]));
    ir.SetVectorReg(dst_reg, ir.PackHalf2x16(vec_f32));
}

void Translator::V_MUL_F32(const GcnInst& inst) {
    const IR::VectorReg dst_reg{inst.dst[0].code};
    ir.SetVectorReg(dst_reg, ir.FPMul(GetSrc(inst.src[0]), GetSrc(inst.src[1])));
}

void Translator::V_CMP_EQ_U32(const GcnInst& inst) {
    const IR::U1 result = ir.IEqual(GetSrc(inst.src[0]), GetSrc(inst.src[1]));
    if (inst.dst[1].field == OperandField::VccLo) {
        return ir.SetVcc(result);
    } else if (inst.dst[1].field == OperandField::ScalarGPR) {
        const IR::ScalarReg dst_reg{inst.dst[1].code};
        return ir.SetScalarReg(dst_reg, IR::U32{ir.Select(result, ir.Imm32(1U), ir.Imm32(0U))});
    }
    UNREACHABLE();
}

void Translator::V_CNDMASK_B32(const GcnInst& inst) {
    const IR::VectorReg dst_reg{inst.dst[0].code};
    const IR::ScalarReg flag_reg{inst.src[2].code};
    const IR::U1 flag = inst.src[2].field == OperandField::ScalarGPR
                            ? ir.INotEqual(ir.GetScalarReg(flag_reg), ir.Imm32(0U))
                            : ir.GetVcc();

    // We can treat the instruction as integer most of the time, but when a source is
    // a floating point constant we will force the other as float for better readability
    // The other operand is also higly likely to be float as well.
    const auto is_float_const = [](OperandField field) {
        return field >= OperandField::ConstFloatPos_0_5 && field <= OperandField::ConstFloatNeg_4_0;
    };
    const bool has_flt_source =
        is_float_const(inst.src[0].field) || is_float_const(inst.src[1].field);
    const IR::U32F32 src0 = GetSrc(inst.src[0], has_flt_source);
    const IR::U32F32 src1 = GetSrc(inst.src[1], has_flt_source);
    const IR::Value result = ir.Select(flag, src1, src0);
    ir.SetVectorReg(dst_reg, IR::U32F32{result});
}

void Translator::V_AND_B32(const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{ir.GetVectorReg(IR::VectorReg(inst.src[1].code))};
    const IR::VectorReg dst_reg{inst.dst[0].code};
    ir.SetVectorReg(dst_reg, ir.BitwiseAnd(src0, src1));
}

void Translator::V_LSHLREV_B32(const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{ir.GetVectorReg(IR::VectorReg(inst.src[1].code))};
    const IR::VectorReg dst_reg{inst.dst[0].code};
    ir.SetVectorReg(dst_reg, ir.ShiftLeftLogical(src1, src0));
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
    const IR::F32 src0{GetSrc(inst.src[0])};
    const IR::F32 src1{GetSrc(inst.src[1])};
    const IR::F32 src2{GetSrc(inst.src[2])};
    SetDst(inst.dst[0], ir.FPFma(src0, src1, src2));
}

void Translator::V_FRACT_F32(const GcnInst& inst) {
    const IR::F32 src0{GetSrc(inst.src[0])};
    const IR::VectorReg dst_reg{inst.dst[0].code};
    ir.SetVectorReg(dst_reg, ir.Fract(src0));
}

void Translator::V_ADD_F32(const GcnInst& inst) {
    const IR::F32 src0{GetSrc(inst.src[0])};
    const IR::F32 src1{GetSrc(inst.src[1])};
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
    const IR::F32 src1{GetSrc(inst.src[1])};
    const IR::F32 src2{GetSrc(inst.src[2])};
    const IR::F32 mmx = ir.FPMin(ir.FPMax(src0, src1), src2);
    SetDst(inst.dst[0], ir.FPMax(ir.FPMin(src0, src1), mmx));
}

void Translator::V_FLOOR_F32(const GcnInst& inst) {
    const IR::F32 src0{GetSrc(inst.src[0])};
    const IR::VectorReg dst_reg{inst.dst[0].code};
    ir.SetVectorReg(dst_reg, ir.FPFloor(src0));
}

void Translator::V_SUB_F32(const GcnInst& inst) {
    const IR::F32 src0{GetSrc(inst.src[0])};
    const IR::F32 src1{GetSrc(inst.src[1])};
    SetDst(inst.dst[0], ir.FPSub(src0, src1));
}

void Translator::V_RCP_F32(const GcnInst& inst) {
    const IR::F32 src0{GetSrc(inst.src[0])};
    SetDst(inst.dst[0], ir.FPRecip(src0));
}

void Translator::V_CMPX_GT_U32(const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{GetSrc(inst.src[1])};
    const IR::U1 result = ir.IGreaterThan(src0, src1, false);
    ir.SetVcc(result);
    ir.SetExec(result);
}

void Translator::V_FMA_F32(const GcnInst& inst) {
    const IR::F32 src0{GetSrc(inst.src[0], true)};
    const IR::F32 src1{GetSrc(inst.src[1], true)};
    const IR::F32 src2{GetSrc(inst.src[2], true)};
    SetDst(inst.dst[0], ir.FPFma(src0, src1, src2));
}

void Translator::V_CMP_F32(ConditionOp op, const GcnInst& inst) {
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
        }
    }();
    ir.SetVcc(result);
}

void Translator::V_MAX_F32(const GcnInst& inst) {
    const IR::F32 src0{GetSrc(inst.src[0], true)};
    const IR::F32 src1{GetSrc(inst.src[1], true)};
    SetDst(inst.dst[0], ir.FPMax(src0, src1));
}

void Translator::V_RSQ_F32(const GcnInst& inst) {
    const IR::F32 src0{GetSrc(inst.src[0], true)};
    SetDst(inst.dst[0], ir.FPRecipSqrt(src0));
}

} // namespace Shader::Gcn
