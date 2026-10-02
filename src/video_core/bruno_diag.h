// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <filesystem>

#include "common/types.h"

// Temporary diagnostics for the GoW3 stripes investigation (gow3BrunoLixFix).
// Event logging is armed by creating a file named "bruno_diag_on" in the working
// directory (checked about once per second). Each time the file appears, the next
// BRUNO_DIAG_EVENTS events (default 900) are logged and the file is deleted.
namespace BrunoDiag {

inline bool On() {
    static std::atomic<int> budget{0};
    static std::atomic<s64> last_check_ms{0};
    static const int events = [] {
        const char* e = std::getenv("BRUNO_DIAG_EVENTS");
        return e ? std::atoi(e) : 900;
    }();

    const s64 now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::steady_clock::now().time_since_epoch())
                           .count();
    s64 last = last_check_ms.load(std::memory_order_relaxed);
    if (now_ms - last > 1000 &&
        last_check_ms.compare_exchange_strong(last, now_ms, std::memory_order_relaxed)) {
        std::error_code ec;
        if (std::filesystem::exists("bruno_diag_on", ec)) {
            std::filesystem::remove("bruno_diag_on", ec);
            budget.store(events, std::memory_order_relaxed);
        }
    }
    if (budget.load(std::memory_order_relaxed) <= 0) {
        return false;
    }
    return budget.fetch_sub(1, std::memory_order_relaxed) > 0;
}

/// Runtime experiment switch: true while a file with this name exists in the working
/// directory (re-checked about once per second). Use one static slot per call site.
struct FileFlag {
    const char* name;
    std::atomic<s64> last_check_ms{0};
    std::atomic<bool> value{false};

    explicit FileFlag(const char* name_) : name{name_} {}

    bool Get() {
        const s64 now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::steady_clock::now().time_since_epoch())
                               .count();
        s64 last = last_check_ms.load(std::memory_order_relaxed);
        if (now_ms - last > 1000 &&
            last_check_ms.compare_exchange_strong(last, now_ms, std::memory_order_relaxed)) {
            std::error_code ec;
            value.store(std::filesystem::exists(name, ec), std::memory_order_relaxed);
        }
        return value.load(std::memory_order_relaxed);
    }
};

/// Experiment switches, read from the environment (BRUNO_<name>=1).
inline bool Flag(const char* name) {
    const char* e = std::getenv(name);
    return e && e[0] == '1';
}

} // namespace BrunoDiag
