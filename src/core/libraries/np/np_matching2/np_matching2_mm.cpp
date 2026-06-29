// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <chrono>
#include <condition_variable>
#include <cstring>
#include <map>
#include <memory>
#include <mutex>

#include "common/logging/log.h"
#include "core/libraries/network/net.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_matching2/np_matching2_internal.h"
#include "core/libraries/np/np_matching2/np_matching2_mm.h"
#include "shadnet.pb.h"
#include "shadnet/client.h"

namespace Libraries::Np::NpMatching2 {

namespace {

struct PendingRequest {
    OrbisNpMatching2ContextId ctx_id = 0;
    OrbisNpMatching2RequestId req_id = 0;
    OrbisNpMatching2Event req_event{};
};

struct MmClientState {
    std::shared_ptr<ShadNet::ShadNetClient> client;
    std::mutex mutex;
    u32 server_addr = 0;
    u16 server_udp_port = 0;

    std::mutex pending_mutex;
    std::map<u64, PendingRequest> pending;

    std::mutex sig_mutex;
    std::condition_variable sig_cv;
    std::map<u64, std::pair<ShadNet::ErrorType, std::vector<u8>>> sig_replies;
};

MmClientState g_mm;

u32 IpStringToAddr(std::string_view ip) {
    u32 a, b, c, d;
    if (std::sscanf(std::string(ip).c_str(), "%u.%u.%u.%u", &a, &b, &c, &d) != 4) {
        return 0;
    }
    return a | (b << 8) | (c << 16) | (d << 24);
}

template <typename T>
std::vector<u8> MakeProtoPayload(const T& msg) {
    std::string bytes;
    (void)msg.SerializeToString(&bytes);
    const u32 len = static_cast<u32>(bytes.size());
    std::vector<u8> out;
    out.reserve(4 + bytes.size());
    out.push_back(static_cast<u8>(len));
    out.push_back(static_cast<u8>(len >> 8));
    out.push_back(static_cast<u8>(len >> 16));
    out.push_back(static_cast<u8>(len >> 24));
    out.insert(out.end(), bytes.begin(), bytes.end());
    return out;
}

void AppendBinAttr(shadnet::MatchingBinAttr* dst, const OrbisNpMatching2BinAttr& src) {
    dst->set_attr_id(src.id);
    if (src.data && src.dataSize > 0) {
        dst->set_data(src.data, src.dataSize);
    }
}

void AppendIntAttr(shadnet::MatchingIntAttr* dst, const OrbisNpMatching2IntAttr& src) {
    dst->set_attr_id(src.id);
    dst->set_attr_value(src.num);
}

std::string ExtractProtoBytes(const std::vector<u8>& payload, size_t offset = 0) {
    if (payload.size() < offset + 4) {
        return {};
    }
    const u32 len = static_cast<u32>(payload[offset]) |
                    (static_cast<u32>(payload[offset + 1]) << 8) |
                    (static_cast<u32>(payload[offset + 2]) << 16) |
                    (static_cast<u32>(payload[offset + 3]) << 24);
    if (payload.size() < offset + 4 + len) {
        return {};
    }
    return std::string(reinterpret_cast<const char*>(payload.data() + offset + 4), len);
}

void DispatchRequestComplete(const PendingRequest& pr, ShadNet::ErrorType error,
                             const std::vector<u8>& body) {
    ContextObject* ctx = ContextManager::Instance().Get(pr.ctx_id);
    if (!ctx) {
        return;
    }

    const s32 error_code = (error == ShadNet::ErrorType::NoError) ? 0 : static_cast<s32>(error);

    if (pr.req_event == ORBIS_NP_MATCHING2_CONTEXT_EVENT_STARTED ||
        pr.req_event == ORBIS_NP_MATCHING2_CONTEXT_EVENT_STOPPED) {
        PendingEvent ev{};
        ev.type = PendingEvent::CONTEXT_CB;
        ev.ctx_id = pr.ctx_id;
        ev.fire_at = std::chrono::steady_clock::now();
        ev.ctx_event = pr.req_event;
        ev.ctx_event_cause = ORBIS_NP_MATCHING2_EVENT_CAUSE_CONTEXT_ACTION;
        ev.error_code = error_code;
        ScheduleEvent(std::move(ev));
        return;
    }

    void* request_data = nullptr;

    if (error_code == 0) {
        const std::string proto = ExtractProtoBytes(body);
        if (pr.req_event == ORBIS_NP_MATCHING2_REQUEST_EVENT_CREATE_JOIN_ROOM ||
            pr.req_event == ORBIS_NP_MATCHING2_REQUEST_EVENT_JOIN_ROOM) {
            shadnet::CreateRoomReply reply;
            if (reply.ParseFromString(proto) && reply.has_details()) {
                request_data = BuildCreateJoinRoomPayload(*ctx, reply.details());
            }
        } else if (pr.req_event == ORBIS_NP_MATCHING2_REQUEST_EVENT_GET_WORLD_INFO_LIST) {
            shadnet::GetWorldInfoListReply reply;
            if (reply.ParseFromString(proto)) {
                request_data = BuildGetWorldInfoListPayload(*ctx, reply);
            }
        } else if (pr.req_event == ORBIS_NP_MATCHING2_REQUEST_EVENT_SEARCH_ROOM) {
            shadnet::SearchRoomReply reply;
            if (reply.ParseFromString(proto)) {
                request_data = BuildSearchRoomPayload(*ctx, reply);
            }
        }
    }

    PendingEvent ev{};
    ev.type = PendingEvent::REQUEST_CB;
    ev.ctx_id = pr.ctx_id;
    ev.fire_at = std::chrono::steady_clock::now();
    ev.req_id = pr.req_id;
    ev.req_event = pr.req_event;
    ev.error_code = error_code;
    ev.request_cb = ctx->default_request_callback;
    ev.request_cb_arg = ctx->default_request_callback_arg;
    ev.request_data = request_data;
    ScheduleEvent(std::move(ev));
}

void BuildMemberUpdate(CallbackPayload& p, const RoomCache& rc, const MemberCache& mc,
                       OrbisNpMatching2EventCause cause) {
    p.event_member = std::make_unique<OrbisNpMatching2RoomMemberDataInternal>();
    OrbisNpMatching2RoomMemberDataInternal& m = *p.event_member;
    m = OrbisNpMatching2RoomMemberDataInternal{};
    m.memberId = mc.member_id;
    m.teamId = mc.team_id;
    m.natType = mc.nat_type;
    m.flagAttr = mc.flag_attr;
    m.joinDate = mc.join_date;
    m.npId = mc.np_id;

    if (mc.group_id != 0) {
        auto grp_it = rc.groups.find(mc.group_id);
        if (grp_it != rc.groups.end()) {
            p.room_groups.resize(1);
            p.room_groups[0] = grp_it->second;
            m.roomGroup = p.room_groups.data();
        }
    }

    p.member_bin_attrs.resize(mc.bins.size());
    p.bin_buffers.reserve(mc.bins.size());
    size_t i = 0;
    for (const auto& [attr_id, bin] : mc.bins) {
        p.bin_buffers.emplace_back(bin.data.begin(), bin.data.end());
        auto& buf = p.bin_buffers.back();
        auto& dst = p.member_bin_attrs[i++];
        dst = OrbisNpMatching2RoomMemberBinAttrInternal{};
        dst.binAttr.id = bin.id;
        dst.binAttr.data = buf.empty() ? nullptr : buf.data();
        dst.binAttr.dataSize = buf.size();
    }
    m.roomMemberBinAttrInternal = p.member_bin_attrs.empty() ? nullptr : p.member_bin_attrs.data();
    m.roomMemberBinAttrInternalNum = p.member_bin_attrs.size();

    p.room_member_update = std::make_unique<OrbisNpMatching2RoomMemberUpdate>();
    OrbisNpMatching2RoomMemberUpdate& u = *p.room_member_update;
    u = OrbisNpMatching2RoomMemberUpdate{};
    u.roomMemberDataInternal = p.event_member.get();
    u.eventCause = cause;
    p.room_event_data = p.room_member_update.get();
}

void HandleRoomEvent(const ShadNet::NotifyRoomEvent& n) {
    ContextObject* ctx =
        ContextManager::Instance().Get(static_cast<OrbisNpMatching2ContextId>(n.ctx_id));
    if (!ctx) {
        return;
    }
    const auto event = static_cast<OrbisNpMatching2Event>(n.event);
    const auto cause = static_cast<OrbisNpMatching2EventCause>(n.event_cause);
    const auto room_id = static_cast<OrbisNpMatching2RoomId>(n.room_id);
    const auto member_id = static_cast<OrbisNpMatching2RoomMemberId>(n.member_id);

    CallbackPayload& p = ctx->room_event_payload;
    p.Reset();

    switch (event) {
    case ORBIS_NP_MATCHING2_ROOM_EVENT_MEMBER_JOINED: {
        auto room_it = ctx->room_cache.find(room_id);
        if (room_it == ctx->room_cache.end()) {
            return;
        }
        MemberCache& mc = room_it->second.members[member_id];
        mc = MemberCache{};
        mc.member_id = member_id;
        mc.team_id = static_cast<OrbisNpMatching2TeamId>(n.member_team_id);
        mc.nat_type = static_cast<OrbisNpMatching2NatType>(n.member_nat_type);
        mc.flag_attr = n.member_flag_attr;
        mc.group_id = static_cast<OrbisNpMatching2RoomGroupId>(n.member_group_id);
        mc.join_date = n.member_join_date;
        mc.addr = IpStringToAddr(n.member_addr);
        mc.port = Libraries::Net::sceNetHtons(static_cast<u16>(n.member_port));
        std::strncpy(mc.np_id.handle.data, n.member_npid.c_str(), sizeof(mc.np_id.handle.data) - 1);
        for (const auto& a : n.member_bin_attrs) {
            MemberBinCache& b = mc.bins[a.attr_id];
            b.id = static_cast<OrbisNpMatching2AttributeId>(a.attr_id);
            b.data = a.data;
        }

        PeerInfo pi{};
        pi.member_id = member_id;
        pi.addr = mc.addr;
        pi.port = mc.port;
        std::strncpy(pi.online_id.data, n.member_npid.c_str(), sizeof(pi.online_id.data) - 1);
        ctx->peers[member_id] = pi;

        BuildMemberUpdate(p, room_it->second, mc, cause);
        break;
    }
    case ORBIS_NP_MATCHING2_ROOM_EVENT_MEMBER_LEFT: {
        auto room_it = ctx->room_cache.find(room_id);
        if (room_it == ctx->room_cache.end()) {
            return;
        }
        auto mem_it = room_it->second.members.find(member_id);
        if (mem_it == room_it->second.members.end()) {
            return;
        }
        BuildMemberUpdate(p, room_it->second, mem_it->second, cause);
        room_it->second.members.erase(mem_it);
        ctx->peers.erase(member_id);
        break;
    }
    case ORBIS_NP_MATCHING2_ROOM_EVENT_KICKEDOUT: {
        p.room_update = std::make_unique<OrbisNpMatching2RoomUpdate>();
        OrbisNpMatching2RoomUpdate& u = *p.room_update;
        u = OrbisNpMatching2RoomUpdate{};
        u.eventCause = cause;
        u.errorCode = n.error_code;
        p.room_event_data = p.room_update.get();
        break;
    }
    default:
        LOG_WARNING(Lib_NpMatching2, "unhandled room event {:#x}", n.event);
        return;
    }

    LOG_DEBUG(Lib_NpMatching2, "RoomEvent ctx={} room={} event={:#x} cause={}", n.ctx_id, n.room_id,
              n.event, n.event_cause);

    PendingEvent ev{};
    ev.type = PendingEvent::ROOM_EVENT_CB;
    ev.ctx_id = ctx->ctx_id;
    ev.fire_at = std::chrono::steady_clock::now();
    ev.room_id = room_id;
    ev.room_event = event;
    ev.room_event_data = p.room_event_data;
    ScheduleEvent(std::move(ev));
}

} // namespace

void OnMatchingReply(ShadNet::CommandType cmd, u64 pkt_id, ShadNet::ErrorType error,
                     const std::vector<u8>& body) {
    if (cmd == ShadNet::CommandType::RequestSignalingInfos) {
        std::lock_guard lock(g_mm.sig_mutex);
        g_mm.sig_replies[pkt_id] = {error, body};
        g_mm.sig_cv.notify_all();
        return;
    }

    PendingRequest pr;
    {
        std::lock_guard lock(g_mm.pending_mutex);
        auto it = g_mm.pending.find(pkt_id);
        if (it == g_mm.pending.end()) {
            LOG_WARNING(Lib_NpMatching2, "reply pkt_id={} cmd={} has no pending entry (have {})",
                        pkt_id, static_cast<u16>(cmd), g_mm.pending.size());
            return;
        }
        pr = it->second;
        g_mm.pending.erase(it);
    }
    LOG_DEBUG(Lib_NpMatching2, "reply pkt_id={} cmd={} -> ctx={} reqId={} event={:#x}", pkt_id,
              static_cast<u16>(cmd), pr.ctx_id, pr.req_id, static_cast<u16>(pr.req_event));
    DispatchRequestComplete(pr, error, body);
}

void SetMmShadNetClient(std::shared_ptr<ShadNet::ShadNetClient> client,
                        std::string_view server_host, u16 tcp_port) {
    {
        std::lock_guard lock(g_mm.mutex);
        g_mm.client = client;
        g_mm.server_addr = client ? client->GetAddrServer() : 0;
        if (g_mm.server_addr == 0) {
            g_mm.server_addr = IpStringToAddr(server_host);
        }
        g_mm.server_udp_port = Libraries::Net::sceNetHtons(static_cast<u16>(tcp_port + 1));
    }

    if (!client) {
        return;
    }

    client->onRoomEvent = [](const ShadNet::NotifyRoomEvent& n) { HandleRoomEvent(n); };
}

void ClearMmShadNetClient() {
    std::shared_ptr<ShadNet::ShadNetClient> old_client;
    {
        std::lock_guard lock(g_mm.mutex);
        old_client = std::move(g_mm.client);
        g_mm.server_addr = 0;
        g_mm.server_udp_port = 0;
    }
    if (old_client) {
        old_client->onRoomEvent = nullptr;
    }
    {
        std::lock_guard lock(g_mm.pending_mutex);
        g_mm.pending.clear();
    }
    g_mm.sig_cv.notify_all();
}

bool IsMmClientRunning() {
    std::lock_guard lock(g_mm.mutex);
    return g_mm.client && g_mm.client->IsAuthenticated();
}

void MmContextStart(OrbisNpMatching2ContextId ctx_id) {
    shadnet::ContextStartRequest req;
    req.set_ctx_id(ctx_id);
    MmSubmitRequest(ctx_id, 0, ORBIS_NP_MATCHING2_CONTEXT_EVENT_STARTED, MmCommand::ContextStart,
                    MakeProtoPayload(req));
}

void MmContextStop(OrbisNpMatching2ContextId ctx_id) {
    shadnet::ContextStopRequest req;
    req.set_ctx_id(ctx_id);
    MmSubmitRequest(ctx_id, 0, ORBIS_NP_MATCHING2_CONTEXT_EVENT_STOPPED, MmCommand::ContextStop,
                    MakeProtoPayload(req));
}

s32 MmSubmitRequest(OrbisNpMatching2ContextId ctx_id, OrbisNpMatching2RequestId req_id,
                    OrbisNpMatching2Event req_event, MmCommand cmd,
                    const std::vector<u8>& payload) {
    std::shared_ptr<ShadNet::ShadNetClient> client;
    {
        std::lock_guard lock(g_mm.mutex);
        client = g_mm.client;
    }
    if (!client || !client->IsAuthenticated()) {
        LOG_ERROR(Lib_NpMatching2, "MmSubmitRequest({}): not connected", static_cast<u16>(cmd));
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }

    const u64 pkt_id = client->SubmitRequest(static_cast<ShadNet::CommandType>(cmd), payload);
    {
        std::lock_guard lock(g_mm.pending_mutex);
        g_mm.pending[pkt_id] = {ctx_id, req_id, req_event};
    }
    LOG_DEBUG(Lib_NpMatching2, "submit cmd={} pkt_id={} ctx={} reqId={}", static_cast<u16>(cmd),
              pkt_id, ctx_id, req_id);
    return ORBIS_OK;
}

s32 MmCreateJoinRoom(OrbisNpMatching2ContextId ctx_id, OrbisNpMatching2RequestId req_id,
                     const OrbisNpMatching2CreateJoinRoomRequest& request) {
    shadnet::CreateRoomRequest req;
    req.set_req_id(req_id);
    req.set_max_slots(request.maxSlot);
    req.set_team_id(request.teamId);
    req.set_world_id(request.worldId);
    req.set_lobby_id(static_cast<u32>(request.lobbyId));
    req.set_flags(request.flags);
    req.set_group_config_count(static_cast<u32>(request.groupConfigs));
    req.set_allowed_user_count(static_cast<u32>(request.allowedUsers));
    req.set_blocked_user_count(static_cast<u32>(request.blockedUsers));
    req.set_internal_bin_attr_count(static_cast<u32>(request.internalBinAttrs));
    for (u64 i = 0; i < request.externalSearchIntAttrs; ++i) {
        AppendIntAttr(req.add_external_search_int_attrs(), request.externalSearchIntAttr[i]);
    }
    for (u64 i = 0; i < request.externalSearchBinAttrs; ++i) {
        AppendBinAttr(req.add_external_search_bin_attrs(), request.externalSearchBinAttr[i]);
    }
    for (u64 i = 0; i < request.externalBinAttrs; ++i) {
        AppendBinAttr(req.add_external_bin_attrs(), request.externalBinAttr[i]);
    }
    for (u64 i = 0; i < request.memberInternalBinAttrs; ++i) {
        AppendBinAttr(req.add_member_bin_attrs(), request.memberInternalBinAttr[i]);
    }
    req.set_join_group_label_present(request.joinGroupLabel != nullptr);
    req.set_room_password_present(request.roomPasswd != nullptr);
    if (request.signalingParam) {
        req.set_sig_type(request.signalingParam->type);
        req.set_sig_flag(request.signalingParam->flag);
        req.set_sig_main_member(request.signalingParam->memberId);
    }
    return MmSubmitRequest(ctx_id, req_id, ORBIS_NP_MATCHING2_REQUEST_EVENT_CREATE_JOIN_ROOM,
                           MmCommand::CreateRoom, MakeProtoPayload(req));
}

s32 MmJoinRoom(OrbisNpMatching2ContextId ctx_id, OrbisNpMatching2RequestId req_id,
               const OrbisNpMatching2JoinRoomRequest& request) {
    shadnet::JoinRoomRequest req;
    req.set_room_id(request.roomId);
    req.set_req_id(req_id);
    req.set_team_id(request.teamId);
    req.set_join_flags(request.flags);
    req.set_blocked_user_count(static_cast<u32>(request.blockedUsers));
    for (u64 i = 0; i < request.roomMemberBinInternalAttrNum; ++i) {
        AppendBinAttr(req.add_member_bin_attrs(), request.roomMemberBinInternalAttr[i]);
    }
    req.set_room_password_present(request.roomPasswd != nullptr);
    req.set_join_group_label_present(request.joinGroupLabel != nullptr);
    return MmSubmitRequest(ctx_id, req_id, ORBIS_NP_MATCHING2_REQUEST_EVENT_JOIN_ROOM,
                           MmCommand::JoinRoom, MakeProtoPayload(req));
}

s32 MmLeaveRoom(OrbisNpMatching2ContextId ctx_id, OrbisNpMatching2RequestId req_id,
                const OrbisNpMatching2LeaveRoomRequest& request) {
    shadnet::LeaveRoomRequest req;
    req.set_room_id(request.roomId);
    req.set_req_id(req_id);
    return MmSubmitRequest(ctx_id, req_id, ORBIS_NP_MATCHING2_REQUEST_EVENT_LEAVE_ROOM,
                           MmCommand::LeaveRoom, MakeProtoPayload(req));
}

s32 MmGetWorldInfoList(OrbisNpMatching2ContextId ctx_id, OrbisNpMatching2RequestId req_id,
                       const OrbisNpMatching2GetWorldInfoListRequest& request) {
    shadnet::GetWorldInfoListRequest req;
    req.set_server_id(request.serverId);
    return MmSubmitRequest(ctx_id, req_id, ORBIS_NP_MATCHING2_REQUEST_EVENT_GET_WORLD_INFO_LIST,
                           MmCommand::GetWorldInfoList, MakeProtoPayload(req));
}

s32 MmSearchRoom(OrbisNpMatching2ContextId ctx_id, OrbisNpMatching2RequestId req_id,
                 const OrbisNpMatching2SearchRoomRequest& request) {
    shadnet::SearchRoomRequest req;
    req.set_world_id(request.worldId);
    req.set_lobby_id(request.lobbyId);
    req.set_range_filter_start(request.rangeFilter.start);
    req.set_range_filter_max(request.rangeFilter.max);
    req.set_flag_filter(request.flags1);
    req.set_flag_attr(request.flags2);
    LOG_DEBUG(Lib_NpMatching2,
              "search world={} lobby={} range={}+{} flags={:#x}/{:#x} intN={} binN={} attrN={}",
              request.worldId, request.lobbyId, request.rangeFilter.start, request.rangeFilter.max,
              request.flags1, request.flags2, request.intFilters, request.binFilters,
              request.attrs);
    for (u64 i = 0; i < request.intFilters; ++i) {
        const auto& f = request.intFilter[i];
        LOG_DEBUG(Lib_NpMatching2, "  intFilter[{}] op={} id={:#x} num={}", i, f.searchOperator,
                  f.attr.id, f.attr.num);
        auto* dst = req.add_int_filters();
        dst->set_op(f.searchOperator);
        dst->set_attr_id(f.attr.id);
        dst->set_attr_value(f.attr.num);
    }
    for (u64 i = 0; i < request.binFilters; ++i) {
        const auto& f = request.binFilter[i];
        auto* dst = req.add_bin_filters();
        dst->set_op(f.searchOperator);
        dst->set_attr_id(f.attr.id);
        if (f.attr.data && f.attr.dataSize > 0) {
            dst->set_data(f.attr.data, f.attr.dataSize);
        }
    }
    for (u64 i = 0; i < request.attrs; ++i) {
        req.add_attr_ids(request.attr[i]);
    }
    return MmSubmitRequest(ctx_id, req_id, ORBIS_NP_MATCHING2_REQUEST_EVENT_SEARCH_ROOM,
                           MmCommand::SearchRoom, MakeProtoPayload(req));
}

u32 GetMmServerAddr() {
    std::lock_guard lock(g_mm.mutex);
    return g_mm.server_addr;
}

u16 GetMmServerUdpPort() {
    std::lock_guard lock(g_mm.mutex);
    return g_mm.server_udp_port;
}

bool RequestSignalingInfos(std::string_view target_online_id, u32* out_addr, u16* out_port) {
    if (!out_addr || !out_port) {
        return false;
    }
    std::shared_ptr<ShadNet::ShadNetClient> client;
    {
        std::lock_guard lock(g_mm.mutex);
        client = g_mm.client;
    }
    if (!client || !client->IsAuthenticated()) {
        return false;
    }

    shadnet::RequestSignalingInfosRequest req;
    req.set_target_npid(std::string(target_online_id));
    const u64 pkt_id =
        client->SubmitRequest(ShadNet::CommandType::RequestSignalingInfos, MakeProtoPayload(req));

    std::pair<ShadNet::ErrorType, std::vector<u8>> reply;
    {
        std::unique_lock lock(g_mm.sig_mutex);
        if (!g_mm.sig_cv.wait_for(lock, std::chrono::seconds(5),
                                  [&] { return g_mm.sig_replies.count(pkt_id) > 0; })) {
            LOG_WARNING(Lib_NpMatching2, "timed out for '{}'", target_online_id);
            return false;
        }
        reply = std::move(g_mm.sig_replies[pkt_id]);
        g_mm.sig_replies.erase(pkt_id);
    }
    if (reply.first != ShadNet::ErrorType::NoError) {
        return false;
    }

    shadnet::RequestSignalingInfosReply rep;
    const std::string proto = ExtractProtoBytes(reply.second);
    if (proto.empty() || !rep.ParseFromString(proto)) {
        return false;
    }
    const std::string addr_str = rep.target_ip();
    const u16 port_host = static_cast<u16>(rep.target_port());
    if (addr_str.empty() || port_host == 0) {
        return false;
    }

    u32 addr_nbo = 0;
    if (Libraries::Net::sceNetInetPton(Net::ORBIS_NET_AF_INET, addr_str.c_str(), &addr_nbo) <= 0) {
        return false;
    }
    *out_addr = addr_nbo;
    *out_port = Libraries::Net::sceNetHtons(port_host);
    LOG_DEBUG(Lib_NpMatching2, "'{}' -> {}:{}", target_online_id, addr_str, port_host);
    return true;
}

} // namespace Libraries::Np::NpMatching2
