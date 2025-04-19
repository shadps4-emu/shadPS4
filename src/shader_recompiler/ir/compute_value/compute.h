// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <unordered_map>
#include <unordered_set>
#include "shader_recompiler/ir/compute_value/imm_value.h"
#include "shader_recompiler/ir/value.h"

// Given a value (inmediate or not), compute all the possible inmediate values
// that can represent. If the value can't be computed statically, the list will
// be empty.

namespace Shader::IR::ComputeValue {

using ImmValueList = std::unordered_set<ImmValue>;
using Cache = std::unordered_map<Inst*, ImmValueList>;

void Compute(const Value& value, ImmValueList& values, Cache& cache);

} // namespace Shader::IR::ComputeValue