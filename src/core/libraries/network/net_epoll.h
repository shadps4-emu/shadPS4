// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/network/net.h"

#include <mutex>
#include <vector>
#include <unistd.h>

#ifdef __linux__
#include <sys/epoll.h>
#endif

namespace Libraries::Net {

struct Epoll {
    std::vector<std::pair<u32 /*netId*/, OrbisNetEpollEvent>> events{};
    const char* name;
    int epoll_fd;

    explicit Epoll(const char* name_) : name(name_), epoll_fd(epoll_create1(0)) {
        ASSERT(epoll_fd != -1);
    }

    bool Destroyed() const noexcept {
        return destroyed;
    }

    void Destroy() noexcept {
        events.clear();
        close(epoll_fd);
        epoll_fd = -1;
        name = nullptr;
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