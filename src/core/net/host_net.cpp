// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <array>
#include <climits>
#include <cstdint>

#include "core/net/host_net.h"

#ifdef _WIN32
#include <mstcpip.h>
#include <wepoll.h>
#ifdef _MSC_VER
#pragma comment(lib, "Ws2_32.lib")
#endif
#ifndef SIO_UDP_CONNRESET
#define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR, 12)
#endif
#define HOST_NET_EPOLL 1
#else
#include <cerrno>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <unistd.h>
#ifdef __linux__
#include <sys/epoll.h>
#include <sys/eventfd.h>
#define HOST_NET_EPOLL 1
#else
#include <sys/event.h>
#include <sys/time.h>
#define HOST_NET_KQUEUE 1
#endif
#endif

namespace Core::Net::Host {

int TimeoutMs(const Deadline& deadline) {
    if (!deadline) {
        return -1;
    }
    const auto left = *deadline - Clock::now();
    if (left <= Clock::duration::zero()) {
        return 0;
    }
    // Round up to avoid waking early. Callers loop anyway.
    const auto ms = std::chrono::ceil<std::chrono::milliseconds>(left).count();
    return static_cast<int>(std::min<s64>(ms, INT_MAX));
}

bool Expired(const Deadline& deadline) {
    return deadline && Clock::now() >= *deadline;
}

bool SetNonBlocking(NativeSocket s) {
#ifdef _WIN32
    u_long on = 1;
    return ioctlsocket(s, FIONBIO, &on) == 0;
#else
    const int flags = fcntl(s, F_GETFL, 0);
    return flags >= 0 && fcntl(s, F_SETFL, flags | O_NONBLOCK) == 0;
#endif
}

#ifndef _WIN32
bool SetCloseOnExec(int fd) {
    const int flags = fcntl(fd, F_GETFD, 0);
    return flags >= 0 && fcntl(fd, F_SETFD, flags | FD_CLOEXEC) == 0;
}
#endif

Deadline DeadlineFromSocketTimeout(std::chrono::microseconds timeout) {
    if (timeout.count() <= 0) {
        return std::nullopt;
    }
    return Clock::now() + timeout;
}

// Errors
Error TranslateNative(int code) {
    struct Entry {
        int host;
        Error error;
    };
#ifdef _WIN32
    static constexpr Entry Table[] = {
        {WSAEINTR, Error::Intr},
        {WSAEBADF, Error::BadF},
        {WSAEACCES, Error::Acces},
        {WSAEFAULT, Error::Fault},
        {WSAEINVAL, Error::Inval},
        {WSAEMFILE, Error::MFile},
        {WSAEWOULDBLOCK, Error::WouldBlock},
        {WSAEINPROGRESS, Error::InProgress},
        {WSAEALREADY, Error::Already},
        {WSAENOTSOCK, Error::NotSock},
        {WSAEDESTADDRREQ, Error::DestAddrReq},
        {WSAEMSGSIZE, Error::MsgSize},
        {WSAEPROTOTYPE, Error::ProtoType},
        {WSAENOPROTOOPT, Error::NoProtoOpt},
        {WSAEPROTONOSUPPORT, Error::ProtoNoSupport},
        {WSAESOCKTNOSUPPORT, Error::SocktNoSupport},
        {WSAEOPNOTSUPP, Error::OpNotSupp},
        {WSAEPFNOSUPPORT, Error::PfNoSupport},
        {WSAEAFNOSUPPORT, Error::AfNoSupport},
        {WSAEADDRINUSE, Error::AddrInUse},
        {WSAEADDRNOTAVAIL, Error::AddrNotAvail},
        {WSAENETDOWN, Error::NetDown},
        {WSAENETUNREACH, Error::NetUnreach},
        {WSAENETRESET, Error::NetReset},
        {WSAECONNABORTED, Error::ConnAborted},
        {WSAECONNRESET, Error::ConnReset},
        {WSAENOBUFS, Error::NoBufs},
        {WSAEISCONN, Error::IsConn},
        {WSAENOTCONN, Error::NotConn},
        {WSAESHUTDOWN, Error::Pipe}, // BSD gives EPIPE after shutdown
        {WSAETOOMANYREFS, Error::TooManyRefs},
        {WSAETIMEDOUT, Error::TimedOut},
        {WSAECONNREFUSED, Error::ConnRefused},
        {WSAELOOP, Error::Loop},
        {WSAENAMETOOLONG, Error::NameTooLong},
        {WSAEHOSTDOWN, Error::HostDown},
        {WSAEHOSTUNREACH, Error::HostUnreach},
        {WSAENOTEMPTY, Error::NotEmpty},
        {WSA_NOT_ENOUGH_MEMORY, Error::NoMem},
        {WSAECANCELLED, Error::Canceled},
    };
#else
    static constexpr Entry Table[] = {
        {EPERM, Error::Perm},
        {ENOENT, Error::NoEnt},
        {EINTR, Error::Intr},
        {EIO, Error::Io},
        {EBADF, Error::BadF},
        {ENOMEM, Error::NoMem},
        {EACCES, Error::Acces},
        {EFAULT, Error::Fault},
        {EBUSY, Error::Busy},
        {EEXIST, Error::Exist},
        {ENODEV, Error::NoDev},
        {EINVAL, Error::Inval},
        {ENFILE, Error::NFile},
        {EMFILE, Error::MFile},
        {ENOSPC, Error::NoSpc},
        {EPIPE, Error::Pipe},
        {EAGAIN, Error::WouldBlock},
#if EWOULDBLOCK != EAGAIN
        {EWOULDBLOCK, Error::WouldBlock},
#endif
        {EINPROGRESS, Error::InProgress},
        {EALREADY, Error::Already},
        {ENOTSOCK, Error::NotSock},
        {EDESTADDRREQ, Error::DestAddrReq},
        {EMSGSIZE, Error::MsgSize},
        {EPROTOTYPE, Error::ProtoType},
        {ENOPROTOOPT, Error::NoProtoOpt},
        {EPROTONOSUPPORT, Error::ProtoNoSupport},
        {ESOCKTNOSUPPORT, Error::SocktNoSupport},
        {EOPNOTSUPP, Error::OpNotSupp},
#if ENOTSUP != EOPNOTSUPP
        {ENOTSUP, Error::OpNotSupp},
#endif
        {EPFNOSUPPORT, Error::PfNoSupport},
        {EAFNOSUPPORT, Error::AfNoSupport},
        {EADDRINUSE, Error::AddrInUse},
        {EADDRNOTAVAIL, Error::AddrNotAvail},
        {ENETDOWN, Error::NetDown},
        {ENETUNREACH, Error::NetUnreach},
        {ENETRESET, Error::NetReset},
        {ECONNABORTED, Error::ConnAborted},
        {ECONNRESET, Error::ConnReset},
        {ENOBUFS, Error::NoBufs},
        {EISCONN, Error::IsConn},
        {ENOTCONN, Error::NotConn},
        {ESHUTDOWN, Error::Pipe},
        {ETOOMANYREFS, Error::TooManyRefs},
        {ETIMEDOUT, Error::TimedOut},
        {ECONNREFUSED, Error::ConnRefused},
        {ELOOP, Error::Loop},
        {ENAMETOOLONG, Error::NameTooLong},
        {EHOSTDOWN, Error::HostDown},
        {EHOSTUNREACH, Error::HostUnreach},
        {ENOTEMPTY, Error::NotEmpty},
        {ECANCELED, Error::Canceled},
    };
#endif
    if (code == 0) {
        return Error::Ok;
    }
    for (const auto& entry : Table) {
        if (entry.host == code) {
            return entry.error;
        }
    }
    return Error::Internal;
}

Error LastError() {
#ifdef _WIN32
    return TranslateNative(WSAGetLastError());
#else
    return TranslateNative(errno);
#endif
}

bool Initialize() {
#ifdef _WIN32
    WSADATA data;
    return WSAStartup(MAKEWORD(2, 2), &data) == 0;
#else
    return true;
#endif
}

void Shutdown() {
#ifdef _WIN32
    WSACleanup();
#endif
}

// Sockets
bool ConfigureSocket(NativeSocket s, int type) {
    if (!SetNonBlocking(s)) {
        return false;
    }
#ifdef _WIN32
    if (type == SOCK_DGRAM) {
        // Otherwise an ICMP port-unreachable from an earlier sendto() makes the next
        // recvfrom() fail with WSAECONNRESET. BSD doesn't do that for UDP.
        BOOL report = FALSE;
        DWORD bytes = 0;
        WSAIoctl(s, SIO_UDP_CONNRESET, &report, sizeof(report), nullptr, 0, &bytes, nullptr,
                 nullptr);
    }
#else
    (void)type;
    SetCloseOnExec(s);
#ifdef SO_NOSIGPIPE
    // Don't let a dead peer SIGPIPE the emulator.
    int on = 1;
    setsockopt(s, SOL_SOCKET, SO_NOSIGPIPE, &on, sizeof(on));
#endif
#endif
    return true;
}

NativeSocket CreateSocket(int family, int type, int protocol, Error* error) {
#ifdef _WIN32
    NativeSocket s = WSASocketW(family, type, protocol, nullptr, 0,
                                WSA_FLAG_OVERLAPPED | WSA_FLAG_NO_HANDLE_INHERIT);
#elif defined(SOCK_NONBLOCK) && defined(SOCK_CLOEXEC)
    NativeSocket s = socket(family, type | SOCK_NONBLOCK | SOCK_CLOEXEC, protocol);
#else
    NativeSocket s = socket(family, type, protocol);
#endif
    if (s == InvalidSocket) {
        *error = LastError();
        return InvalidSocket;
    }
    if (!ConfigureSocket(s, type)) {
        *error = LastError();
        CloseSocket(s);
        return InvalidSocket;
    }
    *error = Error::Ok;
    return s;
}

Error CreateSocketPair(int family, int type, int protocol, NativeSocket out[2]) {
    out[0] = out[1] = InvalidSocket;
#ifdef _WIN32
    (void)family;
    (void)protocol;
    if (type != SOCK_STREAM) {
        return Error::ProtoNoSupport;
    }
    // Connect through a loopback listener and make sure we accepted our own client.
    Error e;
    NativeSocket listener = CreateSocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, &e);
    if (listener == InvalidSocket) {
        return e;
    }
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    socklen_t len = sizeof(addr);
    NativeSocket client = InvalidSocket;
    NativeSocket server = InvalidSocket;
    const auto fail = [&](Error error) {
        CloseSocket(listener);
        CloseSocket(client);
        CloseSocket(server);
        return error;
    };
    if (bind(listener, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0 ||
        listen(listener, 1) != 0 ||
        getsockname(listener, reinterpret_cast<sockaddr*>(&addr), &len) != 0) {
        return fail(LastError());
    }
    client = CreateSocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, &e);
    if (client == InvalidSocket) {
        return fail(e);
    }
    if (connect(client, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0 &&
        LastError() != Error::WouldBlock && LastError() != Error::InProgress) {
        return fail(LastError());
    }
    WSAPOLLFD poll{listener, POLLRDNORM, 0};
    if (WSAPoll(&poll, 1, 5000) != 1) {
        return fail(Error::TimedOut);
    }
    server = Accept(listener, nullptr, nullptr, &e);
    if (server == InvalidSocket) {
        return fail(e);
    }
    sockaddr_in client_name{};
    sockaddr_in server_peer{};
    socklen_t client_len = sizeof(client_name);
    socklen_t peer_len = sizeof(server_peer);
    getsockname(client, reinterpret_cast<sockaddr*>(&client_name), &client_len);
    getpeername(server, reinterpret_cast<sockaddr*>(&server_peer), &peer_len);
    if (client_name.sin_port != server_peer.sin_port) {
        return fail(Error::ConnAborted); // someone else got in first
    }
    WSAPOLLFD writable{client, POLLWRNORM, 0};
    WSAPoll(&writable, 1, 5000);
    CloseSocket(listener);
    out[0] = client;
    out[1] = server;
    return Error::Ok;
#else
    int fds[2];
    if (socketpair(family, type, protocol, fds) != 0) {
        return LastError();
    }
    if (!ConfigureSocket(fds[0], type) || !ConfigureSocket(fds[1], type)) {
        const Error e = LastError();
        CloseSocket(fds[0]);
        CloseSocket(fds[1]);
        return e;
    }
    out[0] = fds[0];
    out[1] = fds[1];
    return Error::Ok;
#endif
}

void CloseSocket(NativeSocket s) {
    if (s == InvalidSocket) {
        return;
    }
#ifdef _WIN32
    closesocket(s);
#else
    close(s); // don't retry on EINTR, the fd may already be gone
#endif
}

Error PrepareBind(NativeSocket s, bool reuse_addr, bool reuse_port) {
#ifdef _WIN32
    // Windows SO_REUSEADDR behaves like BSD SO_REUSEPORT, which is also what games want for
    // multicast. Without reuse, SO_EXCLUSIVEADDRUSE is the closest match to BSD.
    const int opt = (reuse_addr || reuse_port) ? SO_REUSEADDR : SO_EXCLUSIVEADDRUSE;
    BOOL on = TRUE;
    if (setsockopt(s, SOL_SOCKET, opt, reinterpret_cast<const char*>(&on), sizeof(on)) != 0) {
        return LastError();
    }
#else
    int on = reuse_addr ? 1 : 0;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) != 0) {
        return LastError();
    }
