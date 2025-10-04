// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <magic_enum/magic_enum.hpp>
#include "common/assert.h"
#include "shader_recompiler/frontend/translate/translate.h"

namespace Shader::Gcn {

void Translator::EmitScalarAlu(const GcnInst& inst) {
    switch (inst.encoding) {
    case InstEncoding::SOPC: {
        EmitSOPC(inst);
        break;
    }
    case InstEncoding::SOPK: {
        EmitSOPK(inst);
        break;
    }
    default:
        switch (inst.opcode) {
            // SOP2
        case Opcode::S_ADD_U32:
            return S_ADD_U32(inst);
        case Opcode::S_SUB_U32:
            return S_SUB_U32(inst);
        case Opcode::S_ADD_I32:
            return S_ADD_I32(inst);
        case Opcode::S_SUB_I32:
            return S_SUB_I32(inst);
        case Opcode::S_ADDC_U32:
            return S_ADDC_U32(inst);
        case Opcode::S_SUBB_U32:
            return S_SUBB_U32(inst);
        case Opcode::S_MIN_I32:
            return S_MIN_U32(true, inst);
        case Opcode::S_MIN_U32:
            return S_MIN_U32(false, inst);
        case Opcode::S_MAX_I32:
            return S_MAX_U32(true, inst);
        case Opcode::S_MAX_U32:
            return S_MAX_U32(false, inst);
        case Opcode::S_CSELECT_B32:
            return S_CSELECT_B32(inst);
        case Opcode::S_CSELECT_B64:
            return S_CSELECT_B64(inst);
        case Opcode::S_AND_B32:
            return S_AND_B32(NegateMode::None, inst);
        case Opcode::S_AND_B64:
            return S_AND_B64(NegateMode::None, inst);
        case Opcode::S_OR_B32:
            return S_OR_B32(inst);
        case Opcode::S_OR_B64:
            return S_OR_B64(NegateMode::None, false, inst);
        case Opcode::S_XOR_B32:
            return S_XOR_B32(inst);
        case Opcode::S_NOT_B32:
            return S_NOT_B32(inst);
        case Opcode::S_XOR_B64:
            return S_OR_B64(NegateMode::None, true, inst);
        case Opcode::S_ANDN2_B32:
            return S_AND_B32(NegateMode::Src1, inst);
        case Opcode::S_ANDN2_B64:
            return S_AND_B64(NegateMode::Src1, inst);
        case Opcode::S_ORN2_B64:
            return S_OR_B64(NegateMode::Src1, false, inst);
        case Opcode::S_NAND_B32:
            return S_AND_B32(NegateMode::Result, inst);
        case Opcode::S_NAND_B64:
            return S_AND_B64(NegateMode::Result, inst);
        case Opcode::S_NOR_B64:
            return S_OR_B64(NegateMode::Result, false, inst);
        case Opcode::S_XNOR_B64:
            return S_OR_B64(NegateMode::Result, true, inst);
        case Opcode::S_LSHL_B32:
            return S_LSHL_B32(inst);
        case Opcode::S_LSHL_B64:
            return S_LSHL_B64(inst);
        case Opcode::S_LSHR_B32:
            return S_LSHR_B32(inst);
        case Opcode::S_ASHR_I32:
            return S_ASHR_I32(inst);
        case Opcode::S_ASHR_I64:
            return S_ASHR_I64(inst);
        case Opcode::S_BFM_B32:
            return S_BFM_B32(inst);
        case Opcode::S_MUL_I32:
            return S_MUL_I32(inst);
        case Opcode::S_BFE_I32:
            return S_BFE(inst, true);
        case Opcode::S_BFE_U32:
            return S_BFE(inst, false);
        case Opcode::S_ABSDIFF_I32:
            return S_ABSDIFF_I32(inst);

            // SOP1
        case Opcode::S_MOV_B32:
            return S_MOV(inst);
        case Opcode::S_MOV_B64:
            return S_MOV_B64(inst);
        case Opcode::S_NOT_B64:
            return S_NOT_B64(inst);
        case Opcode::S_WQM_B64:
            break;
        case Opcode::S_BREV_B32:
            return S_BREV_B32(inst);
        case Opcode::S_BCNT1_I32_B32:
            return S_BCNT1_I32_B32(inst);
        case Opcode::S_BCNT1_I32_B64:
            return S_BCNT1_I32_B64(inst);
        case Opcode::S_FF1_I32_B32:
            return S_FF1_I32_B32(inst);
        case Opcode::S_FF1_I32_B64:
            return S_FF1_I32_B64(inst);
        case Opcode::S_FLBIT_I32_B32:
            return S_FLBIT_I32_B32(inst);
        case Opcode::S_FLBIT_I32_B64:
            return S_FLBIT_I32_B64(inst);
        case Opcode::S_BITSET0_B32:
            return S_BITSET_B32(inst, 0);
        case Opcode::S_BITSET1_B32:
            return S_BITSET_B32(inst, 1);
        case Opcode::S_AND_SAVEEXEC_B64:
            return S_SAVEEXEC_B64(NegateMode::None, false, inst);
        case Opcode::S_ORN2_SAVEEXEC_B64:
            return S_SAVEEXEC_B64(NegateMode::Src1, true, inst);
        case Opcode::S_ABS_I32:
            return S_ABS_I32(inst);
        default:
            LogMissingOpcode(inst);
        }
        break;
    }
}

void Translator::EmitSOPC(const GcnInst& inst) {
    switch (inst.opcode) {
    case Opcode::S_CMP_EQ_I32:
        return S_CMP(ConditionOp::EQ, true, inst);
    case Opcode::S_CMP_LG_I32:
        return S_CMP(ConditionOp::LG, true, inst);
    case Opcode::S_CMP_GT_I32:
        return S_CMP(ConditionOp::GT, true, inst);
    case Opcode::S_CMP_GE_I32:
        return S_CMP(ConditionOp::GE, true, inst);
    case Opcode::S_CMP_LT_I32:
        return S_CMP(ConditionOp::LT, true, inst);
    case Opcode::S_CMP_LE_I32:
        return S_CMP(ConditionOp::LE, true, inst);

    case Opcode::S_CMP_EQ_U32:
        return S_CMP(ConditionOp::EQ, false, inst);
    case Opcode::S_CMP_LG_U32:
        return S_CMP(ConditionOp::LG, false, inst);
    case Opcode::S_CMP_GT_U32:
        return S_CMP(ConditionOp::GT, false, inst);
    case Opcode::S_CMP_GE_U32:
        return S_CMP(ConditionOp::GE, false, inst);
    case Opcode::S_CMP_LT_U32:
        return S_CMP(ConditionOp::LT, false, inst);
    case Opcode::S_CMP_LE_U32:
        return S_CMP(ConditionOp::LE, false, inst);

    case Opcode::S_BITCMP0_B32:
        return S_BITCMP(false, 32, inst);
    case Opcode::S_BITCMP1_B32:
        return S_BITCMP(true, 32, inst);
    case Opcode::S_BITCMP0_B64:
        return S_BITCMP(false, 64, inst);
    case Opcode::S_BITCMP1_B64:
        return S_BITCMP(true, 64, inst);

    default:
        LogMissingOpcode(inst);
    }
}

void Translator::EmitSOPK(const GcnInst& inst) {
    switch (inst.opcode) {
        // SOPK
    case Opcode::S_MOVK_I32:
        return S_MOVK(inst, false);
    case Opcode::S_CMOVK_I32:
        return S_MOVK(inst, true);
    case Opcode::S_CMPK_EQ_I32:
        return S_CMPK(ConditionOp::EQ, true, inst);
    case Opcode::S_CMPK_LG_I32:
        return S_CMPK(ConditionOp::LG, true, inst);
    case Opcode::S_CMPK_GT_I32:
        return S_CMPK(ConditionOp::GT, true, inst);
    case Opcode::S_CMPK_GE_I32:
        return S_CMPK(ConditionOp::GE, true, inst);
    case Opcode::S_CMPK_LT_I32:
        return S_CMPK(ConditionOp::LT, true, inst);
    case Opcode::S_CMPK_LE_I32:
        return S_CMPK(ConditionOp::LE, true, inst);

    case Opcode::S_CMPK_EQ_U32:
        return S_CMPK(ConditionOp::EQ, false, inst);
    case Opcode::S_CMPK_LG_U32:
        return S_CMPK(ConditionOp::LG, false, inst);
    case Opcode::S_CMPK_GT_U32:
        return S_CMPK(ConditionOp::GT, false, inst);
    case Opcode::S_CMPK_GE_U32:
        return S_CMPK(ConditionOp::GE, false, inst);
    case Opcode::S_CMPK_LT_U32:
        return S_CMPK(ConditionOp::LT, false, inst);
    case Opcode::S_CMPK_LE_U32:
        return S_CMPK(ConditionOp::LE, false, inst);

    case Opcode::S_ADDK_I32:
        return S_ADDK_I32(inst);
    case Opcode::S_MULK_I32:
        return S_MULK_I32(inst);
    default:
        LogMissingOpcode(inst);
    }
}

// SOP2

void Translator::S_ADD_U32(const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{GetSrc(inst.src[1])};
    const IR::U32 result{ir.IAdd(src0, src1)};
    SetDst(inst.dst[0], result);

    // SCC = tmp >= 0x100000000ULL ? 1'1U : 1'0U;
    // The above assumes tmp is a 64-bit value.
    // It should be enough however to test that the truncated result is less than at least one
    // of the operands. In unsigned addition the result is always bigger than both the operands,
    // except in the case of overflow where the truncated result is less than both.
    ir.SetScc(ir.ILessThan(result, src0, false));
}

void Translator::S_SUB_U32(const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{GetSrc(inst.src[1])};
    SetDst(inst.dst[0], ir.ISub(src0, src1));

    // SCC = S1.u > S0.u ? 1'1U : 1'0U;
    ir.SetScc(ir.IGreaterThan(src1, src0, false));
}

void Translator::S_SUBB_U32(const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{GetSrc(inst.src[1])};
    const IR::U32 borrow{ir.Select(ir.GetScc(), ir.Imm32(1U), ir.Imm32(0U))};
    const IR::U32 result{ir.ISub(ir.ISub(src0, src1), borrow)};
    SetDst(inst.dst[0], result);

    const IR::U32 sum_with_borrow{ir.IAdd(src1, borrow)};
    ir.SetScc(ir.ILessThan(src0, sum_with_borrow, false));
}

void Translator::S_ADD_I32(const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{GetSrc(inst.src[1])};
    const IR::U32 result{ir.IAdd(src0, src1)};
    SetDst(inst.dst[0], result);

    // SCC = ((S0.u[31] == S1.u[31]) && (S0.u[31] != Result.u[31]));
    const IR::U32 shift{ir.Imm32(31)};
    const IR::U32 sign0{ir.ShiftRightLogical(src0, shift)};
    const IR::U32 sign1{ir.ShiftRightLogical(src1, shift)};
    const IR::U32 signr{ir.ShiftRightLogical(result, shift)};
    ir.SetScc(ir.LogicalAnd(ir.IEqual(sign0, sign1), ir.INotEqual(sign0, signr)));
}

void Translator::S_SUB_I32(const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{GetSrc(inst.src[1])};
    const IR::U32 result{ir.ISub(src0, src1)};
    SetDst(inst.dst[0], result);

    // SCC = ((S0.u[31] != S1.u[31]) && (S0.u[31] != tmp.u[31]));
    const IR::U32 shift{ir.Imm32(31)};
    const IR::U32 sign0{ir.ShiftRightLogical(src0, shift)};
    const IR::U32 sign1{ir.ShiftRightLogical(src1, shift)};
    const IR::U32 signr{ir.ShiftRightLogical(result, shift)};
    ir.SetScc(ir.LogicalAnd(ir.INotEqual(sign0, sign1), ir.INotEqual(sign0, signr)));
}

void Translator::S_ADDC_U32(const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{GetSrc(inst.src[1])};
    const IR::U32 carry{ir.Select(ir.GetScc(), ir.Imm32(1U), ir.Imm32(0U))};
    SetDst(inst.dst[0], ir.IAdd(ir.IAdd(src0, src1), carry));
}

void Translator::S_MIN_U32(bool is_signed, const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{GetSrc(inst.src[1])};
    const IR::U32 result = ir.IMin(src0, src1, is_signed);
    SetDst(inst.dst[0], result);
    ir.SetScc(ir.IEqual(result, src0));
}

void Translator::S_MAX_U32(bool is_signed, const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{GetSrc(inst.src[1])};
    const IR::U32 result = ir.IMax(src0, src1, is_signed);
    SetDst(inst.dst[0], result);
    ir.SetScc(ir.IEqual(result, src0));
}

void Translator::S_CSELECT_B32(const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{GetSrc(inst.src[1])};
    SetDst(inst.dst[0], IR::U32{ir.Select(ir.GetScc(), src0, src1)});
}

void Translator::S_CSELECT_B64(const GcnInst& inst) {
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
    const IR::U1 src1{get_src(inst.src[1])};
    const IR::U1 result{ir.Select(ir.GetScc(), src0, src1)};
    switch (inst.dst[0].field) {
    case OperandField::VccLo:
        ir.SetVcc(result);
        break;
    case OperandField::ScalarGPR:
        ir.SetThreadBitScalarReg(IR::ScalarReg(inst.dst[0].code), result);
        break;
    default:
        UNREACHABLE();
    }
}

void Translator::S_AND_B32(NegateMode negate, const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    IR::U32 src1{GetSrc(inst.src[1])};
    if (negate == NegateMode::Src1) {
        src1 = ir.BitwiseNot(src1);
    }
    IR::U32 result{ir.BitwiseAnd(src0, src1)};
    if (negate == NegateMode::Result) {
        result = ir.BitwiseNot(result);
    }
    SetDst(inst.dst[0], result);
    ir.SetScc(ir.INotEqual(result, ir.Imm32(0)));
}

void Translator::S_AND_B64(NegateMode negate, const GcnInst& inst) {
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
        case OperandField::SignedConstIntNeg:
            ASSERT_MSG(-s32(operand.code) + SignedConstIntNegMin - 1 == -1,
                       "SignedConstIntNeg must be -1");
            return ir.Imm1(true);
        case OperandField::LiteralConst:
            ASSERT_MSG(operand.code == 0 || operand.code == std::numeric_limits<u32>::max(),
                       "Unsupported literal {:#x}", operand.code);
            return ir.Imm1(operand.code & 1);
        default:
            UNREACHABLE();
        }
    };
    const IR::U1 src0{get_src(inst.src[0])};
    IR::U1 src1{get_src(inst.src[1])};
    if (negate == NegateMode::Src1) {
        src1 = ir.LogicalNot(src1);
    }
    IR::U1 result = ir.LogicalAnd(src0, src1);
    if (negate == NegateMode::Result) {
        result = ir.LogicalNot(result);
    }
    ir.SetScc(result);
    switch (inst.dst[0].field) {
    case OperandField::VccLo:
        ir.SetVcc(result);
        break;
    case OperandField::ScalarGPR:
        ir.SetThreadBitScalarReg(IR::ScalarReg(inst.dst[0].code), result);
        break;
    case OperandField::ExecLo:
        ir.SetExec(result);
        break;
    default:
        UNREACHABLE();
    }
}

