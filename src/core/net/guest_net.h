// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <vector>

#include "core/net/host_net.h"

namespace Core::Net {

namespace P2P {
class Transport;
struct Endpoint;
} // namespace P2P

struct NetResult {
    s64 value; // meaningful only when error == Ok
    Host::Error error;

    static NetResult Ok(s64 value = 0) {
        return {value, Host::Error::Ok};
    }
    static NetResult Fail(Host::Error error) {
        return {-1, error};
    }
};

// --- Net IDs ---------------------------------------------------------------------------------
inline constexpr int MaxSockets = 128;

struct IdAllocator {
    std::function<s32()> allocate;    // a free id (>= 0), or -1 when none is left
    std::function<void(s32)> release; // the id is no longer used
};
// Install before the first object is created. Objects keep the ids they were given.
void SetIdAllocator(IdAllocator allocator);

enum class NetObjectKind { Socket, Epoll, External };

std::optional<NetObjectKind> GetObjectKind(s32 id);
NetResult CloseObject(s32 id);
NetResult ReserveExternalId();
NetResult SignalExternal(s32 id, bool hangup = false);
// Frees the id and removes it from every epoll it is in.
NetResult ReleaseExternalId(s32 id);

// Every socket id, ascending (sceNetGetSockInfo, netstat).
std::vector<s32> ListSockets();
s32 ToOrbisReturn(NetResult result);

// --- Sockets (native and P2P) ----------------------------------------------------------------

NetResult SocketCreate(int family, int type, int protocol);
// socketpair(): two connected sockets, ids stored in ids.
NetResult SocketCreatePair(int family, int type, int protocol, s32 ids[2]);
NetResult SocketBind(s32 id, const sockaddr* addr, socklen_t len);
NetResult SocketListen(s32 id, int backlog); // native and P2P stream
NetResult SocketConnect(s32 id, const sockaddr* addr, socklen_t len);
NetResult SocketAccept(s32 id, sockaddr* addr, socklen_t* len);
// dontwait: guest passed MSG_DONTWAIT (strip it from host_flags,the host is non-blocking).
NetResult SocketSendTo(s32 id, const void* buf, size_t len, int host_flags, bool dontwait,
                       const sockaddr* addr, socklen_t addr_len);
NetResult SocketRecvFrom(s32 id, void* buf, size_t len, int host_flags, bool dontwait,
                         sockaddr* addr, socklen_t* addr_len);
NetResult SocketShutdown(s32 id, int how);                           // native and P2P stream
NetResult SocketGetName(s32 id, sockaddr* addr, socklen_t* len);     // native (getsockname)
NetResult SocketGetPeerName(s32 id, sockaddr* addr, socklen_t* len); // native (getpeername)

// Options without guest-side meaning, passed to the host socket unchanged (native only).
// The caller translates level/name/value into host form first.
NetResult SocketSetHostOption(s32 id, int level, int name, const void* value, socklen_t len);
NetResult SocketGetHostOption(s32 id, int level, int name, void* value, socklen_t* len);

/// Guest-side state of a socket (what getsockopt reports for options this layer owns).
struct SocketInfo {
    int type; // host SOCK_STREAM / SOCK_DGRAM (P2P sockets report their base type)
    bool p2p;
    bool nonblocking;
    bool reuse_addr;
    s64 rcv_timeout_us;
    s64 snd_timeout_us;
    s64 connect_timeout_us; // SO_CONNECTTIMEO
    s64 accept_timeout_us;  // SO_ACCEPTTIMEO
    bool connected;         // connect succeeded or is under way, or the socket was accepted
    bool listening;
    Host::Error pending_error; // SO_ERROR, read and cleared
};
NetResult SocketGetInfo(s32 id, SocketInfo* info);

/// Guest-visible attributes with no host counterpart, kept for getsockopt and debug output.
/// The library gives them meaning. Accepted sockets start with their listener's.
struct SocketAttributes {
    std::array<char, 32> name{}; // debug name (sceNetSocket, SO_NAME), NUL-terminated
    s32 snd_buf = 0;             // SO_SNDBUF as the guest set it; 0 = the PS4 default
    s32 rcv_buf = 0;             // SO_RCVBUF likewise
    s32 policy = 0;              // SO_POLICY: network emulation policy, 0-15
    s32 priority = 16;           // SO_PRIORITY. DISCUSS: default inferred from SDK netstat samples
    // Options the guest may set and read back but that change nothing here, by (level, name).
    std::map<std::pair<s32, s32>, s32> stored_options;
    // STREAM_P2P bound to a non-zero TCP port: may listen, but connecting is EPROTO.
    bool p2p_tcp_port_bound = false;
    // IP_ADD_MEMBERSHIP requests (struct ip_mreq) made before bind that the host refused
    // (Windows wants the socket bound first,BSD does not): joined right after bind.
    std::vector<std::array<u8, 8>> pending_memberships;
};
NetResult SocketGetAttributes(s32 id, SocketAttributes* out);
/// Runs update on the attributes under the socket's lock.
NetResult SocketUpdateAttributes(s32 id, const std::function<void(SocketAttributes&)>& update);

// Socket options the layer owns instead of passing to the host.
NetResult SocketSetNonBlocking(s32 id, bool enable);                     // SO_NBIO / FIONBIO
NetResult SocketSetReuseAddr(s32 id, bool enable);                       // applied at bind
NetResult SocketSetReusePort(s32 id, bool enable);                       // likewise
NetResult SocketSetRecvTimeout(s32 id, std::chrono::microseconds value); // SO_RCVTIMEO
NetResult SocketSetSendTimeout(s32 id, std::chrono::microseconds value); // SO_SNDTIMEO
// Blocking connect and accept give up with EWOULDBLOCK after these,0 waits forever.
NetResult SocketSetConnectTimeout(s32 id, std::chrono::microseconds value); // SO_CONNECTTIMEO
NetResult SocketSetAcceptTimeout(s32 id, std::chrono::microseconds value);  // SO_ACCEPTTIMEO

// Abort (sceNetSocketAbort): wakes every call blocked on the socket at that moment, which then
// fails with EINTR. The socket stays usable. When nothing on a side is blocked, the matching
// preservation flag makes the next blocking call on that side fail instead, once. Receive side:
// recv*, accept. Send side: send*, connect. An epoll being waited on with the socket in it
// reports it once as Host::EvHup (SCE_NET_EPOLLHUP), which also counts as something aborted.
inline constexpr u32 kSocketAbortPreserveRecv = 0x1; // SCE_NET_SOCKET_ABORT_FLAG_RCV_PRESERVATION
inline constexpr u32 kSocketAbortPreserveSend = 0x2; // SCE_NET_SOCKET_ABORT_FLAG_SND_PRESERVATION
// SND_PRESERVATION, and also EINTR once for the send after one that an abort cut short with a
// partial count.
inline constexpr u32 kSocketAbortPreserveSendAgain = 0x4; // ..._SND_PRESERVATION_AGAIN
NetResult SocketAbort(s32 id, u32 flags);
NetResult SocketClose(s32 id);

// --- P2P sockets -----------------------------------------------------------------------------
// Addresses are split into the peer's UDP endpoint and its vport. For streams, the vports are
// the TCP ports inside the encapsulation.

// Installs the shared transport P2P sockets of family use (from the net module's init).
void SetP2PTransport(int family, std::shared_ptr<P2P::Transport> transport);

NetResult P2PSocketCreate(int family, bool stream);
NetResult P2PSocketSetProtection(s32 id, bool crypto, bool signature); // SO_USECRYPTO etc.
NetResult P2PSocketGetProtection(s32 id, bool* crypto, bool* signature);
NetResult P2PSocketBind(s32 id, u16 vport); // 0 picks a free vport
// Stream: TCP handshake (blocking unless non-blocking). Datagram: default peer + filter.
NetResult P2PSocketConnect(s32 id, const P2P::Endpoint& peer, u16 peer_vport);
NetResult P2PSocketAccept(s32 id, P2P::Endpoint* peer, u16* peer_vport);
// Per-message protection added to the socket's own (MSG_USECRYPTO / MSG_USESIGNATURE).
// Datagrams only: a stream's protection is fixed per connection.
struct P2PSendProtection {
    bool crypto = false;
    bool signature = false;
};

// peer == nullptr sends to the connected peer.
NetResult P2PSocketSendTo(s32 id, const void* buf, size_t len, bool dontwait,
                          const P2P::Endpoint* peer, u16 peer_vport,
                          P2PSendProtection protection = {});
// waitall: MSG_WAITALL, for streams (see SocketRecvFrom).
NetResult P2PSocketRecvFrom(s32 id, void* buf, size_t len, bool peek, bool dontwait,
                            P2P::Endpoint* from, u16* from_vport, bool waitall = false);
// Local UDP port of the transport and the bound vport (0 when unbound).
NetResult P2PSocketGetName(s32 id, u16* udp_port, u16* vport);
NetResult P2PSocketGetPeerName(s32 id, P2P::Endpoint* peer, u16* peer_vport);

// --- Epoll -----------------------------------------------------------------------------------

enum class EpollOp { Add, Modify, Delete };

struct GuestEpollEvent {
    u32 events; // Host::EventBits; the export translates them to ORBIS_NET_EPOLL*
    s32 ident;  // socket id
    u64 data;   // guest's user data from EpollControl
};

NetResult EpollCreate();
// events: Host::EventBits, flags: Host::EpollFlags.
NetResult EpollControl(s32 eid, EpollOp op, s32 socket_id, u32 events, u32 flags, u64 data);
// timeout_us < 0 waits forever, 0 polls. Returns the number of events written.
NetResult EpollWait(s32 eid, std::span<GuestEpollEvent> out, s64 timeout_us);
// Same model as SocketAbort, for sceNetEpollWait.
inline constexpr u32 kEpollAbortPreserve = 0x1; // ORBIS_NET_EPOLL_ABORT_FLAG_PRESERVATION
NetResult EpollAbort(s32 eid, u32 flags);
NetResult EpollDestroy(s32 eid);

// --- select() --------------------------------------------------------------------------------

// One socket in a select() call. `interest` and `ready` are Host::EvIn / Host::EvOut.
struct SelectEntry {
    s32 id;
    u32 interest;
    u32 ready; // out
};
// Waits until at least one listed socket is ready as asked, or `timeout_us` passes (negative:
// no limit, 0: just look), then fills in every entry's `ready`. Works for native and P2P
// sockets alike. Returns the number of ready (socket, direction) pairs, as select() counts
// them; EBADF when an id is not a socket. A socket may be listed more than once.
NetResult SocketSelect(std::span<SelectEntry> entries, s64 timeout_us);

} // namespace Core::Net