#ifdef SO_REUSEPORT
    on = reuse_port ? 1 : 0;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on)) != 0) {
        return LastError();
    }
#endif
#endif
    return Error::Ok;
}

void PrepareClose(NativeSocket s, bool guest_blocking) {
    linger value{};
    socklen_t len = sizeof(value);
    if (getsockopt(s, SOL_SOCKET, LingerOption, reinterpret_cast<char*>(&value), &len) != 0 ||
        value.l_onoff == 0 || value.l_linger <= 0) {
        return; // no linger or linger 0, same in every mode
    }
    if (guest_blocking) {
#ifdef _WIN32
        u_long off = 0;
        ioctlsocket(s, FIONBIO, &off);
#else
        const int flags = fcntl(s, F_GETFL, 0);
        if (flags >= 0) {
            fcntl(s, F_SETFL, flags & ~O_NONBLOCK);
        }
#endif
        return;
    }
    value.l_onoff = 0;
    setsockopt(s, SOL_SOCKET, LingerOption, reinterpret_cast<const char*>(&value), sizeof(value));
}

Error Bind(NativeSocket s, const sockaddr* addr, socklen_t len) {
    return bind(s, addr, len) == 0 ? Error::Ok : LastError();
}

Error Listen(NativeSocket s, int backlog) {
    return listen(s, backlog) == 0 ? Error::Ok : LastError();
}

