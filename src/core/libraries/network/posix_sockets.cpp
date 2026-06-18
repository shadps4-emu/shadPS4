// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <vector>
#include <common/assert.h>
#include "common/error.h"
#include "core/libraries/kernel/file_system.h"
#include "core/libraries/kernel/kernel.h"
#include "core/libraries/kernel/time.h"
#include "net.h"
#ifndef _WIN32
#include <sys/stat.h>
#endif
#include "net_error.h"
#include "sockets.h"

namespace Libraries::Net {

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
    if (to == nullptr) {
        res = send(sock, (const char*)msg, len, posix_flags);
    } else {
        sockaddr addr{};
        convertOrbisNetSockaddrToPosix(to, &addr);
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

#define CASE_SETSOCKOPT_INT(guest, host)                                                           \
    case guest: {                                                                                  \
        if (optlen < sizeof(u32)) {                                                                \
            *Libraries::Kernel::__Error() = ORBIS_NET_EINVAL;                                      \
            return -1;                                                                             \
        }                                                                                          \
        return ConvertReturnErrorCode(                                                             \
            setsockopt(sock, native_level, host, static_cast<const char*>(optval), optlen));       \
    }

#define CASE_SETSOCKOPT_VALUE(opt, value)                                                          \
    case opt: {                                                                                    \
        if (optlen < sizeof(*value)) {                                                             \
            *Libraries::Kernel::__Error() = ORBIS_NET_EINVAL;                                      \
        }                                                                                          \
        std::memcpy(value, optval, optlen);                                                        \
        return 0;                                                                                  \
    }

int PosixSocket::SetSocketOptions(int level, int optname, const void* optval, u32 optlen) {
    std::scoped_lock lock{m_mutex};
    s32 native_level = ConvertLevels(level);
    if (native_level == SOL_SOCKET) {
        switch (optname) {
            CASE_SETSOCKOPT_INT(ORBIS_SO_REUSEADDR, SO_REUSEADDR);
            CASE_SETSOCKOPT_INT(ORBIS_SO_KEEPALIVE, SO_KEEPALIVE);
            CASE_SETSOCKOPT_INT(ORBIS_SO_BROADCAST, SO_BROADCAST);
            CASE_SETSOCKOPT_INT(ORBIS_SO_SNDBUF, SO_SNDBUF);
            CASE_SETSOCKOPT_INT(ORBIS_SO_RCVBUF, SO_RCVBUF);
            CASE_SETSOCKOPT_VALUE(ORBIS_SO_REUSEPORT, &sockopt_so_reuseport);
            CASE_SETSOCKOPT_VALUE(ORBIS_SO_ONESBCAST, &sockopt_so_onesbcast);
            CASE_SETSOCKOPT_VALUE(ORBIS_SO_USECRYPTO, &sockopt_so_usecrypto);
            CASE_SETSOCKOPT_VALUE(ORBIS_SO_USESIGNATURE, &sockopt_so_usesignature);
            CASE_SETSOCKOPT_VALUE(ORBIS_NET_SO_ACCEPTTIMEO, &sockopt_so_accepttimeo);
            CASE_SETSOCKOPT_VALUE(ORBIS_NET_SO_CONNECTTIMEO, &sockopt_so_connecttimeo);
        case ORBIS_SO_LINGER: {
            if (optlen < sizeof(OrbisNetLinger)) {
                *Libraries::Kernel::__Error() = ORBIS_NET_EINVAL;
                return -1;
            }

            ::linger native_linger;
            u32 native_len = sizeof(native_linger);
            native_linger.l_onoff = reinterpret_cast<const OrbisNetLinger*>(optval)->l_onoff;
            native_linger.l_linger = reinterpret_cast<const OrbisNetLinger*>(optval)->l_linger;
            return ConvertReturnErrorCode(setsockopt(sock, native_level, SO_LINGER,
                                                     reinterpret_cast<char*>(&native_linger),
                                                     native_len));
        }
        case ORBIS_SO_SNDTIMEO:
        case ORBIS_SO_RCVTIMEO: {
            // Set timeout using a timeval
            if (optlen != sizeof(Kernel::OrbisKernelTimeval)) {
                *Libraries::Kernel::__Error() = ORBIS_NET_EINVAL;
                return -1;
            }

            // Windows setsockopt uses milliseconds, we need to convert
            Kernel::OrbisKernelTimeval set_time =
                *static_cast<const Kernel::OrbisKernelTimeval*>(optval);
            s32 millis = set_time.tv_sec * 1000 + set_time.tv_usec / 1000;

            // Store time in ms
            if (optname == ORBIS_SO_SNDTIMEO) {
                sockopt_so_sndtimeo = millis;
            } else {
                sockopt_so_rcvtimeo = millis;
            }
            const auto native_opt = (optname == ORBIS_SO_SNDTIMEO) ? SO_SNDTIMEO : SO_RCVTIMEO;
#ifdef _WIN32
            return ConvertReturnErrorCode(setsockopt(
                sock, native_level, native_opt, reinterpret_cast<char*>(&millis), sizeof(millis)));
#else
            // POSIX platforms use timeval, it's safe to just pass along the optval
            return ConvertReturnErrorCode(setsockopt(
                sock, native_level, native_opt, reinterpret_cast<const char*>(optval), optlen));
#endif
        }
        case ORBIS_NET_SO_SNDTIMEO:
        case ORBIS_NET_SO_RCVTIMEO: {
            // Set timeout with inputted milliseconds count
            if (optlen != sizeof(u32)) {
                *Libraries::Kernel::__Error() = ORBIS_NET_EINVAL;
                return -1;
            }

            // Store inputted timeout
            u32 millis = *reinterpret_cast<const u32*>(optval);
            if (optname == ORBIS_SO_SNDTIMEO) {
                sockopt_so_sndtimeo = millis;
            } else {
                sockopt_so_rcvtimeo = millis;
            }
            const auto native_opt = (optname == ORBIS_NET_SO_SNDTIMEO) ? SO_SNDTIMEO : SO_RCVTIMEO;
#ifdef _WIN32
            // Windows setsockopt uses milliseconds, it's safe to just pass along the optval
            return ConvertReturnErrorCode(setsockopt(sock, native_level, native_opt,
                                                     static_cast<const char*>(optval), optlen));
#else
            // POSIX platforms use a timeval, we need to convert
            timeval timeout{.tv_sec = millis / 1000, .tv_usec = millis % 1000};
            return ConvertReturnErrorCode(setsockopt(sock, native_level, native_opt,
                                                     reinterpret_cast<char*>(&timeout),
                                                     sizeof(timeout)));
#endif
        }
        case ORBIS_SO_TYPE:
        case ORBIS_SO_ERROR: {
            // type and error cannot be set through setsockopt
            *Libraries::Kernel::__Error() = ORBIS_NET_ENOPROTOOPT;
            return -1;
        }
        case ORBIS_NET_SO_NAME: {
            // This sets the name of the socket
            if (optlen == 0) {
                *Libraries::Kernel::__Error() = ORBIS_NET_EINVAL;
                return -1;
            }
            u64 namelen = std::min<u64>(optlen, 31);
            // Inserts up to 31 characters, then throws a null terminator at index 30
            // Dunno why Sony does that, but this matches decomp.
            name.insert(0, static_cast<const char*>(optval), namelen);
            name.data()[30] = 0;
            return 0;
        }
        case ORBIS_NET_SO_NBIO: {
            if (optlen < sizeof(sockopt_so_nbio)) {
                *Libraries::Kernel::__Error() = ORBIS_NET_EINVAL;
                return -1;
            } else {
                memcpy(&sockopt_so_nbio, optval, sizeof(sockopt_so_nbio));
            }
#ifdef _WIN32
            return ConvertReturnErrorCode(
                ioctlsocket(sock, FIONBIO, reinterpret_cast<u_long*>(&sockopt_so_nbio)));
#else
            return ConvertReturnErrorCode(ioctl(sock, FIONBIO, &sockopt_so_nbio));
#endif
        }
        }
    } else if (native_level == IPPROTO_IP) {
        switch (optname) {
            CASE_SETSOCKOPT_INT(ORBIS_NET_IP_TOS, IP_TOS);
            CASE_SETSOCKOPT_INT(ORBIS_NET_IP_TTL, IP_TTL);
            CASE_SETSOCKOPT_INT(ORBIS_NET_IP_MULTICAST_IF, IP_MULTICAST_IF);
            CASE_SETSOCKOPT_INT(ORBIS_NET_IP_MULTICAST_TTL, IP_MULTICAST_TTL);
            CASE_SETSOCKOPT_INT(ORBIS_NET_IP_HDRINCL, IP_HDRINCL);
            CASE_SETSOCKOPT_VALUE(ORBIS_NET_IP_TTLCHK, &sockopt_ip_ttlchk);
            CASE_SETSOCKOPT_VALUE(ORBIS_NET_IP_MAXTTL, &sockopt_ip_maxttl);
        case ORBIS_NET_IP_ADD_MEMBERSHIP:
        case ORBIS_NET_IP_DROP_MEMBERSHIP: {
            if (optlen < sizeof(ip_mreq)) {
                *Libraries::Kernel::__Error() = ORBIS_NET_EINVAL;
                return -1;
            }
            const auto native_opt =
                optname == ORBIS_NET_IP_ADD_MEMBERSHIP ? IP_ADD_MEMBERSHIP : IP_DROP_MEMBERSHIP;
            return ConvertReturnErrorCode(setsockopt(sock, native_level, native_opt,
                                                     static_cast<const char*>(optval), optlen));
        }
        }
    } else if (native_level == IPPROTO_TCP) {
        switch (optname) {
            CASE_SETSOCKOPT_INT(ORBIS_NET_TCP_NODELAY, TCP_NODELAY);
            CASE_SETSOCKOPT_INT(ORBIS_NET_TCP_MAXSEG, TCP_MAXSEG);
            CASE_SETSOCKOPT_VALUE(ORBIS_NET_TCP_MSS_TO_ADVERTISE, &sockopt_tcp_mss_to_advertise);
        }
    }

    UNREACHABLE_MSG("Unknown level ={} optname ={}", level, optname);
    return 0;
}

#define CASE_GETSOCKOPT(guest, host)                                                               \
    case guest: {                                                                                  \
        socklen_t optlen_temp = *optlen;                                                           \
        auto retval = ConvertReturnErrorCode(                                                      \
            getsockopt(sock, native_level, host, static_cast<char*>(optval), &optlen_temp));       \
        *optlen = optlen_temp;                                                                     \
        return retval;                                                                             \
    }
#define CASE_GETSOCKOPT_VALUE(opt, value)                                                          \
    case opt:                                                                                      \
        *optlen = std::min<u32>(sizeof(value), *optlen);                                           \
        std::memcpy(optval, &value, *optlen);                                                      \
        return 0;

int PosixSocket::GetSocketOptions(int level, int optname, void* optval, u32* optlen) {
    std::scoped_lock lock{m_mutex};
    s32 native_level = ConvertLevels(level);
    if (native_level == SOL_SOCKET) {
        switch (optname) {
            CASE_GETSOCKOPT(ORBIS_SO_REUSEADDR, SO_REUSEADDR);
            CASE_GETSOCKOPT(ORBIS_SO_KEEPALIVE, SO_KEEPALIVE);
            CASE_GETSOCKOPT(ORBIS_SO_BROADCAST, SO_BROADCAST);
            CASE_GETSOCKOPT(ORBIS_SO_SNDBUF, SO_SNDBUF);
            CASE_GETSOCKOPT(ORBIS_SO_RCVBUF, SO_RCVBUF);
            CASE_GETSOCKOPT(ORBIS_SO_TYPE, SO_TYPE);
            CASE_GETSOCKOPT_VALUE(ORBIS_NET_SO_NBIO, sockopt_so_nbio);
            CASE_GETSOCKOPT_VALUE(ORBIS_NET_SO_CONNECTTIMEO, sockopt_so_connecttimeo);
            CASE_GETSOCKOPT_VALUE(ORBIS_NET_SO_ACCEPTTIMEO, sockopt_so_accepttimeo);
            CASE_GETSOCKOPT_VALUE(ORBIS_NET_SO_SNDTIMEO, sockopt_so_sndtimeo);
            CASE_GETSOCKOPT_VALUE(ORBIS_NET_SO_RCVTIMEO, sockopt_so_rcvtimeo);
            CASE_GETSOCKOPT_VALUE(ORBIS_SO_REUSEPORT, sockopt_so_reuseport);
            CASE_GETSOCKOPT_VALUE(ORBIS_SO_ONESBCAST, sockopt_so_onesbcast);
            CASE_GETSOCKOPT_VALUE(ORBIS_SO_USECRYPTO, sockopt_so_usecrypto);
            CASE_GETSOCKOPT_VALUE(ORBIS_SO_USESIGNATURE, sockopt_so_usesignature);
        case ORBIS_NET_SO_NAME: {
            *optlen = std::min<u32>(0x1f, *optlen);
            name.copy(static_cast<char*>(optval), *optlen);
            return 0;
        }
        case ORBIS_SO_LINGER: {
            ::linger native_linger;
            socklen_t native_len = sizeof(native_linger);
            auto retval = getsockopt(sock, native_level, SO_LINGER,
                                     reinterpret_cast<char*>(&native_linger), &native_len);
            OrbisNetLinger guest_linger{native_linger.l_linger, native_linger.l_onoff};
            *optlen = std::min<u32>(sizeof(OrbisNetLinger), *optlen);
            std::memcpy(optval, &guest_linger, *optlen);
            return retval;
        }
        case ORBIS_SO_SNDTIMEO:
        case ORBIS_SO_RCVTIMEO: {
            // Returns timeout as a timeval
            s32 millis = optname == ORBIS_SO_SNDTIMEO ? sockopt_so_sndtimeo : sockopt_so_rcvtimeo;
            Kernel::OrbisKernelTimeval out_time{millis / 1000, millis % 1000};
            *optlen = std::min<u32>(sizeof(out_time), *optlen);
            std::memcpy(optval, &out_time, *optlen);
            return 0;
        }
        case ORBIS_SO_ERROR: {
            socklen_t optlen_temp = *optlen;
            auto retval = ConvertReturnErrorCode(
                getsockopt(sock, level, SO_ERROR, static_cast<char*>(optval), &optlen_temp));
            *static_cast<u32*>(optval) = optlen_temp;
            return retval;
        }
        case ORBIS_NET_SO_ERROR_EX: {
            socklen_t optlen_temp = *optlen;
            auto retval = ConvertReturnErrorCode(
                getsockopt(sock, level, SO_ERROR, static_cast<char*>(optval), &optlen_temp));
            if (optlen_temp != 0) {
                optlen_temp |= ORBIS_NET_ERROR_BASE;
            }
            *static_cast<u32*>(optval) = optlen_temp;
            return retval;
        }
        }
    } else if (native_level == IPPROTO_IP) {
        switch (optname) {
            CASE_GETSOCKOPT(ORBIS_NET_IP_HDRINCL, IP_HDRINCL);
            CASE_GETSOCKOPT(ORBIS_NET_IP_TOS, IP_TOS);
            CASE_GETSOCKOPT(ORBIS_NET_IP_TTL, IP_TTL);
            CASE_GETSOCKOPT(ORBIS_NET_IP_MULTICAST_IF, IP_MULTICAST_IF);
            CASE_GETSOCKOPT(ORBIS_NET_IP_MULTICAST_TTL, IP_MULTICAST_TTL);
            CASE_GETSOCKOPT(ORBIS_NET_IP_ADD_MEMBERSHIP, IP_ADD_MEMBERSHIP);
            CASE_GETSOCKOPT(ORBIS_NET_IP_DROP_MEMBERSHIP, IP_DROP_MEMBERSHIP);
            CASE_GETSOCKOPT_VALUE(ORBIS_NET_IP_TTLCHK, sockopt_ip_ttlchk);
            CASE_GETSOCKOPT_VALUE(ORBIS_NET_IP_MAXTTL, sockopt_ip_maxttl);
        }
    } else if (native_level == IPPROTO_TCP) {
        switch (optname) {
            CASE_GETSOCKOPT(ORBIS_NET_TCP_NODELAY, TCP_NODELAY);
            CASE_GETSOCKOPT(ORBIS_NET_TCP_MAXSEG, TCP_MAXSEG);
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