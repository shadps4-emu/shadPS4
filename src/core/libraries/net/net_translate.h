// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <optional>

#include "core/libraries/net/net_types.h"
#include "core/net/host_net.h"
#include "core/net/p2p_transport.h"

namespace Libraries::Net {

namespace Host = Core::Net::Host;
namespace P2P = Core::Net::P2P;

std::optional<int> ToHostFamily(int orbis_family);
int ToOrbisFamily(int host_family);

struct HostSocketKind {
    int family;
    int type;
    int protocol;
    bool p2p;
};

std::optional<HostSocketKind> ToHostSocketKind(int family, int type, int protocol,
                                               int* orbis_errno);
int ToOrbisSocketType(int host_type, bool p2p);
int ToHostSockaddr(const OrbisNetSockaddr* addr, u32 addrlen, sockaddr_storage* out,
                   socklen_t* out_len);
void ToOrbisSockaddr(const sockaddr* addr, socklen_t len, OrbisNetSockaddr* out, u32* addrlen);
enum class P2PKind { Datagram, Stream };
inline constexpr u16 DefaultP2PUdpPort = 3658;
int ToP2PAddress(const OrbisNetSockaddr* addr, u32 addrlen, P2PKind kind, P2P::Endpoint* endpoint,
                 u16* port);
void ToOrbisP2PAddress(const P2P::Endpoint& endpoint, u16 port, P2PKind kind, OrbisNetSockaddr* out,
                       u32* addrlen);

struct HostMsgFlags {
    int host = 0;
    bool dontwait = false;
    bool peek = false;
    bool peeklen = false;
    bool waitall = false;
    bool crypto = false;
    bool signature = false;
    int unsupported = 0;
};
HostMsgFlags ToHostMsgFlags(int orbis_flags);

enum class OptionValue {
    Int,
    Linger,
    Raw,
};

struct HostOption {
    int level;
    int name;
    OptionValue value;
};

std::optional<HostOption> ToHostOption(int level, int name);
bool IsReservedPort(u16 port);
bool IsReservedP2PVport(u16 vport);
s32 DefaultSendBuffer(int host_type);    // TCP 32768, UDP and RAW 9216
s32 DefaultReceiveBuffer(int host_type); // TCP 65536, UDP and RAW 40960
u32 ToHostEpollEvents(u32 orbis_events);
u32 ToOrbisEpollEvents(u32 host_events);
int ToOrbisErrno(Host::Error error);

} // namespace Libraries::Net
