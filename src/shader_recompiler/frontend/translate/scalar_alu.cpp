// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

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
        case Opcode::S_MOV_B32:
            return S_MOV(inst);
        case Opcode::S_MUL_I32:
            return S_MUL_I32(inst);
        case Opcode::S_AND_SAVEEXEC_B64:
            return S_AND_SAVEEXEC_B64(inst);
        case Opcode::S_MOV_B64:
            return S_MOV_B64(inst);
        case Opcode::S_OR_B64:
            return S_OR_B64(NegateMode::None, false, inst);
        case Opcode::S_NOR_B64:
            return S_OR_B64(NegateMode::Result, false, inst);
        case Opcode::S_XOR_B64:
            return S_OR_B64(NegateMode::None, true, inst);
        case Opcode::S_XNOR_B64:
            return S_OR_B64(NegateMode::Result, true, inst);
        case Opcode::S_ORN2_B64:
            return S_OR_B64(NegateMode::Src1, false, inst);
        case Opcode::S_AND_B64:
            return S_AND_B64(NegateMode::None, inst);
        case Opcode::S_NAND_B64:
            return S_AND_B64(NegateMode::Result, inst);
        case Opcode::S_ANDN2_B64:
            return S_AND_B64(NegateMode::Src1, inst);
        case Opcode::S_NOT_B64:
            return S_NOT_B64(inst);
        case Opcode::S_ADD_I32:
            return S_ADD_I32(inst);
        case Opcode::S_AND_B32:
            return S_AND_B32(NegateMode::None, inst);
        case Opcode::S_NAND_B32:
            return S_AND_B32(NegateMode::Result, inst);
        case Opcode::S_ANDN2_B32:
            return S_AND_B32(NegateMode::Src1, inst);
        case Opcode::S_ASHR_I32:
            return S_ASHR_I32(inst);
        case Opcode::S_OR_B32:
            return S_OR_B32(inst);
        case Opcode::S_XOR_B32:
            return S_XOR_B32(inst);
        case Opcode::S_LSHL_B32:
            return S_LSHL_B32(inst);
        case Opcode::S_LSHR_B32:
            return S_LSHR_B32(inst);
        case Opcode::S_CSELECT_B32:
            return S_CSELECT_B32(inst);
        case Opcode::S_CSELECT_B64:
            return S_CSELECT_B64(inst);
        case Opcode::S_BFE_U32:
            return S_BFE_U32(inst);
        case Opcode::S_BFM_B32:
            return S_BFM_B32(inst);
        case Opcode::S_BREV_B32:
            return S_BREV_B32(inst);
        case Opcode::S_ADD_U32:
            return S_ADD_U32(inst);
        case Opcode::S_ADDC_U32:
            return S_ADDC_U32(inst);
        case Opcode::S_SUB_U32:
        case Opcode::S_SUB_I32:
            return S_SUB_U32(inst);
        case Opcode::S_MIN_U32:
            return S_MIN_U32(false, inst);
        case Opcode::S_MIN_I32:
            return S_MIN_U32(true, inst);
        case Opcode::S_MAX_U32:
            return S_MAX_U32(false, inst);
        case Opcode::S_MAX_I32:
            return S_MAX_U32(true, inst);
        case Opcode::S_ABSDIFF_I32:
            return S_ABSDIFF_I32(inst);
        case Opcode::S_WQM_B64:
            break;
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
    default:
        LogMissingOpcode(inst);
    }
}

