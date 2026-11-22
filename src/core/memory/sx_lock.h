// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2007 Attilio Rao <attilio@freebsd.org>
// SPDX-FileCopyrightText: 2001 Jason Evans <jasone@freebsd.org>
// SPDX-License-Identifier: BSD-2-Clause

#pragma once

#include <atomic>
#include "common/types.h"
#include "core/libraries/kernel/threads/pthread.h"

namespace Core {

class SxLock {
    enum Flags : u64 {
        Shared = 1,
        SharedWaiters = 2,
        ExclusiveWaiters = 4,
        Recursed = 8,
        FlagMask = Shared | SharedWaiters | ExclusiveWaiters | Recursed,
    };

    static constexpr u32 SX_SHARERS_SHIFT = 4;
    static constexpr u64 SX_ONE_SHARER = 1ULL << SX_SHARERS_SHIFT;
    static constexpr u64 SX_LOCK_UNLOCKED = Shared;

    u64 Owner() const {
        return raw & ~FlagMask;
    };

    u64 Sharers() const {
        return Owner() >> SX_SHARERS_SHIFT;
    }

    bool IsXLocked() const {
        return (raw & ~(FlagMask & ~Shared)) == std::bit_cast<u64>(Libraries::Kernel::g_curthread);
    }

public:
    void lock() {
        u64 tid = std::bit_cast<u64>(Libraries::Kernel::g_curthread);
        u64 expected = SX_LOCK_UNLOCKED;
        if (!raw_atomic.compare_exchange_weak(expected, tid, std::memory_order_acquire)) {
            return xlock_hard(tid);
        }
    }

    bool try_lock() {
        if (IsXLocked() && recursable) {
            recurse++;
            raw_atomic |= Recursed;
            return true;
        }
        u64 tid = std::bit_cast<u64>(Libraries::Kernel::g_curthread);
        u64 expected = SX_LOCK_UNLOCKED;
        return raw_atomic.compare_exchange_weak(expected, tid, std::memory_order_acquire);
    }

    void unlock() {
        u64 tid = std::bit_cast<u64>(Libraries::Kernel::g_curthread);
        if (!raw_atomic.compare_exchange_weak(tid, SX_LOCK_UNLOCKED, std::memory_order_release)) {
            return xunlock_hard(tid);
        }
    }

    void lock_shared() {
        u64 x = raw;
        if (!(x & Shared) ||
            !raw_atomic.compare_exchange_weak(x, x + SX_ONE_SHARER, std::memory_order_acquire)) {
            return slock_hard();
        }
    }

    bool try_lock_shared() {
        u64 x = raw;
        for (;;) {
            if (!(x & Shared)) {
                break;
            }
            if (raw_atomic.compare_exchange_weak(x, x + SX_ONE_SHARER)) {
                return true;
            }
        }
        return false;
    }

    void unlock_shared() {
        u64 x = raw;
        if (x == (SX_ONE_SHARER | Shared | ExclusiveWaiters) ||
            !raw_atomic.compare_exchange_weak(x, x - SX_ONE_SHARER, std::memory_order_release)) {
            return sunlock_hard();
        }
    }

    bool try_upgrade() {
        u64 tid = std::bit_cast<u64>(Libraries::Kernel::g_curthread);
        u64 x = raw & ExclusiveWaiters;
        u64 expected = SX_ONE_SHARER | Shared | x;
        return raw_atomic.compare_exchange_weak(expected, tid | x);
    }

    void downgrade();

private:
    void xlock_hard(u64 tid);
    void xunlock_hard(u64 tid);
    void slock_hard();
    void sunlock_hard();

    union {
        std::atomic<u64> raw_atomic;
        volatile u64 raw{};
    };
    bool recursable{};
    u32 recurse{};
};

} // namespace Core