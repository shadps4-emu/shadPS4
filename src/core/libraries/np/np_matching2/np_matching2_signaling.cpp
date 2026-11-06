// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <chrono>
#include <cstring>
#include <memory>
#include <string>

#include "common/logging/log.h"
#include "common/singleton.h"
#include "core/libraries/net/net.h"
#include "core/libraries/net/net_p2p.h"
#include "core/libraries/net/net_util.h"
#include "core/libraries/network/netctl.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_handler/np_handler.h"
#include "core/libraries/np/np_matching2/np_matching2_internal.h"
#include "core/libraries/np/np_matching2/np_matching2_mm.h"
#include "core/libraries/np/np_matching2/np_matching2_signaling.h"
#include "core/libraries/np/np_signaling/np_signaling_state.h"
#include "core/libraries/np/np_signaling/np_signaling_transport.h"
#include "core/libraries/np/signaling_handler.h"

namespace Libraries::Np::NpMatching2 {

namespace {

constexpr s32 kMatching2ConnInactive = 0;

#pragma pack(push, 1)
struct Matching2StunPing {
    u8 cmd = 0x01;
    u8 online_id[ORBIS_NP_ONLINEID_MAX_LENGTH]{};
    u32 local_ip = 0;
};
#pragma pack(pop)
static_assert(sizeof(Matching2StunPing) == 21);

std::string OnlineIdToString(const Libraries::Np::OrbisNpOnlineId& online_id) {
    char buf[ORBIS_NP_ONLINEID_MAX_LENGTH + 1]{};
    std::memcpy(buf, online_id.data, ORBIS_NP_ONLINEID_MAX_LENGTH);
    return std::string(buf);
}

} // namespace

bool SendMatching2StunPing(const ContextObject& ctx) {
    if (!NpSignaling::Transport::Matching2Enabled()) {
        return false;
    }
    if (!ctx.started || ctx.online_id.data[0] == '\0') {
        return false;
    }
    if (!NpSignaling::Transport::EnsureTransport()) {
        return false;
    }

    const u32 server_addr = NpSignaling::Transport::MmServerAddr();
    const u16 server_udp = NpSignaling::Transport::MmServerUdpPort();
    if (server_addr == 0 || server_udp == 0) {
        return false;
    }

    Matching2StunPing ping{};
    ping.cmd = 0x01;
    std::memcpy(ping.online_id, ctx.online_id.data, ORBIS_NP_ONLINEID_MAX_LENGTH);
    ping.local_ip = NpSignaling::Transport::AdvertisedAddr();

    const int rc =
        NpSignaling::Transport::SignalingSendTo(&ping, sizeof(ping), server_addr, server_udp);
    LOG_DEBUG(Lib_NpMatching2,
              "Matching2 STUN ping: ctx={} online_id='{}' server={:#x}:{} local_ip={:#x} rc={}",
              ctx.ctx_id, OnlineIdToString(ctx.online_id), server_addr,
              Libraries::Net::sceNetNtohs(server_udp), ping.local_ip, rc);
    return rc >= 0;
}

void QueueMatching2SignalingEvent(ContextObject& ctx, OrbisNpMatching2RoomId room_id,
                                  OrbisNpMatching2RoomMemberId member_id,
                                  OrbisNpMatching2Event event, s32 error_code) {
    if (!ctx.signaling_callback) {
        LOG_ERROR(Lib_NpMatching2,
                  "Event skipped: ctx={} room={} member={} event={:#x} no callback", ctx.ctx_id,
                  room_id, member_id, static_cast<u16>(event));
        return;
    }

    PendingEvent ev{};
    ev.type = PendingEvent::SIGNALING_CB;
    ev.ctx_id = ctx.ctx_id;
    ev.context_owner = ctx.shared_from_this();
    ev.fire_at = std::chrono::steady_clock::now();
    ev.room_id = room_id;
    ev.member_id = member_id;
    ev.sig_event = event;
    ev.error_code = error_code;
    ScheduleEvent(std::move(ev));
}

void QueueMatching2DeadForRoomPeers(ContextObject& ctx, OrbisNpMatching2RoomId room_id,
                                    s32 error_code) {
    std::vector<OrbisNpMatching2RoomMemberId> dead_members;
    {
        auto cache = NpHandler::GetInstance().LockMatching2Cache(ctx.ctx_id);
        const auto room_it = cache->rooms.find(room_id);
        if (room_it == cache->rooms.end()) {
            return;
        }

        for (const auto& [member_id, member] : room_it->second.members) {
            if (member_id == 0 || member_id == ctx.my_member_id) {
                continue;
            }

            const auto peer_it = cache->peers.find(member_id);
            if (peer_it != cache->peers.end()) {
                peer_it->second.status = kMatching2ConnInactive;
            }
            dead_members.push_back(member_id);
        }
    }

    for (const auto member_id : dead_members) {
        QueueMatching2SignalingEvent(ctx, room_id, member_id,
                                     ORBIS_NP_MATCHING2_SIGNALING_EVENT_DEAD, error_code);
        LOG_INFO(Lib_NpMatching2, "Matching2 signaling dead: ctx={} room={} member={} reason={:#x}",
                 ctx.ctx_id, room_id, member_id, static_cast<u32>(error_code));
    }
    SignalingHandler::StopMatching2(ctx, room_id, error_code);
}

void StartMatching2SignalingRuntime() {
    SignalingHandler::Start();
}

void StopMatching2SignalingRuntime() {
    SignalingHandler::Stop();
}

u32 GetRoomPingUs(const ContextObject& ctx, OrbisNpMatching2RoomId roomId) {
    auto cache = NpHandler::GetInstance().LockMatching2Cache(ctx.ctx_id);
    const auto room_it = cache->rooms.find(roomId);
    if (room_it == cache->rooms.end()) {
        return 0;
    }

    u64 total_ping = 0;
    u32 ping_count = 0;
    for (const auto& [member_id, member] : room_it->second.members) {
        if (member_id == ctx.my_member_id) {
            continue;
        }
        const auto peer_it = cache->peers.find(member_id);
        if (peer_it == cache->peers.end() || peer_it->second.ping_us == 0) {
            continue;
        }
        total_ping += peer_it->second.ping_us;
        ++ping_count;
    }

    return ping_count == 0 ? 0 : static_cast<u32>(total_ping / ping_count);
}

void* BuildSignalingGetPingInfoPayload(ContextObject& ctx, CallbackPayload& p,
                                       OrbisNpMatching2RoomId roomId) {
    p.Reset();

    auto cache = NpHandler::GetInstance().LockMatching2Cache(ctx.ctx_id);
    const auto room_it = cache->rooms.find(roomId);
    p.ping_info_response = std::make_unique<OrbisNpMatching2SignalingGetPingInfoResponse>();
    auto& out = *p.ping_info_response;
    out = {};
    if (room_it != cache->rooms.end()) {
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

// Our own address combined with the external IP from the STUN echo plus our P2P port
static void FillMappedAddr(OrbisNpMatching2SignalingConnectionInfoAddr& out) {
    u32 addr = Common::Singleton<NetUtil::NetUtilInternal>::Instance()->GetExternalIp();
    if (addr == 0) {
        addr = Net::GetP2PAdvertisedAddr();
    }
    out.addr = addr;
    out.port = Net::sceNetHtons(Net::GetP2PAdvertisedPort());
}

s32 FillMatching2ConnectionInfo(const ContextObject& ctx, OrbisNpMatching2RoomId roomId,
                                OrbisNpMatching2RoomMemberId memberId, u32 infoType, void* connInfo,
                                bool a_variant) {
    if (!connInfo) {
        LOG_ERROR(Lib_NpMatching2, "connInfo null");
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    auto cache = NpHandler::GetInstance().LockMatching2Cache(ctx.ctx_id);
    const auto room_it = cache->rooms.find(roomId);
    if (room_it == cache->rooms.end()) {
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
    const auto peer_it = cache->peers.find(memberId);
    const PeerInfo* peer = peer_it != cache->peers.end() ? &peer_it->second : nullptr;

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
        case ORBIS_NP_MATCHING2_SIGNALING_CONN_INFO_MAPPED_ADDR:
            FillMappedAddr(out->address);
            break;
        case ORBIS_NP_MATCHING2_SIGNALING_CONN_INFO_PEER_ADDR:
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
        case ORBIS_NP_MATCHING2_SIGNALING_CONN_INFO_MAPPED_ADDR:
            FillMappedAddr(out->address);
            break;
        case ORBIS_NP_MATCHING2_SIGNALING_CONN_INFO_PEER_ADDR:
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

s32 FillMatching2LocalNetInfo(void* info) {
    auto* out = static_cast<NpSignaling::OrbisNpSignalingNetInfo*>(info);
    if (!out || out->size != sizeof(NpSignaling::OrbisNpSignalingNetInfo)) {
        LOG_ERROR(Lib_NpMatching2, "bad net info (size {})", out ? out->size : 0);
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    auto* netinfo = Common::Singleton<NetUtil::NetUtilInternal>::Instance();
    // GetIp() is empty until something asks for it. RetrieveIp() connects out, so only once.
    if (netinfo->GetIp().empty()) {
        netinfo->RetrieveIp();
    }
    out->localAddr = NpSignaling::ParseIpv4Nbo(netinfo->GetIp());
    if (out->localAddr == 0) {
        out->localAddr = NpSignaling::ParseIpv4Nbo("127.0.0.1");
    }
    out->mappedAddr = 0;
    out->natStatus = 0;
    NetCtl::OrbisNetCtlNatInfo nat_info{};
    nat_info.size = sizeof(nat_info);
    if (NetCtl::sceNetCtlGetNatInfo(&nat_info) >= 0) {
        out->mappedAddr = nat_info.mapped_addr;
        out->natStatus = nat_info.nat_type;
    }
    if (out->mappedAddr == 0) {
        const u32 external = netinfo->GetExternalIp();
        out->mappedAddr = external != 0 ? external : out->localAddr;
    }
    out->_pad_14 = 0;

    LOG_INFO(Lib_NpMatching2, "localAddr={:#x} mappedAddr={:#x} natStatus={}", out->localAddr,
             out->mappedAddr, out->natStatus);
    return ORBIS_OK;
}

} // namespace Libraries::Np::NpMatching2
