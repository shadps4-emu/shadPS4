// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <common/assert.h>
#include "core/libraries/kernel/kernel.h"
#include "net.h"
#include "net_error.h"
#include "sockets.h"

namespace Libraries::Net {

static OrbisNetSockaddr ConvertP2PToPosix(const OrbisNetSockaddr* addr) {
    if (!addr) {
        return OrbisNetSockaddr{};
    }
    OrbisNetSockaddrIn result = *reinterpret_cast<const OrbisNetSockaddrIn*>(addr);
    // Convert ports from network to host order
    const uint16_t port = ntohs(result.sin_port);
    const uint16_t vport = ntohs(result.sin_vport);

    // Combine the two ports in host order, then convert back to network order
    result.sin_port = htons(port + vport);
    result.sin_vport = 0; // Clear virtual port since it's not used in Posix
    return std::bit_cast<OrbisNetSockaddr>(result);
}

static OrbisNetSockaddr ConvertPosixToP2P(const OrbisNetSockaddr* addr) {
    if (!addr) {
        return OrbisNetSockaddr{};
    }
    OrbisNetSockaddrIn result = *reinterpret_cast<const OrbisNetSockaddrIn*>(addr);
    // Attempt to recover original port and virtual port if port was previously combined
    const uint16_t port = ntohs(result.sin_port);
    result.sin_port = htons(port);
    result.sin_vport = 0;
    return std::bit_cast<OrbisNetSockaddr>(result);
}

static int p2pSocketTypeToPosixSocketType(int type) {
    int hostSockType;
    switch (type) {
    case ORBIS_NET_SOCK_DGRAM_P2P:
        hostSockType = SOCK_DGRAM;
        break;
    case ORBIS_NET_SOCK_STREAM_P2P:
        hostSockType = SOCK_STREAM;
        break;
    default:
        hostSockType = -1;
    }
    return hostSockType;
}

P2PSocket::P2PSocket(int domain, int type, int protocol)
    : PosixSocket(domain, p2pSocketTypeToPosixSocketType(type), protocol) {};

int P2PSocket::Bind(const OrbisNetSockaddr* addr, u32 addrlen) {
    const auto p2p_addr = ConvertP2PToPosix(addr);
    return PosixSocket::Bind(&p2p_addr, addrlen);
}

int P2PSocket::Listen(int backlog) {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return 0;
}

int P2PSocket::SendPacket(const void* msg, u32 len, int flags, const OrbisNetSockaddr* to,
                          u32 tolen) {
    const auto p2p_to = ConvertP2PToPosix(to);
    return PosixSocket::SendPacket(msg, len, flags, &p2p_to, tolen);
}

int P2PSocket::ReceivePacket(void* buf, u32 len, int flags, OrbisNetSockaddr* from, u32* fromlen) {
    const auto res = PosixSocket::ReceivePacket(buf, len, flags, from, fromlen);
    if ((res > 0) && from)
        *from = ConvertPosixToP2P(from);

    return res;
}

SocketPtr P2PSocket::Accept(OrbisNetSockaddr* addr, u32* addrlen) {
    const auto res = PosixSocket::Accept(addr, addrlen);
    *addr = ConvertPosixToP2P(addr);
    return res;
}

int P2PSocket::Connect(const OrbisNetSockaddr* addr, u32 namelen) {
    const auto p2p_addr = ConvertP2PToPosix(addr);
    return PosixSocket::Connect(&p2p_addr, namelen);
}

int P2PSocket::GetSocketAddress(OrbisNetSockaddr* name, u32* namelen) {
    const auto res = PosixSocket::GetSocketAddress(name, namelen);
    *name = ConvertPosixToP2P(name);
    return res;
}

int P2PSocket::GetPeerName(OrbisNetSockaddr* addr, u32* namelen) {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return 0;
}

int P2PSocket::fstat(Libraries::Kernel::OrbisKernelStat* stat) {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return 0;
}

} // namespace Libraries::Net