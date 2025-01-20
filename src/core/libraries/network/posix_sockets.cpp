// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <common/assert.h>
#include "net.h"
#include "net_error.h"
#include "sockets.h"

namespace Libraries::Net {

static int ConvertLevels(int level) {
    switch (level) {
    case ORBIS_NET_SOL_SOCKET:
        return SOL_SOCKET;
    case ORBIS_NET_IPPROTO_IP:
        return IPPROTO_IP;
    case ORBIS_NET_IPPROTO_TCP:
        return IPPROTO_TCP;
    }
    return -1;
}

static int ConvertReturnErrorCode(int retval) {
    if (retval < 0) {
#ifdef _WIN32
        int err = WSAGetLastError();
        LOG_ERROR(Lib_Net, "Error occured {}", err);
        switch (err) {
        case WSAENOPROTOOPT:
            return ORBIS_NET_ERROR_ENOPROTOOPT;
        case WSAEINVAL:
            return ORBIS_NET_ERROR_EINVAL;
#else
        LOG_ERROR(Lib_Net, "Error occured {}", errno);
        switch (errno) {
        case ENOPROTOOPT:
            return ORBIS_NET_ERROR_ENOPROTOOPT;
        case EINVAL:
            return ORBIS_NET_ERROR_EINVAL;
#endif
        }
        UNREACHABLE_MSG("Function returned an errorCode = {}", retval);
    }
    // if it is 0 or positive return it as it is
    return retval;
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

int PosixSocket::SetSocketOptions(int level, int optname, const void* optval, unsigned int optlen) {
    level = ConvertLevels(level);
    if (level == SOL_SOCKET) {
        switch (optname) {
        case ORBIS_NET_SO_REUSEADDR:
            return ConvertReturnErrorCode(
                setsockopt(sock, level, SO_REUSEADDR, (const char*)optval, optlen));
        case ORBIS_NET_SO_BROADCAST:
            return ConvertReturnErrorCode(
                setsockopt(sock, level, SO_BROADCAST, (const char*)optval, optlen));
        case ORBIS_NET_SO_SNDTIMEO:
            return ConvertReturnErrorCode(
                setsockopt(sock, level, SO_SNDTIMEO, (const char*)optval, optlen));
        case ORBIS_NET_SO_SNDBUF:
            return ConvertReturnErrorCode(
                setsockopt(sock, level, SO_SNDBUF, (const char*)optval, optlen));
        case ORBIS_NET_SO_RCVBUF:
            return ConvertReturnErrorCode(
                setsockopt(sock, level, SO_RCVBUF, (const char*)optval, optlen));
        case ORBIS_NET_SO_LINGER:
            return ConvertReturnErrorCode(
                setsockopt(sock, level, SO_LINGER, (const char*)optval, optlen));
        case ORBIS_NET_SO_ONESBCAST: {
            if (optlen != sizeof(sockopt_so_onesbcast)) {
                return ORBIS_NET_ERROR_EFAULT;
            }
            memcpy(&sockopt_so_onesbcast, optval, optlen);
            return 0;
        }
        case ORBIS_NET_SO_NBIO: {
            if (optlen != sizeof(sockopt_so_nbio)) {
                return ORBIS_NET_ERROR_EFAULT;
            }
            memcpy(&sockopt_so_nbio, optval, optlen);
#ifdef _WIN32
            return ConvertReturnErrorCode(ioctlsocket(sock, FIONBIO, (u_long*)&sockopt_so_nbio));
#else
            return ConvertReturnErrorCode(ioctl(sock, FIONBIO, &sockopt_so_nbio));
#endif
        }
        }
    }
    if (level == IPPROTO_TCP) {
        switch (optname) {
        case ORBIS_NET_TCP_NODELAY:
            return ConvertReturnErrorCode(
                setsockopt(sock, level, TCP_NODELAY, (const char*)optval, optlen));
        }
    }
    if (level == IPPROTO_IP) {
        switch (optname) {
        case ORBIS_NET_IP_HDRINCL:
            return ConvertReturnErrorCode(
                setsockopt(sock, level, IP_HDRINCL, (const char*)optval, optlen));
        }
    }
    UNREACHABLE_MSG("Unknown level ={} optname ={}", level, optname);
    return 0;
}
int PosixSocket::Bind(const OrbisNetSockaddr* addr, unsigned int addrlen) {
    sockaddr addr2;
    convertOrbisNetSockaddrToPosix(addr, &addr2);
    return ConvertReturnErrorCode(::bind(sock, &addr2, sizeof(sockaddr_in)));
}

int PosixSocket::Listen(int backlog) {
    return ConvertReturnErrorCode(::listen(sock, backlog));
}

int PosixSocket::SendPacket(const void* msg, u32 len, int flags, const OrbisNetSockaddr* to,
                            u32 tolen) {
    if (to != nullptr) {
        sockaddr addr;
        convertOrbisNetSockaddrToPosix(to, &addr);
        return ConvertReturnErrorCode(
            sendto(sock, (const char*)msg, len, flags, &addr, sizeof(sockaddr_in)));
    } else {
        return ConvertReturnErrorCode(send(sock, (const char*)msg, len, flags));
    }
}

int PosixSocket::ReceivePacket(void* buf, u32 len, int flags, OrbisNetSockaddr* from,
                               u32* fromlen) {
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
    sockaddr addr2;
    net_socket new_socket = ::accept(sock, &addr2, (socklen_t*)addrlen);
#ifdef _WIN32
    if (new_socket != INVALID_SOCKET) {
#else
    if (new_socket >= 0) {
#endif
        convertPosixSockaddrToOrbis(&addr2, addr);
        *addrlen = sizeof(OrbisNetSockaddrIn);
        return std::make_shared<PosixSocket>(new_socket);
    }
    return nullptr;
}

} // namespace Libraries::Net