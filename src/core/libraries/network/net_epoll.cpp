// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "common/types.h"
#include "net_epoll.h"

namespace Libraries::Net {

u32 ConvertEpollEventsIn(u32 orbis_events) {
    u32 ret = 0;

    if ((orbis_events & ORBIS_NET_EPOLLIN) != 0) {
        ret |= EPOLLIN;
    }
    if ((orbis_events & ORBIS_NET_EPOLLOUT) != 0) {
        ret |= EPOLLOUT;
    }

    return ret;
}

u32 ConvertEpollEventsOut(u32 epoll_events) {
    u32 ret = 0;

    if ((epoll_events & EPOLLIN) != 0) {
        ret |= ORBIS_NET_EPOLLIN;
    }
    if ((epoll_events & EPOLLOUT) != 0) {
        ret |= ORBIS_NET_EPOLLOUT;
    }
    if ((epoll_events & EPOLLERR) != 0) {
        ret |= ORBIS_NET_EPOLLERR;
    }
    if ((epoll_events & EPOLLHUP) != 0) {
        ret |= ORBIS_NET_EPOLLHUP;
    }

    return ret;
}

int EpollTable::CreateHandle(const char* name) {
    std::scoped_lock lock{m_mutex};

    if (auto it = std::ranges::find_if(epolls, [](const Epoll& e) { return e.Destroyed(); });
        it != epolls.end()) {
        *it = Epoll(name);
        const auto ret = std::distance(epolls.begin(), it);
        LOG_DEBUG(Lib_Net, "epollid = {}", ret);
        return ret;
    } else {
        epolls.emplace_back(name);
        const auto ret = epolls.size() - 1;
        LOG_DEBUG(Lib_Net, "epollid = {}", ret);
        return ret;
    }
}

void EpollTable::DeleteHandle(int d) {
    UNREACHABLE();
}

Epoll* EpollTable::GetEpoll(int epollid) {
    std::scoped_lock lock{m_mutex};
    if (epollid >= epolls.size() || epolls[epollid].Destroyed()) {
        return nullptr;
    }

    return &epolls[epollid];
}

} // namespace Libraries::Net
