// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <mutex>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#include <Windows.h>
using ThreadID = DWORD;
#else
#include <pthread.h>
#include <signal.h>
using ThreadID = pthread_t;
#endif

class SystemState {
    std::mutex guest_threads_mutex{};
    std::vector<ThreadID> guest_threads{};
    bool is_guest_threads_paused = false;
    u64 pause_time{};

public:
    void AddCurrentThreadToGuestList();

    void RemoveCurrentThreadFromGuestList();

    void PauseGuestThreads();

    void ResumeGuestThreads();

    inline bool IsGuestThreadsPaused() const {
        return is_guest_threads_paused;
    }
};
