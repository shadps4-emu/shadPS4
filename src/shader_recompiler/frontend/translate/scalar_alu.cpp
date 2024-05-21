// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/frontend/translate/translate.h"

namespace Shader::Gcn {

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
        }
    }();
    // ir.SetScc(result);
}

} // namespace Shader::Gcn
