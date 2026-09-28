// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <vector>

#include "common/assert.h"
#include "core/libraries/kernel/kernel.h"
#include "net.h"
#include "net_error.h"
#include "p2p_port.h"
#include "sockets.h"

namespace Libraries::Net {
namespace {

void SetP2PErrorFromHost() {
#ifdef _WIN32
    const int error = WSAGetLastError();
    switch (error) {
    case WSAEWOULDBLOCK:
        *Libraries::Kernel::__Error() = ORBIS_NET_EWOULDBLOCK;
        return;
    case WSAEADDRINUSE:
        *Libraries::Kernel::__Error() = ORBIS_NET_EADDRINUSE;
        return;
    case WSAEADDRNOTAVAIL:
        *Libraries::Kernel::__Error() = ORBIS_NET_EADDRNOTAVAIL;
        return;
    case WSAEBADF:
        *Libraries::Kernel::__Error() = ORBIS_NET_EBADF;
        return;
    case WSAEINVAL:
        *Libraries::Kernel::__Error() = ORBIS_NET_EINVAL;
        return;
    case WSAEMSGSIZE:
        *Libraries::Kernel::__Error() = ORBIS_NET_EMSGSIZE;
        return;
    case WSAENETUNREACH:
        *Libraries::Kernel::__Error() = ORBIS_NET_ENETUNREACH;
        return;
    case WSAETIMEDOUT:
        *Libraries::Kernel::__Error() = ORBIS_NET_ETIMEDOUT;
        return;
    default:
        break;
    }
#else
    const int error = errno;
    switch (error) {
    case EAGAIN:
#if EWOULDBLOCK != EAGAIN
    case EWOULDBLOCK:
#endif
        *Libraries::Kernel::__Error() = ORBIS_NET_EWOULDBLOCK;
        return;
    case EADDRINUSE:
        *Libraries::Kernel::__Error() = ORBIS_NET_EADDRINUSE;
        return;
    case EADDRNOTAVAIL:
        *Libraries::Kernel::__Error() = ORBIS_NET_EADDRNOTAVAIL;
        return;
    case EBADF:
        *Libraries::Kernel::__Error() = ORBIS_NET_EBADF;
        return;
    case EINVAL:
        *Libraries::Kernel::__Error() = ORBIS_NET_EINVAL;
        return;
    case EMSGSIZE:
        *Libraries::Kernel::__Error() = ORBIS_NET_EMSGSIZE;
        return;
    case ENETUNREACH:
        *Libraries::Kernel::__Error() = ORBIS_NET_ENETUNREACH;
        return;
    case ETIMEDOUT:
        *Libraries::Kernel::__Error() = ORBIS_NET_ETIMEDOUT;
        return;
    default:
        break;
    }
#endif
    *Libraries::Kernel::__Error() = ORBIS_NET_EINTERNAL;
}

bool ValidAddr(const OrbisNetSockaddr* addr, u32 len) {
    return addr != nullptr && len >= sizeof(OrbisNetSockaddrIn);
}

} // namespace

P2PSocket::P2PSocket(int domain, int type, int protocol)
    : Socket(domain, type, protocol), inbox(kInvalidP2PSocket) {
    if (!CreateP2PInbox(inbox, inbox_addr)) {
        LOG_ERROR(Lib_Net, "P2P: failed to create inbox");
        return;
    }

    LOG_DEBUG(Lib_Net, "P2P: created socket inbox on 127.0.0.1:{}", ntohs(inbox_addr.sin_port));
}

P2PSocket::~P2PSocket() {
    Close();
}

bool P2PSocket::IsValid() const {
    return IsValidP2PSocket(inbox);
}

int P2PSocket::Close() {
    std::scoped_lock lock(m_mutex);

    if (port) {
        port->Release(this);
        port.reset();
    }

    if (IsValidP2PSocket(inbox)) {
        CloseP2PSocket(inbox);
        inbox = kInvalidP2PSocket;
    }

    bound = false;
    connected = false;
    return 0;
}

int P2PSocket::Shutdown(int how) {
    LOG_DEBUG(Lib_Net, "P2P: Shutdown is a no-op for datagram transport");
    return 0;
}

static int* CachedP2POption(P2PSocket* socket, int level, int optname) {
    if (level != ORBIS_NET_SOL_SOCKET) {
        return nullptr;
    }

    switch (optname) {
    case ORBIS_NET_SO_NBIO:
        return &socket->sockopt_so_nbio;
    case ORBIS_NET_SO_REUSEADDR:
        return &socket->sockopt_so_reuseaddr;
    case ORBIS_NET_SO_REUSEPORT:
        return &socket->sockopt_so_reuseport;
    default:
        return nullptr;
    }
}

int P2PSocket::SetSocketOptions(int level, int optname, const void* optval, u32 optlen) {
    if (int* option = CachedP2POption(this, level, optname)) {
        if (optval == nullptr || optlen < sizeof(int)) {
            *Libraries::Kernel::__Error() = ORBIS_NET_EINVAL;
            return -1;
        }
        std::memcpy(option, optval, sizeof(int));
        return 0;
    }

    LOG_DEBUG(Lib_Net, "P2P: ignored socket option level={}, optname={}", level, optname);
    return 0;
}

int P2PSocket::GetSocketOptions(int level, int optname, void* optval, u32* optlen) {
    if (const int* option = CachedP2POption(this, level, optname)) {
        if (optval == nullptr || optlen == nullptr) {
            *Libraries::Kernel::__Error() = ORBIS_NET_EINVAL;
            return -1;
        }
        if (*optlen < sizeof(int)) {
            *optlen = sizeof(int);
            *Libraries::Kernel::__Error() = ORBIS_NET_EFAULT;
            return -1;
        }
        std::memcpy(optval, option, sizeof(int));
        *optlen = sizeof(int);
        return 0;
    }

    if (optval != nullptr && optlen != nullptr && *optlen != 0) {
        std::memset(optval, 0, *optlen);
    }
    return 0;
}

int P2PSocket::Bind(const OrbisNetSockaddr* addr, u32 addrlen) {
    if (!ValidAddr(addr, addrlen)) {
        *Libraries::Kernel::__Error() = ORBIS_NET_EINVAL;
        return -1;
    }

    const auto* in = reinterpret_cast<const OrbisNetSockaddrIn*>(addr);
    std::scoped_lock lock(m_mutex);

    if (bound) {
        *Libraries::Kernel::__Error() = ORBIS_NET_EINVAL;
        return -1;
    }

    if (!IsValidP2PSocket(inbox)) {
        *Libraries::Kernel::__Error() = ORBIS_NET_EBADF;
        return -1;
    }

    auto host_port = P2PPort::Acquire(in->sin_addr, in->sin_port);
    if (!host_port) {
        SetP2PErrorFromHost();
        return -1;
    }

    const bool reusable = sockopt_so_reuseaddr != 0 || sockopt_so_reuseport != 0;
    const u16 assigned_vport = host_port->Claim(in->sin_vport, reusable, this, inbox_addr);
    if (assigned_vport == kP2PSignalingVport) {
        *Libraries::Kernel::__Error() = ORBIS_NET_EADDRINUSE;
        return -1;
    }

    port = std::move(host_port);
    bound_addr = port->BoundAddr();
    bound_port = port->BoundPort();
    bound_vport = assigned_vport;
    bound = true;

    LOG_INFO(Lib_Net, "P2P: bound {}:{} vport={}", ntohl(bound_addr), ntohs(bound_port),
             ntohs(bound_vport));
    return 0;
}

int P2PSocket::Listen(int backlog) {
    return 0;
}

int P2PSocket::EnsureBound() {
    if (bound) {
        return 0;
    }

    OrbisNetSockaddrIn any{};
    any.sin_len = sizeof(any);
    any.sin_family = ORBIS_NET_AF_INET;
    any.sin_addr = htonl(INADDR_ANY);
    any.sin_port = 0;
    any.sin_vport = kP2PSignalingVport;

    auto host_port = P2PPort::Acquire(any.sin_addr, any.sin_port);
    if (!host_port) {
        SetP2PErrorFromHost();
        return -1;
    }

    const u16 assigned_vport = host_port->Claim(any.sin_vport, false, this, inbox_addr);
    if (assigned_vport == kP2PSignalingVport) {
        *Libraries::Kernel::__Error() = ORBIS_NET_EADDRINUSE;
        return -1;
    }

    port = std::move(host_port);
    bound_addr = port->BoundAddr();
    bound_port = port->BoundPort();
    bound_vport = assigned_vport;
    bound = true;
    return 0;
}

int P2PSocket::SendMessage(const OrbisNetMsghdr* msg, int flags) {
    *Libraries::Kernel::__Error() = ORBIS_NET_EAGAIN;
    return -1;
}

int P2PSocket::SendPacket(const void* msg, u32 len, int flags, const OrbisNetSockaddr* to,
                          u32 tolen) {
    if (msg == nullptr && len != 0) {
        *Libraries::Kernel::__Error() = ORBIS_NET_EINVAL;
        return -1;
    }

    std::scoped_lock lock(m_mutex);

    u32 dest_addr = 0;
    u16 dest_port = 0;
    u16 dest_vport = 0;

    if (to != nullptr && tolen >= sizeof(OrbisNetSockaddrIn)) {
        const auto* destination = reinterpret_cast<const OrbisNetSockaddrIn*>(to);
        dest_addr = destination->sin_addr;
        dest_port = destination->sin_port;
        dest_vport = destination->sin_vport;
    } else if (to == nullptr && connected) {
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

    sockaddr_in destination{};
    destination.sin_family = AF_INET;
    destination.sin_addr.s_addr = dest_addr;
    destination.sin_port = dest_port;

    const u16 packet_flags =
        socket_type == ORBIS_NET_SOCK_STREAM_P2P ? kP2PFlagStream : kP2PFlagDgram;

    const int result = port->Send(msg, len, bound_vport, dest_vport, destination, packet_flags);
    if (result < 0) {
        SetP2PErrorFromHost();
        return -1;
    }

    return result;
}

int P2PSocket::ReceiveMessage(OrbisNetMsghdr* msg, int flags) {
    *Libraries::Kernel::__Error() = ORBIS_NET_EAGAIN;
    return -1;
}

int P2PSocket::ReceivePacket(void* buf, u32 len, int flags, OrbisNetSockaddr* from, u32* fromlen) {
    std::scoped_lock lock(receive_mutex);

    if (!IsValidP2PSocket(inbox)) {
        *Libraries::Kernel::__Error() = ORBIS_NET_EBADF;
        return -1;
    }

    const bool non_blocking = sockopt_so_nbio != 0 || (flags & ORBIS_NET_MSG_DONTWAIT) != 0;
    if (!WaitP2PSocketReadable(inbox, non_blocking ? 0 : -1)) {
        *Libraries::Kernel::__Error() = non_blocking ? ORBIS_NET_EAGAIN : ORBIS_NET_EBADF;
        return -1;
    }

    thread_local std::vector<u8> packet(sizeof(P2PInboxHeader) + kP2PMaxDatagram);
    const int host_flags = (flags & ORBIS_NET_MSG_PEEK) ? MSG_PEEK : 0;
    const int received = recvfrom(inbox, reinterpret_cast<char*>(packet.data()),
                                  static_cast<int>(packet.size()), host_flags, nullptr, nullptr);
    if (received < static_cast<int>(sizeof(P2PInboxHeader))) {
        SetP2PErrorFromHost();
        return -1;
    }

    P2PInboxHeader header{};
    std::memcpy(&header, packet.data(), sizeof(header));

    const u32 payload_len = static_cast<u32>(received - sizeof(P2PInboxHeader));
    const u32 copy_len = std::min(len, payload_len);

    if (buf != nullptr && copy_len != 0) {
        std::memcpy(buf, packet.data() + sizeof(P2PInboxHeader), copy_len);
    }

    if (from != nullptr && fromlen != nullptr && *fromlen >= sizeof(OrbisNetSockaddrIn)) {
        auto* sender = reinterpret_cast<OrbisNetSockaddrIn*>(from);
        std::memset(sender, 0, sizeof(*sender));
        sender->sin_len = sizeof(OrbisNetSockaddrIn);
        sender->sin_family = ORBIS_NET_AF_INET;
        sender->sin_addr = header.from_addr;
        sender->sin_port = header.from_port;
        sender->sin_vport = header.from_vport;
        *fromlen = sizeof(OrbisNetSockaddrIn);
    }

    return static_cast<int>(copy_len);
}

SocketPtr P2PSocket::Accept(OrbisNetSockaddr* addr, u32* addrlen) {
    *Libraries::Kernel::__Error() = ORBIS_NET_EAGAIN;
    return nullptr;
}

int P2PSocket::Connect(const OrbisNetSockaddr* addr, u32 namelen) {
    if (!ValidAddr(addr, namelen)) {
        *Libraries::Kernel::__Error() = ORBIS_NET_EINVAL;
        return -1;
    }

    const auto* peer = reinterpret_cast<const OrbisNetSockaddrIn*>(addr);
    std::scoped_lock lock(m_mutex);

    if (EnsureBound() != 0) {
        return -1;
    }

    peer_addr = peer->sin_addr;
    peer_port = peer->sin_port;
    peer_vport = peer->sin_vport;
    connected = true;
    return 0;
}

int P2PSocket::GetSocketAddress(OrbisNetSockaddr* name, u32* namelen) {
    if (name == nullptr || namelen == nullptr || *namelen < sizeof(OrbisNetSockaddrIn)) {
        *Libraries::Kernel::__Error() = ORBIS_NET_EINVAL;
        return -1;
    }

    auto* result = reinterpret_cast<OrbisNetSockaddrIn*>(name);
    std::memset(result, 0, sizeof(*result));
    result->sin_len = sizeof(OrbisNetSockaddrIn);
    result->sin_family = ORBIS_NET_AF_INET;

    {
        std::scoped_lock lock(m_mutex);
        result->sin_addr = bound_addr;
        result->sin_port = bound_port;
        result->sin_vport = bound_vport;
    }

    *namelen = sizeof(OrbisNetSockaddrIn);
    return 0;
}

int P2PSocket::GetPeerName(OrbisNetSockaddr* addr, u32* namelen) {
    if (addr == nullptr || namelen == nullptr || *namelen < sizeof(OrbisNetSockaddrIn)) {
        *Libraries::Kernel::__Error() = ORBIS_NET_EINVAL;
        return -1;
    }

    std::scoped_lock lock(m_mutex);
    if (!connected) {
        *Libraries::Kernel::__Error() = ORBIS_NET_ENOTCONN;
        return -1;
    }

    auto* result = reinterpret_cast<OrbisNetSockaddrIn*>(addr);
    std::memset(result, 0, sizeof(*result));
    result->sin_len = sizeof(OrbisNetSockaddrIn);
    result->sin_family = ORBIS_NET_AF_INET;
    result->sin_addr = peer_addr;
    result->sin_port = peer_port;
    result->sin_vport = peer_vport;
    *namelen = sizeof(OrbisNetSockaddrIn);
    return 0;
}

int P2PSocket::fstat(Libraries::Kernel::OrbisKernelStat* stat) {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return 0;
}

u16 GetP2PConfiguredPort() {
    return 0;
}

u32 GetP2PAdvertisedAddr() {
    return 0;
}

bool EnsureP2PTransport() {
    return true;
}

bool P2PTransportIsReady() {
    return true;
}

int P2PSignalingSendTo(const void* data, u32 len, u32 dest_addr, u16 dest_port) {
    *Libraries::Kernel::__Error() = ORBIS_NET_EAGAIN;
    return -1;
}

int P2PSignalingRecvFrom(void* buf, u32 len, u32* from_addr, u16* from_port) {
    *Libraries::Kernel::__Error() = ORBIS_NET_EAGAIN;
    return -1;
}

int P2PControlSendTo(const void* data, u32 len, u32 dest_addr, u16 dest_port) {
    *Libraries::Kernel::__Error() = ORBIS_NET_EAGAIN;
    return -1;
}

int P2PControlRecvFrom(void* buf, u32 len, u32* from_addr, u16* from_port) {
    *Libraries::Kernel::__Error() = ORBIS_NET_EAGAIN;
    return -1;
}

int P2PMatching2SendTo(const void* data, u32 len, u32 dest_addr, u16 dest_port) {
    *Libraries::Kernel::__Error() = ORBIS_NET_EAGAIN;
    return -1;
}

int P2PMatching2RecvFrom(void* buf, u32 len, u32* from_addr, u16* from_port) {
    *Libraries::Kernel::__Error() = ORBIS_NET_EAGAIN;
    return -1;
}

} // namespace Libraries::Net
