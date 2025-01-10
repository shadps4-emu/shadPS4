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

} // namespace Libraries::Net