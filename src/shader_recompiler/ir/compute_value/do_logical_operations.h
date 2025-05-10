// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "shader_recompiler/ir/compute_value/compute.h"

namespace Shader::IR::ComputeValue {

void DoLogicalOr(ImmValueList& inst_values, const ImmValueList& arg1, const ImmValueList& arg2);
void DoLogicalAnd(ImmValueList& inst_values, const ImmValueList& arg1, const ImmValueList& arg2);
void DoLogicalXor(ImmValueList& inst_values, const ImmValueList& arg1, const ImmValueList& arg2);
void DoLogicalNot(ImmValueList& inst_values, const ImmValueList& arg1);

} // namespace Shader::IR::ComputeValue