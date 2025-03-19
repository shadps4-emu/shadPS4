// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "shader_recompiler/ir/compute_value/compute.h"

namespace Shader::IR::ComputeValue {

void DoBitCastU16F16(ImmValueList& inst_values, const ImmValueList& src_values);
void DoBitCastU32F32(ImmValueList& inst_values, const ImmValueList& src_values);
void DoBitCastU64F64(ImmValueList& inst_values, const ImmValueList& src_values);
void DoBitCastF16U16(ImmValueList& inst_values, const ImmValueList& src_values);
void DoBitCastF32U32(ImmValueList& inst_values, const ImmValueList& src_values);
void DoBitCastF64U64(ImmValueList& inst_values, const ImmValueList& src_values);

} // namespace Shader::IR::ComputeValue
