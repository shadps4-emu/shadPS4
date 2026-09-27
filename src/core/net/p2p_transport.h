// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

// P2P transport: one shared UDP socket carrying DGRAM_P2P datagrams (demultiplexed by vport),
// STREAM_P2P TCP segments (demultiplexed by connection) and signaling.
//
// Threading: a single transport thread blocks on {UDP socket, kick handle} with a timeout equal
// to the earliest TCP timer, so it receives packets and runs retransmission, delayed-ACK,
// persist and TIME-WAIT timers without polling. All state is guarded by one transport mutex;
// guest calls hold it briefly. Readiness is published through per-socket ReadinessFlags, which
// are pollable handles the guest layer waits on and registers in its HostEpoll - so P2P sockets
// wake blocking calls and sceNetEpollWait exactly like native sockets.
//
// Wire framing (P2P header, communication ID, crypto/signature) is delegated to a Codec, so
// the existing, reverse-engineered framing code plugs in unchanged.

#pragma once

#include <atomic>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <thread>

#include "core/net/p2p_tcp.h"

namespace Core::Net::P2P {

/// A UDP address (IPv4 or IPv6), comparable so it can key maps.
struct Endpoint {
    sockaddr_storage addr{};

    static Endpoint FromSockaddr(const sockaddr* sa, socklen_t len);
    static Endpoint IPv4(const char* dotted, u16 port);
    int Family() const {
        return addr.ss_family;
    }
    socklen_t Length() const;
    const sockaddr* Sockaddr() const {
        return reinterpret_cast<const sockaddr*>(&addr);
    }
    u16 Port() const; // host byte order
    bool operator<(const Endpoint& other) const;
    bool operator==(const Endpoint& other) const;
};

struct Protection {
    bool crypto = false;
    bool signature = false;
};

/// Framing hook.
class Codec {
public:
    enum class Kind { Invalid, Stream, Datagram, Signaling };
    struct Decoded {
        Kind kind = Kind::Invalid;
        u16 src_vport = 0; // datagrams only
        u16 dst_vport = 0;
        std::vector<u8> payload; // TCP segment, datagram payload or signaling data
    };

    virtual ~Codec() = default;
    virtual Decoded Decode(std::span<const u8> packet, const Endpoint& from) = 0;
    virtual std::vector<u8> EncodeStream(std::span<const u8> tcp_segment, const Endpoint& to,
                                         Protection protection) = 0;
    virtual std::vector<u8> EncodeDatagram(u16 src_vport, u16 dst_vport,
                                           std::span<const u8> payload, const Endpoint& to,
                                           Protection protection) = 0;
    virtual std::vector<u8> EncodeSignaling(std::span<const u8> data, const Endpoint& to) = 0;
    /// Addresses for the TCP checksum pseudo-header when talking to `peer`.
    virtual PseudoHeader StreamPseudoHeader(const Endpoint& peer) = 0;
};

} // namespace Core::Net::P2P
