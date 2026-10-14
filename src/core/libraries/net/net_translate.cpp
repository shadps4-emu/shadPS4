// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cstddef>
#include <cstring>

#include "core/libraries/net/net_error.h"
#include "core/libraries/net/net_translate.h"

#ifdef _WIN32
#include <afunix.h>
#else
#include <netinet/tcp.h>
#include <sys/un.h>
#endif

namespace Libraries::Net {

std::optional<int> ToHostFamily(int orbis_family) {
    switch (orbis_family) {
    case ORBIS_NET_AF_UNIX:
        return AF_UNIX;
    case ORBIS_NET_AF_INET:
        return AF_INET;
    case ORBIS_NET_AF_INET6:
        return AF_INET6;
    default:
        return std::nullopt;
    }
}

int ToOrbisFamily(int host_family) {
    switch (host_family) {
    case AF_UNIX:
        return ORBIS_NET_AF_UNIX;
    case AF_INET:
        return ORBIS_NET_AF_INET;
    case AF_INET6:
        return ORBIS_NET_AF_INET6;
    default:
        return 0;
    }
}

std::optional<HostSocketKind> ToHostSocketKind(int family, int type, int protocol,
                                               int* orbis_errno) {
    if (family == ORBIS_NET_AF_UNIX) {
        if (type != ORBIS_NET_SOCK_STREAM && type != ORBIS_NET_SOCK_DGRAM) {
            *orbis_errno = ORBIS_NET_EPROTONOSUPPORT;
            return std::nullopt;
        }
        return HostSocketKind{AF_UNIX, type == ORBIS_NET_SOCK_STREAM ? SOCK_STREAM : SOCK_DGRAM, 0,
                              false};
    }
    // PS4 returns EPROTONOSUPPORT here, not EAFNOSUPPORT.
    if (family != ORBIS_NET_AF_INET) {
        *orbis_errno = ORBIS_NET_EPROTONOSUPPORT;
        return std::nullopt;
    }
    const auto host_family = ToHostFamily(family);
    HostSocketKind kind{*host_family, 0, protocol, false};
    switch (type) {
    case ORBIS_NET_SOCK_STREAM:
        kind.type = SOCK_STREAM;
        break;
    case ORBIS_NET_SOCK_DGRAM:
        kind.type = SOCK_DGRAM;
        break;
    case ORBIS_NET_SOCK_RAW:
        kind.type = SOCK_RAW;
        break;
    case ORBIS_NET_SOCK_DGRAM_P2P:
        kind.type = SOCK_DGRAM;
        kind.p2p = true;
        break;
    case ORBIS_NET_SOCK_STREAM_P2P:
        kind.type = SOCK_STREAM;
        kind.p2p = true;
        break;
    default:
        *orbis_errno = ORBIS_NET_EPROTONOSUPPORT;
        return std::nullopt;
    }
    if (type != ORBIS_NET_SOCK_RAW && protocol != 0) {
        const int natural = kind.p2p                   ? -1
                            : kind.type == SOCK_STREAM ? ORBIS_NET_IPPROTO_TCP
                                                       : ORBIS_NET_IPPROTO_UDP;
        if (protocol != natural) {
            *orbis_errno = ORBIS_NET_EPROTOTYPE;
            return std::nullopt;
        }
    }
    return kind;
}

int ToOrbisSocketType(int host_type, bool p2p) {
    if (host_type == SOCK_STREAM) {
        return p2p ? ORBIS_NET_SOCK_STREAM_P2P : ORBIS_NET_SOCK_STREAM;
    }
    if (host_type == SOCK_DGRAM) {
        return p2p ? ORBIS_NET_SOCK_DGRAM_P2P : ORBIS_NET_SOCK_DGRAM;
    }
    return ORBIS_NET_SOCK_RAW;
}

int ToHostSockaddr(const OrbisNetSockaddr* addr, u32 addrlen, sockaddr_storage* out,
                   socklen_t* out_len) {
    if (addr == nullptr) {
        return ORBIS_NET_EFAULT;
    }
    if (addrlen < 2) {
        return ORBIS_NET_EINVAL;
    }
    *out = {};
    switch (addr->sa_family) {
    case ORBIS_NET_AF_INET: {
        if (addrlen < sizeof(OrbisNetSockaddrIn)) {
            return ORBIS_NET_EINVAL;
        }
        OrbisNetSockaddrIn in;
        std::memcpy(&in, addr, sizeof(in)); // may be unaligned
        auto* host = reinterpret_cast<sockaddr_in*>(out);
        host->sin_family = AF_INET;
        host->sin_port = in.sin_port;
        std::memcpy(&host->sin_addr, &in.sin_addr, sizeof(in.sin_addr));
        *out_len = sizeof(sockaddr_in);
        return 0;
    }
    case ORBIS_NET_AF_INET6: {
        if (addrlen < sizeof(OrbisNetSockaddrIn6)) {
            return ORBIS_NET_EINVAL;
        }
        OrbisNetSockaddrIn6 in6;
        std::memcpy(&in6, addr, sizeof(in6));
        auto* host = reinterpret_cast<sockaddr_in6*>(out);
        host->sin6_family = AF_INET6;
        host->sin6_port = in6.sin6_port;
        host->sin6_flowinfo = in6.sin6_flowinfo;
        std::memcpy(&host->sin6_addr, in6.sin6_addr, sizeof(in6.sin6_addr));
        host->sin6_scope_id = in6.sin6_scope_id;
        *out_len = sizeof(sockaddr_in6);
        return 0;
    }
    case ORBIS_NET_AF_UNIX: {
        const u32 size = std::min<u32>(addrlen, sizeof(OrbisNetSockaddrUn));
        const char* path = reinterpret_cast<const char*>(addr) + 2;
        const size_t path_len = strnlen(path, size - 2);
        auto* host = reinterpret_cast<sockaddr_un*>(out);
        if (path_len >= sizeof(host->sun_path)) {
            return ORBIS_NET_ENAMETOOLONG;
        }
        host->sun_family = AF_UNIX;
        std::memcpy(host->sun_path, path, path_len);
        host->sun_path[path_len] = '\0';
        *out_len = static_cast<socklen_t>(offsetof(sockaddr_un, sun_path) + path_len + 1);
        return 0;
    }
    default:
        return ORBIS_NET_EAFNOSUPPORT;
    }
}

namespace {

void CopyOut(const void* full, u32 full_len, OrbisNetSockaddr* out, u32* addrlen) {
    if (out != nullptr) {
        std::memcpy(out, full, std::min(*addrlen, full_len));
    }
    *addrlen = full_len;
}

} // namespace

void ToOrbisSockaddr(const sockaddr* addr, socklen_t len, OrbisNetSockaddr* out, u32* addrlen) {
    if (addr->sa_family == AF_INET && len >= static_cast<socklen_t>(sizeof(sockaddr_in))) {
        const auto* host = reinterpret_cast<const sockaddr_in*>(addr);
        OrbisNetSockaddrIn in{};
        in.sin_len = sizeof(in);
        in.sin_family = ORBIS_NET_AF_INET;
        in.sin_port = host->sin_port;
        std::memcpy(&in.sin_addr, &host->sin_addr, sizeof(in.sin_addr));
        CopyOut(&in, sizeof(in), out, addrlen);
    } else if (addr->sa_family == AF_INET6 && len >= static_cast<socklen_t>(sizeof(sockaddr_in6))) {
        const auto* host = reinterpret_cast<const sockaddr_in6*>(addr);
        OrbisNetSockaddrIn6 in6{};
        in6.sin6_len = sizeof(in6);
        in6.sin6_family = ORBIS_NET_AF_INET6;
        in6.sin6_port = host->sin6_port;
        in6.sin6_flowinfo = host->sin6_flowinfo;
        std::memcpy(in6.sin6_addr, &host->sin6_addr, sizeof(in6.sin6_addr));
        in6.sin6_scope_id = host->sin6_scope_id;
        CopyOut(&in6, sizeof(in6), out, addrlen);
    } else if (addr->sa_family == AF_UNIX) {
        // Unnamed sockets get an empty path.
        const auto* host = reinterpret_cast<const sockaddr_un*>(addr);
        OrbisNetSockaddrUn un{};
        un.sun_len = sizeof(un);
        un.sun_family = ORBIS_NET_AF_UNIX;
        const auto offset = static_cast<socklen_t>(offsetof(sockaddr_un, sun_path));
        if (len > offset) {
            const size_t max = std::min<size_t>(len - offset, sizeof(un.sun_path) - 1);
            std::memcpy(un.sun_path, host->sun_path, strnlen(host->sun_path, max));
        }
        CopyOut(&un, sizeof(un), out, addrlen);
    } else {
        *addrlen = 0;
    }
}

int ToP2PAddress(const OrbisNetSockaddr* addr, u32 addrlen, P2PKind kind, P2P::Endpoint* endpoint,
                 u16* port) {
    if (addr == nullptr) {
        return ORBIS_NET_EFAULT;
    }
    if (addrlen != sizeof(OrbisNetSockaddrIn)) {
        return ORBIS_NET_EINVAL;
    }
    if (addr->sa_family != ORBIS_NET_AF_INET) {
        return ORBIS_NET_EAFNOSUPPORT;
    }
    OrbisNetSockaddrIn in;
    std::memcpy(&in, addr, sizeof(in));
    sockaddr_in host{};
    host.sin_family = AF_INET;
    std::memcpy(&host.sin_addr, &in.sin_addr, sizeof(in.sin_addr));
    if (kind == P2PKind::Datagram) {
        host.sin_port = in.sin_port;
        *port = ntohs(in.sin_vport);
    } else {
        host.sin_port = in.sin_vport != 0 ? in.sin_vport : htons(DefaultP2PUdpPort);
        *port = ntohs(in.sin_port);
    }
    *endpoint = P2P::Endpoint::FromSockaddr(reinterpret_cast<const sockaddr*>(&host), sizeof(host));
    return 0;
}

void ToOrbisP2PAddress(const P2P::Endpoint& endpoint, u16 port, P2PKind kind, OrbisNetSockaddr* out,
                       u32* addrlen) {
    OrbisNetSockaddrIn in{};
    in.sin_len = sizeof(in);
    in.sin_family = ORBIS_NET_AF_INET;
    u16 udp_port = 0; // network byte order
    if (endpoint.Family() == AF_INET) {
        const auto* host = reinterpret_cast<const sockaddr_in*>(&endpoint.addr);
        udp_port = host->sin_port;
        std::memcpy(&in.sin_addr, &host->sin_addr, sizeof(in.sin_addr));
    }
    if (kind == P2PKind::Datagram) {
        in.sin_port = udp_port;
        in.sin_vport = htons(port);
    } else {
        in.sin_port = htons(port);
        in.sin_vport = udp_port;
    }
    CopyOut(&in, sizeof(in), out, addrlen);
}

HostMsgFlags ToHostMsgFlags(int orbis_flags) {
    constexpr int kNoSignal = 0x20000; // FreeBSD MSG_NOSIGNAL, always implied
    HostMsgFlags out{};
    int rest = orbis_flags;
    if ((rest & ORBIS_NET_MSG_PEEKLEN) == ORBIS_NET_MSG_PEEKLEN) {
        out.peeklen = true;
        rest &= ~(ORBIS_NET_MSG_PEEKLEN & ~ORBIS_NET_MSG_PEEK);
    }
    if (rest & ORBIS_NET_MSG_PEEK) {
        out.host |= MSG_PEEK;
        out.peek = true;
        rest &= ~ORBIS_NET_MSG_PEEK;
    }
    if (rest & ORBIS_NET_MSG_WAITALL) {
        out.host |= MSG_WAITALL; // host socket is non-blocking, we loop
        out.waitall = true;
        rest &= ~ORBIS_NET_MSG_WAITALL;
    }
    if (rest & ORBIS_NET_MSG_DONTWAIT) {
        out.dontwait = true;
        rest &= ~ORBIS_NET_MSG_DONTWAIT;
    }
    if (rest & ORBIS_NET_MSG_USECRYPTO) {
        out.crypto = true;
        rest &= ~ORBIS_NET_MSG_USECRYPTO;
    }
    if (rest & ORBIS_NET_MSG_USESIGNATURE) {
        out.signature = true;
        rest &= ~ORBIS_NET_MSG_USESIGNATURE;
    }
    rest &= ~kNoSignal;
    out.unsupported = rest; // e.g. MSG_OOB
    return out;
}

std::optional<HostOption> ToHostOption(int level, int name) {
    switch (level) {
    case ORBIS_NET_SOL_SOCKET:
        switch (name) {
        case ORBIS_NET_SO_KEEPALIVE:
            return HostOption{SOL_SOCKET, SO_KEEPALIVE, OptionValue::Int};
        case ORBIS_NET_SO_BROADCAST:
            return HostOption{SOL_SOCKET, SO_BROADCAST, OptionValue::Int};
        case ORBIS_NET_SO_SNDBUF:
            return HostOption{SOL_SOCKET, SO_SNDBUF, OptionValue::Int};
        case ORBIS_NET_SO_RCVBUF:
            return HostOption{SOL_SOCKET, SO_RCVBUF, OptionValue::Int};
        case ORBIS_NET_SO_SNDLOWAT:
            return HostOption{SOL_SOCKET, SO_SNDLOWAT, OptionValue::Int};
        case ORBIS_NET_SO_RCVLOWAT:
            return HostOption{SOL_SOCKET, SO_RCVLOWAT, OptionValue::Int};
        case ORBIS_NET_SO_LINGER:
            return HostOption{SOL_SOCKET, Core::Net::Host::LingerOption, OptionValue::Linger};
        default:
            return std::nullopt;
        }
    case ORBIS_NET_IPPROTO_IP:
        // Values differ per host (IP_TTL is 2 on Linux), map by name.
        switch (name) {
        case ORBIS_NET_IP_HDRINCL:
            return HostOption{IPPROTO_IP, IP_HDRINCL, OptionValue::Int};
        case ORBIS_NET_IP_TOS:
            return HostOption{IPPROTO_IP, IP_TOS, OptionValue::Int};
        case ORBIS_NET_IP_TTL:
            return HostOption{IPPROTO_IP, IP_TTL, OptionValue::Int};
        case ORBIS_NET_IP_MULTICAST_IF:
            return HostOption{IPPROTO_IP, IP_MULTICAST_IF, OptionValue::Raw};
        case ORBIS_NET_IP_MULTICAST_TTL:
            return HostOption{IPPROTO_IP, IP_MULTICAST_TTL, OptionValue::Int};
        case ORBIS_NET_IP_MULTICAST_LOOP:
            return HostOption{IPPROTO_IP, IP_MULTICAST_LOOP, OptionValue::Int};
        case ORBIS_NET_IP_ADD_MEMBERSHIP:
            return HostOption{IPPROTO_IP, IP_ADD_MEMBERSHIP, OptionValue::Raw};
        case ORBIS_NET_IP_DROP_MEMBERSHIP:
            return HostOption{IPPROTO_IP, IP_DROP_MEMBERSHIP, OptionValue::Raw};
        default:
            return std::nullopt;
        }
    case ORBIS_NET_IPPROTO_TCP:
        switch (name) {
        case ORBIS_NET_TCP_NODELAY:
            return HostOption{IPPROTO_TCP, TCP_NODELAY, OptionValue::Int};
#ifdef TCP_MAXSEG
        case ORBIS_NET_TCP_MAXSEG:
            return HostOption{IPPROTO_TCP, TCP_MAXSEG, OptionValue::Int};
#endif
        default:
            return std::nullopt;
        }
    default:
        return std::nullopt;
    }
}

bool IsReservedPort(u16 port) {
    return (port >= 1 && port <= 1023) || port == 5353 || (port >= 8540 && port <= 8579) ||
           (port >= 9293 && port <= 9310) || port >= 40000;
}

bool IsReservedP2PVport(u16 vport) {
    return vport == 5353 || vport >= 32768;
}

s32 DefaultSendBuffer(int host_type) {
    return host_type == SOCK_STREAM ? 32768 : 9216;
}

s32 DefaultReceiveBuffer(int host_type) {
    return host_type == SOCK_STREAM ? 65536 : 40 * 1024;
}

u32 ToHostEpollEvents(u32 orbis_events) {
    u32 out = 0;
    out |= (orbis_events & ORBIS_NET_EPOLLIN) ? Host::EvIn : 0u;
    out |= (orbis_events & ORBIS_NET_EPOLLOUT) ? Host::EvOut : 0u;
    out |= (orbis_events & ORBIS_NET_EPOLLERR) ? Host::EvErr : 0u;
    out |= (orbis_events & ORBIS_NET_EPOLLHUP) ? Host::EvHup : 0u;
    return out;
}

u32 ToOrbisEpollEvents(u32 host_events) {
    u32 out = 0;
    out |= (host_events & Host::EvIn) ? ORBIS_NET_EPOLLIN : 0u;
    out |= (host_events & Host::EvOut) ? ORBIS_NET_EPOLLOUT : 0u;
    out |= (host_events & Host::EvErr) ? ORBIS_NET_EPOLLERR : 0u;
    out |= (host_events & Host::EvHup) ? ORBIS_NET_EPOLLHUP : 0u;
    out |= (host_events & Host::EvDescId) ? ORBIS_NET_EPOLLDESCID : 0u;
    return out;
}

int ToOrbisErrno(Host::Error error) {
    if (error == Host::Error::NotSock) {
        return ORBIS_NET_EBADF;
    }
    return static_cast<int>(error); // already FreeBSD errno values
}

} // namespace Libraries::Net
