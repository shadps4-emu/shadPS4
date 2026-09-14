// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>
#include <vector>

#include <common/assert.h>
#include "common/error.h"
#include "core/libraries/kernel/file_system.h"
#include "core/libraries/kernel/kernel.h"
#include "net.h"
#include "net_error.h"
#include "p2p_port.h"
#include "sockets.h"

namespace Libraries::Net {

P2PSocket::P2PSocket(int domain, int type, int protocol)
    : Socket(domain, type, protocol), inbox(kInvalidNetSocket) {
    // The inbox exists from creation rather than from Bind: a guest may hand an unbound socket to
    // epoll or select, and Native() has to answer with a real descriptor from the start.
    if (!CreateP2PInbox(inbox, inbox_addr)) {
        LOG_ERROR(Lib_Net, "P2P socket has no inbox: {}", Common::GetLastErrorMsg());
    }
}

P2PSocket::~P2PSocket() {
    if (port) {
        port->Release(this);
    }
    CloseP2PSocket(inbox);
}

bool P2PSocket::IsValid() const {
    return IsValidP2PSocket(inbox);
}

int P2PSocket::Close() {
    std::scoped_lock lock{m_mutex};
    if (port) {
        port->Release(this);
        port.reset();
    }
    // Closing the inbox is also what releases a thread blocked in ReceivePacket.
    CloseP2PSocket(inbox);
    inbox = kInvalidNetSocket;
    bound = false;
    LOG_DEBUG(Lib_Net, "P2P socket closed");
    return 0;
}

// The options that decide how this socket behaves; anything else stays a no-op stub. They are
// cached rather than pushed to the inbox: the inbox is always non-blocking at the OS level, and
// SO_NBIO is honored by choosing how long a receive waits for it to become readable.
static int* CachedP2PSocketOption(P2PSocket* sock, int level, int optname) {
    if (level != ORBIS_NET_SOL_SOCKET) {
        return nullptr;
    }
    switch (optname) {
    case ORBIS_NET_SO_NBIO:
        return &sock->sockopt_so_nbio;
    case ORBIS_NET_SO_REUSEADDR:
        return &sock->sockopt_so_reuseaddr;
    case ORBIS_NET_SO_REUSEPORT:
        return &sock->sockopt_so_reuseport;
    default:
        return nullptr;
    }
}

int P2PSocket::SetSocketOptions(int level, int optname, const void* optval, u32 optlen) {
    if (int* cached = CachedP2PSocketOption(this, level, optname); cached != nullptr) {
        if (optval == nullptr || optlen < sizeof(*cached)) {
            *Libraries::Kernel::__Error() = ORBIS_NET_EINVAL;
            return -1;
        }
        std::memcpy(cached, optval, sizeof(*cached));
        LOG_DEBUG(Lib_Net, "P2P socket option {} = {}", optname, *cached);
        return 0;
    }
    LOG_DEBUG(Lib_Net, "(STUBBED) called, level = {}, optname = {}", level, optname);
    return 0;
}

int P2PSocket::GetSocketOptions(int level, int optname, void* optval, u32* optlen) {
    if (const int* cached = CachedP2PSocketOption(this, level, optname); cached != nullptr) {
        if (optval == nullptr || optlen == nullptr) {
            *Libraries::Kernel::__Error() = ORBIS_NET_EINVAL;
            return -1;
        }
        if (*optlen < sizeof(*cached)) {
            *optlen = sizeof(*cached);
            *Libraries::Kernel::__Error() = ORBIS_NET_EFAULT;
            return -1;
        }
        std::memcpy(optval, cached, sizeof(*cached));
        *optlen = sizeof(*cached);
        return 0;
    }
    LOG_DEBUG(Lib_Net, "(STUBBED) called, level = {}, optname = {}", level, optname);
    if (optval != nullptr && optlen != nullptr && *optlen != 0) {
        std::memset(optval, 0, *optlen);
    }
    return 0;
}

int P2PSocket::Bind(const OrbisNetSockaddr* addr, u32 addrlen) {
    if (addr == nullptr || addrlen < sizeof(OrbisNetSockaddrIn)) {
        *Libraries::Kernel::__Error() = ORBIS_NET_EINVAL;
        return -1;
    }
    const auto* in = reinterpret_cast<const OrbisNetSockaddrIn*>(addr);
    std::scoped_lock lock{m_mutex};
    // A socket binds once; rebinding would leave it claiming two vports, and the first claim would
    // keep receiving traffic the guest thinks it moved away from.
    if (bound) {
        LOG_ERROR(Lib_Net, "P2P socket already bound to vport {}", ntohs(bound_vport));
        *Libraries::Kernel::__Error() = ORBIS_NET_EINVAL;
        return -1;
    }
    if (!IsValidP2PSocket(inbox)) {
        *Libraries::Kernel::__Error() = ORBIS_NET_EBADF;
        return -1;
    }

    auto host_port = P2PPort::Acquire(in->sin_addr, in->sin_port);
    if (!host_port) {
        return ConvertReturnErrorCode(-1);
    }
    const bool reusable = sockopt_so_reuseaddr != 0 || sockopt_so_reuseport != 0;
    const u16 vport = host_port->Claim(in->sin_vport, reusable, this, inbox_addr);
    if (vport == kP2PSignalingVport) {
        LOG_ERROR(Lib_Net, "P2P vport {} already in use", ntohs(in->sin_vport));
        *Libraries::Kernel::__Error() = ORBIS_NET_EADDRINUSE;
        return -1;
    }

    port = std::move(host_port);
    bound_addr = port->BoundAddr();
    bound_port = port->BoundPort();
    bound_vport = vport;
    bound = true;
    LOG_INFO(Lib_Net, "P2P socket bound, port = {}, vport = {}", ntohs(bound_port),
             ntohs(bound_vport));
    return 0;
}

int P2PSocket::EnsureBound() {
    if (bound) {
        return 0;
    }
    // Sending from an unbound socket implicitly binds it, so the peer has somewhere to answer.
    OrbisNetSockaddrIn any{};
    any.sin_len = sizeof(any);
    any.sin_family = ORBIS_NET_AF_INET;
    any.sin_addr = htonl(INADDR_ANY);
    any.sin_port = 0;
    any.sin_vport = kP2PSignalingVport;

    auto host_port = P2PPort::Acquire(any.sin_addr, any.sin_port);
    if (!host_port) {
        return ConvertReturnErrorCode(-1);
    }
    const u16 vport = host_port->Claim(any.sin_vport, false, this, inbox_addr);
    if (vport == kP2PSignalingVport) {
        *Libraries::Kernel::__Error() = ORBIS_NET_EADDRINUSE;
        return -1;
    }
    port = std::move(host_port);
    bound_addr = port->BoundAddr();
    bound_port = port->BoundPort();
    bound_vport = vport;
    bound = true;
    LOG_DEBUG(Lib_Net, "P2P socket implicitly bound, port = {}, vport = {}", ntohs(bound_port),
              ntohs(bound_vport));
    return 0;
}

int P2PSocket::Listen(int backlog) {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return 0;
}

int P2PSocket::SendMessage(const OrbisNetMsghdr* msg, int flags) {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    *Libraries::Kernel::__Error() = ORBIS_NET_EAGAIN;
    return -1;
}

int P2PSocket::SendPacket(const void* msg, u32 len, int flags, const OrbisNetSockaddr* to,
                          u32 tolen) {
    if (msg == nullptr && len != 0) {
        *Libraries::Kernel::__Error() = ORBIS_NET_EINVAL;
        return -1;
    }
    std::scoped_lock lock{m_mutex};

    u32 dest_addr;
    u16 dest_port;
    u16 dest_vport;
    if (to != nullptr && tolen >= sizeof(OrbisNetSockaddrIn)) {
        const auto* dest = reinterpret_cast<const OrbisNetSockaddrIn*>(to);
        dest_addr = dest->sin_addr;
        dest_port = dest->sin_port;
        dest_vport = dest->sin_vport;
    } else if (to == nullptr && connected) {
        // sceNetSend passes no address: the destination is the one Connect recorded.
        dest_addr = peer_addr;
        dest_port = peer_port;
        dest_vport = peer_vport;
    } else {
        *Libraries::Kernel::__Error() = to == nullptr ? ORBIS_NET_EDESTADDRREQ : ORBIS_NET_EINVAL;
        return -1;
    }

    if (EnsureBound() != 0) {
        return -1;
    }

    sockaddr_in dest{};
    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = dest_addr;
    dest.sin_port = dest_port;
    const u16 header_flags =
        socket_type == ORBIS_NET_SOCK_STREAM_P2P ? kP2PFlagStream : kP2PFlagDgram;
    const int sent = port->Send(msg, len, bound_vport, dest_vport, dest, header_flags);
    if (sent < 0) {
        return ConvertReturnErrorCode(-1);
    }
    LOG_DEBUG(Lib_Net, "P2P datagram sent, len = {}, vport = {}", len, ntohs(dest_vport));
    return sent;
}

int P2PSocket::ReceiveMessage(OrbisNetMsghdr* msg, int flags) {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    *Libraries::Kernel::__Error() = ORBIS_NET_EAGAIN;
    return -1;
}

int P2PSocket::ReceivePacket(void* buf, u32 len, int flags, OrbisNetSockaddr* from, u32* fromlen) {
    std::scoped_lock lock{receive_mutex};
    if (!IsValidP2PSocket(inbox)) {
        *Libraries::Kernel::__Error() = ORBIS_NET_EBADF;
        return -1;
    }
    // SO_NBIO is the socket-wide setting; MSG_DONTWAIT makes a single call non-blocking. Either
    // way the OS decides when data is there: nothing here sleeps or spins.
    const bool non_blocking = sockopt_so_nbio != 0 || (flags & ORBIS_NET_MSG_DONTWAIT) != 0;
    if (!WaitP2PSocketReadable(inbox, non_blocking ? 0 : -1)) {
        // A blocking wait only fails when the socket goes away under us, i.e. on Close().
        *Libraries::Kernel::__Error() = non_blocking ? ORBIS_NET_EAGAIN : ORBIS_NET_EBADF;
        return -1;
    }

    thread_local std::vector<u8> datagram(sizeof(P2PInboxHeader) + kP2PMaxDatagram);
    // MSG_PEEK maps onto the inbox directly, so a peeked datagram stays queued for the next read.
    const int peek = (flags & ORBIS_NET_MSG_PEEK) != 0 ? MSG_PEEK : 0;
    const int received = recvfrom(inbox, reinterpret_cast<char*>(datagram.data()),
                                  static_cast<int>(datagram.size()), peek, nullptr, nullptr);
    if (received < static_cast<int>(sizeof(P2PInboxHeader))) {
        *Libraries::Kernel::__Error() = ORBIS_NET_EAGAIN;
        return -1;
    }

    P2PInboxHeader header;
    std::memcpy(&header, datagram.data(), sizeof(header));
    const u32 payload_len = static_cast<u32>(received) - sizeof(P2PInboxHeader);
    const u32 copy_len = std::min<u32>(len, payload_len);
    if (buf != nullptr && copy_len != 0) {
        std::memcpy(buf, datagram.data() + sizeof(P2PInboxHeader), copy_len);
    }
    if (from != nullptr && fromlen != nullptr && *fromlen >= sizeof(OrbisNetSockaddrIn)) {
        auto* in = reinterpret_cast<OrbisNetSockaddrIn*>(from);
        std::memset(in, 0, sizeof(OrbisNetSockaddrIn));
        in->sin_len = sizeof(OrbisNetSockaddrIn);
        in->sin_family = ORBIS_NET_AF_INET;
        in->sin_addr = header.from_addr;
        in->sin_port = header.from_port;
        in->sin_vport = header.from_vport;
        *fromlen = sizeof(OrbisNetSockaddrIn);
    }
    LOG_DEBUG(Lib_Net, "P2P datagram received, len = {}", copy_len);
    return static_cast<int>(copy_len);
}

SocketPtr P2PSocket::Accept(OrbisNetSockaddr* addr, u32* addrlen) {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    *Libraries::Kernel::__Error() = ORBIS_NET_EAGAIN;
    return nullptr;
}

int P2PSocket::Connect(const OrbisNetSockaddr* addr, u32 namelen) {
    if (addr == nullptr || namelen < sizeof(OrbisNetSockaddrIn)) {
        *Libraries::Kernel::__Error() = ORBIS_NET_EINVAL;
        return -1;
    }
    const auto* in = reinterpret_cast<const OrbisNetSockaddrIn*>(addr);
    std::scoped_lock lock{m_mutex};
    // A datagram socket is never connected at the OS level: it has to keep receiving from every
    // peer. Connect only fixes the default destination and the address GetPeerName reports.
    if (EnsureBound() != 0) {
        return -1;
    }
    peer_addr = in->sin_addr;
    peer_port = in->sin_port;
    peer_vport = in->sin_vport;
    connected = true;
    LOG_DEBUG(Lib_Net, "P2P socket connected to port = {}, vport = {}", ntohs(peer_port),
              ntohs(peer_vport));
    return 0;
}

int P2PSocket::GetSocketAddress(OrbisNetSockaddr* name, u32* namelen) {
    if (name == nullptr || namelen == nullptr || *namelen < sizeof(OrbisNetSockaddrIn)) {
        *Libraries::Kernel::__Error() = ORBIS_NET_EINVAL;
        return -1;
    }
    auto* in = reinterpret_cast<OrbisNetSockaddrIn*>(name);
    std::memset(in, 0, sizeof(OrbisNetSockaddrIn));
    in->sin_len = sizeof(OrbisNetSockaddrIn);
    in->sin_family = ORBIS_NET_AF_INET;
    {
        std::scoped_lock lock{m_mutex};
        in->sin_addr = bound_addr;
        in->sin_port = bound_port;
        in->sin_vport = bound_vport;
    }
    *namelen = sizeof(OrbisNetSockaddrIn);
    return 0;
}

int P2PSocket::GetPeerName(OrbisNetSockaddr* addr, u32* namelen) {
    if (addr == nullptr || namelen == nullptr || *namelen < sizeof(OrbisNetSockaddrIn)) {
        *Libraries::Kernel::__Error() = ORBIS_NET_EINVAL;
        return -1;
    }
    std::scoped_lock lock{m_mutex};
    if (!connected) {
        LOG_DEBUG(Lib_Net, "P2P socket has no peer");
        *Libraries::Kernel::__Error() = ORBIS_NET_ENOTCONN;
        return -1;
    }
    auto* in = reinterpret_cast<OrbisNetSockaddrIn*>(addr);
    std::memset(in, 0, sizeof(OrbisNetSockaddrIn));
    in->sin_len = sizeof(OrbisNetSockaddrIn);
    in->sin_family = ORBIS_NET_AF_INET;
    in->sin_addr = peer_addr;
    in->sin_port = peer_port;
    in->sin_vport = peer_vport;
    *namelen = sizeof(OrbisNetSockaddrIn);
    return 0;
}

int P2PSocket::fstat(Libraries::Kernel::OrbisKernelStat* stat) {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    if (stat != nullptr) {
        std::memset(stat, 0, sizeof(*stat));
    }
    return 0;
}

u16 GetP2PConfiguredPort() {
    return 0;
}

u32 GetP2PAdvertisedAddr() {
    return 0;
}

bool EnsureP2PTransport() {
    return false;
}

bool P2PTransportIsReady() {
    return false;
}

int P2PSignalingSendTo(const void* data, u32 len, u32 dest_addr, u16 dest_port) {
    return -1;
}

int P2PSignalingRecvFrom(void* buf, u32 len, u32* from_addr, u16* from_port) {
    return -1;
}

int P2PControlSendTo(const void* data, u32 len, u32 dest_addr, u16 dest_port) {
    return -1;
}

int P2PControlRecvFrom(void* buf, u32 len, u32* from_addr, u16* from_port) {
    return -1;
}

int P2PMatching2SendTo(const void* data, u32 len, u32 dest_addr, u16 dest_port) {
    return -1;
}

int P2PMatching2RecvFrom(void* buf, u32 len, u32* from_addr, u16* from_port) {
    return -1;
}

} // namespace Libraries::Net