void Translator::EmitSOPK(const GcnInst& inst) {
    switch (inst.opcode) {
    case Opcode::S_MOVK_I32:
        return S_MOVK(inst);

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

void Translator::S_MOVK(const GcnInst& inst) {
    const auto simm16 = inst.control.sopk.simm;
    if (simm16 & (1 << 15)) {
        // TODO: need to verify the case of imm sign extension
        UNREACHABLE();
    }
    SetDst(inst.dst[0], ir.Imm32(simm16));
}

void Translator::S_ADDK_I32(const GcnInst& inst) {
    const s32 simm16 = inst.control.sopk.simm;
    SetDst(inst.dst[0], ir.IAdd(GetSrc(inst.dst[0]), ir.Imm32(simm16)));
}

void Translator::S_MULK_I32(const GcnInst& inst) {
    const s32 simm16 = inst.control.sopk.simm;
    SetDst(inst.dst[0], ir.IMul(GetSrc(inst.dst[0]), ir.Imm32(simm16)));
}

void Translator::S_MOV(const GcnInst& inst) {
    SetDst(inst.dst[0], GetSrc(inst.src[0]));
}

void Translator::S_MUL_I32(const GcnInst& inst) {
    SetDst(inst.dst[0], ir.IMul(GetSrc(inst.src[0]), GetSrc(inst.src[1])));
}

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

void Translator::S_AND_SAVEEXEC_B64(const GcnInst& inst) {
    // This instruction normally operates on 64-bit data (EXEC, VCC, SGPRs)
    // However here we flatten it to 1-bit EXEC and 1-bit VCC. For the destination
    // SGPR we have a special IR opcode for SPGRs that act as thread masks.
    const IR::U1 exec{ir.GetExec()};
    const IR::U1 src = [&] {
        switch (inst.src[0].field) {
        case OperandField::VccLo:
            return ir.GetVcc();
        case OperandField::ScalarGPR:
            return ir.GetThreadBitScalarReg(IR::ScalarReg(inst.src[0].code));
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
    const IR::U1 result = ir.LogicalAnd(exec, src);
    ir.SetExec(result);
    ir.SetScc(result);
}

void Translator::S_MOV_B64(const GcnInst& inst) {
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

void Translator::S_AND_B64(NegateMode negate, const GcnInst& inst) {
    const auto get_src = [&](const InstOperand& operand) {
        switch (operand.field) {
        case OperandField::VccLo:
            return ir.GetVcc();
        case OperandField::ExecLo:
            return ir.GetExec();
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

void Translator::S_ADD_I32(const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{GetSrc(inst.src[1])};
    SetDst(inst.dst[0], ir.IAdd(src0, src1));
    // TODO: Overflow flag
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

void Translator::S_ASHR_I32(const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{GetSrc(inst.src[1])};
    const IR::U32 result{ir.ShiftRightArithmetic(src0, src1)};
    SetDst(inst.dst[0], result);
    ir.SetScc(ir.INotEqual(result, ir.Imm32(0)));
}

void Translator::S_OR_B32(const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{GetSrc(inst.src[1])};
    const IR::U32 result{ir.BitwiseOr(src0, src1)};
    SetDst(inst.dst[0], result);
    ir.SetScc(ir.INotEqual(result, ir.Imm32(0)));
}

void Translator::S_XOR_B32(const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{GetSrc(inst.src[1])};
    const IR::U32 result{ir.BitwiseXor(src0, src1)};
    SetDst(inst.dst[0], result);
    ir.SetScc(ir.INotEqual(result, ir.Imm32(0)));
}

void Translator::S_LSHR_B32(const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{GetSrc(inst.src[1])};
    const IR::U32 result{ir.ShiftRightLogical(src0, src1)};
    SetDst(inst.dst[0], result);
    ir.SetScc(ir.INotEqual(result, ir.Imm32(0)));
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

void Translator::S_BFE_U32(const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{GetSrc(inst.src[1])};
    const IR::U32 offset{ir.BitwiseAnd(src1, ir.Imm32(0x1F))};
    const IR::U32 count{ir.BitFieldExtract(src1, ir.Imm32(16), ir.Imm32(7))};
    const IR::U32 result{ir.BitFieldExtract(src0, offset, count)};
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

void Translator::S_BFM_B32(const GcnInst& inst) {
    const IR::U32 src0{ir.BitwiseAnd(GetSrc(inst.src[0]), ir.Imm32(0x1F))};
    const IR::U32 src1{ir.BitwiseAnd(GetSrc(inst.src[1]), ir.Imm32(0x1F))};
    const IR::U32 mask{ir.ISub(ir.ShiftLeftLogical(ir.Imm32(1u), src0), ir.Imm32(1))};
    SetDst(inst.dst[0], ir.ShiftLeftLogical(mask, src1));
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
    default:
        UNREACHABLE();
    }
}

void Translator::S_BREV_B32(const GcnInst& inst) {
    SetDst(inst.dst[0], ir.BitReverse(GetSrc(inst.src[0])));
}

void Translator::S_ADD_U32(const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{GetSrc(inst.src[1])};
    SetDst(inst.dst[0], ir.IAdd(src0, src1));
    // TODO: Carry out
    ir.SetScc(ir.Imm1(false));
}

void Translator::S_SUB_U32(const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{GetSrc(inst.src[1])};
    SetDst(inst.dst[0], ir.ISub(src0, src1));
    // TODO: Carry out
    ir.SetScc(ir.Imm1(false));
}

void Translator::S_GETPC_B64(u32 pc, const GcnInst& inst) {
    // This only really exists to let resource tracking pass know
    // there is an inline cbuf.
    const IR::ScalarReg dst{inst.dst[0].code};
    ir.SetScalarReg(dst, ir.Imm32(pc));
    ir.SetScalarReg(dst + 1, ir.Imm32(0));
}

void Translator::S_ADDC_U32(const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{GetSrc(inst.src[1])};
    const IR::U32 carry{ir.Select(ir.GetScc(), ir.Imm32(1U), ir.Imm32(0U))};
    SetDst(inst.dst[0], ir.IAdd(ir.IAdd(src0, src1), carry));
}

void Translator::S_MAX_U32(bool is_signed, const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{GetSrc(inst.src[1])};
    const IR::U32 result = ir.IMax(src0, src1, is_signed);
    SetDst(inst.dst[0], result);
    ir.SetScc(ir.IEqual(result, src0));
}

void Translator::S_MIN_U32(bool is_signed, const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{GetSrc(inst.src[1])};
    const IR::U32 result = ir.IMin(src0, src1, is_signed);
    SetDst(inst.dst[0], result);
    ir.SetScc(ir.IEqual(result, src0));
}

void Translator::S_ABSDIFF_I32(const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{GetSrc(inst.src[1])};
    const IR::U32 result{ir.IAbs(ir.ISub(src0, src1))};
    SetDst(inst.dst[0], result);
    ir.SetScc(ir.INotEqual(result, ir.Imm32(0)));
}

} // namespace Shader::Gcn
