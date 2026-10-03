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

// On failure sets orbis_errno: EPROTONOSUPPORT for an unsupported family/type, EPROTOTYPE when
// the protocol doesn't match a non-RAW type.
std::optional<HostSocketKind> ToHostSocketKind(int family, int type, int protocol,
                                               int* orbis_errno);
// For SO_TYPE.
int ToOrbisSocketType(int host_type, bool p2p);

// Ignores sa_len, games often leave it 0. Returns a guest errno, 0 on success.
int ToHostSockaddr(const OrbisNetSockaddr* addr, u32 addrlen, sockaddr_storage* out,
                   socklen_t* out_len);

// Truncates to *addrlen like BSD, then stores the full length in *addrlen.
void ToOrbisSockaddr(const sockaddr* addr, socklen_t len, OrbisNetSockaddr* out, u32* addrlen);

// DGRAM_P2P: sin_port is the peer's UDP port, sin_vport the virtual port.
// STREAM_P2P: sin_port is the encapsulated TCP port, sin_vport the peer's UDP port.
enum class P2PKind { Datagram, Stream };

// Used by stream connect when sin_vport is 0.
inline constexpr u16 DefaultP2PUdpPort = 3658;

// port is host order: vport for datagrams, TCP port for streams.
int ToP2PAddress(const OrbisNetSockaddr* addr, u32 addrlen, P2PKind kind, P2P::Endpoint* endpoint,
                 u16* port);
void ToOrbisP2PAddress(const P2P::Endpoint& endpoint, u16 port, P2PKind kind, OrbisNetSockaddr* out,
                       u32* addrlen);

struct HostMsgFlags {
    int host = 0;
    bool dontwait = false; // never passed to the host
    bool peek = false;
    bool peeklen = false; // implies peek
    bool waitall = false; // also set in host
    bool crypto = false;  // P2P datagrams only
    bool signature = false;
    int unsupported = 0; // logged and ignored
};
HostMsgFlags ToHostMsgFlags(int orbis_flags);

enum class OptionValue {
    Int,
    Linger, // field sizes differ on Windows
    Raw,    // same layout everywhere, copied as is
};

struct HostOption {
    int level;
    int name;
    OptionValue value;
};

// Pass-through options only. SO_NBIO, timeouts, SO_REUSEADDR etc. are handled by the caller.
std::optional<HostOption> ToHostOption(int level, int name);

// PS4 refuses explicit binds to 1-1023, 5353, 8540-8579, 9293-9310 and 40000-65535.
// The ephemeral range 49152-65535 is only reachable through port 0.
bool IsReservedPort(u16 port);
// P2P vports: 5353 and 32768-65535.
bool IsReservedP2PVport(u16 vport);

// Datagrams larger than the send buffer fail with EMSGSIZE, as on FreeBSD.
s32 DefaultSendBuffer(int host_type);    // TCP 32768, UDP and RAW 9216
s32 DefaultReceiveBuffer(int host_type); // TCP 65536, UDP and RAW 40960

u32 ToHostEpollEvents(u32 orbis_events);
u32 ToOrbisEpollEvents(u32 host_events);

int ToOrbisErrno(Host::Error error);

} // namespace Libraries::Net
