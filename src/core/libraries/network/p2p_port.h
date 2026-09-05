// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "common/types.h"
#include "core/libraries/network/sockets.h"

namespace Libraries::Net {

// A P2P socket addresses a peer as (address, port, vport): several vports share one host port, so
// every datagram carries this 6-byte header and the receiver routes the payload by dst_vport.
// Layout and flag values follow RPCS3's, which solves the same problem; no reason to invent
// another encapsulation.
constexpr u32 kP2PHeaderSize = 6;
constexpr u16 kP2PFlagDgram = 1;
constexpr u16 kP2PFlagStream = 2;

// Largest datagram the host stack will carry, and therefore the largest buffer either side needs.
constexpr u32 kP2PMaxDatagram = 65535;

constexpr net_socket kInvalidNetSocket =
#ifdef _WIN32
    INVALID_SOCKET;
#else
    -1;
#endif

// vport 0 carries signaling/control traffic, so it is never handed out to a guest: binding vport 0
// asks for an ephemeral one, allocated from here upwards (as RPCS3 does).
constexpr u16 kP2PSignalingVport = 0;
constexpr u16 kP2PFirstEphemeralVport = 30000;

// Prepended by the receive thread when it forwards a demultiplexed datagram into an endpoint's
// inbox, because the loopback hop would otherwise lose the sender. Host-local: never on the wire.
struct P2PInboxHeader {
    u32 from_addr;  // network byte order
    u16 from_port;  // network byte order
    u16 from_vport; // network byte order
};
static_assert(sizeof(P2PInboxHeader) == 8, "P2PInboxHeader must be tightly packed");

/// One real host UDP socket, shared by every vport bound on the same (address, port) pair.
/// Arriving datagrams are demultiplexed by dst_vport and forwarded into the inbox socket of each
/// matching endpoint, which is what lets a guest P2P socket keep a real, pollable fd of its own.
class P2PPort {
public:
    /// Returns the port object owning the real socket for `port` (network byte order; 0 asks the
    /// OS for an ephemeral port), binding it on first use and sharing it afterwards. On failure
    /// returns nullptr with the host socket error left set for the caller to translate.
    static std::shared_ptr<P2PPort> Acquire(u32 addr, u16 port);

    P2PPort(u32 addr, u16 port, net_socket sock);
    ~P2PPort();

    P2PPort(const P2PPort&) = delete;
    P2PPort& operator=(const P2PPort&) = delete;

    /// Claims `vport` for `owner`; datagrams for it are forwarded to `inbox`. A `vport` of 0 gets
    /// an ephemeral one. Returns the claimed vport (network byte order), or 0 when the vport is
    /// already held by a socket that did not opt into sharing — the caller reports EADDRINUSE.
    u16 Claim(u16 vport, bool reusable, const void* owner, const sockaddr_in& inbox);
    void Release(const void* owner);

    /// Sends one datagram with the header prepended. Returns `len` (the payload the guest asked to
    /// send) on success, or -1 with the host socket error left set.
    int Send(const void* data, u32 len, u16 src_vport, u16 dst_vport, const sockaddr_in& dst,
             u16 flags);

    u32 BoundAddr() const {
        return bound_addr;
    }
    u16 BoundPort() const {
        return bound_port;
    }

private:
    void ReceiveLoop();
    void WakeReceiver();

    struct Endpoint {
        const void* owner;
        u16 vport; // network byte order
        bool reusable;
        sockaddr_in inbox;
    };

    u32 bound_addr{}; // network byte order
    u16 bound_port{}; // network byte order
    net_socket sock;
    std::mutex mutex;
    std::vector<Endpoint> endpoints;
    std::atomic<bool> stop{false};
    std::thread receiver;
};

/// True when `sock` is a usable socket handle.
bool IsValidP2PSocket(net_socket sock);
/// Creates the loopback socket that a guest P2P socket exposes as its fd, and reports the address
/// the port's receive thread must forward to. Returns false with the host error set on failure.
bool CreateP2PInbox(net_socket& sock, sockaddr_in& addr);
void CloseP2PSocket(net_socket sock);
/// Waits for `sock` to become readable. `timeout_us` < 0 waits indefinitely, 0 only polls.
bool WaitP2PSocketReadable(net_socket sock, s64 timeout_us);

} // namespace Libraries::Net