Error ShutdownSocket(NativeSocket s, int how) {
    return shutdown(s, how) == 0 ? Error::Ok : LastError();
}

Error Connect(NativeSocket s, const sockaddr* addr, socklen_t len) {
    if (connect(s, addr, len) == 0) {
        return Error::Ok;
    }
    const Error e = LastError();
#ifdef _WIN32
    // Winsock says WSAEWOULDBLOCK where BSD says EINPROGRESS.
    if (e == Error::WouldBlock) {
        return Error::InProgress;
    }
#endif
    return e;
}

Error PendingSocketError(NativeSocket s) {
    int value = 0;
    socklen_t len = sizeof(value);
    if (getsockopt(s, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&value), &len) != 0) {
        return LastError();
    }
    return TranslateNative(value);
}

IoResult BytesReadable(NativeSocket s) {
#ifdef _WIN32
    u_long n = 0;
    if (ioctlsocket(s, FIONREAD, &n) != 0) {
        return {0, LastError()};
    }
#else
    int n = 0;
    if (ioctl(s, FIONREAD, &n) != 0) {
        return {0, LastError()};
    }
#endif
    return {static_cast<s64>(n), Error::Ok};
}

NativeSocket Accept(NativeSocket s, sockaddr* addr, socklen_t* len, Error* error) {
    for (;;) {
#if defined(__linux__) || defined(__FreeBSD__)
        NativeSocket c = accept4(s, addr, len, SOCK_NONBLOCK | SOCK_CLOEXEC);
#else
        NativeSocket c = accept(s, addr, len);
#endif
        if (c == InvalidSocket) {
            const Error e = LastError();
            if (e == Error::Intr) {
                continue;
            }
            *error = e;
            return InvalidSocket;
        }
        // Whether accept() inherits O_NONBLOCK varies by platform.
        if (!ConfigureSocket(c, SOCK_STREAM)) {
            *error = LastError();
            CloseSocket(c);
            return InvalidSocket;
        }
        *error = Error::Ok;
        return c;
    }
}

