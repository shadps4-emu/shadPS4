// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cstring>
#include <unordered_map>
#include <vector>
#include <common/assert.h>
#include <fmt/format.h>
#include "common/error.h"
#include "core/libraries/kernel/file_system.h"
#include "core/libraries/kernel/kernel.h"
#include "net.h"
#ifndef _WIN32
#include <sys/stat.h>
#endif
#include "dns_hook.h"
#include "net_error.h"
#include "sockets.h"

namespace Libraries::Net {

// Format a POSIX IPv4 sockaddr as "a.b.c.d:port" for logging.
static std::string FormatEndpoint(const sockaddr* sa) {
    const auto* in = reinterpret_cast<const sockaddr_in*>(sa);
    const u8* o = reinterpret_cast<const u8*>(&in->sin_addr);
    return fmt::format("{}.{}.{}.{}:{}", o[0], o[1], o[2], o[3], ntohs(in->sin_port));
}

// Remember the last destination logged per socket so connectionless sendto()
// traffic is logged once per unique destination instead of once per packet.
static std::mutex g_dest_log_mutex;
static std::unordered_map<u64, u64> g_last_dest; // sock -> (ip << 16) | port

static bool DestChanged(u64 sock, const sockaddr* sa) {
    const auto* in = reinterpret_cast<const sockaddr_in*>(sa);
    const u64 key = (static_cast<u64>(in->sin_addr.s_addr) << 16) | ntohs(in->sin_port);
    std::lock_guard lock(g_dest_log_mutex);
    auto [it, inserted] = g_last_dest.try_emplace(sock, key);
    if (inserted || it->second != key) {
        it->second = key;
        return true;
    }
    return false;
}

#ifdef _WIN32
#define ERROR_CASE(errname)                                                                        \
    case (WSA##errname):                                                                           \
        *Libraries::Kernel::__Error() = ORBIS_NET_##errname;                                       \
        return -1;
#else
#define ERROR_CASE(errname)                                                                        \
    case (errname):                                                                                \
        *Libraries::Kernel::__Error() = ORBIS_NET_##errname;                                       \
        return -1;
#endif

static int ConvertReturnErrorCode(int retval) {
    if (retval < 0) {
#ifdef _WIN32
        switch (WSAGetLastError()) {
#else
        switch (errno) {
#endif
#ifndef _WIN32 // These errorcodes don't exist in WinSock
            ERROR_CASE(EPERM)
            ERROR_CASE(ENOENT)
            // ERROR_CASE(ESRCH)
            // ERROR_CASE(EIO)
            // ERROR_CASE(ENXIO)
            // ERROR_CASE(E2BIG)
            // ERROR_CASE(ENOEXEC)
            // ERROR_CASE(EDEADLK)
            ERROR_CASE(ENOMEM)
            // ERROR_CASE(ECHILD)
            // ERROR_CASE(EBUSY)
            ERROR_CASE(EEXIST)
            // ERROR_CASE(EXDEV)
            ERROR_CASE(ENODEV)
            // ERROR_CASE(ENOTDIR)
            // ERROR_CASE(EISDIR)
            ERROR_CASE(ENFILE)
            // ERROR_CASE(ENOTTY)
            // ERROR_CASE(ETXTBSY)
            // ERROR_CASE(EFBIG)
            ERROR_CASE(ENOSPC)
            // ERROR_CASE(ESPIPE)
            // ERROR_CASE(EROFS)
            // ERROR_CASE(EMLINK)
            ERROR_CASE(EPIPE)
            // ERROR_CASE(EDOM)
            // ERROR_CASE(ERANGE)
            // ERROR_CASE(ENOLCK)
            // ERROR_CASE(ENOSYS)
            // ERROR_CASE(EIDRM)
            // ERROR_CASE(EOVERFLOW)
            // ERROR_CASE(EILSEQ)
            // ERROR_CASE(ENOTSUP)
            ERROR_CASE(ECANCELED)
            // ERROR_CASE(EBADMSG)
            ERROR_CASE(ENODATA)
            // ERROR_CASE(ENOSR)
            // ERROR_CASE(ENOSTR)
            // ERROR_CASE(ETIME)
#endif
            ERROR_CASE(EINTR)
            ERROR_CASE(EBADF)
            ERROR_CASE(EACCES)
            ERROR_CASE(EFAULT)
            ERROR_CASE(EINVAL)
            ERROR_CASE(EMFILE)
            ERROR_CASE(EWOULDBLOCK)
            ERROR_CASE(EINPROGRESS)
            ERROR_CASE(EALREADY)
            ERROR_CASE(ENOTSOCK)
            ERROR_CASE(EDESTADDRREQ)
            ERROR_CASE(EMSGSIZE)
            ERROR_CASE(EPROTOTYPE)
            ERROR_CASE(ENOPROTOOPT)
            ERROR_CASE(EPROTONOSUPPORT)
#if defined(__APPLE__) || defined(_WIN32)
            ERROR_CASE(EOPNOTSUPP)
#endif
            ERROR_CASE(EAFNOSUPPORT)
            ERROR_CASE(EADDRINUSE)
            ERROR_CASE(EADDRNOTAVAIL)
            ERROR_CASE(ENETDOWN)
            ERROR_CASE(ENETUNREACH)
            ERROR_CASE(ENETRESET)
            ERROR_CASE(ECONNABORTED)
            ERROR_CASE(ECONNRESET)
            ERROR_CASE(ENOBUFS)
            ERROR_CASE(EISCONN)
            ERROR_CASE(ENOTCONN)
            ERROR_CASE(ETIMEDOUT)
            ERROR_CASE(ECONNREFUSED)
            ERROR_CASE(ELOOP)
            ERROR_CASE(ENAMETOOLONG)
            ERROR_CASE(EHOSTUNREACH)
            ERROR_CASE(ENOTEMPTY)
        }
        *Libraries::Kernel::__Error() = ORBIS_NET_EINTERNAL;
        return -1;
    }
    // if it is 0 or positive return it as it is
    return retval;
}

static int ConvertLevels(int level) {
    switch (level) {
    case ORBIS_NET_SOL_SOCKET:
        return SOL_SOCKET;
    case ORBIS_NET_IPPROTO_IP:
        return IPPROTO_IP;
    case ORBIS_NET_IPPROTO_TCP:
        return IPPROTO_TCP;
    case ORBIS_NET_IPPROTO_IPV6:
        return IPPROTO_IPV6;
    }
    return -1;
}

static void convertOrbisNetSockaddrToPosix(const OrbisNetSockaddr* src, sockaddr* dst) {
    if (src == nullptr || dst == nullptr)
        return;
    memset(dst, 0, sizeof(sockaddr));
    const OrbisNetSockaddrIn* src_in = (const OrbisNetSockaddrIn*)src;
    sockaddr_in* dst_in = (sockaddr_in*)dst;
    dst_in->sin_family = src_in->sin_family;
    dst_in->sin_port = src_in->sin_port;
    memcpy(&dst_in->sin_addr, &src_in->sin_addr, 4);
}

static void convertPosixSockaddrToOrbis(sockaddr* src, OrbisNetSockaddr* dst) {
    if (src == nullptr || dst == nullptr)
        return;
    memset(dst, 0, sizeof(OrbisNetSockaddr));
    OrbisNetSockaddrIn* dst_in = (OrbisNetSockaddrIn*)dst;
    sockaddr_in* src_in = (sockaddr_in*)src;
    dst_in->sin_family = static_cast<unsigned char>(src_in->sin_family);
    dst_in->sin_port = src_in->sin_port;
    memcpy(&dst_in->sin_addr, &src_in->sin_addr, 4);
}

bool PosixSocket::IsValid() const {
#ifdef _WIN32
    return sock != INVALID_SOCKET;
#else
    return sock != -1;
#endif
}

int PosixSocket::Close() {
    std::scoped_lock lock{m_mutex};
    DnsHook::Instance().RemoveSpy(static_cast<u64>(sock));
    {
        std::lock_guard dlock(g_dest_log_mutex);
        g_last_dest.erase(static_cast<u64>(sock));
    }
#ifdef _WIN32
    auto out = closesocket(sock);
#else
    auto out = ::close(sock);
#endif
    return ConvertReturnErrorCode(out);
}

int PosixSocket::Bind(const OrbisNetSockaddr* addr, u32 addrlen) {
    std::scoped_lock lock{m_mutex};
    sockaddr addr2;
    convertOrbisNetSockaddrToPosix(addr, &addr2);
    return ConvertReturnErrorCode(::bind(sock, &addr2, sizeof(sockaddr_in)));
}

int PosixSocket::Listen(int backlog) {
    std::scoped_lock lock{m_mutex};
    return ConvertReturnErrorCode(::listen(sock, backlog));
}

static int convertOrbisFlagsToPosix(int sock_type, int sce_flags) {
    int posix_flags = 0;

    if (sce_flags & ORBIS_NET_MSG_PEEK)
        posix_flags |= MSG_PEEK;
#ifndef _WIN32
    if (sce_flags & ORBIS_NET_MSG_DONTWAIT)
        posix_flags |= MSG_DONTWAIT;
#endif
    // MSG_WAITALL is only valid for stream sockets
    if ((sce_flags & ORBIS_NET_MSG_WAITALL) &&
        ((sock_type == ORBIS_NET_SOCK_STREAM) || (sock_type == ORBIS_NET_SOCK_STREAM_P2P)))
        posix_flags |= MSG_WAITALL;

    return posix_flags;
}

// On Windows, MSG_DONTWAIT is not handled natively by recv/send.
// This function uses select() with zero timeout to simulate non-blocking behavior.
static int socket_is_ready(int sock, bool is_read = true) {
    fd_set fds{};
    FD_ZERO(&fds);
    FD_SET(sock, &fds);
    timeval timeout{0, 0};
    int res =
        select(sock + 1, is_read ? &fds : nullptr, is_read ? nullptr : &fds, nullptr, &timeout);
    if (res == 0) {
        *Libraries::Kernel::__Error() = ORBIS_NET_EWOULDBLOCK;
        return -1;
    } else if (res < 0) {
        return ConvertReturnErrorCode(res);
    }
    return res;
}

int PosixSocket::SendMessage(const OrbisNetMsghdr* msg, int flags) {
    std::scoped_lock lock{m_mutex};

#ifdef _WIN32
    int totalSent = 0;
    bool waitAll = (flags & ORBIS_NET_MSG_WAITALL) != 0;
    bool dontWait = (flags & ORBIS_NET_MSG_DONTWAIT) != 0;

    // stream socket with multiple buffers
    bool use_wsamsg =
        (socket_type == ORBIS_NET_SOCK_STREAM || socket_type == ORBIS_NET_SOCK_STREAM_P2P) &&
        msg->msg_iovlen > 1;

    for (int i = 0; i < msg->msg_iovlen; ++i) {
        char* buf = (char*)msg->msg_iov[i].iov_base;
        size_t remaining = msg->msg_iov[i].iov_len;

        while (remaining > 0) {
            if (dontWait) {
                int ready = socket_is_ready(sock, false);
                if (ready <= 0)
                    return ready;
            }

            int sent = 0;
            if (use_wsamsg) {
                // only call WSASendMsg if we have multiple buffers
                LPFN_WSASENDMSG wsasendmsg = nullptr;
                GUID guid = WSAID_WSASENDMSG;
                DWORD bytes = 0;
                if (WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid),
                             &wsasendmsg, sizeof(wsasendmsg), &bytes, nullptr, nullptr) != 0) {
                    // fallback to send()
                    sent = ::send(sock, buf, remaining, 0);
                } else {
                    DWORD bytesSent = 0;
                    int res = wsasendmsg(
                        sock, reinterpret_cast<LPWSAMSG>(const_cast<OrbisNetMsghdr*>(msg)), 0,
                        &bytesSent, nullptr, nullptr);
                    if (res == SOCKET_ERROR)
                        return ConvertReturnErrorCode(WSAGetLastError());
                    sent = bytesSent;
                }
            } else {
                sent = ::send(sock, buf, remaining, 0);
                if (sent == SOCKET_ERROR)
                    return ConvertReturnErrorCode(WSAGetLastError());
            }

            totalSent += sent;
            remaining -= sent;
            buf += sent;

            if (!waitAll)
                break;
        }
    }

    return totalSent;

#else
    int native_flags = convertOrbisFlagsToPosix(socket_type, flags);
    int res = sendmsg(sock, reinterpret_cast<const msghdr*>(msg), native_flags);
    return ConvertReturnErrorCode(res);
#endif
}

int PosixSocket::SendPacket(const void* msg, u32 len, int flags, const OrbisNetSockaddr* to,
                            u32 tolen) {
    std::scoped_lock lock{m_mutex};
    int res = 0;
#ifdef _WIN32
    if (flags & ORBIS_NET_MSG_DONTWAIT) {
        res = socket_is_ready(sock, false);
        if (res <= 0)
            return res;
    }
#endif
    const auto posix_flags = convertOrbisFlagsToPosix(socket_type, flags);

    // DNS override: mark datagram sockets talking to port 53, then try to answer
    // matching queries locally instead of sending them out.
    auto& dns = DnsHook::Instance();
    if (to != nullptr && socket_type == ORBIS_NET_SOCK_DGRAM) {
        sockaddr dst{};
        convertOrbisNetSockaddrToPosix(to, &dst);
        if (ntohs(((sockaddr_in*)&dst)->sin_port) == 53) {
            dns.AddSpy(static_cast<u64>(sock));
        }
    }
    if (dns.IsSpy(static_cast<u64>(sock))) {
        const s32 intercepted = dns.AnalyzeQuery(static_cast<u64>(sock), (const u8*)msg, len);
        if (intercepted >= 0) {
            return intercepted; // swallow the real query; answer is queued
        }
    }

    if (to == nullptr) {
        res = send(sock, (const char*)msg, len, posix_flags);
    } else {
        sockaddr addr{};
        convertOrbisNetSockaddrToPosix(to, &addr);
        if (DestChanged(static_cast<u64>(sock), &addr)) {
            LOG_INFO(Lib_Net, "sendto -> {}", FormatEndpoint(&addr));
        }
        res = sendto(sock, (const char*)msg, len, posix_flags, &addr, tolen);
    }
    return ConvertReturnErrorCode(res);
}

int PosixSocket::ReceiveMessage(OrbisNetMsghdr* msg, int flags) {
    std::scoped_lock lock{receive_mutex};

#ifdef _WIN32
    int totalReceived = 0;
    bool waitAll = (flags & ORBIS_NET_MSG_WAITALL) != 0;
    bool dontWait = (flags & ORBIS_NET_MSG_DONTWAIT) != 0;

    // stream socket with multiple buffers
    bool use_wsarecvmsg =
        (socket_type == ORBIS_NET_SOCK_STREAM || socket_type == ORBIS_NET_SOCK_STREAM_P2P) &&
        msg->msg_iovlen > 1;

    for (int i = 0; i < msg->msg_iovlen; ++i) {
        char* buf = (char*)msg->msg_iov[i].iov_base;
        size_t remaining = msg->msg_iov[i].iov_len;

        while (remaining > 0) {
            // emulate DONTWAIT
            if (dontWait) {
                int ready = socket_is_ready(sock, true);
                if (ready <= 0)
                    return ready; // returns ORBIS_NET_ERROR_EWOULDBLOCK or error
            }

            int received = 0;
            if (use_wsarecvmsg) {
                // only call WSARecvMsg if multiple buffers + stream
                LPFN_WSARECVMSG wsarecvmsg = nullptr;
                GUID guid = WSAID_WSARECVMSG;
                DWORD bytes = 0;
                if (WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid),
                             &wsarecvmsg, sizeof(wsarecvmsg), &bytes, nullptr, nullptr) != 0) {
                    // fallback to recv()
                    received = ::recv(sock, buf, remaining, 0);
                    if (received == SOCKET_ERROR)
                        return ConvertReturnErrorCode(WSAGetLastError());
                } else {
                    DWORD bytesReceived = 0;
                    int res = wsarecvmsg(sock, reinterpret_cast<LPWSAMSG>(msg), &bytesReceived,
                                         nullptr, nullptr);
                    if (res == SOCKET_ERROR)
                        return ConvertReturnErrorCode(WSAGetLastError());
                    received = bytesReceived;
                }
            } else {
                // fallback to recv() for UDP or single-buffer
                received = ::recv(sock, buf, remaining, 0);
                if (received == SOCKET_ERROR)
                    return ConvertReturnErrorCode(WSAGetLastError());
            }

            totalReceived += received;
            remaining -= received;
            buf += received;

            // stop after first receive if WAITALL is not set
            if (!waitAll)
                break;
        }
    }

    return totalReceived;

