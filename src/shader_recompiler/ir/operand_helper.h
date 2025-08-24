// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
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
static inline IR::U32 GetBufferAddressComponent(const Inst* buffer_inst, u32 comp) {
    Inst* address = buffer_inst->Arg(1).InstRecursive();
    ASSERT(address->GetOpcode() == IR::Opcode::CompositeConstructU32x3);
    return IR::U32{address->Arg(comp).Resolve()};
}

static inline U32 GetBufferIndexArg(const Inst* buffer_inst) {
    return GetBufferAddressComponent(buffer_inst, 0);
}

static inline U32 GetBufferVOffsetArg(const Inst* buffer_inst) {
    return GetBufferAddressComponent(buffer_inst, 1);
}

static inline U32 GetBufferSOffsetArg(const Inst* buffer_inst) {
    return GetBufferAddressComponent(buffer_inst, 2);
}

} // namespace Shader::IR