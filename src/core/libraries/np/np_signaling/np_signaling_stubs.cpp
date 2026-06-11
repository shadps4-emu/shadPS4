// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/np/np_signaling/np_signaling_stubs.h"

namespace Libraries::Np::NpSignaling::Stubs {

namespace {
TransportHooks g_transport{};
PeerResolver g_peer_resolver = nullptr;
u32 g_mm_server_addr = 0;
u16 g_mm_server_udp_port = 0;
} // namespace

void SetTransportHooks(const TransportHooks& hooks) {
    g_transport = hooks;
}

int SignalingSendTo(const void* data, u32 len, u32 dest_addr, u16 dest_port) {
    return g_transport.signaling_send ? g_transport.signaling_send(data, len, dest_addr, dest_port)
                                      : -1;
}

int SignalingRecvFrom(void* buf, u32 len, u32* from_addr, u16* from_port) {
    return g_transport.signaling_recv ? g_transport.signaling_recv(buf, len, from_addr, from_port)
                                      : -1;
}

int ControlSendTo(const void* data, u32 len, u32 dest_addr, u16 dest_port) {
    return g_transport.control_send ? g_transport.control_send(data, len, dest_addr, dest_port)
                                    : -1;
}

int ControlRecvFrom(void* buf, u32 len, u32* from_addr, u16* from_port) {
    return g_transport.control_recv ? g_transport.control_recv(buf, len, from_addr, from_port) : -1;
}

bool TransportIsReady() {
    return g_transport.transport_ready ? g_transport.transport_ready() : false;
}

u16 ConfiguredPort() {
    return g_transport.configured_port ? g_transport.configured_port() : 0;
}

void SetPeerResolver(PeerResolver fn) {
    g_peer_resolver = fn;
}

void SetMmServerEndpoint(u32 addr, u16 udp_port) {
    g_mm_server_addr = addr;
    g_mm_server_udp_port = udp_port;
}

bool ResolvePeer(std::string_view online_id, u32* out_addr, u16* out_port) {
    return g_peer_resolver ? g_peer_resolver(online_id, out_addr, out_port) : false;
}

u32 MmServerAddr() {
    return g_mm_server_addr;
}

u16 MmServerUdpPort() {
    return g_mm_server_udp_port;
}

} // namespace Libraries::Np::NpSignaling::Stubs
