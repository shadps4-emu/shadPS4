// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include "shader_recompiler/ir/compute_value/imm_value.h"
#include "shader_recompiler/ir/value.h"

// Given a value (inmediate or not), compute all the possible inmediate values
// that can represent. If the value can't be computed statically, the list will
// be empty.

namespace Shader::IR {

using ImmValueList = boost::container::flat_set<ImmValue>;
using ComputeImmValuesCache = boost::container::flat_map<Inst*, ImmValueList>;

void ComputeImmValues(const Value& value, ImmValueList& values, ComputeImmValuesCache& cache);

} // namespace Shader::IR
