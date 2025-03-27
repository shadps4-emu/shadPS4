// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/cartesian_invoke.h"
#include "shader_recompiler/ir/compute_value/do_integer_operations.h"

namespace Shader::IR::ComputeValue {

void DoIAdd32(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1) {
    Common::CartesianInvoke(ImmValue::Add<Type::U32, false>,
                            std::insert_iterator(inst_values, inst_values.begin()), args0, args1);
}

void DoIAdd64(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1) {
    Common::CartesianInvoke(ImmValue::Add<Type::U64, false>,
                            std::insert_iterator(inst_values, inst_values.begin()), args0, args1);
}

void DoIAddCary32(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1) {
    UNREACHABLE_MSG("IAddCary32 not implemented");
}

void DoISub32(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1) {
    Common::CartesianInvoke(ImmValue::Sub<Type::U32, false>,
                            std::insert_iterator(inst_values, inst_values.begin()), args0, args1);
}

void DoISub64(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1) {
    Common::CartesianInvoke(ImmValue::Sub<Type::U64, false>,
                            std::insert_iterator(inst_values, inst_values.begin()), args0, args1);
}

void DoIMul32(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1) {
    Common::CartesianInvoke(ImmValue::Mul<Type::U32, false>,
                            std::insert_iterator(inst_values, inst_values.begin()), args0, args1);
}

void DoIMul64(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1) {
    Common::CartesianInvoke(ImmValue::Mul<Type::U64, false>,
                            std::insert_iterator(inst_values, inst_values.begin()), args0, args1);
}

void DoSMulExt(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1) {
    UNREACHABLE_MSG("SMulExt not implemented");
}

void DoUMulExt(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1) {
    UNREACHABLE_MSG("UMulExt not implemented");
}

void DoSDiv32(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1) {
    Common::CartesianInvoke(ImmValue::Div<Type::U32, true>,
                            std::insert_iterator(inst_values, inst_values.begin()), args0, args1);
}

void DoUDiv32(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1) {
    Common::CartesianInvoke(ImmValue::Div<Type::U32, false>,
                            std::insert_iterator(inst_values, inst_values.begin()), args0, args1);
}

void DoSMod32(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1) {
    Common::CartesianInvoke(ImmValue::Mod<Type::U32, true>,
                            std::insert_iterator(inst_values, inst_values.begin()), args0, args1);
}

void DoUMod32(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1) {
    Common::CartesianInvoke(ImmValue::Mod<Type::U32, false>,
                            std::insert_iterator(inst_values, inst_values.begin()), args0, args1);
}

void DoINeg32(ImmValueList& inst_values, const ImmValueList& args) {
    Common::CartesianInvoke(ImmValue::Neg<Type::U32>,
                            std::insert_iterator(inst_values, inst_values.begin()), args);
}

void DoINeg64(ImmValueList& inst_values, const ImmValueList& args) {
    Common::CartesianInvoke(ImmValue::Neg<Type::U64>,
                            std::insert_iterator(inst_values, inst_values.begin()), args);
}

void DoIAbs32(ImmValueList& inst_values, const ImmValueList& args) {
    Common::CartesianInvoke(ImmValue::Abs<Type::U32>,
                            std::insert_iterator(inst_values, inst_values.begin()), args);
}

void DoShiftLeftLogical32(ImmValueList& inst_values, const ImmValueList& args,
                          const ImmValueList& shift) {
    Common::CartesianInvoke(ImmValue::LShift<Type::U32>,
                            std::insert_iterator(inst_values, inst_values.begin()), args, shift);
}

void DoShiftLeftLogical64(ImmValueList& inst_values, const ImmValueList& args,
                          const ImmValueList& shift) {
    Common::CartesianInvoke(ImmValue::LShift<Type::U64>,
                            std::insert_iterator(inst_values, inst_values.begin()), args, shift);
}

void DoShiftRightLogical32(ImmValueList& inst_values, const ImmValueList& args,
                           const ImmValueList& shift) {
    Common::CartesianInvoke(ImmValue::RShift<Type::U32, false>,
                            std::insert_iterator(inst_values, inst_values.begin()), args, shift);
}

void DoShiftRightLogical64(ImmValueList& inst_values, const ImmValueList& args,
                           const ImmValueList& shift) {
    Common::CartesianInvoke(ImmValue::RShift<Type::U64, false>,
                            std::insert_iterator(inst_values, inst_values.begin()), args, shift);
}

void DoShiftRightArithmetic32(ImmValueList& inst_values, const ImmValueList& args,
                              const ImmValueList& shift) {
    Common::CartesianInvoke(ImmValue::RShift<Type::U32, true>,
                            std::insert_iterator(inst_values, inst_values.begin()), args, shift);
}

void DoShiftRightArithmetic64(ImmValueList& inst_values, const ImmValueList& args,
                              const ImmValueList& shift) {
    Common::CartesianInvoke(ImmValue::RShift<Type::U64, true>,
                            std::insert_iterator(inst_values, inst_values.begin()), args, shift);
}

void DoBitwiseAnd32(ImmValueList& inst_values, const ImmValueList& args0,
                    const ImmValueList& args1) {
    Common::CartesianInvoke(ImmValue::And<Type::U32>,
                            std::insert_iterator(inst_values, inst_values.begin()), args0, args1);
}

void DoBitwiseAnd64(ImmValueList& inst_values, const ImmValueList& args0,
                    const ImmValueList& args1) {
    Common::CartesianInvoke(ImmValue::And<Type::U64>,
                            std::insert_iterator(inst_values, inst_values.begin()), args0, args1);
}

void DoBitwiseOr32(ImmValueList& inst_values, const ImmValueList& args0,
                   const ImmValueList& args1) {
    Common::CartesianInvoke(ImmValue::Or<Type::U32>,
                            std::insert_iterator(inst_values, inst_values.begin()), args0, args1);
}

void DoBitwiseOr64(ImmValueList& inst_values, const ImmValueList& args0,
                   const ImmValueList& args1) {
    Common::CartesianInvoke(ImmValue::Or<Type::U64>,
                            std::insert_iterator(inst_values, inst_values.begin()), args0, args1);
}

void DoBitwiseXor32(ImmValueList& inst_values, const ImmValueList& args0,
                    const ImmValueList& args1) {
    Common::CartesianInvoke(ImmValue::Xor<Type::U32>,
                            std::insert_iterator(inst_values, inst_values.begin()), args0, args1);
}

void DoBitFieldInsert(ImmValueList& inst_values, const ImmValueList& arg,
                      const ImmValueList& insert, const ImmValueList& offset,
                      const ImmValueList& count) {
    UNREACHABLE_MSG("BitFieldInsert not implemented");
}

void DoBitFieldSExtract(ImmValueList& inst_values, const ImmValueList& arg,
                        const ImmValueList& offset, const ImmValueList& count) {
    UNREACHABLE_MSG("BitFieldSExtract not implemented");
}

void DoBitFieldUExtract(ImmValueList& inst_values, const ImmValueList& arg,
                        const ImmValueList& offset, const ImmValueList& count) {
    UNREACHABLE_MSG("BitFieldUExtract not implemented");
}

void DoBitReverse32(ImmValueList& inst_values, const ImmValueList& arg) {
    UNREACHABLE_MSG("BitReverse32 not implemented");
}

void DoBitCount32(ImmValueList& inst_values, const ImmValueList& arg) {
    UNREACHABLE_MSG("BitCount32 not implemented");
}

void DoBitCount64(ImmValueList& inst_values, const ImmValueList& arg) {
    UNREACHABLE_MSG("BitCount64 not implemented");
}

void DoBitwiseNot32(ImmValueList& inst_values, const ImmValueList& arg) {
    Common::CartesianInvoke(ImmValue::Not<Type::U32>,
                            std::insert_iterator(inst_values, inst_values.begin()), arg);
}

void DoFindSMsb32(ImmValueList& inst_values, const ImmValueList& arg) {
    UNREACHABLE_MSG("FindSMsb32 not implemented");
}

void DoFindUMsb32(ImmValueList& inst_values, const ImmValueList& arg) {
    UNREACHABLE_MSG("FindUMsb32 not implemented");
}

void DoFindILsb32(ImmValueList& inst_values, const ImmValueList& arg) {
    UNREACHABLE_MSG("FindILsb32 not implemented");
}

void DoFindILsb64(ImmValueList& inst_values, const ImmValueList& arg) {
    UNREACHABLE_MSG("FindILsb64 not implemented");
}

void DoSMin32(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1) {
    Common::CartesianInvoke(ImmValue::Min<Type::U32, true>,
                            std::insert_iterator(inst_values, inst_values.begin()), args0, args1);
}

void DoUMin32(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1) {
    Common::CartesianInvoke(ImmValue::Min<Type::U32, false>,
                            std::insert_iterator(inst_values, inst_values.begin()), args0, args1);
}

void DoSMax32(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1) {
    Common::CartesianInvoke(ImmValue::Max<Type::U32, true>,
                            std::insert_iterator(inst_values, inst_values.begin()), args0, args1);
}

void DoUMax32(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1) {
    Common::CartesianInvoke(ImmValue::Max<Type::U32, false>,
                            std::insert_iterator(inst_values, inst_values.begin()), args0, args1);
}

void DoSClamp32(ImmValueList& inst_values, const ImmValueList& value, const ImmValueList& min,
                const ImmValueList& max) {
    Common::CartesianInvoke(ImmValue::Clamp<Type::U32, true>,
                            std::insert_iterator(inst_values, inst_values.begin()), value, min, max);
}

void DoUClamp32(ImmValueList& inst_values, const ImmValueList& value, const ImmValueList& min,
                const ImmValueList& max) {
    Common::CartesianInvoke(ImmValue::Clamp<Type::U32, false>,
                            std::insert_iterator(inst_values, inst_values.begin()), value, min, max);
}

} // namespace Shader::IR::ComputeValue