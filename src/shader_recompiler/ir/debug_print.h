// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/bit_field.h"
#include "shader_recompiler/ir/opcodes.h"
#include "src/common/types.h"

#pragma once

namespace Shader::IR {

constexpr size_t DEBUGPRINT_NUM_FORMAT_ARGS = NumArgsOf(IR::Opcode::DebugPrint) - 1;

union DebugPrintFlags {
    u32 raw;
    // For now, only flag is the number of variadic format args actually used
    // So bitfield not really needed
    BitField<0, 32, u32> num_args;
};

} // namespace Shader::IR