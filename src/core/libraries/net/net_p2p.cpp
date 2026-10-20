// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <chrono>
#include <cstring>
#include <iterator>
#include <memory>
#include <mutex>
#include <span>
#include <string>
#include <utility>
#include <vector>
#include <fmt/format.h>

#include "common/logging/log.h"
#include "core/libraries/net/net.h"
#include "core/libraries/net/net_log.h"
#include "core/libraries/net/net_p2p.h"
#include "core/libraries/net/net_translate.h"
#include "core/net/guest_net.h"
#include "core/net/p2p_codec.h"

namespace Libraries::Net {

namespace {

using Host::Error;

enum SignalingChannel : size_t { Signaling = 0, Control = 1, Matching2 = 2, ChannelCount };

size_t ClassifySignaling(std::span<const u8> packet) {
    if (packet.size() < 5 || std::memcmp(packet.data(), "SHAD", 4) != 0) {
        return Signaling;
    }
    switch (packet[4]) {
    case 0x10:
        return Control;
    case 0x21:
        return Matching2;
    default:
        return Signaling;
    }
}

struct P2PState {
    std::mutex mutex;
    std::shared_ptr<P2P::Transport> transport;
    std::shared_ptr<P2P::Keyring> keyring = std::make_shared<P2P::Keyring>();
    u16 configured_port = 3658; // host order
    u32 advertised_addr = 0;    // network order
    std::chrono::steady_clock::time_point retry_after{};
    std::pair<u16, Error> last_failure{0, Error::Ok};
};

P2PState& P2PStateInstance() {
    static P2PState state;
    return state;
}

void LogBroadcast(std::span<const u8> packet, const P2P::Endpoint& from) {
    char address[INET6_ADDRSTRLEN] = "?";
    const auto* sa = from.Sockaddr();
    if (sa->sa_family == AF_INET) {
        inet_ntop(AF_INET, &reinterpret_cast<const sockaddr_in*>(sa)->sin_addr, address,
                  sizeof(address));
    } else if (sa->sa_family == AF_INET6) {
        inet_ntop(AF_INET6, &reinterpret_cast<const sockaddr_in6*>(sa)->sin6_addr, address,
                  sizeof(address));
    }
    std::string hex;
    const size_t shown = std::min<size_t>(packet.size(), 64);
    hex.reserve(shown * 3);
    for (size_t i = 0; i < shown; ++i) {
        fmt::format_to(std::back_inserter(hex), "{}{:02x}", i == 0 ? "" : " ", packet[i]);
    }
    LOG_WARNING(Lib_Net,
                "P2P broadcast packet received (layout assumed; please report this line): "
                "from {}:{}, {} bytes: {}{}",
                address, from.Port(), packet.size(), hex, packet.size() > shown ? " ..." : "");
}

constexpr u16 P2PPortFallbacks = 16;

u16 StartLocked(P2PState& state, u16 udp_port, bool allow_fallback) {
    if (!Host::Initialize()) {
        return 0;
    }
    P2P::TransportConfig config;
    config.signaling_channels = ChannelCount;
    config.classify_signaling = ClassifySignaling;
    const auto try_port = [&](u16 port, Error* error) {
        auto codec = std::make_unique<P2P::FramingCodec>(state.keyring, LogBroadcast);
        return P2P::Transport::Create(AF_INET, P2P::Endpoint::IPv4("0.0.0.0", port),
                                      std::move(codec), config, error);
    };
    Error error;
    auto transport = try_port(udp_port, &error);
    for (u16 i = 1; !transport && allow_fallback && error == Error::AddrInUse && udp_port != 0 &&
                    i <= P2PPortFallbacks && udp_port + i <= 0xffff;
         ++i) {
        transport = try_port(static_cast<u16>(udp_port + i), &error);
    }
    if (!transport) {
        if (state.last_failure != std::pair{udp_port, error}) {
            state.last_failure = {udp_port, error};
            LOG_ERROR(Lib_Net, "cannot start P2P on UDP port {}: {}", udp_port,
                      ErrnoName(static_cast<int>(error)));
        } else {
            LOG_DEBUG(Lib_Net, "cannot start P2P on UDP port {}: {}", udp_port,
                      ErrnoName(static_cast<int>(error)));
        }
        return 0;
    }
    state.last_failure = {0, Error::Ok};
    const u16 port = transport->BoundPort();
    state.transport = transport;
    Core::Net::SetP2PTransport(AF_INET, std::move(transport));
    if (udp_port != 0 && port != udp_port) {
        LOG_WARNING(Lib_Net,
                    "UDP port {} is in use (another instance?), P2P transport on UDP port {} "
                    "instead. Set p2p_port to choose one.",
                    udp_port, port);
    } else {
        LOG_INFO(Lib_Net, "P2P transport on UDP port {}", port);
    }
    return port;
}

u16 PortToBind(const P2PState& state, const SystemHooks& hooks) {
    return hooks.p2p_port ? hooks.p2p_port() : state.configured_port;
}

std::shared_ptr<P2P::Transport> RunningTransport() {
    auto& state = P2PStateInstance();
    const auto hooks = GetSystemHooks();
    std::shared_ptr<P2P::Transport> transport;
    u16 started = 0;
    {
        std::scoped_lock lock{state.mutex};
        const auto now = std::chrono::steady_clock::now();
        if (!state.transport && now >= state.retry_after) {
            started = StartLocked(state, PortToBind(state, hooks), true);
            if (started == 0) {
                state.retry_after = now + std::chrono::seconds{2};
            }
        }
        transport = state.transport;
    }
    if (started != 0 && hooks.p2p_started) {
        hooks.p2p_started(started);
    }
    return transport;
}

int ChannelSendTo(const void* data, u32 len, u32 dest_addr, u16 dest_port) {
    const auto transport = RunningTransport();
    if (!transport || (data == nullptr && len != 0)) {
        return -1;
    }
    sockaddr_in to{};
    to.sin_family = AF_INET;
    to.sin_addr.s_addr = dest_addr;
    to.sin_port = dest_port;
    const auto endpoint =
        P2P::Endpoint::FromSockaddr(reinterpret_cast<const sockaddr*>(&to), sizeof(to));
    const Error e = transport->SendSignaling(
        {static_cast<const u8*>(data), static_cast<size_t>(len)}, endpoint);
    return e == Error::Ok ? static_cast<int>(len) : -1;
}

int ChannelRecvFrom(size_t channel, void* buf, u32 len, u32* from_addr, u16* from_port) {
    const auto transport = RunningTransport();
    std::vector<u8> packet;
    P2P::Endpoint from;
    if (!transport || !transport->RecvSignaling(channel, &packet, &from)) {
        return -1;
    }
    const u32 n = std::min<u32>(len, static_cast<u32>(packet.size()));
    if (buf != nullptr) {
        std::memcpy(buf, packet.data(), n);
    }
    const auto* in = reinterpret_cast<const sockaddr_in*>(&from.addr);
    if (from_addr != nullptr) {
        *from_addr = in->sin_addr.s_addr;
    }
    if (from_port != nullptr) {
        *from_port = in->sin_port;
    }
    return static_cast<int>(n);
}

} // namespace

void ConfigureP2P(u16 udp_port, u32 advertised_addr) {
    auto& state = P2PStateInstance();
    std::scoped_lock lock{state.mutex};
    state.configured_port = udp_port;
    state.advertised_addr = advertised_addr;
    const u32 ip = sceNetNtohl(advertised_addr);
    LOG_INFO(Lib_Net, "P2P configured: UDP port {}, advertised address {}.{}.{}.{}", udp_port,
             ip >> 24, (ip >> 16) & 0xff, (ip >> 8) & 0xff, ip & 0xff);
}

u16 StartP2P(u16 udp_port) {
    StopP2P();
    auto& state = P2PStateInstance();
    u16 started = 0;
    {
        std::scoped_lock lock{state.mutex};
        started = StartLocked(state, udp_port, false);
        state.retry_after = {};
    }
    const auto hooks = GetSystemHooks();
    if (started != 0 && hooks.p2p_started) {
        hooks.p2p_started(started);
    }
    return started;
}

void StopP2P() {
    auto& state = P2PStateInstance();
    u16 stopped = 0;
    {
        std::scoped_lock lock{state.mutex};
        if (state.transport) {
            stopped = state.transport->BoundPort();
        }
        state.transport.reset();
        state.retry_after = {};
        Core::Net::SetP2PTransport(AF_INET, nullptr);
    }
    if (stopped != 0) {
        LOG_INFO(Lib_Net, "P2P transport on UDP port {} stopped", stopped);
    }
    const auto hooks = GetSystemHooks();
    if (stopped != 0 && hooks.p2p_stopped) {
        hooks.p2p_stopped(stopped);
    }
}

std::shared_ptr<P2P::Keyring> GetP2PKeyring() {
    auto& state = P2PStateInstance();
    std::scoped_lock lock{state.mutex};
    return state.keyring;
}

bool EnsureP2PTransport() {
    return RunningTransport() != nullptr;
}

bool P2PTransportIsReady() {
    auto& state = P2PStateInstance();
    std::scoped_lock lock{state.mutex};
    return state.transport != nullptr;
}

u16 GetP2PConfiguredPort() {
    const auto hooks = GetSystemHooks();
    auto& state = P2PStateInstance();
    std::scoped_lock lock{state.mutex};
    return PortToBind(state, hooks);
}

u16 GetP2PBoundPort() {
    auto& state = P2PStateInstance();
    std::scoped_lock lock{state.mutex};
    return state.transport ? state.transport->BoundPort() : 0;
}

u32 GetP2PAdvertisedAddr() {
    u32 configured = 0;
    {
        auto& state = P2PStateInstance();
        std::scoped_lock lock{state.mutex};
        configured = state.advertised_addr;
    }
    if (configured != 0) {
        return configured;
    }
    const auto hooks = GetSystemHooks();
    return hooks.public_addr ? hooks.public_addr() : 0;
}

u16 GetP2PAdvertisedPort() {
    const u16 bound = GetP2PBoundPort();
    return bound != 0 ? bound : GetP2PConfiguredPort();
}

int P2PSignalingSendTo(const void* data, u32 len, u32 dest_addr, u16 dest_port) {
    return ChannelSendTo(data, len, dest_addr, dest_port);
}

int P2PSignalingRecvFrom(void* buf, u32 len, u32* from_addr, u16* from_port) {
    return ChannelRecvFrom(Signaling, buf, len, from_addr, from_port);
}

int P2PControlSendTo(const void* data, u32 len, u32 dest_addr, u16 dest_port) {
    return ChannelSendTo(data, len, dest_addr, dest_port);
}

int P2PControlRecvFrom(void* buf, u32 len, u32* from_addr, u16* from_port) {
    return ChannelRecvFrom(Control, buf, len, from_addr, from_port);
}

int P2PMatching2SendTo(const void* data, u32 len, u32 dest_addr, u16 dest_port) {
    return ChannelSendTo(data, len, dest_addr, dest_port);
}

int P2PMatching2RecvFrom(void* buf, u32 len, u32* from_addr, u16* from_port) {
    return ChannelRecvFrom(Matching2, buf, len, from_addr, from_port);
}

} // namespace Libraries::Net
