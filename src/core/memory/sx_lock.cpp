// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <xmmintrin.h>
#include "core/memory/sx_lock.h"

constexpr u32 ASX_RETRIES = 10;
constexpr u32 ASX_LOOPS = 64;

namespace Core {

void SxLock::downgrade() {
    u64 x = raw;
    if (!(x & SharedWaiters) &&
        raw_atomic.compare_exchange_weak(x, SX_ONE_SHARER | Shared | (x & ExclusiveWaiters),
                                         std::memory_order_release)) {
        return;
    }

    /*
     * Preserve SX_LOCK_EXCLUSIVE_WAITERS while downgraded to a single
     * shared lock.  If there are any shared waiters, wake them up.
     */
    x = raw;
    raw_atomic.store(SX_ONE_SHARER | Shared | (x & ExclusiveWaiters), std::memory_order_release);
    if (x & SharedWaiters) {
        raw_atomic.notify_all();
    }
}

void SxLock::xlock_hard(u64 tid) {
    using Pthread = ::Libraries::Kernel::Pthread;
    u32 i, spintries = 0;

    /* If we already hold an exclusive lock, then recurse. */
    if (IsXLocked()) {
        ASSERT(recursable);
        recurse++;
        raw_atomic |= Recursed;
        return;
    }

    const auto try_lock = [&] {
        u64 expected = SX_LOCK_UNLOCKED;
        return raw_atomic.compare_exchange_weak(expected, tid);
    };

    while (!try_lock()) {
        /*
         * If the lock is write locked and the owner is
         * running on another CPU, spin until the owner stops
         * running or the state of the lock changes.
         */
        u64 x = raw;
        if (Sharers() && spintries < ASX_RETRIES) {
            spintries++;
            for (i = 0; i < ASX_LOOPS; i++) {
                x = raw;
                if (!(x & Shared) || !Sharers()) {
                    break;
                }
                _mm_pause();
            }
            if (i != ASX_LOOPS) {
                continue;
            }
        }

        x = raw;

        /*
         * If the lock was released while spinning on the
         * sleep queue chain lock, try again.
         */
        if (x == SX_LOCK_UNLOCKED) {
            continue;
        }

        /*
         * If an exclusive lock was released with both shared
         * and exclusive waiters and a shared waiter hasn't
         * woken up and acquired the lock yet, sx_lock will be
         * set to SX_LOCK_UNLOCKED | SX_LOCK_EXCLUSIVE_WAITERS.
         * If we see that value, try to acquire it once.  Note
         * that we have to preserve SX_LOCK_EXCLUSIVE_WAITERS
         * as there are other exclusive waiters still.  If we
         * fail, restart the loop.
         */
        if (x == (SX_LOCK_UNLOCKED | ExclusiveWaiters)) {
            u64 expected = SX_LOCK_UNLOCKED | ExclusiveWaiters;
            if (raw_atomic.compare_exchange_weak(expected, tid | ExclusiveWaiters,
                                                 std::memory_order_acquire)) {
                break;
            }
            continue;
        }

        /*
         * Try to set the SX_LOCK_EXCLUSIVE_WAITERS.  If we fail,
         * than loop back and retry.
         */
        if (!(x & ExclusiveWaiters)) {
            if (!raw_atomic.compare_exchange_weak(x, x | ExclusiveWaiters)) {
                continue;
            }
        }

        /*
         * Since we have been unable to acquire the exclusive
         * lock and the exclusive waiters flag is set, we have
         * to sleep.
         */
        raw_atomic.wait(x);
    }
}

void SxLock::xunlock_hard(u64 tid) {
    /* If the lock is recursed, then unrecurse one level. */
    if (IsXLocked() && recurse) {
        if ((--recurse) == 0) {
            raw_atomic &= ~Recursed;
        }
        return;
    }

    u64 x = SX_LOCK_UNLOCKED;

    /*
     * The wake up algorithm here is quite simple and probably not
     * ideal.  It gives precedence to shared waiters if they are
     * present.  For this condition, we have to preserve the
     * state of the exclusive waiters flag.
     * If interruptible sleeps left the shared queue empty avoid a
     * starvation for the threads sleeping on the exclusive queue by giving
     * them precedence and cleaning up the shared waiters bit anyway.
     */
    if ((raw & SharedWaiters)) {
        x |= (raw & ExclusiveWaiters);
    }

    /* Wake up all the waiters for the specific queue. */
    raw_atomic.store(x, std::memory_order_release);
    raw_atomic.notify_all();
}

void SxLock::slock_hard() {
    /*
     * As with rwlocks, we don't make any attempt to try to block
     * shared locks once there is an exclusive waiter.
     */
    for (;;) {
        u64 x = raw;

        /*
         * If no other thread has an exclusive lock then try to bump up
         * the count of sharers.  Since we have to preserve the state
         * of SX_LOCK_EXCLUSIVE_WAITERS, if we fail to acquire the
         * shared lock loop back and retry.
         */
        if (x & Shared) {
            u64 expected = x;
            if (raw_atomic.compare_exchange_weak(expected, x + SX_ONE_SHARER,
                                                 std::memory_order_acquire)) {
                break;
            }
            continue;
        }

        /*
         * Some other thread already has an exclusive lock, so
         * start the process of blocking.
         */
        x = raw;

        /*
         * The lock could have been released while we spun.
         * In this case loop back and retry.
         */
        if (x & Shared) {
            continue;
        }

        /*
         * Try to set the SX_LOCK_SHARED_WAITERS flag.  If we
         * fail to set it drop the sleep queue lock and loop
         * back.
         */
        if (!(x & SharedWaiters)) {
            if (!raw_atomic.compare_exchange_weak(x, x | SharedWaiters)) {
                continue;
            }
        }

        /*
         * Since we have been unable to acquire the shared lock,
         * we have to sleep.
         */
        raw_atomic.wait(x);
    }
}

void SxLock::sunlock_hard() {
    for (;;) {
        u64 x = raw;

        /*
         * See if there is more than one shared lock held.  If
         * so, just drop one and return.
         */
        if (Sharers() > 1) {
            u64 expected = x;
            if (raw_atomic.compare_exchange_weak(expected, x - SX_ONE_SHARER,
                                                 std::memory_order_release)) {
                break;
            }
            continue;
        }

        /*
         * If there aren't any waiters for an exclusive lock,
         * then try to drop it quickly.
         */
        if (!(x & ExclusiveWaiters)) {
            u64 expected = SX_ONE_SHARER | Shared;
            if (raw_atomic.compare_exchange_weak(expected, SX_LOCK_UNLOCKED,
                                                 std::memory_order_release)) {
                break;
            }
            continue;
        }

        /*
         * At this point, there should just be one sharer with
         * exclusive waiters.
         */
        /*
         * Wake up semantic here is quite simple:
         * Just wake up all the exclusive waiters.
         * Note that the state of the lock could have changed,
         * so if it fails loop back and retry.
         */
        u64 expected = SX_ONE_SHARER | Shared | ExclusiveWaiters;
        if (!raw_atomic.compare_exchange_weak(expected, SX_LOCK_UNLOCKED)) {
            continue;
        }

        raw_atomic.notify_all();
        break;
    }
}

} // namespace Core