// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/frontend/translate/translate.h"

namespace Shader::Gcn {

void Translator::S_MOVK(const GcnInst& inst) {
    const auto simm16 = inst.control.sopk.simm.Value();
    if (simm16 & (1 << 15)) {
        // TODO: need to verify the case of imm sign extension
        UNREACHABLE();
    }
    SetDst(inst.dst[0], ir.Imm32(simm16));
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

    // Mark destination SPGR as an EXEC context. This means we will use 1-bit
    // IR instruction whenever it's loaded.
    switch (inst.dst[0].field) {
    case OperandField::ScalarGPR: {
        const u32 reg = inst.dst[0].code;
        exec_contexts[reg] = true;
        ir.SetThreadBitScalarReg(IR::ScalarReg(reg), exec);
        break;
    }
    case OperandField::VccLo:
        ir.SetVcc(exec);
        break;
    default:
        UNREACHABLE();
    }

    // Update EXEC.
    ir.SetExec(ir.LogicalAnd(exec, src));
}

void Translator::S_MOV_B64(const GcnInst& inst) {
    // TODO: Using VCC as EXEC context.
    if (inst.src[0].field == OperandField::VccLo || inst.dst[0].field == OperandField::VccLo) {
        return;
    }
    if (inst.dst[0].field == OperandField::ScalarGPR && inst.src[0].field == OperandField::ExecLo) {
        // Exec context push
        exec_contexts[inst.dst[0].code] = true;
        ir.SetThreadBitScalarReg(IR::ScalarReg(inst.dst[0].code), ir.GetExec());
    } else if (inst.dst[0].field == OperandField::ExecLo &&
               inst.src[0].field == OperandField::ScalarGPR) {
        // Exec context pop
        exec_contexts[inst.src[0].code] = false;
        ir.SetExec(ir.GetThreadBitScalarReg(IR::ScalarReg(inst.src[0].code)));
    } else if (inst.dst[0].field == OperandField::ExecLo &&
               inst.src[0].field == OperandField::ConstZero) {
        ir.SetExec(ir.Imm1(false));
    } else {
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

void Translator::S_AND_B32(const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{GetSrc(inst.src[1])};
    const IR::U32 result{ir.BitwiseAnd(src0, src1)};
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
    SetDst(inst.dst[0], ir.Imm32(pc));
}

void Translator::S_ADDC_U32(const GcnInst& inst) {
    const IR::U32 src0{GetSrc(inst.src[0])};
    const IR::U32 src1{GetSrc(inst.src[1])};
    SetDst(inst.dst[0], ir.IAdd(ir.IAdd(src0, src1), ir.GetSccLo()));
}

} // namespace Shader::Gcn