#else
    int native_flags = convertOrbisFlagsToPosix(socket_type, flags);
    int res = recvmsg(sock, reinterpret_cast<msghdr*>(msg), native_flags);
    return ConvertReturnErrorCode(res);
#endif
}

int PosixSocket::ReceivePacket(void* buf, u32 len, int flags, OrbisNetSockaddr* from,
                               u32* fromlen) {
    std::scoped_lock lock{receive_mutex};
    int res = 0;
#ifdef _WIN32
    if (flags & ORBIS_NET_MSG_DONTWAIT) {
        res = socket_is_ready(sock);
        if (res <= 0)
            return res;
    }
#endif
    const auto posix_flags = convertOrbisFlagsToPosix(socket_type, flags);

    // DNS override: hand back a forged answer if one is queued for this socket.
    auto& dns = DnsHook::Instance();
    if (dns.IsSpy(static_cast<u64>(sock)) && dns.HasQueued(static_cast<u64>(sock))) {
        const auto packet = dns.PopPacket(static_cast<u64>(sock));
        const u32 copy_len = std::min<u32>(len, static_cast<u32>(packet.size()));
        memcpy(buf, packet.data(), copy_len);
        if (from != nullptr) {
            sockaddr_in src{};
            src.sin_family = AF_INET;
            src.sin_port = htons(53);
            src.sin_addr.s_addr = dns.GetDnsServerAddr();
            convertPosixSockaddrToOrbis((sockaddr*)&src, from);
            if (fromlen)
                *fromlen = sizeof(OrbisNetSockaddrIn);
        }
        return static_cast<int>(copy_len);
    }

    if (from == nullptr) {
        res = recv(sock, (char*)buf, len, posix_flags);
    } else {
        sockaddr addr{};
        socklen_t addrlen = sizeof(addr);
        res = recvfrom(sock, (char*)buf, len, posix_flags, &addr,
                       (fromlen && *fromlen <= sizeof(addr) ? (socklen_t*)fromlen : &addrlen));
        if (res > 0)
            convertPosixSockaddrToOrbis(&addr, from);
    }

    return ConvertReturnErrorCode(res);
}

