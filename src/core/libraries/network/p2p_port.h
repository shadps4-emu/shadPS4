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

// UDP payload sent between P2P endpoints:
//   u16 destination vport (network byte order)
//   u16 source vport      (network byte order)
//   u16 flags             (network byte order)
constexpr u32 kP2PHeaderSize = 6;
constexpr u16 kP2PFlagDgram = 1;
constexpr u16 kP2PFlagStream = 2;
constexpr u32 kP2PMaxDatagram = 65535;

constexpr net_socket kInvalidP2PSocket =
#ifdef _WIN32
    INVALID_SOCKET;
#else
    -1;
#endif

constexpr u16 kP2PSignalingVport = 0;
constexpr u16 kP2PFirstEphemeralVport = 30000;

// Internal header used only between P2PPort and a socket's loopback inbox.
// It is NOT placed on the network.
struct P2PInboxHeader {
    u32 from_addr;
    u16 from_port;
    u16 from_vport;
};
static_assert(sizeof(P2PInboxHeader) == 8);

class P2PPort {
public:
    static std::shared_ptr<P2PPort> Acquire(u32 addr, u16 port);

    P2PPort(u32 addr, u16 port, net_socket sock);
    ~P2PPort();

    P2PPort(const P2PPort&) = delete;
    P2PPort& operator=(const P2PPort&) = delete;

    u16 Claim(u16 vport, bool reusable, const void* owner, const sockaddr_in& inbox);
    void Release(const void* owner);

    int Send(const void* data, u32 len, u16 src_vport, u16 dst_vport,
             const sockaddr_in& destination, u16 flags);

    u32 BoundAddr() const;
    u16 BoundPort() const;

private:
    struct Endpoint {
        const void* owner{};
        u16 vport{};
        bool reusable{};
        sockaddr_in inbox{};
    };

    void ReceiveLoop();
    void WakeReceiver();

    u32 bound_addr{};
    u16 bound_port{};
    net_socket sock{kInvalidP2PSocket};

    std::mutex mutex;
    std::vector<Endpoint> endpoints;
    std::atomic<bool> stop{false};
    std::thread receiver;
};

bool IsValidP2PSocket(net_socket sock);
bool CreateP2PInbox(net_socket& sock, sockaddr_in& addr);
void CloseP2PSocket(net_socket sock);
bool WaitP2PSocketReadable(net_socket sock, s64 timeout_us);

} // namespace Libraries::Net
