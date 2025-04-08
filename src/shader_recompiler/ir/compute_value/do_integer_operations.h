// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "shader_recompiler/ir/compute_value/compute.h"

namespace Shader::IR::ComputeValue {

void DoIAdd32(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1);
void DoIAdd64(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1);
void DoIAddCary32(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1);
void DoISub32(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1);
void DoISub64(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1);
void DoIMul32(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1);
void DoIMul64(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1);
void DoSMulExt(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1);
void DoUMulExt(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1);
void DoSDiv32(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1);
void DoUDiv32(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1);
void DoSMod32(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1);
void DoUMod32(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1);
void DoINeg32(ImmValueList& inst_values, const ImmValueList& args);
void DoINeg64(ImmValueList& inst_values, const ImmValueList& args);
void DoIAbs32(ImmValueList& inst_values, const ImmValueList& args);
void DoShiftLeftLogical32(ImmValueList& inst_values, const ImmValueList& args,
                          const ImmValueList& shift);
void DoShiftLeftLogical64(ImmValueList& inst_values, const ImmValueList& args,
                          const ImmValueList& shift);
void DoShiftRightLogical32(ImmValueList& inst_values, const ImmValueList& args,
                           const ImmValueList& shift);
void DoShiftRightLogical64(ImmValueList& inst_values, const ImmValueList& args,
                           const ImmValueList& shift);
void DoShiftRightArithmetic32(ImmValueList& inst_values, const ImmValueList& args,
                              const ImmValueList& shift);
void DoShiftRightArithmetic64(ImmValueList& inst_values, const ImmValueList& args,
                              const ImmValueList& shift);
void DoBitwiseAnd32(ImmValueList& inst_values, const ImmValueList& args0,
                    const ImmValueList& args1);
void DoBitwiseAnd64(ImmValueList& inst_values, const ImmValueList& args0,
                    const ImmValueList& args1);
void DoBitwiseOr32(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1);
void DoBitwiseOr64(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1);
void DoBitwiseXor32(ImmValueList& inst_values, const ImmValueList& args0,
                    const ImmValueList& args1);
void DoBitFieldInsert(ImmValueList& inst_values, const ImmValueList& arg,
                      const ImmValueList& insert, const ImmValueList& offset,
                      const ImmValueList& count);
void DoBitFieldSExtract(ImmValueList& inst_values, const ImmValueList& arg,
                        const ImmValueList& offset, const ImmValueList& count);
void DoBitFieldUExtract(ImmValueList& inst_values, const ImmValueList& arg,
                        const ImmValueList& offset, const ImmValueList& count);
void DoBitReverse32(ImmValueList& inst_values, const ImmValueList& arg);
void DoBitCount32(ImmValueList& inst_values, const ImmValueList& arg);
void DoBitCount64(ImmValueList& inst_values, const ImmValueList& arg);
void DoBitwiseNot32(ImmValueList& inst_values, const ImmValueList& arg);
void DoFindSMsb32(ImmValueList& inst_values, const ImmValueList& arg);
void DoFindUMsb32(ImmValueList& inst_values, const ImmValueList& arg);
void DoFindILsb32(ImmValueList& inst_values, const ImmValueList& arg);
void DoFindILsb64(ImmValueList& inst_values, const ImmValueList& arg);
void DoSMin32(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1);
void DoUMin32(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1);
void DoSMax32(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1);
void DoUMax32(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1);
void DoSMinTri32(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1, const ImmValueList& args2);
void DoUMinTri32(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1, const ImmValueList& args2);
void DoSMaxTri32(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1, const ImmValueList& args2);
void DoUMaxTri32(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1, const ImmValueList& args2);
void DoSMedTri32(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1, const ImmValueList& args2);
void DoUMedTri32(ImmValueList& inst_values, const ImmValueList& args0, const ImmValueList& args1, const ImmValueList& args2);
void DoSClamp32(ImmValueList& inst_values, const ImmValueList& value, const ImmValueList& min,
                const ImmValueList& max);
void DoUClamp32(ImmValueList& inst_values, const ImmValueList& value, const ImmValueList& min,
                const ImmValueList& max);

} // namespace Shader::IR::ComputeValue