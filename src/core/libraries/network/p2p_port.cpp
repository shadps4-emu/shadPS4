// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <climits>
#include <cstring>
#include <map>

#ifndef _WIN32
#include <poll.h>
#endif

#include "common/logging/log.h"
#include "common/thread.h"
#include "core/emulator_settings.h"
#include "core/libraries/network/p2p_port.h"

namespace Libraries::Net {

// The receive thread must notice a stopping port even with no traffic at all; it is woken
// explicitly, so this only bounds the worst case.
constexpr s64 kReceivePollTimeoutUs = 100'000;

bool IsValidP2PSocket(net_socket sock) {
    return sock != kInvalidNetSocket;
}

// A failure has to survive the cleanup that follows it: closing the socket and logging both run
// through the OS and would otherwise overwrite the error the caller is about to translate.
static int LastSocketError() {
#ifdef _WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}

constexpr int kSocketErrorMsgSize =
#ifdef _WIN32
    WSAEMSGSIZE;
#else
    EMSGSIZE;
#endif

static void RestoreSocketError(int error) {
#ifdef _WIN32
    WSASetLastError(error);
#else
    errno = error;
#endif
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

static bool SetNonBlocking(net_socket sock) {
#ifdef _WIN32
    u_long mode = 1;
    return ioctlsocket(sock, FIONBIO, &mode) == 0;
#else
    int mode = 1;
    return ioctl(sock, FIONBIO, &mode) == 0;
#endif
}

bool WaitP2PSocketReadable(net_socket sock, s64 timeout_us) {
    if (!IsValidP2PSocket(sock)) {
        return false;
    }
    // poll() rather than select(): on POSIX FD_SETSIZE bounds the descriptor *value*, so an
    // FD_SET of a descriptor >= 1024 writes past the stack fd_set. This runs on every P2P
    // receive and on every port thread's poll, so an emulator with many open files would
    // eventually corrupt its stack here. poll has no such limit on any supported platform.
    pollfd pfd{};
    pfd.fd = sock;
    pfd.events = POLLIN;
    const int timeout_ms =
        timeout_us < 0 ? -1 : static_cast<int>(std::min<s64>(timeout_us / 1000, INT_MAX));
#ifdef _WIN32
    const int res = WSAPoll(&pfd, 1, timeout_ms);
#else
    const int res = ::poll(&pfd, 1, timeout_ms);
#endif
    return res > 0 && (pfd.revents & (POLLIN | POLLHUP | POLLERR)) != 0;
}

bool CreateP2PInbox(net_socket& sock, sockaddr_in& addr) {
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (!IsValidP2PSocket(sock)) {
        return false;
    }
    // The inbox only ever receives forwards from the port's receive thread, which runs in this
    // process, so loopback with an OS-chosen port is all it needs.
    sockaddr_in bind_addr{};
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    bind_addr.sin_port = 0;
    if (::bind(sock, reinterpret_cast<const sockaddr*>(&bind_addr), sizeof(bind_addr)) != 0) {
        const int error = LastSocketError();
        CloseP2PSocket(sock);
        sock = kInvalidNetSocket;
        RestoreSocketError(error);
        return false;
    }
    socklen_t len = sizeof(addr);
    std::memset(&addr, 0, sizeof(addr));
    if (getsockname(sock, reinterpret_cast<sockaddr*>(&addr), &len) != 0) {
        const int error = LastSocketError();
        CloseP2PSocket(sock);
        sock = kInvalidNetSocket;
        RestoreSocketError(error);
        return false;
    }
    // A guest recv must never block on the OS call itself: blocking is decided by SO_NBIO and
    // MSG_DONTWAIT and implemented by waiting for readability first.
    SetNonBlocking(sock);
    return true;
}

// Ports are shared: two guest sockets bound to different vports of the same host port must end up
// on the same OS socket, or the second bind would fail with EADDRINUSE against ourselves.
static std::mutex g_ports_mutex;
static std::map<u64, std::weak_ptr<P2PPort>> g_ports;

static u64 PortKey(u32 addr, u16 port) {
    return (static_cast<u64>(addr) << 16) | port;
}

std::shared_ptr<P2PPort> P2PPort::Acquire(u32 addr, u16 port) {
    // With networking turned off the socket still has to exist — titles probe their own signaling
    // port through the loopback and stall without the reply — but nothing should leave the host,
    // so it binds the loopback only. That also keeps Windows from raising a firewall prompt.
    const bool online = EmulatorSettings.IsConnectedToNetwork();
    const u32 bind_addr = online ? addr : htonl(INADDR_LOOPBACK);

    std::scoped_lock lock{g_ports_mutex};
    if (port != 0) {
        if (const auto it = g_ports.find(PortKey(bind_addr, port)); it != g_ports.end()) {
            if (auto existing = it->second.lock()) {
                return existing;
            }
        }
    }

    net_socket sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (!IsValidP2PSocket(sock)) {
        LOG_ERROR(Lib_Net, "P2P: cannot create the host socket for port {}", ntohs(port));
        return nullptr;
    }
    sockaddr_in local{};
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = bind_addr;
    local.sin_port = port;
    // No SO_REUSEADDR on purpose: a second emulator instance must fail loudly here instead of
    // silently stealing the first one's datagrams.
    if (::bind(sock, reinterpret_cast<const sockaddr*>(&local), sizeof(local)) != 0) {
        const int error = LastSocketError();
        LOG_ERROR(Lib_Net, "P2P: cannot bind host port {} (already in use by another process?)",
                  ntohs(port));
        CloseP2PSocket(sock);
        RestoreSocketError(error);
        return nullptr;
    }
    socklen_t len = sizeof(local);
    if (getsockname(sock, reinterpret_cast<sockaddr*>(&local), &len) != 0) {
        const int error = LastSocketError();
        CloseP2PSocket(sock);
        RestoreSocketError(error);
        return nullptr;
    }
    SetNonBlocking(sock);

    auto p2p_port = std::make_shared<P2PPort>(local.sin_addr.s_addr, local.sin_port, sock);
    g_ports[PortKey(bind_addr, local.sin_port)] = p2p_port;
    LOG_INFO(Lib_Net, "P2P: host socket bound, addr = {:#010x}, port = {}, online = {}",
             ntohl(local.sin_addr.s_addr), ntohs(local.sin_port), online);
    return p2p_port;
}

P2PPort::P2PPort(u32 addr, u16 port, net_socket sock)
    : bound_addr(addr), bound_port(port), sock(sock) {
    receiver = std::thread([this] { ReceiveLoop(); });
}

P2PPort::~P2PPort() {
    stop.store(true);
    WakeReceiver();
    if (receiver.joinable()) {
        receiver.join();
    }
    CloseP2PSocket(sock);
    LOG_INFO(Lib_Net, "P2P: host socket closed, port = {}", ntohs(bound_port));
}

void P2PPort::WakeReceiver() {
    // An empty datagram is shorter than the header, so the loop drops it after waking up.
    sockaddr_in self{};
    self.sin_family = AF_INET;
    self.sin_addr.s_addr = bound_addr == htonl(INADDR_ANY) ? htonl(INADDR_LOOPBACK) : bound_addr;
    self.sin_port = bound_port;
    sendto(sock, "", 0, 0, reinterpret_cast<const sockaddr*>(&self), sizeof(self));
}

u16 P2PPort::Claim(u16 vport, bool reusable, const void* owner, const sockaddr_in& inbox) {
    std::scoped_lock lock{mutex};
    if (vport == kP2PSignalingVport) {
        // Binding vport 0 asks for an ephemeral one; vport 0 itself stays reserved for signaling.
        u16 candidate = kP2PFirstEphemeralVport;
        while (candidate != 0) {
            const u16 be = htons(candidate);
            if (std::none_of(endpoints.begin(), endpoints.end(),
                             [be](const Endpoint& e) { return e.vport == be; })) {
                vport = be;
                break;
            }
            ++candidate;
        }
        if (vport == kP2PSignalingVport) {
            return 0;
        }
    } else {
        // A vport can hold several sockets, but only when they all opt in, like a real stack.
        const bool taken = std::any_of(endpoints.begin(), endpoints.end(), [&](const Endpoint& e) {
            return e.vport == vport && !(e.reusable && reusable);
        });
        if (taken) {
            return 0;
        }
    }
    endpoints.push_back({owner, vport, reusable, inbox});
    return vport;
}

void P2PPort::Release(const void* owner) {
    std::scoped_lock lock{mutex};
    std::erase_if(endpoints, [owner](const Endpoint& e) { return e.owner == owner; });
}

int P2PPort::Send(const void* data, u32 len, u16 src_vport, u16 dst_vport, const sockaddr_in& dst,
                  u16 flags) {
    if (len > kP2PMaxDatagram - kP2PHeaderSize) {
        // Every other -1 out of here leaves the host error set for the caller to translate; this
        // one has to set it too, or the guest gets whatever error happened to be left over.
        RestoreSocketError(kSocketErrorMsgSize);
        return -1;
    }
    // One contiguous buffer per sending thread: the copy costs nothing next to the syscall, and a
    // scatter-gather send would need a different call on every platform.
    thread_local std::vector<u8> buffer;
    buffer.resize(kP2PHeaderSize + len);
    const u16 header[3] = {dst_vport, src_vport, htons(flags)};
    std::memcpy(buffer.data(), header, kP2PHeaderSize);
    if (len != 0) {
        std::memcpy(buffer.data() + kP2PHeaderSize, data, len);
    }
    const int sent =
        sendto(sock, reinterpret_cast<const char*>(buffer.data()), static_cast<int>(buffer.size()),
               0, reinterpret_cast<const sockaddr*>(&dst), sizeof(dst));
    if (sent < 0) {
        return -1;
    }
    // The guest counts payload bytes, not what encapsulation added.
    return static_cast<int>(len);
}

void P2PPort::ReceiveLoop() {
    Common::SetCurrentThreadName("shadPS4:P2PPort");
    std::vector<u8> datagram(kP2PMaxDatagram);
    std::vector<u8> forward(sizeof(P2PInboxHeader) + kP2PMaxDatagram);
    std::vector<Endpoint> targets;

    while (!stop.load()) {
        if (!WaitP2PSocketReadable(sock, kReceivePollTimeoutUs)) {
            continue;
        }
        sockaddr_in from{};
        socklen_t from_len = sizeof(from);
        const int received = recvfrom(sock, reinterpret_cast<char*>(datagram.data()),
                                      static_cast<int>(datagram.size()), 0,
                                      reinterpret_cast<sockaddr*>(&from), &from_len);
        if (received < static_cast<int>(kP2PHeaderSize)) {
            // Our own wake-up, or a stray datagram from something that is not a P2P peer.
            continue;
        }
        u16 header[3];
        std::memcpy(header, datagram.data(), kP2PHeaderSize);
        const u16 dst_vport = header[0];
        const u16 src_vport = header[1];
        const u32 payload_len = static_cast<u32>(received) - kP2PHeaderSize;

        targets.clear();
        {
            std::scoped_lock lock{mutex};
            for (const auto& endpoint : endpoints) {
                if (endpoint.vport == dst_vport) {
                    targets.push_back(endpoint);
                }
            }
        }
        if (targets.empty()) {
            LOG_DEBUG(Lib_Net, "P2P: datagram for unbound vport {} dropped, len = {}",
                      ntohs(dst_vport), payload_len);
            continue;
        }

        const P2PInboxHeader inbox_header{from.sin_addr.s_addr, from.sin_port, src_vport};
        std::memcpy(forward.data(), &inbox_header, sizeof(inbox_header));
        if (payload_len != 0) {
            std::memcpy(forward.data() + sizeof(inbox_header), datagram.data() + kP2PHeaderSize,
                        payload_len);
        }
        const int forward_len = static_cast<int>(sizeof(inbox_header) + payload_len);
        for (const auto& target : targets) {
            // Delivery is a loopback hop into the endpoint's own socket, which is what makes the
            // guest fd readable; if the inbox is full the datagram is dropped, as a real stack
            // does.
            sendto(sock, reinterpret_cast<const char*>(forward.data()), forward_len, 0,
                   reinterpret_cast<const sockaddr*>(&target.inbox), sizeof(target.inbox));
        }
        // Trace, not debug: the guest-visible receive already logs one line per datagram, and this
        // one only adds the routing detail — worth having while bringing the transport up, not
        // worth doubling the log of every session that runs with Lib.Net at debug level.
        LOG_TRACE(Lib_Net, "P2P: datagram routed, vport = {}, len = {}, endpoints = {}",
                  ntohs(dst_vport), payload_len, targets.size());
    }
}

} // namespace Libraries::Net