IoResult SendTo(NativeSocket s, const void* buf, size_t len, int flags, const sockaddr* addr,
                socklen_t addr_len) {
#ifdef MSG_NOSIGNAL
    flags |= MSG_NOSIGNAL;
#endif
#ifdef _WIN32
    const int n_len = static_cast<int>(std::min<size_t>(len, INT_MAX));
    const char* p = static_cast<const char*>(buf);
    const int n = addr ? sendto(s, p, n_len, flags, addr, addr_len) : send(s, p, n_len, flags);
#else
    ssize_t n;
    do {
        n = addr ? sendto(s, buf, len, flags, addr, addr_len) : send(s, buf, len, flags);
    } while (n < 0 && errno == EINTR);
#endif
    if (n < 0) {
        return {-1, LastError()};
    }
    return {static_cast<s64>(n), Error::Ok};
}

IoResult RecvFrom(NativeSocket s, void* buf, size_t len, int flags, sockaddr* addr,
                  socklen_t* addr_len) {
#ifdef _WIN32
    const int n_len = static_cast<int>(std::min<size_t>(len, INT_MAX));
    char* p = static_cast<char*>(buf);
    const int n = addr ? recvfrom(s, p, n_len, flags, addr, addr_len) : recv(s, p, n_len, flags);
#else
    ssize_t n;
    do {
        n = addr ? recvfrom(s, buf, len, flags, addr, addr_len) : recv(s, buf, len, flags);
    } while (n < 0 && errno == EINTR);
#endif
    if (n < 0) {
        const Error e = LastError();
#ifdef _WIN32
        // Winsock fails truncated datagrams with WSAEMSGSIZE. BSD returns the length read.
        if (e == Error::MsgSize) {
            return {n_len, Error::Ok};
        }
#endif
        return {-1, e};
    }
    return {static_cast<s64>(n), Error::Ok};
}

