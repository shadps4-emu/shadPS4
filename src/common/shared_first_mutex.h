// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>

namespace Common {
// Like std::shared_mutex, but reader has priority over writer.
class SharedFirstMutex {
public:
    SharedFirstMutex() = default;
    ~SharedFirstMutex() = default;

    SharedFirstMutex(const SharedFirstMutex&) = delete;
    SharedFirstMutex& operator=(const SharedFirstMutex&) = delete;

    void lock() {
        std::unique_lock lk{m_mtx};
        ++m_waiting_writers;
        m_cv.wait(lk, [this] { return m_shared_count == 0 && !m_write_held; });
        --m_waiting_writers;
        m_write_held = true;
    }

    bool try_lock() {
        std::unique_lock lk{m_mtx, std::try_to_lock};
        if (!lk) {
            return false;
        }
        if (m_shared_count != 0 || m_write_held) {
            return false;
        }
        m_write_held = true;
        return true;
    }

    template <class Rep, class Period>
    bool try_lock_for(const std::chrono::duration<Rep, Period>& rel_time) {
        return try_lock_until(std::chrono::steady_clock::now() + rel_time);
    }

    template <class Clock, class Duration>
    bool try_lock_until(const std::chrono::time_point<Clock, Duration>& abs_time) {
        std::unique_lock lk{m_mtx};
        ++m_waiting_writers;
        const bool acquired =
            m_cv.wait_until(lk, abs_time, [this] { return m_shared_count == 0 && !m_write_held; });
        --m_waiting_writers;
        if (acquired) {
            m_write_held = true;
        }
        return acquired;
    }

    void unlock() {
        {
            std::lock_guard lk{m_mtx};
            m_write_held = false;
        }
        m_cv.notify_all();
    }

    void lock_shared() {
        std::unique_lock lk{m_mtx};
        m_cv.wait(lk, [this] { return !m_write_held; });
        ++m_shared_count;
    }

    bool try_lock_shared() {
        std::unique_lock lk{m_mtx, std::try_to_lock};
        if (!lk) {
            return false;
        }
        if (m_write_held) {
            return false;
        }
        ++m_shared_count;
        return true;
    }

    template <class Rep, class Period>
    bool try_lock_shared_for(const std::chrono::duration<Rep, Period>& rel_time) {
        return try_lock_shared_until(std::chrono::steady_clock::now() + rel_time);
    }

    template <class Clock, class Duration>
    bool try_lock_shared_until(const std::chrono::time_point<Clock, Duration>& abs_time) {
        std::unique_lock lk{m_mtx};
        const bool acquired = m_cv.wait_until(lk, abs_time, [this] { return !m_write_held; });
        if (acquired) {
            ++m_shared_count;
        }
        return acquired;
    }

    void unlock_shared() {
        bool notify = false;
        {
            std::lock_guard lk{m_mtx};
            if (--m_shared_count == 0) {
                notify = true;
            }
        }
        if (notify) {
            m_cv.notify_all();
        }
    }

private:
    std::mutex m_mtx;
    std::condition_variable m_cv;
    // Number of threads currently holding a shared (read) lock.
    int m_shared_count{0};
    // True while a thread holds the exclusive (write) lock.
    bool m_write_held{false};
    // Number of threads blocked in lock() / try_lock_for() / try_lock_until()
    // waiting to acquire the exclusive lock
    int m_waiting_writers{0};
};

} // namespace Common
