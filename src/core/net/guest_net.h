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
    s64 value; // only valid when error == Ok
    Host::Error error;

    static NetResult Ok(s64 value = 0) {
        return {value, Host::Error::Ok};
    }
    static NetResult Fail(Host::Error error) {
        return {-1, error};
    }
};

inline constexpr int MaxSockets = 128;

struct IdAllocator {
    std::function<s32()> allocate; // -1 when out of ids
    std::function<void(s32)> release;
};
// Must be set before any object is created.
void SetIdAllocator(IdAllocator allocator);

enum class NetObjectKind { Socket, Epoll, External };

std::optional<NetObjectKind> GetObjectKind(s32 id);
NetResult CloseObject(s32 id);
NetResult ReserveExternalId();
NetResult SignalExternal(s32 id, bool hangup = false);
// Also removes the id from any epoll it's in.
NetResult ReleaseExternalId(s32 id);

// Sorted ascending.
std::vector<s32> ListSockets();
s32 ToOrbisReturn(NetResult result);

NetResult SocketCreate(int family, int type, int protocol);
NetResult SocketCreatePair(int family, int type, int protocol, s32 ids[2]);
NetResult SocketBind(s32 id, const sockaddr* addr, socklen_t len);
NetResult SocketListen(s32 id, int backlog);
NetResult SocketConnect(s32 id, const sockaddr* addr, socklen_t len);
NetResult SocketAccept(s32 id, sockaddr* addr, socklen_t* len);
// dontwait is the guest's MSG_DONTWAIT. Keep it out of host_flags, host sockets never block.
NetResult SocketSendTo(s32 id, const void* buf, size_t len, int host_flags, bool dontwait,
                       const sockaddr* addr, socklen_t addr_len);
NetResult SocketRecvFrom(s32 id, void* buf, size_t len, int host_flags, bool dontwait,
                         sockaddr* addr, socklen_t* addr_len);
NetResult SocketShutdown(s32 id, int how);
NetResult SocketGetName(s32 id, sockaddr* addr, socklen_t* len);
NetResult SocketGetPeerName(s32 id, sockaddr* addr, socklen_t* len);

// Native only. Passed straight to the host, so the caller must convert to host values.
NetResult SocketSetHostOption(s32 id, int level, int name, const void* value, socklen_t len);
NetResult SocketGetHostOption(s32 id, int level, int name, void* value, socklen_t* len);

struct SocketInfo {
    int type; // P2P sockets report their base type
    bool p2p;
    bool nonblocking;
    bool reuse_addr;
    s64 rcv_timeout_us;
    s64 snd_timeout_us;
    s64 connect_timeout_us;
    s64 accept_timeout_us;
    bool connected; // also true while connecting and for accepted sockets
    bool listening;
    Host::Error pending_error; // SO_ERROR, cleared on read
};
NetResult SocketGetInfo(s32 id, SocketInfo* info);

// Guest-only socket state for getsockopt. Accepted sockets copy their listener's.
struct SocketAttributes {
    std::array<char, 32> name{}; // SO_NAME
    s32 snd_buf = 0;             // 0 = PS4 default
    s32 rcv_buf = 0;
    s32 policy = 0;    // 0-15
    s32 priority = 16; // TODO: find real value
    // Stored for getsockopt only, keyed by (level, name).
    std::map<std::pair<s32, s32>, s32> stored_options;
    // A STREAM_P2P socket bound to a non-zero TCP port can listen, but connect gives EPROTO.
    bool p2p_tcp_port_bound = false;
    // ip_mreq joins issued before bind. Windows refuses them on an unbound socket, so they
    // are retried after bind.
    std::vector<std::array<u8, 8>> pending_memberships;
};
NetResult SocketGetAttributes(s32 id, SocketAttributes* out);
// Runs update under the socket lock.
NetResult SocketUpdateAttributes(s32 id, const std::function<void(SocketAttributes&)>& update);

// Options handled here instead of on the host socket. Reuse flags take effect at bind.
NetResult SocketSetNonBlocking(s32 id, bool enable);
// FIONREAD. Native sockets only.
NetResult SocketBytesReadable(s32 id);
NetResult SocketSetReuseAddr(s32 id, bool enable);
NetResult SocketSetReusePort(s32 id, bool enable);
NetResult SocketSetRecvTimeout(s32 id, std::chrono::microseconds value);
NetResult SocketSetSendTimeout(s32 id, std::chrono::microseconds value);
// Blocking connect/accept fail with EWOULDBLOCK after this. 0 waits forever.
NetResult SocketSetConnectTimeout(s32 id, std::chrono::microseconds value);
NetResult SocketSetAcceptTimeout(s32 id, std::chrono::microseconds value);

// Calls blocked on the socket fail with EINTR, the socket stays usable. If nothing is blocked
// on a side, its preserve flag makes the next blocking call there fail once instead.
// Recv side is recv*/accept, send side is send*/connect. An epoll waiting on the socket
// reports EvHup for it once, which also counts as an abort.
inline constexpr u32 kSocketAbortPreserveRecv = 0x1;
inline constexpr u32 kSocketAbortPreserveSend = 0x2;
inline constexpr u32 kSocketAbortPreserveSendAgain = 0x4;
NetResult SocketAbort(s32 id, u32 flags);
NetResult SocketClose(s32 id);
void SetP2PTransport(int family, std::shared_ptr<P2P::Transport> transport);
NetResult P2PSocketCreate(int family, bool stream);
NetResult P2PSocketSetProtection(s32 id, bool crypto, bool signature);
NetResult P2PSocketGetProtection(s32 id, bool* crypto, bool* signature);
NetResult P2PSocketBind(s32 id, u16 vport); // 0 = any free vport
NetResult P2PSocketConnect(s32 id, const P2P::Endpoint& peer, u16 peer_vport);
NetResult P2PSocketAccept(s32 id, P2P::Endpoint* peer, u16* peer_vport);
// MSG_USECRYPTO / MSG_USESIGNATURE, on top of the socket's own. Datagrams only, stream
// protection is fixed per connection.
struct P2PSendProtection {
    bool crypto = false;
    bool signature = false;
};
NetResult P2PSocketSendTo(s32 id, const void* buf, size_t len, bool dontwait,
                          const P2P::Endpoint* peer, u16 peer_vport,
                          P2PSendProtection protection = {});
NetResult P2PSocketRecvFrom(s32 id, void* buf, size_t len, bool peek, bool dontwait,
                            P2P::Endpoint* from, u16* from_vport, bool waitall = false);
// Transport's local UDP port and the bound vport (0 if unbound).
NetResult P2PSocketGetName(s32 id, u16* udp_port, u16* vport);
NetResult P2PSocketGetPeerName(s32 id, P2P::Endpoint* peer, u16* peer_vport);

enum class EpollOp { Add, Modify, Delete };

struct GuestEpollEvent {
    u32 events; // Host::EventBits
    s32 ident;  // socket id
    u64 data;
};

NetResult EpollCreate();
NetResult EpollControl(s32 eid, EpollOp op, s32 socket_id, u32 events, u32 flags, u64 data);
NetResult EpollWait(s32 eid, std::span<GuestEpollEvent> out, s64 timeout_us);
inline constexpr u32 kEpollAbortPreserve = 0x1;
NetResult EpollAbort(s32 eid, u32 flags);
NetResult EpollDestroy(s32 eid);

// interest and ready are Host::EvIn / Host::EvOut.
struct SelectEntry {
    s32 id;
    u32 interest;
    u32 ready;
};
NetResult SocketSelect(std::span<SelectEntry> entries, s64 timeout_us);

} // namespace Core::Net