SocketPtr PosixSocket::Accept(OrbisNetSockaddr* addr, u32* addrlen) {
    std::scoped_lock lock{m_mutex};
    sockaddr addr2;
    socklen_t len = sizeof(addr2);
    net_socket new_socket = ::accept(sock, &addr2, &len);
#ifdef _WIN32
    if (new_socket != INVALID_SOCKET) {
#else
    if (new_socket >= 0) {
#endif
        if (addr && addrlen) {
            convertPosixSockaddrToOrbis(&addr2, addr);
            *addrlen = sizeof(OrbisNetSockaddrIn);
        }
        return std::make_shared<PosixSocket>(new_socket);
    }
    ConvertReturnErrorCode(new_socket);
    return nullptr;
}

int PosixSocket::Connect(const OrbisNetSockaddr* addr, u32 namelen) {
    std::scoped_lock lock{m_mutex};
    sockaddr addr2;
    convertOrbisNetSockaddrToPosix(addr, &addr2);

    // DNS override: a connected UDP socket to port 53 will later send() with no
    // destination, so mark it here to catch those queries too.
    if (socket_type == ORBIS_NET_SOCK_DGRAM && ntohs(((sockaddr_in*)&addr2)->sin_port) == 53) {
        DnsHook::Instance().AddSpy(static_cast<u64>(sock));
    }

    LOG_INFO(Lib_Net, "connect -> {}", FormatEndpoint(&addr2));

    int result = ::connect(sock, &addr2, sizeof(sockaddr_in));
#ifdef _WIN32
    // Winsock returns EWOULDBLOCK where real hardware returns EINPROGRESS
    // Step in here on errors to address this.
    if (result == -1) {
        if (WSAGetLastError() == WSAEWOULDBLOCK) {
            WSASetLastError(WSAEINPROGRESS);
        }
    }
#endif
    LOG_DEBUG(Lib_Net, "raw connect result = {}, errno = {}", result,
              result == -1 ? Common::GetLastErrorMsg() : "none");
    return ConvertReturnErrorCode(result);
}

int PosixSocket::GetSocketAddress(OrbisNetSockaddr* name, u32* namelen) {
    std::scoped_lock lock{m_mutex};
    sockaddr addr;
    convertOrbisNetSockaddrToPosix(name, &addr);
    if (name != nullptr) {
        *namelen = sizeof(sockaddr_in);
    }
    int res = getsockname(sock, &addr, (socklen_t*)namelen);
    if (res >= 0) {
        convertPosixSockaddrToOrbis(&addr, name);
        *namelen = sizeof(OrbisNetSockaddrIn);
    }
    return ConvertReturnErrorCode(res);
}

#define CASE_SETSOCKOPT(opt)                                                                       \
    case ORBIS_NET_##opt:                                                                          \
        return ConvertReturnErrorCode(                                                             \
            setsockopt(sock, native_level, opt, (const char*)optval, optlen))

#define CASE_SETSOCKOPT_VALUE(opt, value)                                                          \
    case opt:                                                                                      \
        if (optlen != sizeof(*value)) {                                                            \
            *Libraries::Kernel::__Error() = ORBIS_NET_EFAULT;                                      \
            return -1;                                                                             \
        }                                                                                          \
        memcpy(value, optval, optlen);                                                             \
        return 0

int PosixSocket::SetSocketOptions(int level, int optname, const void* optval, u32 optlen) {
    std::scoped_lock lock{m_mutex};
    s32 native_level = ConvertLevels(level);
    ::linger native_linger;
    if (native_level == SOL_SOCKET) {
        switch (optname) {
            CASE_SETSOCKOPT(SO_REUSEADDR);
            CASE_SETSOCKOPT(SO_KEEPALIVE);
            CASE_SETSOCKOPT(SO_BROADCAST);
            CASE_SETSOCKOPT(SO_SNDBUF);
            CASE_SETSOCKOPT(SO_RCVBUF);
            CASE_SETSOCKOPT_VALUE(ORBIS_NET_SO_CONNECTTIMEO, &sockopt_so_connecttimeo);
            CASE_SETSOCKOPT_VALUE(ORBIS_NET_SO_REUSEPORT, &sockopt_so_reuseport);
            CASE_SETSOCKOPT_VALUE(ORBIS_NET_SO_USECRYPTO, &sockopt_so_usecrypto);
            CASE_SETSOCKOPT_VALUE(ORBIS_NET_SO_USESIGNATURE, &sockopt_so_usesignature);
        case ORBIS_NET_SO_SNDTIMEO:
        case ORBIS_NET_SO_RCVTIMEO: {
            if (optlen != sizeof(int)) {
                *Libraries::Kernel::__Error() = ORBIS_NET_ERROR_EFAULT;
                return -1;
            }
            std::vector<char> val;
            const auto optname_nat = (optname == ORBIS_NET_SO_SNDTIMEO) ? SO_SNDTIMEO : SO_RCVTIMEO;
            int timeout_us = *(const int*)optval;
#ifdef _WIN32
            DWORD timeout = timeout_us / 1000;
#else
            timeval timeout{.tv_sec = timeout_us / 1000000, .tv_usec = timeout_us % 1000000};
#endif
            val.insert(val.end(), (char*)&timeout, (char*)&timeout + sizeof(timeout));
            optlen = sizeof(timeout);
            return ConvertReturnErrorCode(
                setsockopt(sock, native_level, optname_nat, val.data(), optlen));
        }
        case ORBIS_NET_SO_ONESBCAST: {

            if (optlen != sizeof(sockopt_so_onesbcast)) {
                *Libraries::Kernel::__Error() = ORBIS_NET_ERROR_EFAULT;
                return -1;
            }
            memcpy(&sockopt_so_onesbcast, optval, optlen);
            return ConvertReturnErrorCode(
                setsockopt(sock, native_level, SO_BROADCAST, (const char*)optval, optlen));
        }
        case ORBIS_NET_SO_TYPE:
        case ORBIS_NET_SO_ERROR: {
            *Libraries::Kernel::__Error() = ORBIS_NET_ENOPROTOOPT;
            return -1;
        }
        case ORBIS_NET_SO_LINGER: {
            if (socket_type != ORBIS_NET_SOCK_STREAM) {
                *Libraries::Kernel::__Error() = ORBIS_NET_EPROCUNAVAIL;
                return -1;
            }
            if (optlen < sizeof(OrbisNetLinger)) {
                LOG_ERROR(Lib_Net, "size missmatched! optlen = {} OrbisNetLinger={}", optlen,
                          sizeof(OrbisNetLinger));
                *Libraries::Kernel::__Error() = ORBIS_NET_EINVAL;
                return -1;
            }

            const void* native_val = &native_linger;
            u32 native_len = sizeof(native_linger);
            native_linger.l_onoff = reinterpret_cast<const OrbisNetLinger*>(optval)->l_onoff;
            native_linger.l_linger = reinterpret_cast<const OrbisNetLinger*>(optval)->l_linger;
            return ConvertReturnErrorCode(
                setsockopt(sock, native_level, SO_LINGER, (const char*)native_val, native_len));
        }

        case ORBIS_NET_SO_NAME:
            *Libraries::Kernel::__Error() = ORBIS_NET_EINVAL;
            return -1; // don't support set for name
        case ORBIS_NET_SO_NBIO: {
            if (optlen < sizeof(sockopt_so_nbio)) {
                *Libraries::Kernel::__Error() = ORBIS_NET_EINVAL;
                return -1;
            } else {
                memcpy(&sockopt_so_nbio, optval, sizeof(sockopt_so_nbio));
            }
#ifdef _WIN32
            static_assert(sizeof(u_long) == sizeof(sockopt_so_nbio),
                          "type used for ioctlsocket value does not have the expected size");
            return ConvertReturnErrorCode(ioctlsocket(sock, FIONBIO, (u_long*)&sockopt_so_nbio));
#else
            return ConvertReturnErrorCode(ioctl(sock, FIONBIO, &sockopt_so_nbio));
#endif
        }
        case ORBIS_NET_SO_ACCEPTTIMEO:
            LOG_ERROR(Lib_Net, "Unhandled option ORBIS_NET_SO_ACCEPTTIMEO");
            return ORBIS_OK;
        }
    } else if (native_level == IPPROTO_IP) {
        switch (optname) {
            // CASE_SETSOCKOPT(IP_HDRINCL);
            CASE_SETSOCKOPT(IP_TOS);
            CASE_SETSOCKOPT(IP_TTL);
            CASE_SETSOCKOPT(IP_MULTICAST_IF);
            CASE_SETSOCKOPT(IP_MULTICAST_TTL);
            CASE_SETSOCKOPT(IP_MULTICAST_LOOP);
            CASE_SETSOCKOPT(IP_ADD_MEMBERSHIP);
            CASE_SETSOCKOPT(IP_DROP_MEMBERSHIP);
            CASE_SETSOCKOPT_VALUE(ORBIS_NET_IP_TTLCHK, &sockopt_ip_ttlchk);
            CASE_SETSOCKOPT_VALUE(ORBIS_NET_IP_MAXTTL, &sockopt_ip_maxttl);
        case ORBIS_NET_IP_HDRINCL: {
            if (socket_type != ORBIS_NET_SOCK_RAW) {
                *Libraries::Kernel::__Error() = ORBIS_NET_EPROCUNAVAIL;
                return -1;
            }
            return ConvertReturnErrorCode(
                setsockopt(sock, native_level, optname, (const char*)optval, optlen));
        }
        }
    } else if (native_level == IPPROTO_TCP) {
        switch (optname) {
            CASE_SETSOCKOPT(TCP_NODELAY);
            CASE_SETSOCKOPT(TCP_MAXSEG);
            CASE_SETSOCKOPT_VALUE(ORBIS_NET_TCP_MSS_TO_ADVERTISE, &sockopt_tcp_mss_to_advertise);
        }
    }

    UNREACHABLE_MSG("Unknown level ={} optname ={}", level, optname);
    return 0;
}

#define CASE_GETSOCKOPT(opt)                                                                       \
    case ORBIS_NET_##opt: {                                                                        \
        socklen_t optlen_temp = *optlen;                                                           \
        auto retval = ConvertReturnErrorCode(                                                      \
            getsockopt(sock, native_level, opt, (char*)optval, &optlen_temp));                     \
        *optlen = optlen_temp;                                                                     \
        return retval;                                                                             \
    }
