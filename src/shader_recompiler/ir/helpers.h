// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

// Helpers for accessing instruction operands and buffer address metadata.

#include "shader_recompiler/ir/microinstruction.h"
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
    Inst* address = buffer_inst->Arg(1).Inst();
    ASSERT(address->GetOpcode() == IR::Opcode::CompositeConstructU32x3);
    return IR::U32{address->Arg(comp)};
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

static inline void TouchReadConstAddress(Inst* inst, u16 flatbuf_off_dw) {
    if (inst->GetOpcode() == Opcode::ReadConstBuffer) {
        auto inst_info = inst->Flags<BufferInstInfo>();
        inst_info.flatbuf_off_dw.Assign(flatbuf_off_dw);
        inst_info.address_touched.Assign(true);
        inst->SetFlags(inst_info);
        return;
    }
    if (inst->GetOpcode() == Opcode::ReadConst) {
        auto inst_info = inst->Flags<ReadConstInstInfo>();
        inst_info.flatbuf_off_dw.Assign(flatbuf_off_dw);
        inst_info.address_touched.Assign(true);
        inst->SetFlags(inst_info);
        return;
    }
    UNREACHABLE_MSG("Instruction not supported");
}

static inline u16 GetFlatBufferOffset(const Inst* inst) {
    if (inst->GetOpcode() == Opcode::ReadConstBuffer) {
        auto inst_info = inst->Flags<BufferInstInfo>();
        return inst_info.flatbuf_off_dw.Value();
    }
    if (inst->GetOpcode() == Opcode::ReadConst) {
        auto inst_info = inst->Flags<ReadConstInstInfo>();
        return inst_info.flatbuf_off_dw.Value();
    }
    UNREACHABLE_MSG("Instruction not supported");
}

static inline bool IsReadConstAddressTouched(const Inst* inst) {
    if (inst->GetOpcode() == Opcode::ReadConstBuffer) {
        auto inst_info = inst->Flags<BufferInstInfo>();
        return inst_info.address_touched.Value();
    }
    if (inst->GetOpcode() == Opcode::ReadConst) {
        auto inst_info = inst->Flags<ReadConstInstInfo>();
        return inst_info.address_touched.Value();
    }
    UNREACHABLE_MSG("Instruction not supported");
}

} // namespace Shader::IR
