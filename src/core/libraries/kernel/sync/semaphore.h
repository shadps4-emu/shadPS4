// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include <chrono>

#include "common/assert.h"
#include "common/types.h"

#ifdef _WIN64
#include <windows.h>
#else
#include <semaphore>
#endif

namespace Libraries::Kernel {

template <s64 max>
class Semaphore {
public:
    Semaphore(s32 initialCount)
#ifndef _WIN64
        : sem{initialCount}
#endif
    {
#ifdef _WIN64
        sem = CreateSemaphore(nullptr, initialCount, max, nullptr);
        ASSERT(sem);
#endif
    }

    ~Semaphore() {
#ifdef _WIN64
        CloseHandle(sem);
#endif
    }

    void release() {
#ifdef _WIN64
        ReleaseSemaphore(sem, 1, nullptr);
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
        }
#else
        sem.acquire();
#endif
    }

    bool try_acquire() {
#ifdef _WIN64
        return WaitForSingleObjectEx(sem, 0, true) == WAIT_OBJECT_0;
#else
        return sem.try_acquire();
#endif
    }

    template <class Rep, class Period>
    bool try_acquire_for(const std::chrono::duration<Rep, Period>& rel_time) {
#ifdef _WIN64
        const auto rel_time_ms = std::chrono::ceil<std::chrono::milliseconds>(rel_time);
        const u64 timeout_ms = static_cast<u64>(rel_time_ms.count());

        if (timeout_ms == 0) {
            return false;
        }

        return WaitForSingleObjectEx(sem, timeout_ms, true) == WAIT_OBJECT_0;
#else
        return sem.try_acquire_for(rel_time);
#endif
    }

    template <class Clock, class Duration>
    bool try_acquire_until(const std::chrono::time_point<Clock, Duration>& abs_time) {
#ifdef _WIN64
        const auto now = Clock::now();
        if (now >= abs_time) {
            return false;
        }

        const auto rel_time = std::chrono::ceil<std::chrono::milliseconds>(abs_time - now);
        const u64 timeout_ms = static_cast<u64>(rel_time.count());
        if (timeout_ms == 0) {
            return false;
        }

        u64 res = WaitForSingleObjectEx(sem, static_cast<u64>(timeout_ms), true);
        return res == WAIT_OBJECT_0;
#else
        return sem.try_acquire_until(abs_time);
#endif
    }

private:
#ifdef _WIN64
    HANDLE sem;
#else
    std::counting_semaphore<max> sem;
#endif
};

using BinarySemaphore = Semaphore<1>;
using CountingSemaphore = Semaphore<0x7FFFFFFF /*ORBIS_KERNEL_SEM_VALUE_MAX*/>;

} // namespace Libraries::Kernel