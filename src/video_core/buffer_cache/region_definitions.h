// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include "common/bit_array.h"
#include "common/types.h"

namespace VideoCore {

constexpr u64 PAGES_PER_WORD = 64;
constexpr u64 BYTES_PER_PAGE = 4_KB;

constexpr u64 HIGHER_PAGE_BITS = 22;
constexpr u64 HIGHER_PAGE_SIZE = 1ULL << HIGHER_PAGE_BITS;
constexpr u64 HIGHER_PAGE_MASK = HIGHER_PAGE_SIZE - 1ULL;
constexpr u64 NUM_REGION_PAGES = HIGHER_PAGE_SIZE / BYTES_PER_PAGE;

enum class Type {
    CPU,
    GPU,
    Writeable,
};

using RegionBits = Common::BitArray<NUM_REGION_PAGES>;

} // namespace VideoCore