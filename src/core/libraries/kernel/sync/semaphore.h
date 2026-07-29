// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>

#include "common/assert.h"
#include "common/types.h"

#ifdef _WIN64
#include <windows.h>
#elif defined(__APPLE__)
#include <dispatch/dispatch.h>
#else
#include <semaphore>
#endif

namespace Libraries::Kernel {

template <s64 max>
class Semaphore {
public:
    Semaphore(s32 initialCount)
#if !defined(_WIN64) && !defined(__APPLE__)
        : sem{initialCount}
#endif
    {
#ifdef _WIN64
        sem = CreateSemaphore(nullptr, initialCount, max, nullptr);
        ASSERT_MSG(sem != nullptr, "Failed to create Win32 semaphore");
#elif defined(__APPLE__)
        sem = dispatch_semaphore_create(initialCount);
        ASSERT_MSG(sem != nullptr, "Failed to create dispatch semaphore");
#endif
    }

    ~Semaphore() {
#ifdef _WIN64
        CloseHandle(sem);
#elif defined(__APPLE__)
        dispatch_release(sem);
#endif
    }

    void release() {
#ifdef _WIN64
        ASSERT_MSG(ReleaseSemaphore(sem, 1, nullptr) != 0, "Failed to release Win32 semaphore: {}",
                   GetLastError());
#elif defined(__APPLE__)
        dispatch_semaphore_signal(sem);
#else
        sem.release();
#endif
    }

    void acquire() {
#ifdef _WIN64
        for (;;) {
            u64 res = WaitForSingleObjectEx(sem, INFINITE, true);
            if (res == WAIT_OBJECT_0) {
                return;
            }
            ASSERT_MSG(res == WAIT_IO_COMPLETION,
                       "Unexpected Win32 semaphore wait result {:#x}: {}", res, GetLastError());
        }
#elif defined(__APPLE__)
        for (;;) {
            const auto res = dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER);
            if (res == 0) {
                return;
            }
        }
#else
        sem.acquire();
#endif
    }

    bool try_acquire() {
#ifdef _WIN64
        return WaitForSingleObjectEx(sem, 0, true) == WAIT_OBJECT_0;
#elif defined(__APPLE__)
        return dispatch_semaphore_wait(sem, DISPATCH_TIME_NOW) == 0;
#else
        return sem.try_acquire();
#endif
    }

    // Consume a pending permit without entering an alertable wait. Windows needs a dedicated
    // non-alertable call; the native Darwin and standard C++ semaphore calls already behave so.
    bool try_acquire_pending() {
#ifdef _WIN64
        return WaitForSingleObjectEx(sem, 0, false) == WAIT_OBJECT_0;
#elif defined(__APPLE__)
        return dispatch_semaphore_wait(sem, DISPATCH_TIME_NOW) == 0;
#else
        return sem.try_acquire();
#endif
    }

    template <class Rep, class Period>
    bool try_acquire_for(const std::chrono::duration<Rep, Period>& rel_time) {
#ifdef _WIN64
        using Clock = std::chrono::steady_clock;
        if (rel_time <= std::chrono::duration<Rep, Period>::zero()) {
            return try_acquire_pending();
        }

        const auto now = Clock::now();
        const auto clock_duration = std::chrono::duration_cast<Clock::duration>(rel_time);
        const auto deadline = clock_duration < Clock::time_point::max() - now
                                  ? now + clock_duration
                                  : Clock::time_point::max();
        for (;;) {
            const auto current = Clock::now();
            if (current >= deadline) {
                return try_acquire_pending();
            }

            const auto remaining_ms =
                std::chrono::ceil<std::chrono::milliseconds>(deadline - current);
            constexpr auto MaxFiniteWait = static_cast<s64>(INFINITE) - 1;
            const DWORD timeout_ms =
                static_cast<DWORD>(std::min<s64>(remaining_ms.count(), MaxFiniteWait));
            const DWORD res = WaitForSingleObjectEx(sem, timeout_ms, true);
            if (res == WAIT_OBJECT_0) {
                return true;
            }
            if (res != WAIT_IO_COMPLETION && res != WAIT_TIMEOUT) {
                return false;
            }
        }
#elif defined(__APPLE__)
        const auto rel_time_ns = std::chrono::ceil<std::chrono::nanoseconds>(rel_time);
        const auto timeout = dispatch_time(DISPATCH_TIME_NOW, rel_time_ns.count());
        return dispatch_semaphore_wait(sem, timeout) == 0;
#else
        return sem.try_acquire_for(rel_time);
#endif
    }

    template <class Clock, class Duration>
    bool try_acquire_until(const std::chrono::time_point<Clock, Duration>& abs_time) {
        const auto current = Clock::now();
        if (current >= abs_time) {
            return try_acquire_pending();
        }
        return try_acquire_for(abs_time - current);
    }

private:
#ifdef _WIN64
    HANDLE sem;
#elif defined(__APPLE__)
    dispatch_semaphore_t sem;
#else
    std::counting_semaphore<max> sem;
#endif
};

using BinarySemaphore = Semaphore<1>;
// A thread can receive one normal synchronization wake and one cancellation wake concurrently.
using WakeSemaphore = Semaphore<2>;
using CountingSemaphore = Semaphore<0x7FFFFFFF /*ORBIS_KERNEL_SEM_VALUE_MAX*/>;

} // namespace Libraries::Kernel