#define CASE_GETSOCKOPT_VALUE(opt, value)                                                          \
    case opt:                                                                                      \
        if (*optlen < sizeof(value)) {                                                             \
            *optlen = sizeof(value);                                                               \
            *Libraries::Kernel::__Error() = ORBIS_NET_EFAULT;                                      \
            return -1;                                                                             \
        }                                                                                          \
        *optlen = sizeof(value);                                                                   \
        *(decltype(value)*)optval = value;                                                         \
        return 0;

int PosixSocket::GetSocketOptions(int level, int optname, void* optval, u32* optlen) {
    std::scoped_lock lock{m_mutex};
    s32 native_level = ConvertLevels(level);
    if (native_level == SOL_SOCKET) {
        switch (optname) {
            CASE_GETSOCKOPT(SO_REUSEADDR);
            CASE_GETSOCKOPT(SO_KEEPALIVE);
            CASE_GETSOCKOPT(SO_BROADCAST);
            CASE_GETSOCKOPT(SO_LINGER);
            CASE_GETSOCKOPT(SO_SNDBUF);
            CASE_GETSOCKOPT(SO_RCVBUF);
            CASE_GETSOCKOPT(SO_SNDTIMEO);
            CASE_GETSOCKOPT(SO_RCVTIMEO);
            CASE_GETSOCKOPT(SO_ERROR);
            CASE_GETSOCKOPT(SO_TYPE);
            CASE_GETSOCKOPT_VALUE(ORBIS_NET_SO_NBIO, sockopt_so_nbio);
            CASE_GETSOCKOPT_VALUE(ORBIS_NET_SO_CONNECTTIMEO, sockopt_so_connecttimeo);
            CASE_GETSOCKOPT_VALUE(ORBIS_NET_SO_REUSEPORT, sockopt_so_reuseport);
            CASE_GETSOCKOPT_VALUE(ORBIS_NET_SO_ONESBCAST, sockopt_so_onesbcast);
            CASE_GETSOCKOPT_VALUE(ORBIS_NET_SO_USECRYPTO, sockopt_so_usecrypto);
            CASE_GETSOCKOPT_VALUE(ORBIS_NET_SO_USESIGNATURE, sockopt_so_usesignature);
            CASE_GETSOCKOPT_VALUE(ORBIS_NET_SO_NAME,
                                  (char)0); // writes an empty string to the output buffer
        case ORBIS_NET_SO_ERROR_EX: {
            socklen_t optlen_temp = *optlen;
            auto retval = ConvertReturnErrorCode(
                getsockopt(sock, level, SO_ERROR, (char*)optval, &optlen_temp));
            *optlen = optlen_temp;
            if (retval < 0) {
                s32 r = *Libraries::Kernel::__Error();
                *Libraries::Kernel::__Error() = 0;
                return r;
            }
            return retval;
        }
        }
    } else if (native_level == IPPROTO_IP) {
        switch (optname) {
            CASE_GETSOCKOPT(IP_HDRINCL);
            CASE_GETSOCKOPT(IP_TOS);
            CASE_GETSOCKOPT(IP_TTL);
            CASE_GETSOCKOPT(IP_MULTICAST_IF);
            CASE_GETSOCKOPT(IP_MULTICAST_TTL);
            CASE_GETSOCKOPT(IP_MULTICAST_LOOP);
            CASE_GETSOCKOPT(IP_ADD_MEMBERSHIP);
            CASE_GETSOCKOPT(IP_DROP_MEMBERSHIP);
            CASE_GETSOCKOPT_VALUE(ORBIS_NET_IP_TTLCHK, sockopt_ip_ttlchk);
            CASE_GETSOCKOPT_VALUE(ORBIS_NET_IP_MAXTTL, sockopt_ip_maxttl);
        }
    } else if (native_level == IPPROTO_TCP) {
        switch (optname) {
            CASE_GETSOCKOPT(TCP_NODELAY);
            CASE_GETSOCKOPT(TCP_MAXSEG);
            CASE_GETSOCKOPT_VALUE(ORBIS_NET_TCP_MSS_TO_ADVERTISE, sockopt_tcp_mss_to_advertise);
        }
    }
    UNREACHABLE_MSG("Unknown level ={} optname ={}", level, optname);
    return 0;
}

int PosixSocket::GetPeerName(OrbisNetSockaddr* name, u32* namelen) {
    std::scoped_lock lock{m_mutex};

    sockaddr addr;
    convertOrbisNetSockaddrToPosix(name, &addr);
    if (name != nullptr) {
        *namelen = sizeof(sockaddr_in);
    }
    int res = ::getpeername(sock, &addr, (socklen_t*)namelen);
    if (res >= 0) {
        convertPosixSockaddrToOrbis(&addr, name);
        *namelen = sizeof(OrbisNetSockaddrIn);
    }
    return ConvertReturnErrorCode(res);
}

int PosixSocket::fstat(Libraries::Kernel::OrbisKernelStat* sb) {
#ifdef _WIN32
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    sb->st_mode = 0000777u | 0140000u;
    return 0;
#else
    struct stat st{};
    int result = ::fstat(sock, &st);
    sb->st_mode = 0000777u | 0140000u;
    sb->st_size = st.st_size;
    sb->st_blocks = st.st_blocks;
    sb->st_blksize = st.st_blksize;
    // sb->st_flags = st.st_flags;
    return ConvertReturnErrorCode(result);
#endif
}

} // namespace Libraries::Net