// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/bit_field.h"
#include "common/types.h"
namespace OrderedCount {

enum class ShaderType : u32 {
    Cs = 0,
    Ps = 1,
    Vs = 2,
    Gs = 3,
};

enum class Op : u32 {
    Add = 0,
    Swap = 1,
    Wrap = 3,
};

union Flags {
    // (offset1 << 8) | offset0
    BitField<2, 6, u32> packer_id;
    BitField<8, 1, u32> wave_release;
    BitField<9, 1, u32> wave_done;
    BitField<10, 2, ShaderType> shader_type;
    BitField<12, 2, Op> instruction_type;

    u32 raw;
};

// TODO try tweaking stride for perf, will have contention

static constexpr u32 NumOrderedCounters = 8;
static constexpr u32 CounterStride = 4;

enum class UtilityBufferOffsets : u32 {
    NextWorkgroupIndex = 0,
    Counters = 32,
    CounterEnd = Counters + (NumOrderedCounters * CounterStride) + 4,
};

constexpr u32 UtilityBufferSize = static_cast<u32>(UtilityBufferOffsets::CounterEnd);

} // namespace OrderedCount