void Translator::S_OR_B32(const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{GetSrc(inst.src[1])};
    const IR::U32 result{ir.BitwiseOr(src0, src1)};
    SetDst(inst.dst[0], result);
    ir.SetScc(ir.INotEqual(result, ir.Imm32(0)));
}

void Translator::S_OR_B64(NegateMode negate, bool is_xor, const GcnInst& inst) {
    const auto get_src = [&](const InstOperand& operand) {
        switch (operand.field) {
        case OperandField::ExecLo:
            return ir.GetExec();
        case OperandField::VccLo:
            return ir.GetVcc();
        case OperandField::ScalarGPR:
            return ir.GetThreadBitScalarReg(IR::ScalarReg(operand.code));
        default:
            UNREACHABLE();
        }
    };

    const IR::U1 src0{get_src(inst.src[0])};
    IR::U1 src1{get_src(inst.src[1])};
    if (negate == NegateMode::Src1) {
        src1 = ir.LogicalNot(src1);
    }
    IR::U1 result = is_xor ? ir.LogicalXor(src0, src1) : ir.LogicalOr(src0, src1);
    if (negate == NegateMode::Result) {
        result = ir.LogicalNot(result);
    }
    ir.SetScc(result);
    switch (inst.dst[0].field) {
    case OperandField::VccLo:
        ir.SetVcc(result);
        break;
    case OperandField::ScalarGPR:
        ir.SetThreadBitScalarReg(IR::ScalarReg(inst.dst[0].code), result);
        break;
    default:
        UNREACHABLE();
    }
}

