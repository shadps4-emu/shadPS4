// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/network/net.h"

#include <atomic>
#include <deque>
#include <memory>
#include <mutex>
#include <vector>

#ifdef _WIN32
#include <wepoll.h>
#endif

#if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__)
#include <sys/epoll.h>
#include <unistd.h>
#endif

namespace Libraries::Net {

#ifdef _WIN32
using epoll_handle = HANDLE;
#else
using epoll_handle = int;
#endif

struct Epoll {
    std::vector<std::pair<u32 /*netId*/, OrbisNetEpollEvent>> events{};
    std::string name;
    epoll_handle epoll_fd;
    std::deque<u32> async_resolutions{};
    std::atomic<bool> aborted{false};
#ifdef __linux__
    int abort_fd = -1;
#endif

    explicit Epoll(const char* name_);

    // Signal the epoll to abort any blocking wait
    void Abort();

    // Drain the abort fd after waking up, reset aborted flag
    void ClearAbort();

    bool Destroyed() const noexcept {
        return destroyed;
    }

    void Destroy() noexcept;

private:
    bool destroyed{};
};

u32 ConvertEpollEventsIn(u32 orbis_events);
u32 ConvertEpollEventsOut(u32 epoll_events);

class EpollTable {
public:
    EpollTable() = default;
    virtual ~EpollTable() = default;

    int CreateHandle(const char* name);
    void DeleteHandle(int d);
    Epoll* GetEpoll(int d);

private:
    std::vector<std::unique_ptr<Epoll>> epolls;
    std::mutex m_mutex;
};

} // namespace Libraries::Net
