// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/network/net.h"

#include <mutex>
#include <vector>

#ifdef _WIN32
#include <wepoll.h>
#endif

#if defined(__linux__) || defined(__APPLE__)
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
    const char* name;
    epoll_handle epoll_fd;

    explicit Epoll(const char* name_) : name(name_), epoll_fd(epoll_create1(0)) {
        ASSERT(epoll_fd != -1);
        if (name == nullptr) {
            name = "anon";
        }
    }

    bool Destroyed() const noexcept {
        return destroyed;
    }

    void Destroy() noexcept {
        events.clear();
#ifdef _WIN32
        epoll_close(epoll_fd);
#else
        close(epoll_fd);
#endif
        epoll_fd = -1;
        name = "";
        destroyed = true;
    }

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
    std::vector<Epoll> epolls;
    std::mutex m_mutex;
};

} // namespace Libraries::Net