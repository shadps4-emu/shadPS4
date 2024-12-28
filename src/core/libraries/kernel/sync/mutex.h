// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <chrono>

#include "common/types.h"

#ifdef _WIN64
#include <windows.h>
#else
#include <mutex>
#endif

namespace Libraries::Kernel {

class TimedMutex {
public:
    TimedMutex();
    ~TimedMutex();

    void lock();
    bool try_lock();

    void unlock();

    template <class Rep, class Period>
    bool try_lock_for(const std::chrono::duration<Rep, Period>& rel_time) {
#ifdef _WIN64
        constexpr auto zero = std::chrono::duration<Rep, Period>::zero();
        const auto now = std::chrono::steady_clock::now();

        std::chrono::steady_clock::time_point abs_time = now;
        if (rel_time > zero) {
            constexpr auto max = (std::chrono::steady_clock::time_point::max)();
            if (abs_time < max - rel_time) {
                abs_time += rel_time;
            } else {
                abs_time = max;
            }
        }

        return try_lock_until(abs_time);
#else
        return mtx.try_lock_for(rel_time);
#endif
    }

    template <class Clock, class Duration>
    bool try_lock_until(const std::chrono::time_point<Clock, Duration>& abs_time) {
#ifdef _WIN64
        for (;;) {
            const auto now = Clock::now();
            if (abs_time <= now) {
                return false;
            }

            const auto rel_ms = std::chrono::ceil<std::chrono::milliseconds>(abs_time - now);
            u64 res = WaitForSingleObjectEx(mtx, static_cast<u64>(rel_ms.count()), true);
            if (res == WAIT_OBJECT_0) {
                return true;
            } else if (res == WAIT_TIMEOUT) {
                return false;
            }
        }
#else
        return mtx.try_lock_until(abs_time);
#endif
    }

private:
#ifdef _WIN64
    HANDLE mtx;
#else
    std::timed_mutex mtx;
#endif
};

} // namespace Libraries::Kernel