// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/cartesian_invoke.h"
#include "shader_recompiler/ir/compute_value/do_logical_operations.h"

namespace Shader::IR::ComputeValue {

void DoLogicalOr(ImmValueList& inst_values, const ImmValueList& arg1, const ImmValueList& arg2) {
    Common::CartesianInvoke(ImmValue::Or<Type::U1>,
                            std::insert_iterator(inst_values, inst_values.end()), arg1, arg2);
}

void DoLogicalAnd(ImmValueList& inst_values, const ImmValueList& arg1, const ImmValueList& arg2) {
    Common::CartesianInvoke(ImmValue::And<Type::U1>,
                            std::insert_iterator(inst_values, inst_values.end()), arg1, arg2);
}

void DoLogicalXor(ImmValueList& inst_values, const ImmValueList& arg1, const ImmValueList& arg2) {
    Common::CartesianInvoke(ImmValue::Xor<Type::U1>,
                            std::insert_iterator(inst_values, inst_values.end()), arg1, arg2);
}

void DoLogicalNot(ImmValueList& inst_values, const ImmValueList& arg1) {
    Common::CartesianInvoke(ImmValue::Not<Type::U1>,
                            std::insert_iterator(inst_values, inst_values.end()), arg1);
}

} // namespace Shader::IR::ComputeValue