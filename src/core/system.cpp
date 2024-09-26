// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "common/native_clock.h"
#include "libraries/kernel/event_queues.h"
#include "libraries/kernel/time_management.h"
#include "libraries/system/msgdialog.h"
#include "system.h"

void SystemState::AddCurrentThreadToGuestList() {
    std::lock_guard lock{guest_threads_mutex};
    ThreadID id;
#ifdef _WIN32
    id = GetCurrentThreadId();
#else
    id = pthread_self();
#endif
    guest_threads.push_back(id);
}

void SystemState::RemoveCurrentThreadFromGuestList() {
    std::lock_guard lock{guest_threads_mutex};
    ThreadID id;
#ifdef _WIN32
    id = GetCurrentThreadId();
#else
    id = pthread_self();
#endif
    std::erase_if(guest_threads, [&](const ThreadID& v) { return v == id; });
}

void SystemState::PauseGuestThreads() {
    using namespace Libraries::MsgDialog;
    std::lock_guard lock{guest_threads_mutex};
    if (is_guest_threads_paused) {
        return;
    }

    for (const auto& id : guest_threads) {
#ifdef _WIN32
        const HANDLE hd = OpenThread(THREAD_SUSPEND_RESUME, FALSE, id);
        SuspendThread(hd);
        CloseHandle(hd);
#else
        pthread_kill(id, SIGUSR1);
#endif
    }
    pause_time = Libraries::Kernel::Dev::GetClock()->GetUptime();
    is_guest_threads_paused = true;
}

void SystemState::ResumeGuestThreads() {
    std::lock_guard lock{guest_threads_mutex};
    if (!is_guest_threads_paused) {
        return;
    }

    u64 delta_time = Libraries::Kernel::Dev::GetClock()->GetUptime() - pause_time;
    Libraries::Kernel::Dev::GetInitialPtc() += delta_time;
    for (const auto& id : guest_threads) {
#ifdef _WIN32
        const HANDLE hd = OpenThread(THREAD_SUSPEND_RESUME, FALSE, id);
        ResumeThread(hd);
        CloseHandle(hd);
#else
        pthread_kill(id, SIGUSR1);
#endif
    }
    is_guest_threads_paused = false;
}