WakeHandle::WakeHandle() {
#if defined(__linux__)
    read_ = write_ = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
#elif defined(_WIN32)
    // WSAPoll and wepoll only take sockets, so use a loopback UDP pair.
    read_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    write_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (read_ == InvalidSocket || write_ == InvalidSocket) {
        Reset();
        return;
    }
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    int addr_len = sizeof(addr);
    sockaddr_in peer{};
    int peer_len = sizeof(peer);
    const bool ok = bind(read_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0 &&
                    getsockname(read_, reinterpret_cast<sockaddr*>(&addr), &addr_len) == 0 &&
                    connect(write_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0 &&
                    getsockname(write_, reinterpret_cast<sockaddr*>(&peer), &peer_len) == 0 &&
                    // Only accept datagrams from our writer.
                    connect(read_, reinterpret_cast<sockaddr*>(&peer), sizeof(peer)) == 0 &&
                    ConfigureSocket(read_, SOCK_DGRAM) && ConfigureSocket(write_, SOCK_DGRAM);
    if (!ok) {
        Reset();
    }
#else
    int fds[2];
    if (pipe(fds) != 0) {
        return;
    }
    read_ = fds[0];
    write_ = fds[1];
    for (const int fd : fds) {
        if (!SetNonBlocking(fd) || !SetCloseOnExec(fd)) {
            Reset();
            return;
        }
    }
#endif
}

WakeHandle::~WakeHandle() {
    Reset();
}

void WakeHandle::Reset() {
#if defined(_WIN32)
    CloseSocket(read_);
    CloseSocket(write_);
#else
    if (read_ >= 0) {
        close(read_);
    }
    if (write_ >= 0 && write_ != read_) {
        close(write_);
    }
#endif
    read_ = write_ = InvalidSocket;
}

void WakeHandle::Signal() const {
    // Fails only when already full, which means already signalled.
#if defined(__linux__)
    const u64 one = 1;
    [[maybe_unused]] const auto r = write(write_, &one, sizeof(one));
#elif defined(_WIN32)
    const char byte = 1;
    send(write_, &byte, 1, 0);
#else
    const char byte = 1;
    [[maybe_unused]] const auto r = write(write_, &byte, 1);
#endif
}

void WakeHandle::Drain() const {
    char buf[64]; // eventfd needs at least 8
#ifdef _WIN32
    while (recv(read_, buf, sizeof(buf), 0) > 0) {
    }
#else
    while (read(read_, buf, sizeof(buf)) > 0) {
    }
#endif
}

WaitResult WaitOne(NativeSocket s, u32 interest, const WakeHandle& wake, Deadline deadline) {
#ifdef _WIN32
    WSAPOLLFD fds[2]{};
#else
    pollfd fds[2]{};
#endif
    fds[0].fd = s;
    fds[0].events = static_cast<short>(((interest & Readable) ? POLLIN : 0) |
                                       ((interest & Writable) ? POLLOUT : 0));
    fds[1].fd = wake.PollHandle();
    fds[1].events = POLLIN;

    for (;;) {
        fds[0].revents = fds[1].revents = 0;
#ifdef _WIN32
        const int n = WSAPoll(fds, 2, TimeoutMs(deadline));
#else
        const int n = poll(fds, 2, TimeoutMs(deadline));
#endif
        if (n < 0) {
#ifndef _WIN32
            if (errno == EINTR) {
                continue;
            }
#endif
            return WaitResult::Failed;
        }
        if (fds[1].revents != 0) {
            return WaitResult::Woken; // abort wins
        }
        if (fds[0].revents != 0) {
            return WaitResult::Ready; // the retry reports any error
        }
        if (Expired(deadline)) {
            return WaitResult::TimedOut;
        }
        // Woke early, wait again.
    }
}

#if HOST_NET_EPOLL // epoll or wepoll

u32 ToEpoll(u32 events, u32 flags) {
    u32 ev = EPOLLRDHUP;
    if (events & EvIn) {
        ev |= EPOLLIN;
    }
    if (events & EvOut) {
        ev |= EPOLLOUT;
    }
    if (flags & OneShot) {
        ev |= EPOLLONESHOT;
    }
    return ev;
}

u32 FromEpoll(u32 ev) {
    u32 out = 0;
    if (ev & EPOLLIN) {
        out |= EvIn;
    }
    if (ev & EPOLLOUT) {
        out |= EvOut;
    }
    if (ev & EPOLLERR) {
        out |= EvErr;
    }
    if (ev & (EPOLLHUP | EPOLLRDHUP)) {
        out |= EvHup; // TODO: guest may need RDHUP kept apart from HUP
    }
    return out;
}

Error EpollError() {
#ifdef _WIN32
    // wepoll gives Win32 codes, not WSA ones. EEXIST/ENOENT are checked by the guest layer.
    return Error::Internal;
#else
    return LastError();
#endif
}

HostEpoll::HostEpoll() {
#ifdef _WIN32
    handle_ = epoll_create1(0);
#else
    handle_ = epoll_create1(EPOLL_CLOEXEC);
#endif
    if (!Valid() || !wake_.Valid()) {
        return;
    }
    epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.u64 = ReservedTag;
    epoll_ctl(handle_, EPOLL_CTL_ADD, wake_.PollHandle(), &ev);
}

HostEpoll::~HostEpoll() {
    if (!Valid()) {
        return;
    }
#ifdef _WIN32
    epoll_close(handle_);
#else
    close(handle_);
#endif
}

bool HostEpoll::Valid() const {
#ifdef _WIN32
    return handle_ != nullptr;
#else
    return handle_ >= 0;
#endif
}

Error HostEpoll::Add(NativeSocket s, u64 tag, u32 events, u32 flags) {
    epoll_event ev{};
    ev.events = ToEpoll(events, flags);
    ev.data.u64 = tag;
    return epoll_ctl(handle_, EPOLL_CTL_ADD, s, &ev) == 0 ? Error::Ok : EpollError();
}

Error HostEpoll::Modify(NativeSocket s, u64 tag, u32 events, u32 flags) {
    epoll_event ev{};
    ev.events = ToEpoll(events, flags);
    ev.data.u64 = tag;
    return epoll_ctl(handle_, EPOLL_CTL_MOD, s, &ev) == 0 ? Error::Ok : EpollError();
}

Error HostEpoll::Remove(NativeSocket s) {
    epoll_event ev{}; // must be non-null before Linux 2.6.9
    return epoll_ctl(handle_, EPOLL_CTL_DEL, s, &ev) == 0 ? Error::Ok : EpollError();
}

EpollWaitResult HostEpoll::Wait(std::span<EpollEvent> out, Deadline deadline) {
    std::array<epoll_event, 64> buf;
    const int cap = static_cast<int>(std::min(out.size(), buf.size()));
    if (cap == 0) {
        return {0, false, Error::Inval};
    }
    for (;;) {
        const int n = epoll_wait(handle_, buf.data(), cap, TimeoutMs(deadline));
        if (n < 0) {
#ifndef _WIN32
            if (errno == EINTR) {
                continue;
            }
#endif
            return {0, false, EpollError()};
        }
        EpollWaitResult r{0, false, Error::Ok};
        for (int i = 0; i < n; ++i) {
            if (buf[i].data.u64 == ReservedTag) {
                r.woken = true;
                continue;
            }
            out[r.count++] = {buf[i].data.u64, FromEpoll(buf[i].events)};
        }
        if (n > 0 || Expired(deadline)) {
            return r;
        }
    }
}

#else // HOST_NET_KQUEUE (macOS, FreeBSD)

void* TagToUdata(u64 tag) {
    static_assert(sizeof(void*) >= sizeof(u64), "tags need 64-bit udata");
    return reinterpret_cast<void*>(static_cast<uintptr_t>(tag));
}

u64 UdataToTag(void* udata) {
    return static_cast<u64>(reinterpret_cast<uintptr_t>(udata));
}

Error KqueueApply(int kq, int s, u64 tag, u32 events, u32 flags) {
    u16 arm = EV_ADD | EV_ENABLE | EV_RECEIPT;
    if (flags & OneShot) {
        arm |= EV_DISPATCH; // like EPOLLONESHOT
    }
    const u16 disarm = EV_DELETE | EV_RECEIPT;
    // TODO: with no EvIn/EvOut, epoll still reports ERR/HUP but kqueue reports nothing.
    // A hidden EVFILT_READ would fix it but give spurious EvIn.
    struct kevent changes[2];
    struct kevent results[2];
    EV_SET(&changes[0], s, EVFILT_READ, (events & EvIn) ? arm : disarm, 0, 0, TagToUdata(tag));
    EV_SET(&changes[1], s, EVFILT_WRITE, (events & EvOut) ? arm : disarm, 0, 0, TagToUdata(tag));
    const int n = kevent(kq, changes, 2, results, 2, nullptr);
    if (n < 0) {
        return LastError();
    }
    for (int i = 0; i < n; ++i) {
        // EV_RECEIPT returns an EV_ERROR entry per change, data 0 on success. ENOENT is
        // deleting a filter that was never added.
        if ((results[i].flags & EV_ERROR) && results[i].data != 0 && results[i].data != ENOENT) {
            return TranslateNative(static_cast<int>(results[i].data));
        }
    }
    return Error::Ok;
}

HostEpoll::HostEpoll() {
    handle_ = kqueue();
    if (handle_ < 0 || !wake_.Valid()) {
        return;
    }
    SetCloseOnExec(handle_);
    struct kevent kev;
    EV_SET(&kev, wake_.PollHandle(), EVFILT_READ, EV_ADD, 0, 0, TagToUdata(ReservedTag));
    kevent(handle_, &kev, 1, nullptr, 0, nullptr);
}

HostEpoll::~HostEpoll() {
    if (handle_ >= 0) {
        close(handle_);
    }
}

bool HostEpoll::Valid() const {
    return handle_ >= 0;
}

// Guest layer checks EEXIST/ENOENT, so Add and Modify are the same here.
Error HostEpoll::Add(NativeSocket s, u64 tag, u32 events, u32 flags) {
    return KqueueApply(handle_, s, tag, events, flags);
}

Error HostEpoll::Modify(NativeSocket s, u64 tag, u32 events, u32 flags) {
    return KqueueApply(handle_, s, tag, events, flags);
}

Error HostEpoll::Remove(NativeSocket s) {
    return KqueueApply(handle_, s, 0, 0, 0);
}

EpollWaitResult HostEpoll::Wait(std::span<EpollEvent> out, Deadline deadline) {
    std::array<struct kevent, 64> buf;
    const int cap = static_cast<int>(std::min(out.size(), buf.size()));
    if (cap == 0) {
        return {0, false, Error::Inval};
    }
    for (;;) {
        timespec ts{};
        timespec* pts = nullptr;
        if (deadline) {
            const auto left = std::max(*deadline - Clock::now(), Clock::duration::zero());
            const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(left).count();
            ts.tv_sec = static_cast<time_t>(ns / 1'000'000'000);
            ts.tv_nsec = static_cast<long>(ns % 1'000'000'000);
            pts = &ts;
        }
        const int n = kevent(handle_, nullptr, 0, buf.data(), cap, pts);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return {0, false, LastError()};
        }
        EpollWaitResult r{0, false, Error::Ok};
        for (int i = 0; i < n; ++i) {
            const u64 tag = UdataToTag(buf[i].udata);
            if (tag == ReservedTag) {
                r.woken = true;
                continue;
            }
            u32 ev = buf[i].filter == EVFILT_READ ? EvIn : EvOut;
            if (buf[i].flags & EV_EOF) {
                ev |= EvHup;
                if (buf[i].fflags != 0) {
                    ev |= EvErr; // fflags holds the socket error
                }
            }
            // kqueue reports read and write separately, merge them.
            bool merged = false;
            for (s32 j = 0; j < r.count; ++j) {
                if (out[j].tag == tag) {
                    out[j].events |= ev;
                    merged = true;
                    break;
                }
            }
            if (!merged) {
                out[r.count++] = {tag, ev};
            }
        }
        if (n > 0 || Expired(deadline)) {
            return r;
        }
    }
}

#endif

void HostEpoll::Wake() {
    wake_.Signal();
}

void HostEpoll::DrainWake() {
    wake_.Drain();
}

} // namespace Core::Net::Host
