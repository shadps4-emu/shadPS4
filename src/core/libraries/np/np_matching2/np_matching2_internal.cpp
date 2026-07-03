// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <utility>

#include "common/logging/log.h"
#include "common/thread.h"
#include "core/libraries/network/net.h"
#include "core/libraries/np/np_common.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_matching2/np_matching2_internal.h"
#include "shadnet.pb.h"

namespace Libraries::Np::NpMatching2 {

NpMatching2State g_state;

namespace {

u32 IpStringToAddr(std::string_view ip) {
    u32 a, b, c, d;
    if (std::sscanf(std::string(ip).c_str(), "%u.%u.%u.%u", &a, &b, &c, &d) != 4) {
        return 0;
    }
    return a | (b << 8) | (c << 16) | (d << 24);
}

} // namespace

void BuildCreateJoinRoomPayloadCommon(ContextObject& ctx,
                                      const shadnet::CreateJoinRoomResponse& resp) {
    CallbackPayload& p = ctx.request_payload;
    const auto& rd = resp.room_data();

    p.room_groups.resize(rd.groups_size());
    for (int i = 0; i < rd.groups_size(); ++i) {
        const auto& g = rd.groups(i);
        auto& dst = p.room_groups[i];
        dst.id = static_cast<OrbisNpMatching2RoomGroupId>(g.group_id());
        dst.hasPasswd = g.has_passwd();
        dst.hasLabel = g.has_label();
        std::memset(dst.label.data, 0, sizeof(dst.label.data));
        std::memcpy(dst.label.data, g.label().data(),
                    std::min(g.label().size(), sizeof(dst.label.data)));
        dst.slots = g.slot_count();
        dst.groupMembers = g.num_members();
    }

    p.room_bin_attrs.resize(rd.bin_attrs_internal_size());
    for (int i = 0; i < rd.bin_attrs_internal_size(); ++i) {
        const auto& a = rd.bin_attrs_internal(i);
        p.bin_buffers.emplace_back(a.data().begin(), a.data().end());
        auto& buf = p.bin_buffers.back();
        auto& dst = p.room_bin_attrs[i];
        dst = {};
        dst.lastUpdate.tick = a.update_date();
        dst.memberId = static_cast<OrbisNpMatching2RoomMemberId>(a.update_member_id());
        dst.binAttr.id = static_cast<OrbisNpMatching2AttributeId>(a.attr_id());
        dst.binAttr.data = buf.empty() ? nullptr : buf.data();
        dst.binAttr.dataSize = buf.size();
    }
    p.room_data = std::make_unique<OrbisNpMatching2RoomDataInternal>();
    auto& room = *p.room_data;
    room = {};
    room.publicSlots = static_cast<u16>(rd.public_slots());
    room.privateSlots = static_cast<u16>(rd.private_slots());
    room.openPublicSlots = static_cast<u16>(rd.open_public_slots());
    room.openPrivateSlots = static_cast<u16>(rd.open_private_slots());
    room.maxSlot = static_cast<u16>(rd.max_slot());
    room.serverId = static_cast<OrbisNpMatching2ServerId>(rd.server_id());
    room.worldId = rd.world_id();
    room.lobbyId = rd.lobby_id();
    room.roomId = rd.room_id();
    room.passwdSlotMask = rd.passwd_slot_mask();
    room.joinedSlotMask = rd.joined_slot_mask();
    room.flags = rd.flags();
    room.roomGroup = p.room_groups.empty() ? nullptr : p.room_groups.data();
    room.roomGroups = p.room_groups.size();
    room.internalBinAttr = p.room_bin_attrs.empty() ? nullptr : p.room_bin_attrs.data();
    room.internalBinAttrs = p.room_bin_attrs.size();

    if (ctx.room_id != 0 && ctx.room_id != room.roomId) {
        ctx.room_cache.erase(ctx.room_id);
    }
    ctx.peers.clear();

    RoomCache& rc = ctx.room_cache[room.roomId];
    rc = RoomCache{};
    rc.num_slots = room.maxSlot;
    rc.mask_password = room.passwdSlotMask;
    rc.server_id = room.serverId;
    rc.world_id = room.worldId;
    rc.lobby_id = room.lobbyId;
    rc.room_id = room.roomId;
    rc.max_slot = room.maxSlot;
    rc.public_slots = room.publicSlots;
    rc.private_slots = room.privateSlots;
    rc.open_public_slots = room.openPublicSlots;
    rc.open_private_slots = room.openPrivateSlots;
    rc.passwd_slot_mask = room.passwdSlotMask;
    rc.joined_slot_mask = room.joinedSlotMask;
    rc.flags = room.flags;
    rc.owner = resp.me_member_id() == resp.owner_member_id();
    rc.signaling_type = ORBIS_NP_MATCHING2_SIGNALING_TYPE_MESH;
    rc.signaling_main_member = static_cast<OrbisNpMatching2RoomMemberId>(resp.owner_member_id());
    rc.bin_attrs_internal.resize(p.room_bin_attrs.size());
    rc.bin_buffers.clear();
    rc.bin_buffers.reserve(p.room_bin_attrs.size());
    for (size_t i = 0; i < p.room_bin_attrs.size(); ++i) {
        const auto& a = p.room_bin_attrs[i].binAttr;
        if (a.data && a.dataSize > 0) {
            rc.bin_buffers.emplace_back(a.data, a.data + a.dataSize);
        } else {
            rc.bin_buffers.emplace_back();
        }
        auto& buf = rc.bin_buffers.back();
        auto& dst = rc.bin_attrs_internal[i];
        dst = OrbisNpMatching2RoomBinAttrInternal{};
        dst.lastUpdate = p.room_bin_attrs[i].lastUpdate;
        dst.memberId = p.room_bin_attrs[i].memberId;
        dst.binAttr.id = a.id;
        dst.binAttr.data = buf.empty() ? nullptr : buf.data();
        dst.binAttr.dataSize = buf.size();
    }
    ctx.room_id = room.roomId;
    ctx.my_member_id = static_cast<OrbisNpMatching2RoomMemberId>(resp.me_member_id());
    ctx.is_room_owner = rc.owner;
    for (const auto& g : p.room_groups) {
        rc.groups[g.id] = g;
    }
    const int member_count = resp.members_size();
    for (int i = 0; i < member_count; ++i) {
        const auto& m = resp.members(i);
        MemberCache& mc = rc.members[static_cast<OrbisNpMatching2RoomMemberId>(m.member_id())];
        mc = MemberCache{};
        mc.member_id = static_cast<OrbisNpMatching2RoomMemberId>(m.member_id());
        mc.team_id = static_cast<OrbisNpMatching2TeamId>(m.team_id());
        mc.nat_type = static_cast<OrbisNpMatching2NatType>(m.nat_type());
        mc.flag_attr = m.flag_attr();
        mc.group_id = static_cast<OrbisNpMatching2RoomGroupId>(m.group_id());
        mc.join_date = m.join_date();
        mc.addr = IpStringToAddr(m.addr());
        mc.port = Libraries::Net::sceNetHtons(static_cast<u16>(m.port()));
        std::strncpy(mc.np_id.handle.data, m.npid().c_str(), sizeof(mc.np_id.handle.data) - 1);
        mc.account_id = static_cast<Libraries::Np::OrbisNpAccountId>(m.account_id());
        mc.platform = static_cast<Libraries::Np::OrbisNpPlatformType>(m.platform());
        for (const auto& a : m.bin_attrs_internal()) {
            MemberBinCache& b = mc.bins[a.attr_id()];
            b.id = static_cast<OrbisNpMatching2AttributeId>(a.attr_id());
            b.update_date = a.update_date();
            b.data.assign(a.data().begin(), a.data().end());
        }
        if (mc.member_id != ctx.my_member_id) {
            PeerInfo peer{};
            peer.member_id = mc.member_id;
            peer.addr = mc.addr;
            peer.port = mc.port;
            std::strncpy(peer.online_id.data, m.npid().c_str(), sizeof(peer.online_id.data) - 1);
            ctx.peers[mc.member_id] = peer;
        }
    }
}

OrbisNpMatching2RoomGroup* FindPayloadGroup(CallbackPayload& p, OrbisNpMatching2RoomGroupId id) {
    for (auto& group : p.room_groups) {
        if (group.id == id) {
            return &group;
        }
    }
    return nullptr;
}

void ReserveMemberBinAttrs(CallbackPayload& p, const shadnet::CreateJoinRoomResponse& resp) {
    size_t total = 0;
    for (const auto& member : resp.members()) {
        total += static_cast<size_t>(member.bin_attrs_internal_size());
    }
    p.member_bin_attrs.reserve(total);
}

OrbisNpMatching2RoomMemberBinAttrInternal* AppendMemberBinAttrs(
    CallbackPayload& p, const shadnet::MatchingRoomMemberData& member, u64& count) {
    const size_t first = p.member_bin_attrs.size();
    count = static_cast<u64>(member.bin_attrs_internal_size());
    for (const auto& a : member.bin_attrs_internal()) {
        p.bin_buffers.emplace_back(a.data().begin(), a.data().end());
        auto& buf = p.bin_buffers.back();
        auto& dst = p.member_bin_attrs.emplace_back();
        dst = OrbisNpMatching2RoomMemberBinAttrInternal{};
        dst.lastUpdate.tick = a.update_date();
        dst.binAttr.id = static_cast<OrbisNpMatching2AttributeId>(a.attr_id());
        dst.binAttr.data = buf.empty() ? nullptr : buf.data();
        dst.binAttr.dataSize = buf.size();
    }
    return count == 0 ? nullptr : &p.member_bin_attrs[first];
}

void* BuildCreateJoinRoomPayload(ContextObject& ctx, const shadnet::CreateJoinRoomResponse& resp) {
    CallbackPayload& p = ctx.request_payload;
    p.Reset();
    BuildCreateJoinRoomPayloadCommon(ctx, resp);

    const int member_count = resp.members_size();
    p.member_data.resize(member_count);
    ReserveMemberBinAttrs(p, resp);
    for (int i = 0; i < member_count; ++i) {
        const auto& m = resp.members(i);
        auto& dst = p.member_data[i];
        dst = OrbisNpMatching2RoomMemberDataInternal{};
        dst.next = (i + 1 < member_count) ? &p.member_data[i + 1] : nullptr;
        dst.joinDate = m.join_date();
        std::strncpy(dst.npId.handle.data, m.npid().c_str(), sizeof(dst.npId.handle.data) - 1);
        dst.memberId = static_cast<OrbisNpMatching2RoomMemberId>(m.member_id());
        dst.teamId = static_cast<OrbisNpMatching2TeamId>(m.team_id());
        dst.natType = static_cast<OrbisNpMatching2NatType>(m.nat_type());
        dst.flagAttr = m.flag_attr();
        if (m.is_owner()) {
            dst.flagAttr |= ORBIS_NP_MATCHING2_ROOMMEMBER_FLAG_ATTR_OWNER;
        }
        dst.roomGroup = FindPayloadGroup(p, static_cast<OrbisNpMatching2RoomGroupId>(m.group_id()));
        dst.roomMemberBinAttrInternal =
            AppendMemberBinAttrs(p, m, dst.roomMemberBinAttrInternalNum);
    }

    p.create_join_response = std::make_unique<OrbisNpMatching2CreateJoinRoomResponse>();
    auto& out = *p.create_join_response;
    out.roomData = p.room_data.get();
    out.members.members = p.member_data.empty() ? nullptr : p.member_data.data();
    out.members.membersNum = p.member_data.size();
    out.members.me = nullptr;
    out.members.owner = nullptr;
    for (auto& md : p.member_data) {
        if (md.memberId == static_cast<OrbisNpMatching2RoomMemberId>(resp.me_member_id())) {
            out.members.me = &md;
        }
        if (md.memberId == static_cast<OrbisNpMatching2RoomMemberId>(resp.owner_member_id())) {
            out.members.owner = &md;
        }
    }

    p.request_data = p.create_join_response.get();
    return p.request_data;
}

void* BuildCreateJoinRoomPayloadA(ContextObject& ctx, const shadnet::CreateJoinRoomResponse& resp) {
    CallbackPayload& p = ctx.request_payload;
    p.Reset();
    BuildCreateJoinRoomPayloadCommon(ctx, resp);

    const int member_count = resp.members_size();
    p.member_data_a.resize(member_count);
    ReserveMemberBinAttrs(p, resp);
    for (int i = 0; i < member_count; ++i) {
        const auto& m = resp.members(i);
        auto& dst = p.member_data_a[i];
        dst = OrbisNpMatching2RoomMemberDataInternalA{};
        dst.next = (i + 1 < member_count) ? &p.member_data_a[i + 1] : nullptr;
        dst.joinDateTicks.tick = m.join_date();
        dst.user.accountId = static_cast<Libraries::Np::OrbisNpAccountId>(m.account_id());
        dst.user.platform = static_cast<Libraries::Np::OrbisNpPlatformType>(m.platform());
        std::strncpy(dst.onlineId.data, m.npid().c_str(), sizeof(dst.onlineId.data) - 1);
        dst.memberId = static_cast<OrbisNpMatching2RoomMemberId>(m.member_id());
        dst.teamId = static_cast<OrbisNpMatching2TeamId>(m.team_id());
        dst.natType = static_cast<OrbisNpMatching2NatType>(m.nat_type());
        dst.flags = m.flag_attr();
        if (m.is_owner()) {
            dst.flags |= ORBIS_NP_MATCHING2_ROOMMEMBER_FLAG_ATTR_OWNER;
        }
        dst.roomGroup = FindPayloadGroup(p, static_cast<OrbisNpMatching2RoomGroupId>(m.group_id()));
        dst.roomMemberInternalBinAttr = AppendMemberBinAttrs(p, m, dst.roomMemberInternalBinAttrs);
    }

    p.create_join_response_a = std::make_unique<OrbisNpMatching2CreateJoinRoomResponseA>();
    auto& out = *p.create_join_response_a;
    out.roomData = p.room_data.get();
    out.members.members = p.member_data_a.empty() ? nullptr : p.member_data_a.data();
    out.members.membersNum = p.member_data_a.size();
    out.members.me = nullptr;
    out.members.owner = nullptr;
    for (auto& md : p.member_data_a) {
        if (md.memberId == static_cast<OrbisNpMatching2RoomMemberId>(resp.me_member_id())) {
            out.members.me = &md;
        }
        if (md.memberId == static_cast<OrbisNpMatching2RoomMemberId>(resp.owner_member_id())) {
            out.members.owner = &md;
        }
    }

    p.request_data = p.create_join_response_a.get();
    return p.request_data;
}

void* BuildLeaveRoomPayload(ContextObject& ctx, const shadnet::LeaveRoomReply& resp) {
    CallbackPayload& p = ctx.request_payload;
    p.Reset();

    p.leave_room_response = std::make_unique<OrbisNpMatching2LeaveRoomResponse>();
    p.leave_room_response->roomId = static_cast<OrbisNpMatching2RoomId>(resp.room_id());

    if (ctx.room_id == p.leave_room_response->roomId) {
        ctx.room_cache.erase(ctx.room_id);
        ctx.peers.clear();
        ctx.room_id = 0;
        ctx.my_member_id = 0;
        ctx.is_room_owner = false;
    }

    p.request_data = p.leave_room_response.get();
    return p.request_data;
}

void* BuildGetWorldInfoListPayload(ContextObject& ctx, const shadnet::GetWorldInfoListReply& resp) {
    CallbackPayload& p = ctx.request_payload;
    p.Reset();

    p.world_list.resize(resp.worlds_size());
    for (int i = 0; i < resp.worlds_size(); ++i) {
        const auto& w = resp.worlds(i);
        auto& dst = p.world_list[i];
        dst = OrbisNpMatching2World{};
        dst.worldId = static_cast<OrbisNpMatching2WorldId>(w.world_id());
        dst.lobbiesNum = w.lobbies_num();
        dst.maxLobbyMembersNum = w.max_lobby_members();
        dst.lobbyMembersNum = w.lobby_members_num();
        dst.roomsNum = w.rooms_num();
        dst.roomMembersNum = w.room_members_num();
    }
    for (size_t i = 0; i + 1 < p.world_list.size(); ++i) {
        p.world_list[i].next = &p.world_list[i + 1];
    }

    p.world_info_response = std::make_unique<OrbisNpMatching2GetWorldInfoListResponse>();
    p.world_info_response->world = p.world_list.empty() ? nullptr : p.world_list.data();
    p.world_info_response->worldNum = p.world_list.size();

    p.request_data = p.world_info_response.get();
    return p.request_data;
}

void* BuildSearchRoomPayload(ContextObject& ctx, const shadnet::SearchRoomReply& resp) {
    CallbackPayload& p = ctx.request_payload;
    p.Reset();

    const int room_count = resp.rooms_size();
    p.room_data_external.resize(room_count);
    for (int i = 0; i < room_count; ++i) {
        const auto& r = resp.rooms(i);
        auto& dst = p.room_data_external[i];
        dst = OrbisNpMatching2RoomDataExternal{};
        dst.next = (i + 1 < room_count) ? &p.room_data_external[i + 1] : nullptr;
        dst.maxSlot = static_cast<u16>(r.max_slot());
        dst.curMembers = static_cast<u16>(r.cur_members());
        dst.flags = r.flags();
        dst.serverId = static_cast<OrbisNpMatching2ServerId>(r.server_id());
        dst.worldId = static_cast<OrbisNpMatching2WorldId>(r.world_id());
        dst.lobbyId = r.lobby_id();
        dst.roomId = r.room_id();
        dst.passwdSlotMask = r.passwd_slot_mask();
        dst.joinedSlotMask = r.joined_slot_mask();
        dst.publicSlots = static_cast<u16>(r.public_slots());
        dst.privateSlots = static_cast<u16>(r.private_slots());
        dst.openPublicSlots = static_cast<u16>(r.open_public_slots());
        dst.openPrivateSlots = static_cast<u16>(r.open_private_slots());

        if (!r.owner_npid().empty()) {
            auto& npid = p.ext_owner_npids.emplace_back();
            npid = Libraries::Np::OrbisNpId{};
            std::strncpy(npid.handle.data, r.owner_npid().c_str(), sizeof(npid.handle.data) - 1);
            dst.ownerNpId = &npid;
        }

        if (r.groups_size() > 0) {
            OrbisNpMatching2RoomGroupInfo* first = nullptr;
            for (int g = 0; g < r.groups_size(); ++g) {
                const auto& src = r.groups(g);
                auto& gi = p.ext_room_groups.emplace_back();
                gi = OrbisNpMatching2RoomGroupInfo{};
                gi.id = static_cast<OrbisNpMatching2RoomGroupId>(src.group_id());
                gi.hasPasswd = src.has_passwd();
                gi.slots = src.slot_count();
                gi.groupMembers = src.num_members();
                if (!first) {
                    first = &gi;
                }
            }
            dst.roomGroup = first;
            dst.roomGroups = static_cast<u64>(r.groups_size());
        }

        if (r.external_search_int_attrs_size() > 0) {
            OrbisNpMatching2IntAttr* first = nullptr;
            for (int a = 0; a < r.external_search_int_attrs_size(); ++a) {
                const auto& src = r.external_search_int_attrs(a);
                auto& ia = p.ext_int_attrs.emplace_back();
                ia = OrbisNpMatching2IntAttr{};
                ia.id = static_cast<OrbisNpMatching2AttributeId>(src.attr_id());
                ia.num = src.attr_value();
                if (!first) {
                    first = &ia;
                }
            }
            dst.externalSearchIntAttr = first;
            dst.externalSearchIntAttrs = static_cast<u64>(r.external_search_int_attrs_size());
        }

        auto build_bin = [&](const auto& src_attrs, OrbisNpMatching2BinAttr*& out_ptr,
                             u64& out_num) {
            if (src_attrs.empty()) {
                return;
            }
            OrbisNpMatching2BinAttr* first = nullptr;
            for (const auto& src : src_attrs) {
                p.bin_buffers.emplace_back(src.data().begin(), src.data().end());
                auto& buf = p.bin_buffers.back();
                auto& ba = p.ext_bin_attrs.emplace_back();
                ba = OrbisNpMatching2BinAttr{};
                ba.id = static_cast<OrbisNpMatching2AttributeId>(src.attr_id());
                ba.data = buf.empty() ? nullptr : buf.data();
                ba.dataSize = buf.size();
                if (!first) {
                    first = &ba;
                }
            }
            out_ptr = first;
            out_num = static_cast<u64>(src_attrs.size());
        };
        build_bin(r.external_search_bin_attrs(), dst.externalSearchBinAttr,
                  dst.externalSearchBinAttrs);
        build_bin(r.external_bin_attrs(), dst.externalBinAttr, dst.externalBinAttrs);
    }

    p.search_room_response = std::make_unique<OrbisNpMatching2SearchRoomResponse>();
    auto& out = *p.search_room_response;
    out = OrbisNpMatching2SearchRoomResponse{};
    out.range.start = resp.range_start();
    out.range.total = resp.range_total();
    out.range.results = resp.range_result();
    out.roomDataExt = p.room_data_external.empty() ? nullptr : p.room_data_external.data();

    p.request_data = p.search_room_response.get();
    return p.request_data;
}

void* BuildGetRoomDataInternalPayload(ContextObject& ctx, OrbisNpMatching2RoomId room_id) {
    const auto rc_it = ctx.room_cache.find(room_id);
    if (rc_it == ctx.room_cache.end()) {
        return nullptr;
    }
    const RoomCache& rc = rc_it->second;

    CallbackPayload& p = ctx.request_payload;
    p.Reset();

    p.room_groups.clear();
    p.room_groups.reserve(rc.groups.size());
    for (const auto& [gid, g] : rc.groups) {
        p.room_groups.push_back(g);
    }

    p.room_bin_attrs.resize(rc.bin_attrs_internal.size());
    p.bin_buffers.clear();
    p.bin_buffers.reserve(rc.bin_attrs_internal.size());
    for (size_t i = 0; i < rc.bin_attrs_internal.size(); ++i) {
        const auto& src = rc.bin_attrs_internal[i];
        const auto* data = static_cast<const u8*>(src.binAttr.data);
        if (data && src.binAttr.dataSize > 0) {
            p.bin_buffers.emplace_back(data, data + src.binAttr.dataSize);
        } else {
            p.bin_buffers.emplace_back();
        }
        auto& buf = p.bin_buffers.back();
        auto& dst = p.room_bin_attrs[i];
        dst = OrbisNpMatching2RoomBinAttrInternal{};
        dst.lastUpdate = src.lastUpdate;
        dst.binAttr.id = src.binAttr.id;
        dst.binAttr.data = buf.empty() ? nullptr : buf.data();
        dst.binAttr.dataSize = buf.size();
    }

    p.room_data = std::make_unique<OrbisNpMatching2RoomDataInternal>();
    auto& room = *p.room_data;
    room = {};
    room.publicSlots = rc.public_slots;
    room.privateSlots = rc.private_slots;
    room.openPublicSlots = rc.open_public_slots;
    room.openPrivateSlots = rc.open_private_slots;
    room.maxSlot = rc.max_slot;
    room.serverId = rc.server_id;
    room.worldId = rc.world_id;
    room.lobbyId = rc.lobby_id;
    room.roomId = rc.room_id;
    room.passwdSlotMask = rc.passwd_slot_mask;
    room.joinedSlotMask = rc.joined_slot_mask;
    room.flags = rc.flags;
    room.roomGroup = p.room_groups.empty() ? nullptr : p.room_groups.data();
    room.roomGroups = p.room_groups.size();
    room.internalBinAttr = p.room_bin_attrs.empty() ? nullptr : p.room_bin_attrs.data();
    room.internalBinAttrs = p.room_bin_attrs.size();

    p.member_data.clear();
    p.member_data.reserve(rc.members.size());
    for (const auto& [member_id, mc] : rc.members) {
        auto& dst = p.member_data.emplace_back();
        dst = OrbisNpMatching2RoomMemberDataInternal{};
        dst.joinDate = mc.join_date;
        dst.npId = mc.np_id;
        dst.memberId = mc.member_id;
        dst.teamId = mc.team_id;
        dst.natType = mc.nat_type;
        dst.flagAttr = mc.flag_attr;
    }
    for (size_t i = 0; i < p.member_data.size(); ++i) {
        p.member_data[i].next = (i + 1 < p.member_data.size()) ? &p.member_data[i + 1] : nullptr;
    }

    p.create_join_response = std::make_unique<OrbisNpMatching2CreateJoinRoomResponse>();
    auto& out = *p.create_join_response;
    out = OrbisNpMatching2CreateJoinRoomResponse{};
    out.roomData = p.room_data.get();
    out.members.members = p.member_data.empty() ? nullptr : p.member_data.data();
    out.members.membersNum = p.member_data.size();
    for (auto& md : p.member_data) {
        if (md.memberId == ctx.my_member_id) {
            out.members.me = &md;
        }
        if (md.flagAttr & ORBIS_NP_MATCHING2_ROOMMEMBER_FLAG_ATTR_OWNER) {
            out.members.owner = &md;
        }
    }

    p.request_data = p.create_join_response.get();
    return p.request_data;
}

ContextManager& ContextManager::Instance() {
    static ContextManager instance;
    return instance;
}

ContextObject* ContextManager::GetLocked(OrbisNpMatching2ContextId ctx_id) {
    if (ctx_id == 0 || ctx_id > kMaxContexts || !m_used[ctx_id]) {
        return nullptr;
    }
    return &m_contexts[ctx_id];
}

s32 ContextManager::CreateContext(const OrbisNpId* owner_np_id, OrbisNpServiceLabel service_label,
                                  OrbisNpMatching2ContextId* out_ctx_id) {
    if (!out_ctx_id) {
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    std::lock_guard lock(m_mutex);

    OrbisNpMatching2ContextId id = 0;
    for (u32 i = 0; i < kMaxContexts; ++i) {
        const OrbisNpMatching2ContextId candidate =
            static_cast<OrbisNpMatching2ContextId>(((m_next_id - 1 + i) % kMaxContexts) + 1);
        if (!m_used[candidate]) {
            id = candidate;
            break;
        }
    }
    if (id == 0) {
        return ORBIS_NP_MATCHING2_ERROR_CONTEXT_MAX;
    }
    m_next_id = static_cast<OrbisNpMatching2ContextId>((id % kMaxContexts) + 1);

    ContextObject& ctx = m_contexts[id];
    ctx.Reset();
    ctx.ctx_id = id;
    ctx.service_label = service_label;
    if (owner_np_id) {
        ctx.owner_np_id = *owner_np_id;
        ctx.online_id = owner_np_id->handle;
    }
    ctx.context_callback = m_pending_context_callback;
    ctx.context_callback_arg = m_pending_context_callback_arg;
    m_used[id] = true;

    *out_ctx_id = id;
    LOG_DEBUG(Lib_NpMatching2, "context created: id={} online_id={} serviceLabel={:#x}", id,
              ctx.online_id.data, service_label);
    return ORBIS_OK;
}

bool ContextManager::Check(OrbisNpMatching2ContextId ctx_id) {
    std::lock_guard lock(m_mutex);
    return GetLocked(ctx_id) != nullptr;
}

ContextObject* ContextManager::Get(OrbisNpMatching2ContextId ctx_id) {
    std::lock_guard lock(m_mutex);
    return GetLocked(ctx_id);
}

bool ContextManager::Destroy(OrbisNpMatching2ContextId ctx_id) {
    std::lock_guard lock(m_mutex);
    if (!GetLocked(ctx_id)) {
        return false;
    }
    m_contexts[ctx_id].Reset();
    m_used[ctx_id] = false;
    LOG_DEBUG(Lib_NpMatching2, "context destroyed: id={}", ctx_id);
    return true;
}

s32 ContextManager::Start(OrbisNpMatching2ContextId ctx_id) {
    std::lock_guard lock(m_mutex);
    ContextObject* ctx = GetLocked(ctx_id);
    if (!ctx) {
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }
    if (ctx->started) {
        return ORBIS_NP_MATCHING2_ERROR_CONTEXT_ALREADY_STARTED;
    }
    ctx->started = true;
    LOG_DEBUG(Lib_NpMatching2, "context started: id={}", ctx_id);
    return ORBIS_OK;
}

s32 ContextManager::Stop(OrbisNpMatching2ContextId ctx_id) {
    std::lock_guard lock(m_mutex);
    ContextObject* ctx = GetLocked(ctx_id);
    if (!ctx) {
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }
    if (!ctx->started) {
        return ORBIS_NP_MATCHING2_ERROR_CONTEXT_NOT_STARTED;
    }
    ctx->started = false;
    LOG_DEBUG(Lib_NpMatching2, "context stopped: id={}", ctx_id);
    return ORBIS_OK;
}

void ContextManager::ApplyContextCallback(OrbisNpMatching2ContextCallback callback, void* arg) {
    std::lock_guard lock(m_mutex);
    m_pending_context_callback = callback;
    m_pending_context_callback_arg = arg;
    for (u32 id = 1; id <= kMaxContexts; ++id) {
        if (m_used[id]) {
            m_contexts[id].context_callback = callback;
            m_contexts[id].context_callback_arg = arg;
        }
    }
}

void ContextManager::Reset() {
    std::lock_guard lock(m_mutex);
    for (auto& ctx : m_contexts) {
        ctx.Reset();
    }
    m_used = {};
    m_next_id = 1;
    m_pending_context_callback = nullptr;
    m_pending_context_callback_arg = nullptr;
}

OrbisNpMatching2RequestId AllocRequestId() {
    std::lock_guard lock(g_state.mutex);
    const OrbisNpMatching2RequestId id = g_state.next_request_id++;
    if (g_state.next_request_id == 0) {
        g_state.next_request_id = 1;
    }
    return id;
}

static bool g_initialized = false;

bool IsInitialized() {
    return g_initialized;
}

void SetInitialized(bool initialized) {
    g_initialized = initialized;
}

void StoreRequestCallback(ContextObject* ctx, const OrbisNpMatching2RequestOptParam* requestOpt) {
    if (requestOpt && requestOpt->callback) {
        ctx->default_request_callback = requestOpt->callback;
        ctx->default_request_callback_arg = requestOpt->arg;
    }
}

namespace {

void FireEvent(const PendingEvent& ev) {
    ContextObject* ctx = ContextManager::Instance().Get(ev.ctx_id);
    if (!ctx) {
        LOG_WARNING(Lib_NpMatching2, "FireEvent type={} dropped: ctx={} not found",
                    static_cast<int>(ev.type), ev.ctx_id);
        return;
    }
    switch (ev.type) {
    case PendingEvent::CONTEXT_CB:
        if (ctx->context_callback) {
            LOG_DEBUG(Lib_NpMatching2, "callback CONTEXT ctx={} event={:#x} cause={} err={:#x}",
                      ev.ctx_id, static_cast<u16>(ev.ctx_event),
                      static_cast<u8>(ev.ctx_event_cause), ev.error_code);
            ctx->context_callback(ev.ctx_id, ev.ctx_event, ev.ctx_event_cause, ev.error_code,
                                  ctx->context_callback_arg);
        } else {
            LOG_WARNING(Lib_NpMatching2, "callback CONTEXT ctx={} event={:#x} SKIPPED: no callback",
                        ev.ctx_id, static_cast<u16>(ev.ctx_event));
        }
        break;
    case PendingEvent::REQUEST_CB:
        if (ev.request_cb) {
            LOG_DEBUG(Lib_NpMatching2,
                      "callback REQUEST ctx={} reqId={} event={:#x} err={:#x} data={}", ev.ctx_id,
                      ev.req_id, static_cast<u16>(ev.req_event), ev.error_code,
                      fmt::ptr(ev.request_data));
            ev.request_cb(ev.ctx_id, ev.req_id, ev.req_event, ev.error_code, ev.request_data,
                          ev.request_cb_arg);
        } else {
            LOG_WARNING(Lib_NpMatching2,
                        "callback REQUEST ctx={} reqId={} event={:#x} SKIPPED: no callback",
                        ev.ctx_id, ev.req_id, static_cast<u16>(ev.req_event));
        }
        break;
    case PendingEvent::SIGNALING_CB:
        if (ctx->signaling_callback) {
            LOG_DEBUG(Lib_NpMatching2,
                      "callback SIGNALING ctx={} room={} member={} event={:#x} err={:#x}",
                      ev.ctx_id, ev.room_id, ev.member_id, static_cast<u16>(ev.sig_event),
                      ev.error_code);
            ctx->signaling_callback(ev.ctx_id, ev.room_id, ev.member_id, ev.sig_event,
                                    ev.error_code, ctx->signaling_callback_arg);
        } else {
            LOG_WARNING(Lib_NpMatching2,
                        "callback SIGNALING ctx={} room={} event={:#x} SKIPPED: no callback",
                        ev.ctx_id, ev.room_id, static_cast<u16>(ev.sig_event));
        }
        break;
    case PendingEvent::ROOM_EVENT_CB:
        if (ctx->room_event_callback) {
            LOG_DEBUG(Lib_NpMatching2, "callback ROOM_EVENT ctx={} room={} event={:#x} data={}",
                      ev.ctx_id, ev.room_id, static_cast<u16>(ev.room_event),
                      fmt::ptr(ev.room_event_data));
            ctx->room_event_callback(ev.ctx_id, ev.room_id, ev.room_event, ev.room_event_data,
                                     ctx->room_event_callback_arg);
        } else {
            LOG_WARNING(Lib_NpMatching2,
                        "callback ROOM_EVENT ctx={} room={} event={:#x} SKIPPED: no callback",
                        ev.ctx_id, ev.room_id, static_cast<u16>(ev.room_event));
        }
        break;
    case PendingEvent::LOBBY_EVENT_CB:
        if (ctx->lobby_event_callback) {
            LOG_DEBUG(Lib_NpMatching2, "callback LOBBY_EVENT ctx={} lobby={} event={:#x} data={}",
                      ev.ctx_id, ev.lobby_id, static_cast<u16>(ev.lobby_event),
                      fmt::ptr(ev.lobby_event_data));
            ctx->lobby_event_callback(ev.ctx_id, ev.lobby_id, ev.lobby_event, ev.lobby_event_data,
                                      ctx->lobby_event_callback_arg);
        } else {
            LOG_WARNING(Lib_NpMatching2,
                        "callback LOBBY_EVENT ctx={} lobby={} event={:#x} SKIPPED: no callback",
                        ev.ctx_id, ev.lobby_id, static_cast<u16>(ev.lobby_event));
        }
        break;
    case PendingEvent::LOBBY_MESSAGE_CB:
        if (ctx->lobby_message_callback) {
            LOG_DEBUG(Lib_NpMatching2, "callback LOBBY_MESSAGE ctx={} lobby={} src={} event={:#x}",
                      ev.ctx_id, ev.lobby_id, ev.src_member_id, static_cast<u16>(ev.msg_event));
            ctx->lobby_message_callback(ev.ctx_id, ev.lobby_id, ev.src_member_id, ev.msg_event,
                                        ev.message_data, ctx->lobby_message_callback_arg);
        } else {
            LOG_WARNING(Lib_NpMatching2,
                        "callback LOBBY_MESSAGE ctx={} lobby={} SKIPPED: no callback", ev.ctx_id,
                        ev.lobby_id);
        }
        break;
    case PendingEvent::ROOM_MESSAGE_CB:
        if (ctx->room_message_callback) {
            LOG_DEBUG(Lib_NpMatching2, "callback ROOM_MESSAGE ctx={} room={} src={} event={:#x}",
                      ev.ctx_id, ev.room_id, ev.src_member_id, static_cast<u16>(ev.msg_event));
            ctx->room_message_callback(ev.ctx_id, ev.room_id, ev.src_member_id, ev.msg_event,
                                       ev.message_data, ctx->room_message_callback_arg);
        } else {
            LOG_WARNING(Lib_NpMatching2,
                        "callback ROOM_MESSAGE ctx={} room={} SKIPPED: no callback", ev.ctx_id,
                        ev.room_id);
        }
        break;
    }
}

} // namespace

PS4_SYSV_ABI void* EventDispatcherThreadMain(void*) {
    Common::SetCurrentThreadName("Matching2:Dispatch");
    std::unique_lock lock(g_state.queue_mutex);
    while (g_state.dispatch_running.load()) {
        const auto now = std::chrono::steady_clock::now();

        std::vector<PendingEvent> ready;
        auto nearest = std::chrono::steady_clock::time_point::max();
        for (auto it = g_state.pending_events.begin(); it != g_state.pending_events.end();) {
            if (it->fire_at <= now) {
                ready.push_back(std::move(*it));
                it = g_state.pending_events.erase(it);
            } else {
                nearest = std::min(nearest, it->fire_at);
                ++it;
            }
        }

        if (!ready.empty()) {
            lock.unlock();
            for (const PendingEvent& ev : ready) {
                FireEvent(ev);
            }
            lock.lock();
            continue;
        }

        if (nearest == std::chrono::steady_clock::time_point::max()) {
            g_state.queue_cv.wait(lock);
        } else {
            g_state.queue_cv.wait_until(lock, nearest);
        }
    }
    return nullptr;
}

void InitEventDispatcher() {
    if (g_state.dispatch_running.exchange(true)) {
        return;
    }
    const s32 rc = NpCommon::sceNpCreateThread(
        &g_state.dispatch_thread, EventDispatcherThreadMain, nullptr,
        Libraries::Kernel::ORBIS_KERNEL_PRIO_FIFO_DEFAULT,
        ORBIS_NP_MATCHING2_THREAD_STACK_SIZE_DEFAULT, 0, "SceNpMatching2Ex");
    if (rc < 0) {
        LOG_ERROR(Lib_NpMatching2, "failed to create event dispatcher thread: {:#x}", rc);
        g_state.dispatch_running.store(false);
        g_state.dispatch_thread = nullptr;
        return;
    }
    LOG_DEBUG(Lib_NpMatching2, "event dispatcher thread created");
}

void TermEventDispatcher() {
    if (!g_state.dispatch_running.exchange(false)) {
        return;
    }
    g_state.queue_cv.notify_all();
    if (g_state.dispatch_thread) {
        NpCommon::sceNpJoinThread(g_state.dispatch_thread, nullptr);
        g_state.dispatch_thread = nullptr;
    }
    std::lock_guard lock(g_state.queue_mutex);
    g_state.pending_events.clear();
}

void ScheduleEvent(PendingEvent ev) {
    {
        std::lock_guard lock(g_state.queue_mutex);
        g_state.pending_events.push_back(std::move(ev));
    }
    g_state.queue_cv.notify_all();
}

} // namespace Libraries::Np::NpMatching2
