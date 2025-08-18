// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

// Some helpers to get operand indices of instructions by name to make it a bit safer.
// Just a start, not widely used

#include "shader_recompiler/ir/value.h"

namespace Shader::IR {

// use namespaces. Enums would be better choice, but annoyingly need casting to size_t to use
// as indices

namespace LoadBufferArgs {
static const size_t Handle = 0;
static const size_t Address = 1;
}; // namespace LoadBufferArgs

namespace StoreBufferArgs {
static const size_t Handle = 0;
static const size_t Address = 1;
static const size_t Data = 2;
}; // namespace StoreBufferArgs

static_assert(LoadBufferArgs::Handle == StoreBufferArgs::Handle);
static_assert(LoadBufferArgs::Address == StoreBufferArgs::Address);

// Get certain components of buffer address argument, used in Load/StoreBuffer variants.
// We keep components separate as u32x3, before combining after sharp tracking
U32 GetBufferIndexArg(const Inst* buffer_inst);
U32 GetBufferVOffsetArg(const Inst* buffer_inst);
U32 GetBufferSOffsetArg(const Inst* buffer_inst);

} // namespace Shader::IR