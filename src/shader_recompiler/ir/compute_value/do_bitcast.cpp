// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/ir/compute_value/do_bitcast.h"

namespace Shader::IR::ComputeValue {

void DoBitCastU16F16(ImmValueList& inst_values, const ImmValueList& src_values) {
    inst_values.insert(src_values.begin(), src_values.end());
}

void DoBitCastU32F32(ImmValueList& inst_values, const ImmValueList& src_values) {
    inst_values.insert(src_values.begin(), src_values.end());
}

void DoBitCastU64F64(ImmValueList& inst_values, const ImmValueList& src_values) {
    inst_values.insert(src_values.begin(), src_values.end());
}

void DoBitCastF16U16(ImmValueList& inst_values, const ImmValueList& src_values) {
    inst_values.insert(src_values.begin(), src_values.end());
}

void DoBitCastF32U32(ImmValueList& inst_values, const ImmValueList& src_values) {
    inst_values.insert(src_values.begin(), src_values.end());
}

void DoBitCastF64U64(ImmValueList& inst_values, const ImmValueList& src_values) {
    inst_values.insert(src_values.begin(), src_values.end());
}

} // namespace Shader::IR::ComputeValue
