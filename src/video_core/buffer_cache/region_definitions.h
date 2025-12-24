// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include "common/types.h"

namespace VideoCore {

constexpr u64 PAGES_PER_WORD = 64;
constexpr u64 BYTES_PER_PAGE = 4_KB;
constexpr u64 BYTES_PER_WORD = PAGES_PER_WORD * BYTES_PER_PAGE;

constexpr u64 HIGHER_PAGE_BITS = 24;
constexpr u64 HIGHER_PAGE_SIZE = 1ULL << HIGHER_PAGE_BITS;
constexpr u64 HIGHER_PAGE_MASK = HIGHER_PAGE_SIZE - 1ULL;
constexpr u64 NUM_REGION_WORDS = HIGHER_PAGE_SIZE / BYTES_PER_WORD;

enum class Type : u8 {
    CPU = 1 << 0,
    GPU = 1 << 1,
};

enum class LockOp : u8 {
    Lock = 1 << 0,
    Unlock = 1 << 1,
    Both = Lock | Unlock,
};

constexpr bool operator&(LockOp a, LockOp b) noexcept {
    return static_cast<u8>(a) & static_cast<u8>(b);
}

constexpr LockOp operator|(LockOp a, LockOp b) noexcept {
    return static_cast<LockOp>(static_cast<u8>(a) | static_cast<u8>(b));
}

struct Bounds {
    u64 start_word;
    u64 start_page;
    u64 end_word;
    u64 end_page;
};

constexpr Bounds MIN_BOUNDS = {
    .start_word = NUM_REGION_WORDS - 1,
    .start_page = PAGES_PER_WORD - 1,
    .end_word = 0,
    .end_page = 0,
};

struct RegionBits {
    using AtomicT = std::atomic<u64>;

    constexpr void Fill(u64 value) {
        data.fill(value);
    }

    constexpr bool GetPage(u64 page) const {
        return data[page / PAGES_PER_WORD] & (1ULL << (page % PAGES_PER_WORD));
    }

    constexpr AtomicT& operator[](u64 index) {
        return reinterpret_cast<AtomicT&>(data[index]);
    }

private:
    alignas(64) std::array<u64, NUM_REGION_WORDS> data;
};

} // namespace VideoCore
