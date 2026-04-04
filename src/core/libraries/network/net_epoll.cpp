// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <memory>
#include "common/assert.h"
#include "common/logging/log.h"
#include "common/types.h"
#include "net_epoll.h"
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#elif defined(__linux__)
#include <sys/eventfd.h>
#else
#include <fcntl.h>
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

    // Set up the abort wake mechanism and register it in epoll with sentinel data.fd = -1.
    // Linux: eventfd, macOS/BSD: self-pipe, Windows: loopback UDP socket.
#ifdef _WIN32
    abort_sock = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    ASSERT(abort_sock != INVALID_SOCKET);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0; // OS picks an ephemeral port
    ASSERT(::bind(abort_sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0);
    // Connect to self so we can use send() in Abort()
    socklen_t addrlen = sizeof(addr);
    ASSERT(::getsockname(abort_sock, reinterpret_cast<sockaddr*>(&addr), &addrlen) == 0);
    ASSERT(::connect(abort_sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0);
    u_long nonblocking = 1;
    ASSERT(::ioctlsocket(abort_sock, FIONBIO, &nonblocking) == 0);
    epoll_event ev = {.events = EPOLLIN, .data = {.fd = -1}};
    ASSERT(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, abort_sock, &ev) == 0);
#elif defined(__linux__)
    abort_fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    ASSERT(abort_fd != -1);
    epoll_event ev = {.events = EPOLLIN, .data = {.fd = -1}};
    ASSERT(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, abort_fd, &ev) == 0);
#else
    // Self-pipe for macOS/BSD
    int r = ::pipe(abort_pipe);
    ASSERT(r == 0);
    // Set both ends non-blocking
    for (int i = 0; i < 2; ++i) {
        int flags = ::fcntl(abort_pipe[i], F_GETFL);
        ::fcntl(abort_pipe[i], F_SETFL, flags | O_NONBLOCK);
        ::fcntl(abort_pipe[i], F_SETFD, FD_CLOEXEC);
    }
    epoll_event ev = {.events = EPOLLIN, .data = {.fd = -1}};
    ASSERT(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, abort_pipe[0], &ev) == 0);
#endif
}

void Epoll::Abort() {
    aborted.store(true, std::memory_order_release);
#ifdef _WIN32
    if (abort_sock != INVALID_SOCKET) {
        char byte = 1;
        (void)::send(abort_sock, &byte, 1, 0);
    }
#elif defined(__linux__)
    if (abort_fd != -1) {
        uint64_t val = 1;
        (void)::write(abort_fd, &val, sizeof(val));
    }
#else
    if (abort_pipe[1] != -1) {
        char byte = 1;
        (void)::write(abort_pipe[1], &byte, 1);
    }
#endif
}

void Epoll::ClearAbort() {
    aborted.store(false, std::memory_order_release);
#ifdef _WIN32
    if (abort_sock != INVALID_SOCKET) {
        char buf[64];
        // Drain any pending data
        while (::recv(abort_sock, buf, sizeof(buf), 0) > 0) {
        }
    }
#elif defined(__linux__)
    if (abort_fd != -1) {
        uint64_t val;
        (void)::read(abort_fd, &val, sizeof(val));
    }
#else
    if (abort_pipe[0] != -1) {
        char buf[64];
        // Drain the pipe
        while (::read(abort_pipe[0], buf, sizeof(buf)) > 0) {
        }
    }
#endif
}

void Epoll::Destroy() noexcept {
    events.clear();
#ifdef _WIN32
    if (abort_sock != INVALID_SOCKET) {
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, abort_sock, nullptr);
        ::closesocket(abort_sock);
        abort_sock = INVALID_SOCKET;
    }
    epoll_close(epoll_fd);
    epoll_fd = nullptr;
#else
#ifdef __linux__
    if (abort_fd != -1) {
        close(abort_fd);
        abort_fd = -1;
    }
#else
    for (int i = 0; i < 2; ++i) {
        if (abort_pipe[i] != -1) {
            close(abort_pipe[i]);
            abort_pipe[i] = -1;
        }
    }
#endif
    close(epoll_fd);
    epoll_fd = -1;
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
