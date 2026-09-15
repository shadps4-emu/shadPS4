// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include <climits>
#include <thread>
#include <utility>

#include "common/types.h"

#ifdef _WIN64
#include <windows.h>
#else
#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif

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
    None = 0,
    Lock = 1 << 0,
    Unlock = 1 << 1,
    Both = Lock | Unlock,
};

enum class StateOp : u8 {
    None = 0,
    Set = 1,
    Clear = 2,
};

constexpr bool operator&(Type a, Type b) noexcept {
    return std::to_underlying(a) & std::to_underlying(b);
}

constexpr Type operator|(Type a, Type b) noexcept {
    return static_cast<Type>(std::to_underlying(a) | std::to_underlying(b));
}

constexpr bool operator&(LockOp a, LockOp b) noexcept {
    return std::to_underlying(a) & std::to_underlying(b);
}

constexpr LockOp operator|(LockOp a, LockOp b) noexcept {
    return static_cast<LockOp>(std::to_underlying(a) | std::to_underlying(b));
}

struct Bounds {
    u64 start_word;
    u64 start_page;
    u64 end_word;
    u64 end_page;

    constexpr bool Contains(u64 word) const {
        return word >= start_word && word <= end_word;
    }

    constexpr u64 WordMask() const {
        return (~u64{0} >> (63 - (end_word - start_word))) << start_word;
    }
};

constexpr Bounds MIN_BOUNDS = {
    .start_word = NUM_REGION_WORDS - 1,
    .start_page = PAGES_PER_WORD - 1,
    .end_word = 0,
    .end_page = 0,
};

class WordLock {
public:
    void Lock(u64 mask) noexcept {
        const u32 lo = static_cast<u32>(mask);
        const u32 hi = static_cast<u32>(mask >> 32);
        if (lo) {
            LockHalf(0, lo);
        }
        if (hi) {
            LockHalf(1, hi);
        }
    }

    void Unlock(u64 mask) noexcept {
        const u32 lo = static_cast<u32>(mask);
        const u32 hi = static_cast<u32>(mask >> 32);
        if (hi) {
            UnlockHalf(1, hi);
        }
        if (lo) {
            UnlockHalf(0, lo);
        }
    }

private:
    static constexpr int kSpinCount = 16;
    static constexpr int kSpinRelax = 14;

    static void CpuRelax() noexcept {
#if defined(__x86_64__) || defined(__i386__)
        __builtin_ia32_pause();
#elif defined(__aarch64__) || defined(__arm__)
        asm volatile("yield" ::: "memory");
#endif
    }

    // Acquire every bit in mask within half.
    void LockHalf(u32 half, u32 mask) noexcept {
        auto& lock = state[half];
        u32 current = lock.load(std::memory_order_relaxed);
        for (;;) {
            while ((current & mask) == 0) {
                if (lock.compare_exchange_weak(current, current | mask, std::memory_order_acquire,
                                               std::memory_order_relaxed)) {
                    return;
                }
            }

            bool free_again = false;
            for (int i = 0; i < kSpinCount; ++i) {
                if (i < kSpinRelax) {
                    CpuRelax();
                } else {
                    std::this_thread::yield();
                }
                current = lock.load(std::memory_order_relaxed);
                if ((current & mask) == 0) {
                    free_again = true;
                    break;
                }
            }
            if (free_again) {
                continue;
            }

            WaitHalf(half, mask);
            current = lock.load(std::memory_order_relaxed);
        }
    }

    void WaitHalf(u32 half, u32 mask) noexcept {
        waiters[half].fetch_add(1, std::memory_order_seq_cst);
        const u32 current = state[half].load(std::memory_order_seq_cst);
        if (current & mask) {
            FutexWait(&state[half], current);
        }
        waiters[half].fetch_sub(1, std::memory_order_release);
    }

    void UnlockHalf(u32 half, u32 mask) noexcept {
        state[half].fetch_and(~mask, std::memory_order_seq_cst);
        if (waiters[half].load(std::memory_order_seq_cst) != 0) {
            FutexWake(&state[half]);
        }
    }

    static void FutexWait(std::atomic<u32>* addr, u32 expected) noexcept {
        static_assert(sizeof(std::atomic<u32>) == sizeof(u32));
        static_assert(std::atomic<u32>::is_always_lock_free);
#ifdef _WIN64
        ::WaitOnAddress(addr, &expected, sizeof(expected), INFINITE);
#else
        ::syscall(SYS_futex, reinterpret_cast<u32*>(addr), FUTEX_WAIT_PRIVATE, expected, nullptr,
                  nullptr, 0);
#endif
    }

    static void FutexWake(std::atomic<u32>* addr) noexcept {
#ifdef _WIN64
        ::WakeByAddressAll(addr);
#else
        ::syscall(SYS_futex, reinterpret_cast<u32*>(addr), FUTEX_WAKE_PRIVATE, INT_MAX, nullptr,
                  nullptr, 0);
#endif
    }

    std::array<std::atomic<u32>, 2> state{};
    std::array<std::atomic<u32>, 2> waiters{};
};

static_assert(sizeof(WordLock) == 16);

struct RegionBits {
    constexpr void Fill(u64 value) {
        data.fill(value);
    }

    constexpr bool GetPage(u64 page) const {
        return data[page / PAGES_PER_WORD] & (1ULL << (page % PAGES_PER_WORD));
    }

    constexpr u64& operator[](u64 index) {
        return data[index];
    }

private:
    alignas(64) std::array<u64, NUM_REGION_WORDS> data;
};

} // namespace VideoCore
