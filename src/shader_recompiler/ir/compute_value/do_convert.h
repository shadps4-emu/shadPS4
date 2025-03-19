// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "shader_recompiler/ir/compute_value/compute.h"

namespace Shader::IR::ComputeValue {

void DoConvertS32F32(ImmValueList& inst_values, const ImmValueList& args);
void DoConvertS32F64(ImmValueList& inst_values, const ImmValueList& args);
void DoConvertU32F32(ImmValueList& inst_values, const ImmValueList& args);
void DoConvertF16F32(ImmValueList& inst_values, const ImmValueList& args);
void DoConvertF32F16(ImmValueList& inst_values, const ImmValueList& args);
void DoConvertF32F64(ImmValueList& inst_values, const ImmValueList& args);
void DoConvertF64F32(ImmValueList& inst_values, const ImmValueList& args);
void DoConvertF32S32(ImmValueList& inst_values, const ImmValueList& args);
void DoConvertF32U32(ImmValueList& inst_values, const ImmValueList& args);
void DoConvertF64S32(ImmValueList& inst_values, const ImmValueList& args);
void DoConvertF64U32(ImmValueList& inst_values, const ImmValueList& args);
void DoConvertF32U16(ImmValueList& inst_values, const ImmValueList& args);
void DoConvertU16U32(ImmValueList& inst_values, const ImmValueList& args);
void DoConvertU32U16(ImmValueList& inst_values, const ImmValueList& args);

} // namespace Shader::IR::ComputeValue