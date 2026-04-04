// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "net.h"
#include "net_error.h"
#include "sockets.h"

namespace Libraries::Net {

P2PSocket::P2PSocket(int domain, int type, int protocol)
    : PosixSocket(AF_INET, type == ORBIS_NET_SOCK_DGRAM_P2P ? SOCK_DGRAM : SOCK_STREAM,
                  type == ORBIS_NET_SOCK_DGRAM_P2P ? IPPROTO_UDP : IPPROTO_TCP) {
    // Store the original PS4 socket type so GetSocketOptions(SO_TYPE) returns the P2P value
    socket_type = type;
    LOG_INFO(Lib_Net, "P2P socket created: type={} (os_type={}), fd={}", type,
             type == ORBIS_NET_SOCK_DGRAM_P2P ? int(SOCK_DGRAM) : int(SOCK_STREAM), sock);
}

int P2PSocket::GetSocketOptions(int level, int optname, void* optval, u32* optlen) {
    // Intercept SO_TYPE to return the PS4 P2P socket type, not the native OS type
    if (ConvertLevels(level) == SOL_SOCKET && optname == ORBIS_NET_SO_TYPE) {
        *(int*)optval = socket_type;
        *optlen = sizeof(int);
        return 0;
    }
    return PosixSocket::GetSocketOptions(level, optname, optval, optlen);
}

int P2PSocket::Bind(const OrbisNetSockaddr* addr, u32 addrlen) {
    // TODO: vport multiplexing not implemented, on PS4, P2P sockets multiplex
    // virtual ports over a single UDP association. We extract the vport here for
    // future use but don't perform any demuxing.
    const auto* orbis_addr = reinterpret_cast<const OrbisNetSockaddrIn*>(addr);
    if (orbis_addr && orbis_addr->sin_family == ORBIS_NET_AF_INET) {
        vport = orbis_addr->sin_vport;
        if (vport != 0) {
            LOG_WARNING(Lib_Net, "P2P bind with vport={} — vport multiplexing is not implemented",
                        vport);
        }
        LOG_INFO(Lib_Net, "P2P bind: port={}, vport={}", ntohs(orbis_addr->sin_port), vport);
    }
    return PosixSocket::Bind(addr, addrlen);
}

int P2PSocket::Connect(const OrbisNetSockaddr* addr, u32 namelen) {
    // TODO: vport multiplexing not implemented
    const auto* orbis_addr = reinterpret_cast<const OrbisNetSockaddrIn*>(addr);
    if (orbis_addr && orbis_addr->sin_family == ORBIS_NET_AF_INET) {
        vport = orbis_addr->sin_vport;
        if (vport != 0) {
            LOG_WARNING(Lib_Net,
                        "P2P connect with vport={} — vport multiplexing is not implemented", vport);
        }
        LOG_INFO(Lib_Net, "P2P connect: vport={}", vport);
    }
    return PosixSocket::Connect(addr, namelen);
}

SocketPtr P2PSocket::Accept(OrbisNetSockaddr* addr, u32* addrlen) {
    std::scoped_lock lock{m_mutex};
    sockaddr native_addr;
    socklen_t len = sizeof(native_addr);
    net_socket new_sock = ::accept(sock, &native_addr, &len);
#ifdef _WIN32
    if (new_sock == INVALID_SOCKET) {
#else
    if (new_sock < 0) {
#endif
        ConvertReturnErrorCode(-1);
        return nullptr;
    }
    if (addr && addrlen) {
        convertPosixSockaddrToOrbis(&native_addr, addr);
        *addrlen = sizeof(OrbisNetSockaddrIn);
    }
    return std::make_shared<P2PSocket>(new_sock, socket_type, vport);
}

} // namespace Libraries::Net
