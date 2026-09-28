// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <map>

#include "common/logging/log.h"
#include "common/thread.h"
#include "core/libraries/network/p2p_port.h"

namespace Libraries::Net {
namespace {

constexpr s64 kReceivePollTimeoutUs = 100'000;

std::mutex g_ports_mutex;
std::map<u64, std::weak_ptr<P2PPort>> g_ports;

u64 PortKey(u32 addr, u16 port) {
    return (static_cast<u64>(addr) << 16) | static_cast<u64>(port);
}

int LastSocketError() {
#ifdef _WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}

void RestoreSocketError(int error) {
#ifdef _WIN32
    WSASetLastError(error);
#else
    errno = error;
#endif
}

bool SetNonBlocking(net_socket socket_handle) {
#ifdef _WIN32
    u_long mode = 1;
    return ioctlsocket(socket_handle, FIONBIO, &mode) == 0;
#else
    int mode = 1;
    return ioctl(socket_handle, FIONBIO, &mode) == 0;
#endif
}

} // namespace

bool IsValidP2PSocket(net_socket sock) {
    return sock != kInvalidP2PSocket;
}

void CloseP2PSocket(net_socket sock) {
    if (!IsValidP2PSocket(sock)) {
        return;
    }
#ifdef _WIN32
    closesocket(sock);
#else
    ::close(sock);
#endif
}

bool WaitP2PSocketReadable(net_socket sock, s64 timeout_us) {
    if (!IsValidP2PSocket(sock)) {
        return false;
    }

    fd_set read_set;
    FD_ZERO(&read_set);
    FD_SET(sock, &read_set);

    timeval timeout{};
    if (timeout_us >= 0) {
        timeout.tv_sec = static_cast<long>(timeout_us / 1'000'000);
        timeout.tv_usec = static_cast<long>(timeout_us % 1'000'000);
    }

#ifdef _WIN32
    const int result = select(0, &read_set, nullptr, nullptr, timeout_us < 0 ? nullptr : &timeout);
#else
    const int result =
        select(sock + 1, &read_set, nullptr, nullptr, timeout_us < 0 ? nullptr : &timeout);
#endif

    return result > 0 && FD_ISSET(sock, &read_set);
}

bool CreateP2PInbox(net_socket& sock, sockaddr_in& addr) {
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (!IsValidP2PSocket(sock)) {
        return false;
    }

    sockaddr_in local{};
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    local.sin_port = htons(0);

    if (::bind(sock, reinterpret_cast<const sockaddr*>(&local), sizeof(local)) != 0) {
        const int error = LastSocketError();
        CloseP2PSocket(sock);
        sock = kInvalidP2PSocket;
        RestoreSocketError(error);
        return false;
    }

    socklen_t len = sizeof(addr);
    std::memset(&addr, 0, sizeof(addr));
    if (getsockname(sock, reinterpret_cast<sockaddr*>(&addr), &len) != 0) {
        const int error = LastSocketError();
        CloseP2PSocket(sock);
        sock = kInvalidP2PSocket;
        RestoreSocketError(error);
        return false;
    }

    SetNonBlocking(sock);
    return true;
}

std::shared_ptr<P2PPort> P2PPort::Acquire(u32 addr, u16 port) {
    std::scoped_lock lock(g_ports_mutex);

    const u64 key = PortKey(addr, port);
    if (const auto it = g_ports.find(key); it != g_ports.end()) {
        if (auto existing = it->second.lock()) {
            return existing;
        }
        g_ports.erase(it);
    }

    net_socket host_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (!IsValidP2PSocket(host_socket)) {
        return nullptr;
    }

    sockaddr_in local{};
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = addr;
    local.sin_port = port;

    if (::bind(host_socket, reinterpret_cast<const sockaddr*>(&local), sizeof(local)) != 0) {
        CloseP2PSocket(host_socket);
        return nullptr;
    }

    socklen_t len = sizeof(local);
    if (getsockname(host_socket, reinterpret_cast<sockaddr*>(&local), &len) != 0) {
        const int error = LastSocketError();
        CloseP2PSocket(host_socket);
        RestoreSocketError(error);
        return nullptr;
    }

    SetNonBlocking(host_socket);

    auto result = std::make_shared<P2PPort>(local.sin_addr.s_addr, local.sin_port, host_socket);
    g_ports[PortKey(addr, local.sin_port)] = result;

    LOG_INFO(Lib_Net, "P2P: UDP transport bound addr={:#010x} port={}",
             ntohl(local.sin_addr.s_addr), ntohs(local.sin_port));
    return result;
}

P2PPort::P2PPort(u32 addr, u16 port, net_socket socket_handle)
    : bound_addr(addr), bound_port(port), sock(socket_handle) {
    receiver = std::thread([this] { ReceiveLoop(); });
}

P2PPort::~P2PPort() {
    stop.store(true);
    WakeReceiver();
    if (receiver.joinable()) {
        receiver.join();
    }
    CloseP2PSocket(sock);
}

u32 P2PPort::BoundAddr() const {
    return bound_addr;
}

u16 P2PPort::BoundPort() const {
    return bound_port;
}

void P2PPort::WakeReceiver() {
    if (!IsValidP2PSocket(sock) || bound_port == 0) {
        return;
    }

    sockaddr_in self{};
    self.sin_family = AF_INET;
    self.sin_addr.s_addr = bound_addr == htonl(INADDR_ANY) ? htonl(INADDR_LOOPBACK) : bound_addr;
    self.sin_port = bound_port;
    sendto(sock, "", 0, 0, reinterpret_cast<const sockaddr*>(&self), sizeof(self));
}

u16 P2PPort::Claim(u16 vport, bool reusable, const void* owner, const sockaddr_in& inbox) {
    std::scoped_lock lock(mutex);

    if (vport == kP2PSignalingVport) {
        for (u32 candidate = kP2PFirstEphemeralVport; candidate <= 0xFFFF; ++candidate) {
            const u16 candidate_be = htons(static_cast<u16>(candidate));
            const bool used =
                std::any_of(endpoints.begin(), endpoints.end(),
                            [candidate_be](const Endpoint& e) { return e.vport == candidate_be; });
            if (!used) {
                vport = candidate_be;
                break;
            }
        }
        if (vport == kP2PSignalingVport) {
            return 0;
        }
    } else {
        const bool conflict =
            std::any_of(endpoints.begin(), endpoints.end(), [&](const Endpoint& e) {
                return e.vport == vport && !(e.reusable && reusable);
            });
        if (conflict) {
            return 0;
        }
    }

    endpoints.push_back({owner, vport, reusable, inbox});
    return vport;
}

void P2PPort::Release(const void* owner) {
    std::scoped_lock lock(mutex);
    endpoints.erase(
        std::remove_if(endpoints.begin(), endpoints.end(),
                       [owner](const Endpoint& endpoint) { return endpoint.owner == owner; }),
        endpoints.end());
}

int P2PPort::Send(const void* data, u32 len, u16 src_vport, u16 dst_vport,
                  const sockaddr_in& destination, u16 flags) {
    if (len > kP2PMaxDatagram - kP2PHeaderSize) {
#ifdef _WIN32
        WSASetLastError(WSAEMSGSIZE);
#else
        errno = EMSGSIZE;
#endif
        return -1;
    }

    std::vector<u8> packet(kP2PHeaderSize + len);
    const u16 header[3] = {dst_vport, src_vport, htons(flags)};
    std::memcpy(packet.data(), header, sizeof(header));
    if (len != 0) {
        std::memcpy(packet.data() + kP2PHeaderSize, data, len);
    }

    const int result =
        sendto(sock, reinterpret_cast<const char*>(packet.data()), static_cast<int>(packet.size()),
               0, reinterpret_cast<const sockaddr*>(&destination), sizeof(destination));
    if (result < 0) {
        return -1;
    }

    return static_cast<int>(len);
}

void P2PPort::ReceiveLoop() {
    Common::SetCurrentThreadName("shadPS4:P2PPort");

    std::vector<u8> packet(kP2PMaxDatagram);
    std::vector<u8> forward(sizeof(P2PInboxHeader) + kP2PMaxDatagram);
    std::vector<Endpoint> targets;

    while (!stop.load()) {
        if (!WaitP2PSocketReadable(sock, kReceivePollTimeoutUs)) {
            continue;
        }

        sockaddr_in from{};
        socklen_t from_len = sizeof(from);
        const int received =
            recvfrom(sock, reinterpret_cast<char*>(packet.data()), static_cast<int>(packet.size()),
                     0, reinterpret_cast<sockaddr*>(&from), &from_len);
        if (received < static_cast<int>(kP2PHeaderSize)) {
            continue;
        }

        u16 header[3]{};
        std::memcpy(header, packet.data(), sizeof(header));
        const u16 dst_vport = header[0];
        const u16 src_vport = header[1];
        const u16 flags = ntohs(header[2]);
        (void)flags;

        const u32 payload_len = static_cast<u32>(received - kP2PHeaderSize);

        targets.clear();
        {
            std::scoped_lock lock(mutex);
            for (const Endpoint& endpoint : endpoints) {
                if (endpoint.vport == dst_vport) {
                    targets.push_back(endpoint);
                }
            }
        }

        if (targets.empty()) {
            LOG_DEBUG(Lib_Net, "P2P: dropping datagram for vport={}, len={}", ntohs(dst_vport),
                      payload_len);
            continue;
        }

        P2PInboxHeader inbox_header{from.sin_addr.s_addr, from.sin_port, src_vport};
        std::memcpy(forward.data(), &inbox_header, sizeof(inbox_header));
        if (payload_len != 0) {
            std::memcpy(forward.data() + sizeof(inbox_header), packet.data() + kP2PHeaderSize,
                        payload_len);
        }

        const int forward_len = static_cast<int>(sizeof(inbox_header) + payload_len);
        for (const Endpoint& endpoint : targets) {
            sendto(sock, reinterpret_cast<const char*>(forward.data()), forward_len, 0,
                   reinterpret_cast<const sockaddr*>(&endpoint.inbox), sizeof(endpoint.inbox));
        }
    }
}

} // namespace Libraries::Net
