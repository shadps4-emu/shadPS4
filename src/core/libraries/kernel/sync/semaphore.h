// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

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
        ASSERT(sem);
#elif defined(__APPLE__)
        sem = dispatch_semaphore_create(initialCount);
        ASSERT(sem);
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
        ReleaseSemaphore(sem, 1, nullptr);
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

    template <class Rep, class Period>
    bool try_acquire_for(const std::chrono::duration<Rep, Period>& rel_time) {
#ifdef _WIN64
        const auto start_time = std::chrono::high_resolution_clock::now();
        auto rel_time_ms = std::chrono::ceil<std::chrono::milliseconds>(rel_time);

        while (rel_time_ms.count() > 0) {
            u64 timeout_ms = static_cast<u64>(rel_time_ms.count());
            u64 res = WaitForSingleObjectEx(sem, timeout_ms, true);
            if (res == WAIT_OBJECT_0) {
                return true;
            } else if (res == WAIT_IO_COMPLETION) {
                auto elapsed_time = std::chrono::high_resolution_clock::now() - start_time;
                rel_time_ms -= std::chrono::duration_cast<std::chrono::milliseconds>(elapsed_time);
            } else {
                return false;
            }
        }

        return false;
#elif defined(__APPLE__)
        const auto rel_time_ns = std::chrono::ceil<std::chrono::nanoseconds>(rel_time).count();
        const auto timeout = dispatch_time(DISPATCH_TIME_NOW, rel_time_ns);
        return dispatch_semaphore_wait(sem, timeout) == 0;
#else
        return sem.try_acquire_for(rel_time);
#endif
    }

    template <class Clock, class Duration>
    bool try_acquire_until(const std::chrono::time_point<Clock, Duration>& abs_time) {
#ifdef _WIN64
        const auto start_time = Clock::now();
        if (start_time >= abs_time) {
            return false;
        }

        auto rel_time = std::chrono::ceil<std::chrono::milliseconds>(abs_time - start_time);
        while (rel_time.count() > 0) {
            u64 timeout_ms = static_cast<u64>(rel_time.count());
            u64 res = WaitForSingleObjectEx(sem, timeout_ms, true);
            if (res == WAIT_OBJECT_0) {
                return true;
            } else if (res == WAIT_IO_COMPLETION) {
                auto elapsed_time = Clock::now() - start_time;
                rel_time -= std::chrono::duration_cast<std::chrono::milliseconds>(elapsed_time);
            } else {
                return false;
            }
        }

        return false;
#elif defined(__APPLE__)
        auto abs_s = std::chrono::time_point_cast<std::chrono::seconds>(abs_time);
        auto abs_ns = std::chrono::time_point_cast<std::chrono::nanoseconds>(abs_time) -
                      std::chrono::time_point_cast<std::chrono::nanoseconds>(abs_s);
        const timespec abs_timespec = {
            .tv_sec = abs_s.time_since_epoch().count(),
            .tv_nsec = abs_ns.count(),
        };
        const auto timeout = dispatch_walltime(&abs_timespec, 0);
        return dispatch_semaphore_wait(sem, timeout) == 0;
#else
        return sem.try_acquire_until(abs_time);
#endif
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
using CountingSemaphore = Semaphore<0x7FFFFFFF /*ORBIS_KERNEL_SEM_VALUE_MAX*/>;

} // namespace Libraries::Kernel