void Translator::S_XOR_B32(const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{GetSrc(inst.src[1])};
    const IR::U32 result{ir.BitwiseXor(src0, src1)};
    SetDst(inst.dst[0], result);
    ir.SetScc(ir.INotEqual(result, ir.Imm32(0)));
}

void Translator::S_NOT_B32(const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 result{ir.BitwiseNot(src0)};
    SetDst(inst.dst[0], result);
    ir.SetScc(ir.INotEqual(result, ir.Imm32(0)));
}

void Translator::S_LSHL_B32(const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{GetSrc(inst.src[1])};
    const IR::U32 result = ir.ShiftLeftLogical(src0, ir.BitwiseAnd(src1, ir.Imm32(0x1F)));
    SetDst(inst.dst[0], result);
    ir.SetScc(ir.INotEqual(result, ir.Imm32(0)));
}

void Translator::S_LSHL_B64(const GcnInst& inst) {
    const IR::U64 src0{GetSrc64(inst.src[0])};
    const IR::U64 src1{GetSrc64(inst.src[1])};
    const IR::U64 result = ir.ShiftLeftLogical(src0, ir.BitwiseAnd(src1, ir.Imm64(u64(0x3F))));
    SetDst64(inst.dst[0], result);
    ir.SetScc(ir.INotEqual(result, ir.Imm64(u64(0))));
}

