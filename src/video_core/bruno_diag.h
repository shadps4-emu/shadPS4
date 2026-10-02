// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include <chrono>
#include <cstdlib>

#include "common/types.h"

// Temporary diagnostics for the GoW3 stripes investigation (gow3BrunoLixFix).
// Events are logged only during a short window after BRUNO_DIAG_START seconds
// (default 100) and up to BRUNO_DIAG_EVENTS events (default 700).
namespace BrunoDiag {

inline bool On() {
    static const auto start = std::chrono::steady_clock::now();
    static const int start_s = [] {
        const char* e = std::getenv("BRUNO_DIAG_START");
        return e ? std::atoi(e) : 100;
    }();
    static std::atomic<int> budget = [] {
        const char* e = std::getenv("BRUNO_DIAG_EVENTS");
        return e ? std::atoi(e) : 700;
    }();
    if (start_s < 0) {
        return false;
    }
    const auto secs =
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start)
            .count();
    if (secs < start_s) {
        return false;
    }
    return budget.fetch_sub(1) > 0;
}

/// Experiment switches, read once from the environment (BRUNO_<name>=1).
inline bool Flag(const char* name) {
    const char* e = std::getenv(name);
    return e && e[0] == '1';
}

} // namespace BrunoDiag
