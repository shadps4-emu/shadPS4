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

bool SetNonBlocking(NativeSocket s) {
#ifdef _WIN32
    u_long on = 1;
    return ioctlsocket(s, FIONBIO, &on) == 0;
#else
    const int flags = fcntl(s, F_GETFL, 0);
    return flags >= 0 && fcntl(s, F_SETFL, flags | O_NONBLOCK) == 0;
#endif
}

// Errors
Error TranslateNative(int code) {
    // One entry per FreeBSD errno a socket call can produce. Host codes with no equivalent map
    // to the EINTERNAL
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
        {WSAESHUTDOWN, Error::Pipe}, // BSD reports EPIPE for a send after shutdown
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
        // Without this, an ICMP port-unreachable caused by an earlier sendto() makes the next
        // recvfrom() fail with WSAECONNRESET. BSD never does that for UDP.
        BOOL report = FALSE;
        DWORD bytes = 0;
        WSAIoctl(s, SIO_UDP_CONNRESET, &report, sizeof(report), nullptr, 0, &bytes, nullptr,
                 nullptr);
    }
#else
    (void)type;
    SetCloseOnExec(s);
#ifdef SO_NOSIGPIPE
    // macOS/FreeBSD: a dead peer must not raise SIGPIPE and kill the emulator.
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

void CloseSocket(NativeSocket s) {
    if (s == InvalidSocket) {
        return;
    }
#ifdef _WIN32
    closesocket(s);
#else
    close(s); // never retry on EINTR: the fd may already be released
#endif
}

Error Connect(NativeSocket s, const sockaddr* addr, socklen_t len) {
    if (connect(s, addr, len) == 0) {
        return Error::Ok;
    }
    const Error e = LastError();
#ifdef _WIN32
    // Winsock reports a started non-blocking connect as WSAEWOULDBLOCK,BSD says EINPROGRESS.
    // A repeat call while pending gives WSAEALREADY
    if (e == Error::WouldBlock) {
        return Error::InProgress;
    }
#endif
    return e;
}

} // namespace Core::Net::Host