void Translator::S_LSHR_B32(const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{GetSrc(inst.src[1])};
    const IR::U32 shift_amt = ir.BitwiseAnd(src1, ir.Imm32(0x1F));
    const IR::U32 result = ir.ShiftRightLogical(src0, shift_amt);
    SetDst(inst.dst[0], result);
    ir.SetScc(ir.INotEqual(result, ir.Imm32(0)));
}

void Translator::S_ASHR_I32(const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{GetSrc(inst.src[1])};
    const IR::U32 result{ir.ShiftRightArithmetic(src0, ir.BitwiseAnd(src1, ir.Imm32(0x1F)))};
    SetDst(inst.dst[0], result);
    ir.SetScc(ir.INotEqual(result, ir.Imm32(0)));
}

void Translator::S_ASHR_I64(const GcnInst& inst) {
    const IR::U64 src0{GetSrc64(inst.src[0])};
    const IR::U64 src1{GetSrc64(inst.src[1])};
    const IR::U64 result{ir.ShiftRightArithmetic(src0, ir.BitwiseAnd(src1, ir.Imm64(u64(0x3F))))};
    SetDst64(inst.dst[0], result);
    ir.SetScc(ir.INotEqual(result, ir.Imm64(u64(0))));
}

void Translator::S_BFM_B32(const GcnInst& inst) {
    const IR::U32 src0{ir.BitwiseAnd(GetSrc(inst.src[0]), ir.Imm32(0x1F))};
    const IR::U32 src1{ir.BitwiseAnd(GetSrc(inst.src[1]), ir.Imm32(0x1F))};
    const IR::U32 mask{ir.ISub(ir.ShiftLeftLogical(ir.Imm32(1u), src0), ir.Imm32(1))};
    SetDst(inst.dst[0], ir.ShiftLeftLogical(mask, src1));
}

