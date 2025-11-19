// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <vector>
#include <common/assert.h>
#include "common/error.h"
#include "core/libraries/kernel/file_system.h"
#include "core/libraries/kernel/kernel.h"
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

int PosixSocket::SendMessage(const OrbisNetMsghdr* msg, int flags) {
    std::scoped_lock lock{m_mutex};
#ifdef _WIN32
    DWORD bytesSent = 0;
    LPFN_WSASENDMSG wsasendmsg = nullptr;
    GUID guid = WSAID_WSASENDMSG;
    DWORD bytes = 0;

    if (WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), &wsasendmsg,
                 sizeof(wsasendmsg), &bytes, nullptr, nullptr) != 0) {
        return ConvertReturnErrorCode(-1);
    }

    int res = wsasendmsg(sock, reinterpret_cast<LPWSAMSG>(const_cast<OrbisNetMsghdr*>(msg)), flags,
                         &bytesSent, nullptr, nullptr);

    if (res == SOCKET_ERROR) {
        return ConvertReturnErrorCode(-1);
    }
    return static_cast<int>(bytesSent);
#else
    int res = sendmsg(sock, reinterpret_cast<const msghdr*>(msg), flags);
    return ConvertReturnErrorCode(res);
#endif
}

int PosixSocket::SendPacket(const void* msg, u32 len, int flags, const OrbisNetSockaddr* to,
                            u32 tolen) {
    std::scoped_lock lock{m_mutex};
    if (to != nullptr) {
        sockaddr addr;
        convertOrbisNetSockaddrToPosix(to, &addr);
        return ConvertReturnErrorCode(
            sendto(sock, (const char*)msg, len, flags, &addr, sizeof(sockaddr_in)));
    } else {
        return ConvertReturnErrorCode(send(sock, (const char*)msg, len, flags));
    }
}

int PosixSocket::ReceiveMessage(OrbisNetMsghdr* msg, int flags) {
    std::scoped_lock lock{receive_mutex};
#ifdef _WIN32
    LPFN_WSARECVMSG wsarecvmsg = nullptr;
    GUID guid = WSAID_WSARECVMSG;
    DWORD bytes = 0;

    if (WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), &wsarecvmsg,
                 sizeof(wsarecvmsg), &bytes, nullptr, nullptr) != 0) {
        return ConvertReturnErrorCode(-1);
    }

    DWORD bytesReceived = 0;
    int res = wsarecvmsg(sock, reinterpret_cast<LPWSAMSG>(msg), &bytesReceived, nullptr, nullptr);

    if (res == SOCKET_ERROR) {
        return ConvertReturnErrorCode(-1);
    }
    return static_cast<int>(bytesReceived);
#else
    int res = recvmsg(sock, reinterpret_cast<msghdr*>(msg), flags);
    return ConvertReturnErrorCode(res);
#endif
}

int PosixSocket::ReceivePacket(void* buf, u32 len, int flags, OrbisNetSockaddr* from,
                               u32* fromlen) {
    std::scoped_lock lock{receive_mutex};
    if (from != nullptr) {
        sockaddr addr;
        int res = recvfrom(sock, (char*)buf, len, flags, &addr, (socklen_t*)fromlen);
        convertPosixSockaddrToOrbis(&addr, from);
        *fromlen = sizeof(OrbisNetSockaddrIn);
        return ConvertReturnErrorCode(res);
    } else {
        return ConvertReturnErrorCode(recv(sock, (char*)buf, len, flags));
    }
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