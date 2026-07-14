// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstring>
#include <limits>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>

#include "common/logging/log.h"
#include "core/libraries/network/net.h"
#include "core/libraries/network/sockets.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_matching2/np_matching2_internal.h"
#include "core/libraries/np/np_matching2/np_matching2_mm.h"
#include "core/libraries/np/np_matching2/np_matching2_signaling.h"
#include "core/libraries/np/np_signaling/np_signaling_stubs.h"

namespace Libraries::Np::NpMatching2 {

namespace {

constexpr s32 kMatching2ConnInactive = 0;
constexpr s32 kMatching2ConnPending = 1;
constexpr s32 kMatching2ConnActive = 2;
constexpr auto kMatching2HandshakeRetry = std::chrono::milliseconds(350);
constexpr auto kMatching2HandshakeTimeout = std::chrono::seconds(10);
constexpr auto kMatching2StunPingInterval = std::chrono::seconds(5);

std::atomic<bool> g_matching2_stop{false};
std::mutex g_matching2_thread_mutex;
std::thread g_matching2_thread;

enum class Matching2HandshakeKind : u8 {
    Offer = 1,
    Accept = 2,
    Check = 3,
    CheckAck = 4,
};

#pragma pack(push, 1)
struct Matching2HandshakePacket {
    u8 magic[4] = {'S', 'H', 'A', 'D'};
    u8 type = 0x21;
    u8 kind = 0;
    u64 room_id = 0;
    u16 from_member_id = 0;
    u16 to_member_id = 0;
    u8 online_id_from[16]{};
    u32 mapped_addr = 0;
    u16 mapped_port = 0;
    u16 reserved = 0;
    u64 nonce = 0;
};
#pragma pack(pop)
static_assert(sizeof(Matching2HandshakePacket) == 0x32);

#pragma pack(push, 1)
struct Matching2StunPing {
    u8 cmd = 0x01;
    u8 online_id[ORBIS_NP_ONLINEID_MAX_LENGTH]{};
    u32 local_ip = 0;
};
#pragma pack(pop)
static_assert(sizeof(Matching2StunPing) == 21);

bool HasMatching2Magic(const Matching2HandshakePacket& pkt) {
    return pkt.magic[0] == 'S' && pkt.magic[1] == 'H' && pkt.magic[2] == 'A' &&
           pkt.magic[3] == 'D' && pkt.type == 0x21;
}

std::string OnlineIdToString(const Libraries::Np::OrbisNpOnlineId& online_id) {
    char buf[ORBIS_NP_ONLINEID_MAX_LENGTH + 1]{};
    std::memcpy(buf, online_id.data, ORBIS_NP_ONLINEID_MAX_LENGTH);
    return std::string(buf);
}

bool ShouldConnectToPeer(const RoomCache& room, OrbisNpMatching2RoomMemberId self,
                         OrbisNpMatching2RoomMemberId peer) {
    if (peer == 0 || peer == self) {
        return false;
    }
    if (room.signaling_type == ORBIS_NP_MATCHING2_SIGNALING_TYPE_NONE) {
        return false;
    }
    if (room.signaling_type == ORBIS_NP_MATCHING2_SIGNALING_TYPE_STAR &&
        room.signaling_main_member != 0) {
        return self == room.signaling_main_member || peer == room.signaling_main_member;
    }
    return true;
}

bool ResolvePeerEndpoint(const MemberCache& member, PeerInfo& peer) {
    if (peer.addr != 0 && peer.port != 0) {
        return true;
    }
    if (member.addr != 0 && member.port != 0) {
        peer.addr = member.addr;
        peer.port = member.port;
        if (peer.online_id.data[0] == 0) {
            peer.online_id = member.np_id.handle;
        }
        return true;
    }

    const std::string online_id(member.np_id.handle.data);
    u32 resolved_addr = 0;
    u16 resolved_port = 0;
    if (!online_id.empty() && RequestSignalingInfos(online_id, &resolved_addr, &resolved_port)) {
        peer.addr = resolved_addr;
        peer.port = resolved_port;
    }
    if (peer.online_id.data[0] == 0) {
        peer.online_id = member.np_id.handle;
    }
    return peer.addr != 0 && peer.port != 0;
}

void MarkMatching2PeerActive(ContextObject& ctx, OrbisNpMatching2RoomId room_id,
                             OrbisNpMatching2RoomMemberId member_id, u32 addr, u16 port) {
    if (member_id == 0 || member_id == ctx.my_member_id) {
        return;
    }

    PeerInfo& peer = ctx.peers[member_id];
    peer.member_id = member_id;
    if (addr != 0) {
        peer.addr = addr;
    }
    if (port != 0) {
        peer.port = port;
    }
    const bool first_active = peer.status != kMatching2ConnActive;
    peer.status = kMatching2ConnActive;
    peer.handshake_started = true;

    if (first_active && !peer.sent_established) {
        peer.sent_established = true;
        QueueMatching2SignalingEvent(ctx, room_id, member_id,
                                     ORBIS_NP_MATCHING2_SIGNALING_EVENT_ESTABLISHED, ORBIS_OK);
        LOG_INFO(Lib_NpMatching2,
                 "Matching2 signaling established: ctx={} room={} member={} addr={:#x}:{}",
                 ctx.ctx_id, room_id, member_id, peer.addr, Libraries::Net::sceNetNtohs(peer.port));
    }
}

bool SendMatching2Handshake(ContextObject& ctx, OrbisNpMatching2RoomId room_id,
                            OrbisNpMatching2RoomMemberId member_id, Matching2HandshakeKind kind,
                            u64 nonce) {
    auto room_it = ctx.room_cache.find(room_id);
    if (room_it == ctx.room_cache.end()) {
        return false;
    }
    auto member_it = room_it->second.members.find(member_id);
    if (member_it == room_it->second.members.end()) {
        return false;
    }

    PeerInfo& peer = ctx.peers[member_id];
    peer.member_id = member_id;
    if (!ResolvePeerEndpoint(member_it->second, peer)) {
        LOG_WARNING(Lib_NpMatching2, "Matching2 signaling: unresolved endpoint room={} member={}",
                    room_id, member_id);
        return false;
    }

    Matching2HandshakePacket pkt{};
    pkt.kind = static_cast<u8>(kind);
    pkt.room_id = room_id;
    pkt.from_member_id = ctx.my_member_id;
    pkt.to_member_id = member_id;
    std::memcpy(pkt.online_id_from, ctx.online_id.data, ORBIS_NP_ONLINEID_MAX_LENGTH);
    pkt.mapped_addr = Net::GetP2PAdvertisedAddr();
    pkt.mapped_port = Net::GetP2PConfiguredPort() != 0
                          ? Libraries::Net::sceNetHtons(Net::GetP2PConfiguredPort())
                          : 0;
    pkt.nonce = nonce;

    const int rc = Net::P2PMatching2SendTo(&pkt, sizeof(pkt), peer.addr, peer.port);
    const auto now = std::chrono::steady_clock::now();
    peer.last_send = now;
    if (kind == Matching2HandshakeKind::Check) {
        peer.last_check_send = now;
    }
    LOG_DEBUG(Lib_NpMatching2,
              "Matching2 handshake send kind={} ctx={} room={} {}->{} dst={:#x}:{} rc={}",
              static_cast<u8>(kind), ctx.ctx_id, room_id, ctx.my_member_id, member_id, peer.addr,
              Libraries::Net::sceNetNtohs(peer.port), rc);
    return rc >= 0;
}

ContextObject* FindContextForMatching2Packet(const Matching2HandshakePacket& pkt) {
    for (u32 id = 1; id <= ContextManager::kMaxContexts; ++id) {
        ContextObject* ctx =
            ContextManager::Instance().Get(static_cast<OrbisNpMatching2ContextId>(id));
        if (!ctx || ctx->room_id != pkt.room_id || ctx->my_member_id != pkt.to_member_id) {
            continue;
        }
        if (ctx->room_cache.find(static_cast<OrbisNpMatching2RoomId>(pkt.room_id)) !=
            ctx->room_cache.end()) {
            return ctx;
        }
    }
    return nullptr;
}

void HandleMatching2HandshakePacket(u32 from_addr, u16 from_port,
                                    const Matching2HandshakePacket& pkt) {
    if (!HasMatching2Magic(pkt)) {
        return;
    }

    ContextObject* ctx = FindContextForMatching2Packet(pkt);
    if (!ctx) {
        LOG_DEBUG(Lib_NpMatching2, "Matching2 handshake: no ctx for room={} to_member={}",
                  pkt.room_id, pkt.to_member_id);
        return;
    }

    const auto room_id = static_cast<OrbisNpMatching2RoomId>(pkt.room_id);
    const auto member_id = static_cast<OrbisNpMatching2RoomMemberId>(pkt.from_member_id);
    auto room_it = ctx->room_cache.find(room_id);
    if (room_it == ctx->room_cache.end() ||
        !ShouldConnectToPeer(room_it->second, ctx->my_member_id, member_id)) {
        return;
    }

    PeerInfo& peer = ctx->peers[member_id];
    peer.member_id = member_id;
    peer.addr = pkt.mapped_addr != 0 ? pkt.mapped_addr : from_addr;
    peer.port = pkt.mapped_port != 0 ? pkt.mapped_port : from_port;
    peer.status =
        peer.status == kMatching2ConnActive ? kMatching2ConnActive : kMatching2ConnPending;
    peer.handshake_started = true;
    SetNpOnlineId(peer.online_id,
                  std::string_view(reinterpret_cast<const char*>(pkt.online_id_from),
                                   ORBIS_NP_ONLINEID_MAX_LENGTH));

    const auto kind = static_cast<Matching2HandshakeKind>(pkt.kind);
    switch (kind) {
    case Matching2HandshakeKind::Offer:
        SendMatching2Handshake(*ctx, room_id, member_id, Matching2HandshakeKind::Accept, 0);
        break;
    case Matching2HandshakeKind::Accept:
        peer.sent_check = true;
        SendMatching2Handshake(*ctx, room_id, member_id, Matching2HandshakeKind::Check, peer.nonce);
        break;
    case Matching2HandshakeKind::Check:
        SendMatching2Handshake(*ctx, room_id, member_id, Matching2HandshakeKind::CheckAck,
                               pkt.nonce);
        MarkMatching2PeerActive(*ctx, room_id, member_id, peer.addr, peer.port);
        break;
    case Matching2HandshakeKind::CheckAck:
        if (pkt.nonce == 0 || pkt.nonce == peer.nonce) {
            if (peer.last_check_send.time_since_epoch().count() != 0) {
                const auto rtt = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::steady_clock::now() - peer.last_check_send);
                peer.ping_us =
                    static_cast<u32>(std::min<u64>(static_cast<u64>(std::max<s64>(0, rtt.count())),
                                                   std::numeric_limits<u32>::max()));
                peer.last_check_send = {};
            }
            MarkMatching2PeerActive(*ctx, room_id, member_id, peer.addr, peer.port);
        }
        break;
    default:
        break;
    }
}

void Matching2HandshakeThreadMain() {
    auto last_stun_ping = std::chrono::steady_clock::time_point{};
    while (!g_matching2_stop.load(std::memory_order_relaxed)) {
        Matching2HandshakePacket pkt{};
        u32 from_addr = 0;
        u16 from_port = 0;
        const int rc = Net::P2PMatching2RecvFrom(&pkt, sizeof(pkt), &from_addr, &from_port);
        if (rc == sizeof(pkt)) {
            HandleMatching2HandshakePacket(from_addr, from_port, pkt);
        }

        const auto now = std::chrono::steady_clock::now();
        const bool should_stun_ping = last_stun_ping.time_since_epoch().count() == 0 ||
                                      now - last_stun_ping >= kMatching2StunPingInterval;
        for (u32 id = 1; id <= ContextManager::kMaxContexts; ++id) {
            ContextObject* ctx =
                ContextManager::Instance().Get(static_cast<OrbisNpMatching2ContextId>(id));
            if (!ctx) {
                continue;
            }
            if (should_stun_ping) {
                SendMatching2StunPing(*ctx);
            }
            if (ctx->room_id == 0) {
                continue;
            }
            auto room_it = ctx->room_cache.find(ctx->room_id);
            if (room_it == ctx->room_cache.end()) {
                continue;
            }
            for (auto& [member_id, peer] : ctx->peers) {
                if (peer.status != kMatching2ConnPending || !peer.handshake_started) {
                    continue;
                }
                if (peer.last_send.time_since_epoch().count() != 0 &&
                    now - peer.last_send > kMatching2HandshakeTimeout) {
                    peer.status = kMatching2ConnInactive;
                    QueueMatching2SignalingEvent(*ctx, ctx->room_id, member_id,
                                                 ORBIS_NP_MATCHING2_SIGNALING_EVENT_DEAD,
                                                 ORBIS_NP_MATCHING2_SIGNALING_ERROR_TIMEOUT);
                    continue;
                }
                if (peer.last_send.time_since_epoch().count() == 0 ||
                    now - peer.last_send >= kMatching2HandshakeRetry) {
                    SendMatching2Handshake(*ctx, ctx->room_id, member_id,
                                           peer.sent_check ? Matching2HandshakeKind::Check
                                                           : Matching2HandshakeKind::Offer,
                                           peer.sent_check ? peer.nonce : 0);
                }
            }
        }
        if (should_stun_ping) {
            last_stun_ping = now;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}

} // namespace

bool SendMatching2StunPing(const ContextObject& ctx) {
    if (!NpSignaling::Stubs::Matching2Enabled()) {
        return false;
    }
    if (!ctx.started || ctx.online_id.data[0] == '\0') {
        return false;
    }
    if (!NpSignaling::Stubs::EnsureTransport()) {
        return false;
    }

    const u32 server_addr = NpSignaling::Stubs::MmServerAddr();
    const u16 server_udp = NpSignaling::Stubs::MmServerUdpPort();
    if (server_addr == 0 || server_udp == 0) {
        return false;
    }

    Matching2StunPing ping{};
    ping.cmd = 0x01;
    std::memcpy(ping.online_id, ctx.online_id.data, ORBIS_NP_ONLINEID_MAX_LENGTH);
    ping.local_ip = NpSignaling::Stubs::AdvertisedAddr();

    const int rc =
        NpSignaling::Stubs::SignalingSendTo(&ping, sizeof(ping), server_addr, server_udp);
    LOG_DEBUG(Lib_NpMatching2,
              "Matching2 STUN ping: ctx={} online_id='{}' server={:#x}:{} local_ip={:#x} rc={}",
              ctx.ctx_id, OnlineIdToString(ctx.online_id), server_addr,
              Libraries::Net::sceNetNtohs(server_udp), ping.local_ip, rc);
    return rc >= 0;
}

void QueueMatching2SignalingEvent(ContextObject& ctx, OrbisNpMatching2RoomId room_id,
                                  OrbisNpMatching2RoomMemberId member_id,
                                  OrbisNpMatching2Event event, s32 error_code) {
    PendingEvent ev{};
    ev.type = PendingEvent::SIGNALING_CB;
    ev.ctx_id = ctx.ctx_id;
    ev.fire_at = std::chrono::steady_clock::now();
    ev.room_id = room_id;
    ev.member_id = member_id;
    ev.sig_event = event;
    ev.error_code = error_code;
    ScheduleEvent(std::move(ev));
}

void StartMatching2PeerHandshake(ContextObject& ctx, OrbisNpMatching2RoomId room_id,
                                 OrbisNpMatching2RoomMemberId member_id) {
    auto room_it = ctx.room_cache.find(room_id);
    if (room_it == ctx.room_cache.end() ||
        !ShouldConnectToPeer(room_it->second, ctx.my_member_id, member_id)) {
        return;
    }
    auto member_it = room_it->second.members.find(member_id);
    if (member_it == room_it->second.members.end()) {
        return;
    }

    Net::EnsureP2PTransport();

    PeerInfo& peer = ctx.peers[member_id];
    peer.member_id = member_id;
    if (peer.status == kMatching2ConnActive) {
        return;
    }
    ResolvePeerEndpoint(member_it->second, peer);
    peer.status = kMatching2ConnPending;
    peer.handshake_started = true;
    peer.sent_check = false;
    if (peer.nonce == 0) {
        peer.nonce = static_cast<u64>(std::chrono::steady_clock::now().time_since_epoch().count() ^
                                      (static_cast<u64>(ctx.ctx_id) << 48) ^
                                      (static_cast<u64>(member_id) << 16));
    }
    SendMatching2Handshake(ctx, room_id, member_id, Matching2HandshakeKind::Offer, 0);
}

void StartMatching2SignalingForRoomPeers(ContextObject& ctx, OrbisNpMatching2RoomId room_id) {
    const auto room_it = ctx.room_cache.find(room_id);
    if (room_it == ctx.room_cache.end()) {
        return;
    }
    for (const auto& [member_id, member] : room_it->second.members) {
        StartMatching2PeerHandshake(ctx, room_id, member_id);
    }
}

void QueueMatching2DeadForRoomPeers(ContextObject& ctx, OrbisNpMatching2RoomId room_id,
                                    s32 error_code) {
    auto room_it = ctx.room_cache.find(room_id);
    if (room_it == ctx.room_cache.end()) {
        return;
    }

    for (const auto& [member_id, member] : room_it->second.members) {
        if (member_id == 0 || member_id == ctx.my_member_id) {
            continue;
        }

        auto peer_it = ctx.peers.find(member_id);
        if (peer_it != ctx.peers.end()) {
            peer_it->second.status = kMatching2ConnInactive;
            peer_it->second.handshake_started = false;
            peer_it->second.sent_check = false;
        }

        QueueMatching2SignalingEvent(ctx, room_id, member_id,
                                     ORBIS_NP_MATCHING2_SIGNALING_EVENT_DEAD, error_code);
        LOG_INFO(Lib_NpMatching2, "Matching2 signaling dead: ctx={} room={} member={} reason={:#x}",
                 ctx.ctx_id, room_id, member_id, static_cast<u32>(error_code));
    }
}

void StartMatching2HandshakeThread() {
    std::lock_guard lock(g_matching2_thread_mutex);
    if (g_matching2_thread.joinable()) {
        return;
    }
    g_matching2_stop.store(false, std::memory_order_relaxed);
    g_matching2_thread = std::thread(Matching2HandshakeThreadMain);
}

void StopMatching2HandshakeThread() {
    {
        std::lock_guard lock(g_matching2_thread_mutex);
        g_matching2_stop.store(true, std::memory_order_relaxed);
    }
    if (g_matching2_thread.joinable()) {
        g_matching2_thread.join();
    }
}

u32 GetRoomPingUs(const ContextObject& ctx, OrbisNpMatching2RoomId roomId) {
    const auto room_it = ctx.room_cache.find(roomId);
    if (room_it == ctx.room_cache.end()) {
        return 0;
    }

    u64 total_ping = 0;
    u32 ping_count = 0;
    for (const auto& [member_id, member] : room_it->second.members) {
        if (member_id == ctx.my_member_id) {
            continue;
        }
        const auto peer_it = ctx.peers.find(member_id);
        if (peer_it == ctx.peers.end() || peer_it->second.ping_us == 0) {
            continue;
        }
        total_ping += peer_it->second.ping_us;
        ++ping_count;
    }

    return ping_count == 0 ? 0 : static_cast<u32>(total_ping / ping_count);
}

void* BuildSignalingGetPingInfoPayload(ContextObject& ctx, OrbisNpMatching2RoomId roomId) {
    CallbackPayload& p =
        ctx.request_payload_override ? *ctx.request_payload_override : ctx.request_payload;
    p.Reset();

    const auto room_it = ctx.room_cache.find(roomId);
    p.ping_info_response = std::make_unique<OrbisNpMatching2SignalingGetPingInfoResponse>();
    auto& out = *p.ping_info_response;
    out = {};
    if (room_it != ctx.room_cache.end()) {
        out.serverId = room_it->second.server_id;
        out.worldId = room_it->second.world_id;
        out.roomId = room_it->second.room_id;
    } else {
        out.serverId = ctx.server_id;
        out.worldId = ctx.world_id;
        out.roomId = roomId;
    }
    out.rtt = GetRoomPingUs(ctx, roomId);

    p.request_data = p.ping_info_response.get();
    return p.request_data;
}

s32 FillMatching2ConnectionInfo(const ContextObject& ctx, OrbisNpMatching2RoomId roomId,
                                OrbisNpMatching2RoomMemberId memberId, u32 infoType, void* connInfo,
                                bool a_variant) {
    if (!connInfo) {
        LOG_ERROR(Lib_NpMatching2, "connInfo null");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    const auto room_it = ctx.room_cache.find(roomId);
    if (room_it == ctx.room_cache.end()) {
        LOG_INFO(Lib_NpMatching2, "room={} not cached for connection info", roomId);
        if (a_variant) {
            *static_cast<OrbisNpMatching2SignalingConnectionInfoA*>(connInfo) = {};
        } else {
            *static_cast<OrbisNpMatching2SignalingConnectionInfo*>(connInfo) = {};
        }
        return ORBIS_OK;
    }

    const auto member_it = room_it->second.members.find(memberId);
    const MemberCache* member =
        member_it != room_it->second.members.end() ? &member_it->second : nullptr;
    const auto peer_it = ctx.peers.find(memberId);
    const PeerInfo* peer = peer_it != ctx.peers.end() ? &peer_it->second : nullptr;

    if (a_variant) {
        auto* out = static_cast<OrbisNpMatching2SignalingConnectionInfoA*>(connInfo);
        *out = {};
        switch (infoType) {
        case ORBIS_NP_MATCHING2_SIGNALING_CONN_INFO_RTT:
            out->rtt = peer ? peer->ping_us : 0;
            break;
        case ORBIS_NP_MATCHING2_SIGNALING_CONN_INFO_BANDWIDTH:
            out->bandwidth = 0;
            break;
        case ORBIS_NP_MATCHING2_SIGNALING_CONN_INFO_PEER_ADDR:
        case ORBIS_NP_MATCHING2_SIGNALING_CONN_INFO_MAPPED_ADDR:
            if (peer) {
                out->address.addr = peer->addr;
                out->address.port = peer->port;
            } else if (member) {
                out->address.addr = member->addr;
                out->address.port = member->port;
            }
            break;
        case ORBIS_NP_MATCHING2_SIGNALING_CONN_INFO_PACKET_LOSS:
            out->packetLoss = 0;
            break;
        case ORBIS_NP_MATCHING2_SIGNALING_CONN_INFO_PEER_ADDRESS_A:
            if (member) {
                out->peerAddrA.accountId = member->account_id;
                out->peerAddrA.platform = member->platform;
            }
            break;
        default:
            LOG_WARNING(Lib_NpMatching2, "unsupported connection info A type={}", infoType);
            return ORBIS_NP_MATCHING2_SIGNALING_ERROR_INVALID_ARGUMENT;
        }
    } else {
        auto* out = static_cast<OrbisNpMatching2SignalingConnectionInfo*>(connInfo);
        *out = {};
        switch (infoType) {
        case ORBIS_NP_MATCHING2_SIGNALING_CONN_INFO_RTT:
            out->rtt = peer ? peer->ping_us : 0;
            break;
        case ORBIS_NP_MATCHING2_SIGNALING_CONN_INFO_BANDWIDTH:
            out->bandwidth = 0;
            break;
        case ORBIS_NP_MATCHING2_SIGNALING_CONN_INFO_PEER_NP_ID:
        case ORBIS_NP_MATCHING2_SIGNALING_CONN_INFO_PEER_NPID:
            if (member) {
                out->npId = member->np_id;
            }
            break;
        case ORBIS_NP_MATCHING2_SIGNALING_CONN_INFO_PEER_ADDR:
        case ORBIS_NP_MATCHING2_SIGNALING_CONN_INFO_MAPPED_ADDR:
            if (peer) {
                out->address.addr = peer->addr;
                out->address.port = peer->port;
            } else if (member) {
                out->address.addr = member->addr;
                out->address.port = member->port;
            }
            break;
        case ORBIS_NP_MATCHING2_SIGNALING_CONN_INFO_PACKET_LOSS:
            out->packetLoss = 0;
            break;
        default:
            LOG_WARNING(Lib_NpMatching2, "unsupported connection info type={}", infoType);
            return ORBIS_NP_MATCHING2_SIGNALING_ERROR_INVALID_ARGUMENT;
        }
    }

    LOG_INFO(Lib_NpMatching2, "connection info{}: ctx={} room={} member={} type={} rtt={}",
             a_variant ? "A" : "", ctx.ctx_id, roomId, memberId, infoType,
             peer ? peer->ping_us : 0);
    return ORBIS_OK;
}

} // namespace Libraries::Np::NpMatching2