void Translator::S_MUL_I32(const GcnInst& inst) {
    SetDst(inst.dst[0], ir.IMul(GetSrc(inst.src[0]), GetSrc(inst.src[1])));
}

void Translator::S_BFE(const GcnInst& inst, bool is_signed) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{GetSrc(inst.src[1])};
    const IR::U32 offset{ir.BitwiseAnd(src1, ir.Imm32(0x1F))};
    const IR::U32 count{ir.BitFieldExtract(src1, ir.Imm32(16), ir.Imm32(7))};
    const IR::U32 result{ir.BitFieldExtract(src0, offset, count, is_signed)};
    SetDst(inst.dst[0], result);
    ir.SetScc(ir.INotEqual(result, ir.Imm32(0)));
}

void Translator::S_ABSDIFF_I32(const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{GetSrc(inst.src[1])};
    const IR::U32 result{ir.IAbs(ir.ISub(src0, src1))};
    SetDst(inst.dst[0], result);
    ir.SetScc(ir.INotEqual(result, ir.Imm32(0)));
}

// SOPK

void Translator::S_MOVK(const GcnInst& inst, bool is_conditional) {
    const s16 simm16 = inst.control.sopk.simm;
    // do the sign extension
    const s32 simm32 = static_cast<s32>(simm16);
    IR::U32 val = ir.Imm32(simm32);
    if (is_conditional) {
        // if !SCC its a NOP
        val = IR::U32{ir.Select(ir.GetScc(), val, GetSrc(inst.dst[0]))};
    }
    SetDst(inst.dst[0], val);
}

