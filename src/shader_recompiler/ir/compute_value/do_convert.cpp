// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/cartesian_invoke.h"
#include "shader_recompiler/ir/compute_value/do_convert.h"

namespace Shader::IR::ComputeValue {

void DoConvertS32F32(ImmValueList& inst_values, const ImmValueList& args) {
    Common::CartesianInvoke(ImmValue::Convert<Type::U32, true, Type::F32, true>,
                            std::insert_iterator(inst_values, inst_values.begin()), args);
}

void DoConvertS32F64(ImmValueList& inst_values, const ImmValueList& args) {
    Common::CartesianInvoke(ImmValue::Convert<Type::U32, true, Type::F64, true>,
                            std::insert_iterator(inst_values, inst_values.begin()), args);
}

void DoConvertU32F32(ImmValueList& inst_values, const ImmValueList& args) {
    Common::CartesianInvoke(ImmValue::Convert<Type::U32, false, Type::F32, true>,
                            std::insert_iterator(inst_values, inst_values.begin()), args);
}

void DoConvertF16F32(ImmValueList& inst_values, const ImmValueList& args) {
    // Common::CartesianInvoke(ImmValue::Convert<Type::F16, true, Type::F32, true>,
    // std::insert_iterator(inst_values, inst_values.begin()), args);
    UNREACHABLE_MSG("F32 to F16 conversion is not implemented");
}

void DoConvertF32F16(ImmValueList& inst_values, const ImmValueList& args) {
    // Common::CartesianInvoke(ImmValue::Convert<Type::F32, true, Type::F16, true>,
    // std::insert_iterator(inst_values, inst_values.begin()), args);
    UNREACHABLE_MSG("F16 to F32 conversion is not implemented");
}

void DoConvertF32F64(ImmValueList& inst_values, const ImmValueList& args) {
    Common::CartesianInvoke(ImmValue::Convert<Type::F32, true, Type::F64, true>,
                            std::insert_iterator(inst_values, inst_values.begin()), args);
}

void DoConvertF64F32(ImmValueList& inst_values, const ImmValueList& args) {
    Common::CartesianInvoke(ImmValue::Convert<Type::F64, true, Type::F32, true>,
                            std::insert_iterator(inst_values, inst_values.begin()), args);
}

void DoConvertF32S32(ImmValueList& inst_values, const ImmValueList& args) {
    Common::CartesianInvoke(ImmValue::Convert<Type::F32, true, Type::U32, true>,
                            std::insert_iterator(inst_values, inst_values.begin()), args);
}

void DoConvertF32U32(ImmValueList& inst_values, const ImmValueList& args) {
    Common::CartesianInvoke(ImmValue::Convert<Type::F32, true, Type::U32, false>,
                            std::insert_iterator(inst_values, inst_values.begin()), args);
}

void DoConvertF64S32(ImmValueList& inst_values, const ImmValueList& args) {
    Common::CartesianInvoke(ImmValue::Convert<Type::F64, true, Type::U32, true>,
                            std::insert_iterator(inst_values, inst_values.begin()), args);
}

void DoConvertF64U32(ImmValueList& inst_values, const ImmValueList& args) {
    Common::CartesianInvoke(ImmValue::Convert<Type::F64, true, Type::U32, false>,
                            std::insert_iterator(inst_values, inst_values.begin()), args);
}

void DoConvertF32U16(ImmValueList& inst_values, const ImmValueList& args) {
    Common::CartesianInvoke(ImmValue::Convert<Type::F32, true, Type::U16, false>,
                            std::insert_iterator(inst_values, inst_values.begin()), args);
}

void DoConvertU16U32(ImmValueList& inst_values, const ImmValueList& args) {
    Common::CartesianInvoke(ImmValue::Convert<Type::U16, false, Type::U32, false>,
                            std::insert_iterator(inst_values, inst_values.begin()), args);
}

void DoConvertU32U16(ImmValueList& inst_values, const ImmValueList& args) {
    Common::CartesianInvoke(ImmValue::Convert<Type::U32, false, Type::U16, false>,
                            std::insert_iterator(inst_values, inst_values.begin()), args);
}

} // namespace Shader::IR::ComputeValue