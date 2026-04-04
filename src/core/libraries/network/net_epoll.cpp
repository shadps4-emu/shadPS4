// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <memory>
#include "common/assert.h"
#include "common/logging/log.h"
#include "common/types.h"
#include "net_epoll.h"
#ifdef __linux__
#include <sys/eventfd.h>
#endif

namespace Libraries::Net {

u32 ConvertEpollEventsIn(u32 orbis_events) {
    u32 ret = 0;

#ifndef __FreeBSD__
    if ((orbis_events & ORBIS_NET_EPOLLIN) != 0) {
        ret |= EPOLLIN;
    }
    if ((orbis_events & ORBIS_NET_EPOLLOUT) != 0) {
        ret |= EPOLLOUT;
    }
#endif

    return ret;
}

u32 ConvertEpollEventsOut(u32 epoll_events) {
    u32 ret = 0;

#ifndef __FreeBSD__
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
#endif

    return ret;
}

Epoll::Epoll(const char* name_) : name(name_ ? name_ : "anon"), epoll_fd(epoll_create1(0)) {
#ifdef _WIN32
    ASSERT(epoll_fd != nullptr);
#else
    ASSERT(epoll_fd != -1);
#endif
#ifdef __linux__
    abort_fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    ASSERT(abort_fd != -1);
    epoll_event ev = {.events = EPOLLIN, .data = {.fd = -1}};
    ASSERT(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, abort_fd, &ev) == 0);
#endif
}

void Epoll::Abort() {
    aborted.store(true, std::memory_order_release);
#ifdef __linux__
    if (abort_fd != -1) {
        uint64_t val = 1;
        ::write(abort_fd, &val, sizeof(val));
    }
#endif
}

void Epoll::ClearAbort() {
    aborted.store(false, std::memory_order_release);
#ifdef __linux__
    if (abort_fd != -1) {
        uint64_t val;
        ::read(abort_fd, &val, sizeof(val));
    }
#endif
}

void Epoll::Destroy() noexcept {
    events.clear();
#ifdef _WIN32
    epoll_close(epoll_fd);
    epoll_fd = nullptr;
#else
    close(epoll_fd);
    epoll_fd = -1;
#ifdef __linux__
    if (abort_fd != -1) {
        close(abort_fd);
        abort_fd = -1;
    }
#endif
#endif
    name = "";
    destroyed = true;
}

int EpollTable::CreateHandle(const char* name) {
    std::scoped_lock lock{m_mutex};

    if (auto it = std::ranges::find_if(epolls, [](const auto& e) { return e && e->Destroyed(); });
        it != epolls.end()) {
        *it = std::make_unique<Epoll>(name);
        const auto ret = std::distance(epolls.begin(), it);
        LOG_DEBUG(Lib_Net, "epollid = {}", ret);
        return ret;
    } else {
        epolls.push_back(std::make_unique<Epoll>(name));
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
    if (epollid >= epolls.size() || !epolls[epollid] || epolls[epollid]->Destroyed()) {
        return nullptr;
    }

    return epolls[epollid].get();
}

} // namespace Libraries::Net