void Translator::S_CMPK(ConditionOp cond, bool is_signed, const GcnInst& inst) {
    const s32 simm16 = inst.control.sopk.simm;
    const IR::U32 lhs = GetSrc(inst.dst[0]);
    const IR::U32 rhs = ir.Imm32(simm16);
    const IR::U1 result = [&] {
        switch (cond) {
        case ConditionOp::EQ:
            return ir.IEqual(lhs, rhs);
        case ConditionOp::LG:
            return ir.INotEqual(lhs, rhs);
        case ConditionOp::GT:
            return ir.IGreaterThan(lhs, rhs, is_signed);
        case ConditionOp::GE:
            return ir.IGreaterThanEqual(lhs, rhs, is_signed);
        case ConditionOp::LT:
            return ir.ILessThan(lhs, rhs, is_signed);
        case ConditionOp::LE:
            return ir.ILessThanEqual(lhs, rhs, is_signed);
        default:
            UNREACHABLE();
        }
    }();
    ir.SetScc(result);
}

void Translator::S_ADDK_I32(const GcnInst& inst) {
    const s32 simm16 = inst.control.sopk.simm;
    SetDst(inst.dst[0], ir.IAdd(GetSrc(inst.dst[0]), ir.Imm32(simm16)));
}

void Translator::S_MULK_I32(const GcnInst& inst) {
    const s32 simm16 = inst.control.sopk.simm;
    SetDst(inst.dst[0], ir.IMul(GetSrc(inst.dst[0]), ir.Imm32(simm16)));
}

// SOP1

void Translator::S_MOV(const GcnInst& inst) {
    SetDst(inst.dst[0], GetSrc(inst.src[0]));
}

void Translator::S_MOV_B64(const GcnInst& inst) {
    // Moving SGPR to SGPR is used for thread masks, like most operations, but it can also be used
    // for moving sharps.
    if (inst.dst[0].field == OperandField::ScalarGPR &&
        inst.src[0].field == OperandField::ScalarGPR) {
        ir.SetScalarReg(IR::ScalarReg(inst.dst[0].code),
                        ir.GetScalarReg(IR::ScalarReg(inst.src[0].code)));
        ir.SetScalarReg(IR::ScalarReg(inst.dst[0].code + 1),
                        ir.GetScalarReg(IR::ScalarReg(inst.src[0].code + 1)));
    }
    const IR::U1 src = [&] {
        switch (inst.src[0].field) {
        case OperandField::VccLo:
            return ir.GetVcc();
        case OperandField::ExecLo:
            return ir.GetExec();
        case OperandField::ScalarGPR:
            return ir.GetThreadBitScalarReg(IR::ScalarReg(inst.src[0].code));
        case OperandField::ConstZero:
            return ir.Imm1(false);
        default:
            UNREACHABLE();
        }
    }();
    switch (inst.dst[0].field) {
    case OperandField::ScalarGPR:
        ir.SetThreadBitScalarReg(IR::ScalarReg(inst.dst[0].code), src);
        break;
    case OperandField::ExecLo:
        ir.SetExec(src);
        break;
    case OperandField::VccLo:
        ir.SetVcc(src);
        break;
    default:
        UNREACHABLE();
    }
}

