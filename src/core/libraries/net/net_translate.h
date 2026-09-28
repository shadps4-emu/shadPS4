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

// --- Families and socket types ---------------------------------------------------------------

std::optional<int> ToHostFamily(int orbis_family);
int ToOrbisFamily(int host_family);

struct HostSocketKind {
    int family;   // host AF_*
    int type;     // host SOCK_STREAM / SOCK_DGRAM / SOCK_RAW
    int protocol; // host IPPROTO_*
    bool p2p;
};

// Fails with the guest errno to report: EPROTONOSUPPORT for a family other than AF_INET and
// AF_UNIX or an unknown type (and for AF_UNIX anything but stream and datagram), EPROTOTYPE
// for a protocol that does not match a non-RAW type.
std::optional<HostSocketKind> ToHostSocketKind(int family, int type, int protocol,
                                               int* orbis_errno);
// Guest SO_TYPE value.
int ToOrbisSocketType(int host_type, bool p2p);

// --- Addresses -------------------------------------------------------------------------------

// Guest sockaddr to host. sa_len is not trusted (games often leave it 0); the family and the
// buffer length decide. Returns the guest errno on failure (0 on success).
int ToHostSockaddr(const OrbisNetSockaddr* addr, u32 addrlen, sockaddr_storage* out,
                   socklen_t* out_len);

// Host sockaddr to guest, with sa_len filled in. Copies at most *addrlen bytes (truncating,
// as BSD does) and stores the full length in *addrlen.
void ToOrbisSockaddr(const sockaddr* addr, socklen_t len, OrbisNetSockaddr* out, u32* addrlen);

// How the fields of a P2P sockaddr are used:
//   DGRAM_P2P:  sin_port = the peer's UDP port,  sin_vport = the virtual port.
//   STREAM_P2P: sin_port = the TCP port inside the encapsulation,sin_vport = the peer's UDP port.
enum class P2PKind { Datagram, Stream };

/// The UDP port a stream connect uses when sin_vport is 0
inline constexpr u16 DefaultP2PUdpPort = 3658;

// port is the virtual port (datagrams) or the TCP port (streams), in host byte order.
// Another length is EINVAL, another family EAFNOSUPPORT.
int ToP2PAddress(const OrbisNetSockaddr* addr, u32 addrlen, P2PKind kind, P2P::Endpoint* endpoint,
                 u16* port);
void ToOrbisP2PAddress(const P2P::Endpoint& endpoint, u16 port, P2PKind kind, OrbisNetSockaddr* out,
                       u32* addrlen);

// --- Message flags ---------------------------------------------------------------------------

struct HostMsgFlags {
    int host = 0;          // flags for the host call
    bool dontwait = false; // handled by the guest layer, never passed to the host
    bool peek = false;
    bool peeklen = false;   // MSG_PEEKLEN: report the size waiting (implies peek)
    bool waitall = false;   // MSG_WAITALL (the guest layer loops; also in host)
    bool crypto = false;    // MSG_USECRYPTO: P2P datagrams, this message only
    bool signature = false; // MSG_USESIGNATURE likewise
    int unsupported = 0;    // guest bits with no translation (reported, then ignored)
};
HostMsgFlags ToHostMsgFlags(int orbis_flags);

// --- Socket options --------------------------------------------------------------------------

enum class OptionValue {
    Int,    // 4-byte integer on both sides
    Linger, // OrbisNetLinger <-> host struct linger (fields differ in size on Windows)
    Raw,    // same layout on all platforms (ip_mreq, in_addr): copied unchanged
};

struct HostOption {
    int level;
    int name;
    OptionValue value;
};

// Options passed through to the host socket. Options the guest layer owns (SO_NBIO,
// timeouts, SO_REUSEADDR, SO_TYPE, SO_ERROR, P2P protection) are handled by the caller.
std::optional<HostOption> ToHostOption(int level, int name);

// --- PS4 limits ------------------------------------------------------------------------------

// Local ports the system reserves binding one explicitly fails. For UDP, TCP and TCP over UDPP2P
// ports: 1-1023, 5353, 8540-8579, 9293-9310 and 40000-65535 (only port 0 reaches the ephemeral
// range 49152-65535).
bool IsReservedPort(u16 port);
// The same for UDPP2P virtual ports: 5353 and 32768-65535.
bool IsReservedP2PVport(u16 vport);

// Default socket buffer sizes . A datagram larger than the send buffer
// fails with EMSGSIZE, as on FreeBSD.
s32 DefaultSendBuffer(int host_type);    // TCP 32768, UDP and RAW 9216
s32 DefaultReceiveBuffer(int host_type); // TCP 65536, UDP and RAW 40960

// --- Epoll and errors ------------------------------------------------------------------------

u32 ToHostEpollEvents(u32 orbis_events);
u32 ToOrbisEpollEvents(u32 host_events);

// Guest errno for a host-layer error
int ToOrbisErrno(Host::Error error);

} // namespace Libraries::Net
