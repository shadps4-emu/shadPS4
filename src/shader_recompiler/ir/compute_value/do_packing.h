// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "shader_recompiler/ir/compute_value/compute.h"

namespace Shader::IR::ComputeValue {

void DoPackUint2x32(ImmValueList& inst_values, const ImmValueList& args0);
void DoUnpackUint2x32(ImmValueList& inst_values, const ImmValueList& args0);
void DoPackFloat2x32(ImmValueList& inst_values, const ImmValueList& args0);
void DoPackUnorm2x16(ImmValueList& inst_values, const ImmValueList& args0);
void DoUnpackUnorm2x16(ImmValueList& inst_values, const ImmValueList& args0);
void DoPackSnorm2x16(ImmValueList& inst_values, const ImmValueList& args0);
void DoUnpackSnorm2x16(ImmValueList& inst_values, const ImmValueList& args0);
void DoPackUint2x16(ImmValueList& inst_values, const ImmValueList& args0);
void DoUnpackUint2x16(ImmValueList& inst_values, const ImmValueList& args0);
void DoPackSint2x16(ImmValueList& inst_values, const ImmValueList& args0);
void DoUnpackSint2x16(ImmValueList& inst_values, const ImmValueList& args0);
void DoPackHalf2x16(ImmValueList& inst_values, const ImmValueList& args0);
void DoUnpackHalf2x16(ImmValueList& inst_values, const ImmValueList& args0);
void DoPackUnorm4x8(ImmValueList& inst_values, const ImmValueList& args0);
void DoUnpackUnorm4x8(ImmValueList& inst_values, const ImmValueList& args0);
void DoPackSnorm4x8(ImmValueList& inst_values, const ImmValueList& args0);
void DoUnpackSnorm4x8(ImmValueList& inst_values, const ImmValueList& args0);
void DoPackUint4x8(ImmValueList& inst_values, const ImmValueList& args0);
void DoUnpackUint4x8(ImmValueList& inst_values, const ImmValueList& args0);
void DoPackSint4x8(ImmValueList& inst_values, const ImmValueList& args0);
void DoUnpackSint4x8(ImmValueList& inst_values, const ImmValueList& args0);
void DoPackUfloat10_11_11(ImmValueList& inst_values, const ImmValueList& args0);
void DoUnpackUfloat10_11_11(ImmValueList& inst_values, const ImmValueList& args0);
void DoPackUnorm2_10_10_10(ImmValueList& inst_values, const ImmValueList& args0);
void DoUnpackUnorm2_10_10_10(ImmValueList& inst_values, const ImmValueList& args0);
void DoPackSnorm2_10_10_10(ImmValueList& inst_values, const ImmValueList& args0);
void DoUnpackSnorm2_10_10_10(ImmValueList& inst_values, const ImmValueList& args0);
void DoPackUint2_10_10_10(ImmValueList& inst_values, const ImmValueList& args0);
void DoUnpackUint2_10_10_10(ImmValueList& inst_values, const ImmValueList& args0);
void DoPackSint2_10_10_10(ImmValueList& inst_values, const ImmValueList& args0);
void DoUnpackSint2_10_10_10(ImmValueList& inst_values, const ImmValueList& args0);

} // namespace Shader::IR::ComputeValue