void Translator::S_NOT_B64(const GcnInst& inst) {
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
    const IR::U1 result = ir.LogicalNot(src0);
    ir.SetScc(result);
    switch (inst.dst[0].field) {
    case OperandField::VccLo:
        ir.SetVcc(result);
        break;
    case OperandField::ScalarGPR:
        ir.SetThreadBitScalarReg(IR::ScalarReg(inst.dst[0].code), result);
        break;
    case OperandField::ExecLo:
        ir.SetExec(result);
        break;
    default:
        UNREACHABLE();
    }
}

void Translator::S_BREV_B32(const GcnInst& inst) {
    SetDst(inst.dst[0], ir.BitReverse(GetSrc(inst.src[0])));
}

void Translator::S_BCNT1_I32_B32(const GcnInst& inst) {
    const IR::U32 result = ir.BitCount(GetSrc(inst.src[0]));
    SetDst(inst.dst[0], result);
    ir.SetScc(ir.INotEqual(result, ir.Imm32(0)));
}

void Translator::S_BCNT1_I32_B64(const GcnInst& inst) {
    const IR::U32 result = ir.BitCount(GetSrc64(inst.src[0]));
    SetDst(inst.dst[0], result);
    ir.SetScc(ir.INotEqual(result, ir.Imm32(0)));
}

void Translator::S_FF1_I32_B32(const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 result{ir.FindILsb(src0)};
    SetDst(inst.dst[0], result);
}

void Translator::S_FF1_I32_B64(const GcnInst& inst) {
    const auto src = [&] {
        switch (inst.src[0].field) {
        case OperandField::ScalarGPR:
            return ir.GetThreadBitScalarReg(IR::ScalarReg(inst.src[0].code));
        case OperandField::VccLo:
            return ir.GetVcc();
        case OperandField::ExecLo:
            return ir.GetExec();
        default:
            UNREACHABLE_MSG("unhandled operand type {}", magic_enum::enum_name(inst.src[0].field));
        }
    }();
    const IR::U32 result{ir.BallotFindLsb(ir.Ballot(src))};

    SetDst(inst.dst[0], result);
}

void Translator::S_FLBIT_I32_B32(const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    // Gcn wants the MSB position counting from the left, but SPIR-V counts from the rightmost (LSB)
    // position
    const IR::U32 msb_pos = ir.FindUMsb(src0);
    const IR::U32 pos_from_left = ir.ISub(ir.Imm32(31), msb_pos);
    // Select 0xFFFFFFFF if src0 was 0
    const IR::U1 cond = ir.INotEqual(src0, ir.Imm32(0));
    SetDst(inst.dst[0], IR::U32{ir.Select(cond, pos_from_left, ir.Imm32(~0U))});
}

void Translator::S_FLBIT_I32_B64(const GcnInst& inst) {
    const IR::U64 src0{GetSrc64(inst.src[0])};
    // Gcn wants the MSB position counting from the left, but SPIR-V counts from the rightmost (LSB)
    // position
    const IR::U32 msb_pos = ir.FindUMsb(src0);
    const IR::U32 pos_from_left = ir.ISub(ir.Imm32(63), msb_pos);
    // Select 0xFFFFFFFF if src0 was 0
    const IR::U1 cond = ir.INotEqual(src0, ir.Imm64(u64(0u)));
    SetDst(inst.dst[0], IR::U32{ir.Select(cond, pos_from_left, ir.Imm32(~0U))});
}

