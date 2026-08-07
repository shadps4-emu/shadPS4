// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <chrono>
#include <condition_variable>
#include <mutex>

namespace Common {

// Like std::shared_mutex, but reader has priority over writer.
class SharedFirstMutex {
public:
    void lock() {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this]() { return !writer_active && readers == 0; });
        writer_active = true;
    }

    bool try_lock() {
        std::lock_guard<std::mutex> lock(mtx);
        if (writer_active || readers > 0) {
            return false;
        }
        writer_active = true;
        return true;
    }

    template <typename Clock, typename Duration>
    bool try_lock_until(const std::chrono::time_point<Clock, Duration>& abs_time) {
        std::unique_lock<std::mutex> lock(mtx);
        if (!cv.wait_until(lock, abs_time, [this]() { return !writer_active && readers == 0; })) {
            return false;
        }
        writer_active = true;
        return true;
    }

    void unlock() {
        std::lock_guard<std::mutex> lock(mtx);
        writer_active = false;
        cv.notify_all();
    }

    void lock_shared() {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this]() { return !writer_active; });
        ++readers;
    }

    bool try_lock_shared() {
        std::lock_guard<std::mutex> lock(mtx);
        if (writer_active) {
            return false;
        }
        ++readers;
        return true;
    }

    template <typename Clock, typename Duration>
    bool try_lock_shared_until(const std::chrono::time_point<Clock, Duration>& abs_time) {
        std::unique_lock<std::mutex> lock(mtx);
        if (!cv.wait_until(lock, abs_time, [this]() { return !writer_active; })) {
            return false;
        }
        ++readers;
        return true;
    }

    void unlock_shared() {
        std::lock_guard<std::mutex> lock(mtx);
        if (--readers == 0) {
            cv.notify_all();
        }
    }

private:
    std::mutex mtx;
    std::condition_variable cv;
    int readers = 0;
    bool writer_active = false;
};

} // namespace Common