void Translator::S_BITSET_B32(const GcnInst& inst, u32 bit_value) {
    const IR::U32 old_value{GetSrc(inst.dst[0])};
    const IR::U32 offset{ir.BitFieldExtract(GetSrc(inst.src[0]), ir.Imm32(0U), ir.Imm32(5U))};
    const IR::U32 result{ir.BitFieldInsert(old_value, ir.Imm32(bit_value), offset, ir.Imm32(1U))};
    SetDst(inst.dst[0], result);
}

void Translator::S_SAVEEXEC_B64(NegateMode negate, bool is_or, const GcnInst& inst) {
    // This instruction normally operates on 64-bit data (EXEC, VCC, SGPRs)
    // However here we flatten it to 1-bit EXEC and 1-bit VCC. For the destination
    // SGPR we have a special IR opcode for SPGRs that act as thread masks.
    IR::U1 exec{ir.GetExec()};
    const IR::U1 src = [&] {
        switch (inst.src[0].field) {
        case OperandField::VccLo:
            return ir.GetVcc();
        case OperandField::ScalarGPR:
            return ir.GetThreadBitScalarReg(IR::ScalarReg(inst.src[0].code));
        case OperandField::ExecLo:
            return ir.GetExec();
        default:
            UNREACHABLE();
        }
    }();

    switch (inst.dst[0].field) {
    case OperandField::ScalarGPR:
        ir.SetThreadBitScalarReg(IR::ScalarReg(inst.dst[0].code), exec);
        break;
    case OperandField::VccLo:
        ir.SetVcc(exec);
        break;
    default:
        UNREACHABLE();
    }

    // Update EXEC.
    if (negate == NegateMode::Src1) {
        exec = ir.LogicalNot(exec);
    }
    IR::U1 result = is_or ? ir.LogicalOr(exec, src) : ir.LogicalAnd(exec, src);
    if (negate == NegateMode::Result) {
        result = ir.LogicalNot(result);
    }
    ir.SetExec(result);
    ir.SetScc(result);
}

void Translator::S_ABS_I32(const GcnInst& inst) {
    const auto result = ir.IAbs(GetSrc(inst.src[0]));
    SetDst(inst.dst[0], result);
    ir.SetScc(ir.INotEqual(result, ir.Imm32(0)));
}

// SOPC

void Translator::S_CMP(ConditionOp cond, bool is_signed, const GcnInst& inst) {
    const IR::U32 lhs = GetSrc(inst.src[0]);
    const IR::U32 rhs = GetSrc(inst.src[1]);
    const IR::U1 result = [&] {
        switch (cond) {
        case ConditionOp::EQ:
            return ir.IEqual(lhs, rhs);
        case ConditionOp::LG:
            return ir.INotEqual(lhs, rhs);
        case ConditionOp::GT:
            return ir.IGreaterThan(lhs, rhs, is_signed);
        case ConditionOp::GE:
            return ir.IGreaterThanEqual(lhs, rhs, is_signed);
        case ConditionOp::LT:
            return ir.ILessThan(lhs, rhs, is_signed);
        case ConditionOp::LE:
            return ir.ILessThanEqual(lhs, rhs, is_signed);
        default:
            UNREACHABLE();
        }
    }();
    ir.SetScc(result);
}

void Translator::S_BITCMP(bool compare_mode, u32 bits, const GcnInst& inst) {
    const IR::U1 result = [&] {
        const IR::U32 src0 = GetSrc(inst.src[0]);
        const IR::U32 src1 = GetSrc(inst.src[1]);

        IR::U32 mask;
        switch (bits) {
        case 32:
            mask = ir.Imm32(0x1f);
            break;
        case 64:
            mask = ir.Imm32(0x3f);
            break;
        default:
            UNREACHABLE();
        }

        const IR::U32 bitpos{ir.BitwiseAnd(src1, mask)};
        const IR::U32 bittest{ir.BitwiseAnd(ir.ShiftRightLogical(src0, bitpos), ir.Imm32(1))};

        if (!compare_mode) {
            return ir.IEqual(bittest, ir.Imm32(0));
        } else {
            return ir.IEqual(bittest, ir.Imm32(1));
        }
    }();
    ir.SetScc(result);
}

} // namespace Shader::Gcn
