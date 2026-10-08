// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#include <utility>
#include <fmt/format.h>

#include "common/logging/log.h"
#include "common/thread.h"
#include "core/libraries/net/net.h"
#include "core/libraries/np/np_common.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_handler.h"
#include "core/libraries/np/np_matching2/np_matching2_internal.h"
#include "shadnet.pb.h"

namespace Libraries::Np::NpMatching2 {

namespace {

u32 IpStringToAddr(std::string_view ip) {
    u32 a, b, c, d;
    if (std::sscanf(std::string(ip).c_str(), "%u.%u.%u.%u", &a, &b, &c, &d) != 4) {
        return 0;
    }
    return a | (b << 8) | (c << 16) | (d << 24);
}

std::string HexPreview(const void* data, size_t size, size_t max_bytes = 32) {
    if (!data || size == 0) {
        return {};
    }

    const auto* bytes = static_cast<const u8*>(data);
    const size_t count = std::min(size, max_bytes);
    std::string out;
    out.reserve(count * 3 + 4);
    for (size_t i = 0; i < count; ++i) {
        char buf[4]{};
        std::snprintf(buf, sizeof(buf), "%02x", bytes[i]);
        if (i != 0) {
            out.push_back(' ');
        }
        out.append(buf);
    }
    if (size > count) {
        out.append(" ...");
    }
    return out;
}

std::string HexPreview(std::string_view data, size_t max_bytes = 32) {
    return HexPreview(data.data(), data.size(), max_bytes);
}

template <typename Reply>
void ReserveExternalRoomPayloadStorage(CallbackPayload& p, const Reply& resp) {
    size_t groups = 0;
    size_t int_attrs = 0;
    size_t bin_attrs = 0;
    size_t bin_buffers = 0;
    size_t owners = 0;

    for (int i = 0; i < resp.rooms_size(); ++i) {
        const auto& r = resp.rooms(i);
        groups += static_cast<size_t>(r.groups_size());
        int_attrs += static_cast<size_t>(r.external_search_int_attrs_size());
        const size_t room_bin_attrs = static_cast<size_t>(r.external_search_bin_attrs_size()) +
                                      static_cast<size_t>(r.external_bin_attrs_size());
        bin_attrs += room_bin_attrs;
        bin_buffers += room_bin_attrs;
        if (!r.owner_npid().empty()) {
            ++owners;
        }
    }

    p.ext_room_groups.reserve(groups);
    p.ext_int_attrs.reserve(int_attrs);
    p.ext_bin_attrs.reserve(bin_attrs);
    p.bin_buffers.reserve(bin_buffers);
    p.ext_owner_npids.reserve(owners);
}

} // namespace

void BuildCreateJoinRoomPayloadCommon(ContextObject& ctx, CallbackPayload& p,
                                      const shadnet::CreateJoinRoomResponse& resp) {
    const auto& rd = resp.room_data();
    auto cache = NpHandler::GetInstance().LockMatching2Cache(ctx.ctx_id);

    LOG_DEBUG(Lib_NpMatching2,
              "Room payload: room={} me={} owner={} max={} public/open={}/{} "
              "private/open={}/{} flags={:#x} passwdMask={:#x} joinedMask={:#x} groups={} "
              "internalBinAttrs={} members={}",
              rd.room_id(), resp.me_member_id(), resp.owner_member_id(), rd.max_slot(),
              rd.public_slots(), rd.open_public_slots(), rd.private_slots(),
              rd.open_private_slots(), rd.flags(), rd.passwd_slot_mask(), rd.joined_slot_mask(),
              rd.groups_size(), rd.bin_attrs_internal_size(), resp.members_size());

    p.room_groups.resize(rd.groups_size());
    for (int i = 0; i < rd.groups_size(); ++i) {
        const auto& g = rd.groups(i);
        LOG_DEBUG(Lib_NpMatching2,
                  "  roomGroup[{}] id={} slots={} members={} hasPasswd={} hasLabel={} labelSize={}",
                  i, g.group_id(), g.slot_count(), g.num_members(), g.has_passwd(), g.has_label(),
                  g.label().size());
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
        LOG_DEBUG(Lib_NpMatching2,
                  "  roomInternalBin[{}] id={:#x} updateMember={} updateDate={} size={} data={}", i,
                  a.attr_id(), a.update_member_id(), a.update_date(), a.data().size(),
                  HexPreview(a.data(), 128));
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
        cache->rooms.erase(ctx.room_id);
    }
    cache->peers.clear();

    RoomCache& rc = cache->rooms[room.roomId];
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
        LOG_DEBUG(Lib_NpMatching2,
                  "  member[{}] id={} npid='{}' owner={} team={} nat={} flags={:#x} group={} "
                  "addr={}:{} account={} platform={} bins={}",
                  i, m.member_id(), m.npid(), m.is_owner(), m.team_id(), m.nat_type(),
                  m.flag_attr(), m.group_id(), m.addr(), m.port(), m.account_id(), m.platform(),
                  m.bin_attrs_internal_size());
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
        SetNpId(mc.np_id, m.npid());
        mc.account_id = static_cast<Libraries::Np::OrbisNpAccountId>(m.account_id());
        mc.platform = static_cast<Libraries::Np::OrbisNpPlatformType>(m.platform());
        for (const auto& a : m.bin_attrs_internal()) {
            LOG_INFO(Lib_NpMatching2, "    memberBin id={:#x} updateDate={} size={} data={}",
                     a.attr_id(), a.update_date(), a.data().size(), HexPreview(a.data(), 128));
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
            SetNpOnlineId(peer.online_id, m.npid());
            cache->peers[mc.member_id] = peer;
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

void* BuildCreateJoinRoomPayload(ContextObject& ctx, CallbackPayload& p,
                                 const shadnet::CreateJoinRoomResponse& resp) {
    p.Reset();
    BuildCreateJoinRoomPayloadCommon(ctx, p, resp);

    const int member_count = resp.members_size();
    p.member_data.resize(member_count);
    ReserveMemberBinAttrs(p, resp);
    for (int i = 0; i < member_count; ++i) {
        const auto& m = resp.members(i);
        auto& dst = p.member_data[i];
        dst = OrbisNpMatching2RoomMemberDataInternal{};
        dst.next = (i + 1 < member_count) ? &p.member_data[i + 1] : nullptr;
        dst.joinDate = m.join_date();
        SetNpId(dst.npId, m.npid());
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

void* BuildCreateJoinRoomPayloadA(ContextObject& ctx, CallbackPayload& p,
                                  const shadnet::CreateJoinRoomResponse& resp) {
    p.Reset();
    BuildCreateJoinRoomPayloadCommon(ctx, p, resp);

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
        SetNpOnlineId(dst.onlineId, m.npid());
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

void* BuildLeaveRoomPayload(ContextObject& ctx, CallbackPayload& p,
                            const shadnet::LeaveRoomReply& resp) {
    p.Reset();

    p.leave_room_response = std::make_unique<OrbisNpMatching2LeaveRoomResponse>();
    p.leave_room_response->roomId = static_cast<OrbisNpMatching2RoomId>(resp.room_id());

    if (ctx.room_id == p.leave_room_response->roomId) {
        auto cache = NpHandler::GetInstance().LockMatching2Cache(ctx.ctx_id);
        cache->rooms.erase(ctx.room_id);
        cache->peers.clear();
        ctx.room_id = 0;
        ctx.my_member_id = 0;
        ctx.is_room_owner = false;
    }

    p.request_data = p.leave_room_response.get();
    return p.request_data;
}

void* BuildGetWorldInfoListPayload(ContextObject& ctx, CallbackPayload& p,
                                   const shadnet::GetWorldInfoListReply& resp) {
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

void* BuildSearchRoomPayload(ContextObject& ctx, CallbackPayload& p,
                             const shadnet::SearchRoomReply& resp) {
    p.Reset();
    ReserveExternalRoomPayloadStorage(p, resp);

    const int room_count = resp.rooms_size();
    LOG_INFO(Lib_NpMatching2, "Search response rooms={}", room_count);
    p.room_data_external.resize(room_count);
    for (int i = 0; i < room_count; ++i) {
        const auto& r = resp.rooms(i);
        LOG_DEBUG(Lib_NpMatching2,
                  "  room[{}] id={} max={} cur={} public/open={}/{} private/open={}/{} "
                  "flags={:#x} groups={} searchInt={} searchBin={} extBin={}",
                  i, r.room_id(), r.max_slot(), r.cur_members(), r.public_slots(),
                  r.open_public_slots(), r.private_slots(), r.open_private_slots(), r.flags(),
                  r.groups_size(), r.external_search_int_attrs_size(),
                  r.external_search_bin_attrs_size(), r.external_bin_attrs_size());
        for (int a = 0; a < r.external_search_int_attrs_size(); ++a) {
            const auto& src = r.external_search_int_attrs(a);
            LOG_DEBUG(Lib_NpMatching2, "    searchInt[{}] id={:#x} value={}", a, src.attr_id(),
                      src.attr_value());
        }
        for (int a = 0; a < r.external_search_bin_attrs_size(); ++a) {
            const auto& src = r.external_search_bin_attrs(a);
            LOG_DEBUG(Lib_NpMatching2, "    searchBin[{}] id={:#x} size={} data={}", a,
                      src.attr_id(), src.data().size(), HexPreview(src.data(), 128));
        }
        for (int a = 0; a < r.external_bin_attrs_size(); ++a) {
            const auto& src = r.external_bin_attrs(a);
            LOG_DEBUG(Lib_NpMatching2, "    extBin[{}] id={:#x} size={} data={}", a, src.attr_id(),
                      src.data().size(), HexPreview(src.data(), 128));
        }
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
            SetNpId(npid, r.owner_npid());
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

void* BuildSearchRoomPayloadA(ContextObject& ctx, CallbackPayload& p,
                              const shadnet::SearchRoomReply& resp) {
    p.Reset();
    ReserveExternalRoomPayloadStorage(p, resp);

    const int room_count = resp.rooms_size();
    LOG_INFO(Lib_NpMatching2, "Search response rooms={} range start={} total={} results={}",
             room_count, resp.range_start(), resp.range_total(), resp.range_result());
    p.room_data_external_a.resize(room_count);
    for (int i = 0; i < room_count; ++i) {
        const auto& r = resp.rooms(i);
        LOG_DEBUG(Lib_NpMatching2,
                  "  roomA[{}] id={} max={} cur={} public/open={}/{} private/open={}/{} "
                  "flags={:#x} groups={} searchInt={} searchBin={} extBin={} owner='{}' "
                  "account={} platform={}",
                  i, r.room_id(), r.max_slot(), r.cur_members(), r.public_slots(),
                  r.open_public_slots(), r.private_slots(), r.open_private_slots(), r.flags(),
                  r.groups_size(), r.external_search_int_attrs_size(),
                  r.external_search_bin_attrs_size(), r.external_bin_attrs_size(), r.owner_npid(),
                  r.owner_account_id(), r.owner_platform());
        for (int a = 0; a < r.external_search_int_attrs_size(); ++a) {
            const auto& src = r.external_search_int_attrs(a);
            LOG_DEBUG(Lib_NpMatching2, "    searchIntA[{}] id={:#x} value={}", a, src.attr_id(),
                      src.attr_value());
        }
        for (int a = 0; a < r.external_search_bin_attrs_size(); ++a) {
            const auto& src = r.external_search_bin_attrs(a);
            LOG_DEBUG(Lib_NpMatching2, "    searchBinA[{}] id={:#x} size={} data={}", a,
                      src.attr_id(), src.data().size(), HexPreview(src.data(), 128));
        }
        for (int a = 0; a < r.external_bin_attrs_size(); ++a) {
            const auto& src = r.external_bin_attrs(a);
            LOG_DEBUG(Lib_NpMatching2, "    extBinA[{}] id={:#x} size={} data={}", a, src.attr_id(),
                      src.data().size(), HexPreview(src.data(), 128));
        }
        auto& dst = p.room_data_external_a[i];
        dst = OrbisNpMatching2RoomDataExternalA{};
        dst.next = (i + 1 < room_count) ? &p.room_data_external_a[i + 1] : nullptr;
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
            SetNpOnlineId(dst.ownerOnlineId, r.owner_npid());
        }
        dst.owner.accountId = static_cast<Libraries::Np::OrbisNpAccountId>(r.owner_account_id());
        dst.owner.platform = static_cast<Libraries::Np::OrbisNpPlatformType>(r.owner_platform());

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

    p.search_room_response_a = std::make_unique<OrbisNpMatching2SearchRoomResponseA>();
    auto& out = *p.search_room_response_a;
    out = OrbisNpMatching2SearchRoomResponseA{};
    out.range.start = resp.range_start();
    out.range.total = resp.range_total();
    out.range.results = resp.range_result();
    out.roomDataExt = p.room_data_external_a.empty() ? nullptr : p.room_data_external_a.data();

    p.request_data = p.search_room_response_a.get();
    return p.request_data;
}

void* BuildGetRoomDataExternalListPayload(ContextObject& ctx, CallbackPayload& p,
                                          const shadnet::GetRoomDataExternalListReply& resp) {
    p.Reset();
    ReserveExternalRoomPayloadStorage(p, resp);

    const int room_count = resp.rooms_size();
    LOG_INFO(Lib_NpMatching2, "getRoomDataExternalList response rooms={}", room_count);
    p.room_data_external.resize(room_count);
    for (int i = 0; i < room_count; ++i) {
        const auto& r = resp.rooms(i);
        LOG_DEBUG(Lib_NpMatching2,
                  "  room[{}] id={} max={} cur={} public/open={}/{} private/open={}/{} "
                  "flags={:#x} groups={} searchInt={} searchBin={} extBin={}",
                  i, r.room_id(), r.max_slot(), r.cur_members(), r.public_slots(),
                  r.open_public_slots(), r.private_slots(), r.open_private_slots(), r.flags(),
                  r.groups_size(), r.external_search_int_attrs_size(),
                  r.external_search_bin_attrs_size(), r.external_bin_attrs_size());
        for (int a = 0; a < r.external_search_int_attrs_size(); ++a) {
            const auto& src = r.external_search_int_attrs(a);
            LOG_DEBUG(Lib_NpMatching2, "    searchInt[{}] id={:#x} value={}", a, src.attr_id(),
                      src.attr_value());
        }
        for (int a = 0; a < r.external_search_bin_attrs_size(); ++a) {
            const auto& src = r.external_search_bin_attrs(a);
            LOG_DEBUG(Lib_NpMatching2, "    searchBin[{}] id={:#x} size={} data={}", a,
                      src.attr_id(), src.data().size(), HexPreview(src.data(), 128));
        }
        for (int a = 0; a < r.external_bin_attrs_size(); ++a) {
            const auto& src = r.external_bin_attrs(a);
            LOG_DEBUG(Lib_NpMatching2, "    extBin[{}] id={:#x} size={} data={}", a, src.attr_id(),
                      src.data().size(), HexPreview(src.data(), 128));
        }
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
            SetNpId(npid, r.owner_npid());
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

    p.room_data_external_list_response =
        std::make_unique<OrbisNpMatching2GetRoomDataExternalListResponse>();
    auto& out = *p.room_data_external_list_response;
    out = OrbisNpMatching2GetRoomDataExternalListResponse{};
    out.roomDataExternal = p.room_data_external.empty() ? nullptr : p.room_data_external.data();
    out.roomDataExternalNum = static_cast<u64>(p.room_data_external.size());

    p.request_data = p.room_data_external_list_response.get();
    return p.request_data;
}

void* BuildGetRoomDataExternalListPayloadA(ContextObject& ctx, CallbackPayload& p,
                                           const shadnet::GetRoomDataExternalListReply& resp) {
    p.Reset();
    ReserveExternalRoomPayloadStorage(p, resp);

    const int room_count = resp.rooms_size();
    p.room_data_external_a.resize(room_count);
    for (int i = 0; i < room_count; ++i) {
        const auto& r = resp.rooms(i);
        auto& dst = p.room_data_external_a[i];
        dst = OrbisNpMatching2RoomDataExternalA{};
        dst.next = (i + 1 < room_count) ? &p.room_data_external_a[i + 1] : nullptr;
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
            SetNpOnlineId(dst.ownerOnlineId, r.owner_npid());
        }
        dst.owner.accountId = static_cast<Libraries::Np::OrbisNpAccountId>(r.owner_account_id());
        dst.owner.platform = static_cast<Libraries::Np::OrbisNpPlatformType>(r.owner_platform());

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

    p.room_data_external_list_response_a =
        std::make_unique<OrbisNpMatching2GetRoomDataExternalListResponseA>();
    auto& out = *p.room_data_external_list_response_a;
    out = OrbisNpMatching2GetRoomDataExternalListResponseA{};
    out.roomDataExternal = p.room_data_external_a.empty() ? nullptr : p.room_data_external_a.data();
    out.roomDataExternalNum = static_cast<u64>(p.room_data_external_a.size());

    p.request_data = p.room_data_external_list_response_a.get();
    return p.request_data;
}

void* BuildGetRoomMemberDataExternalListPayload(
    ContextObject& ctx, CallbackPayload& p,
    const shadnet::GetRoomMemberDataExternalListReply& resp) {
    p.Reset();

    const int member_count = resp.members_size();
    p.member_data_external.resize(member_count);
    for (int i = 0; i < member_count; ++i) {
        const auto& src = resp.members(i);
        auto& dst = p.member_data_external[i];
        dst = OrbisNpMatching2RoomMemberDataExternal{};
        dst.next = (i + 1 < member_count) ? &p.member_data_external[i + 1] : nullptr;
        SetNpId(dst.npId, src.npid());
        dst.joinDate.tick = src.join_date();
        dst.role = static_cast<OrbisNpMatching2Role>(src.role());
    }

    p.room_member_data_external_list_response =
        std::make_unique<OrbisNpMatching2GetRoomMemberDataExternalListResponse>();
    auto& out = *p.room_member_data_external_list_response;
    out = OrbisNpMatching2GetRoomMemberDataExternalListResponse{};
    out.roomMemberDataExternal =
        p.member_data_external.empty() ? nullptr : p.member_data_external.data();
    out.roomMemberDataExternalNum = static_cast<u64>(p.member_data_external.size());

    p.request_data = p.room_member_data_external_list_response.get();
    return p.request_data;
}

void* BuildGetRoomMemberDataExternalListPayloadA(
    ContextObject& ctx, CallbackPayload& p,
    const shadnet::GetRoomMemberDataExternalListReply& resp) {
    p.Reset();

    const int member_count = resp.members_size();
    p.member_data_external_a.resize(member_count);
    for (int i = 0; i < member_count; ++i) {
        const auto& src = resp.members(i);
        auto& dst = p.member_data_external_a[i];
        dst = OrbisNpMatching2RoomMemberDataExternalA{};
        dst.next = (i + 1 < member_count) ? &p.member_data_external_a[i + 1] : nullptr;
        dst.user.accountId = static_cast<Libraries::Np::OrbisNpAccountId>(src.account_id());
        dst.user.platform = static_cast<Libraries::Np::OrbisNpPlatformType>(src.platform());
        SetNpOnlineId(dst.onlineId, src.npid());
        dst.joinDate.tick = src.join_date();
        dst.role = static_cast<OrbisNpMatching2Role>(src.role());
    }

    p.room_member_data_external_list_response_a =
        std::make_unique<OrbisNpMatching2GetRoomMemberDataExternalListResponseA>();
    auto& out = *p.room_member_data_external_list_response_a;
    out = OrbisNpMatching2GetRoomMemberDataExternalListResponseA{};
    out.roomMemberDataExternal =
        p.member_data_external_a.empty() ? nullptr : p.member_data_external_a.data();
    out.roomMemberDataExternalNum = static_cast<u64>(p.member_data_external_a.size());

    p.request_data = p.room_member_data_external_list_response_a.get();
    return p.request_data;
}

void* BuildGetUserInfoListPayload(ContextObject& ctx, CallbackPayload& p,
                                  const shadnet::GetUserInfoListReply& resp) {
    p.Reset();

    size_t total_bin_attrs = 0;
    for (int i = 0; i < resp.users_size(); ++i) {
        total_bin_attrs += static_cast<size_t>(resp.users(i).user_bin_attrs_size());
    }
    p.user_bin_attrs.reserve(total_bin_attrs);
    p.bin_buffers.reserve(total_bin_attrs);

    const int user_count = resp.users_size();
    p.user_info.resize(user_count);
    for (int i = 0; i < user_count; ++i) {
        const auto& u = resp.users(i);
        auto& dst = p.user_info[i];
        dst = OrbisNpMatching2UserInfo{};
        dst.next = (i + 1 < user_count) ? &p.user_info[i + 1] : nullptr;
        SetNpId(dst.npId, u.npid());

        if (u.user_bin_attrs_size() > 0) {
            OrbisNpMatching2BinAttr* first = nullptr;
            for (int a = 0; a < u.user_bin_attrs_size(); ++a) {
                const auto& src = u.user_bin_attrs(a);
                p.bin_buffers.emplace_back(src.data().begin(), src.data().end());
                auto& buf = p.bin_buffers.back();
                auto& ba = p.user_bin_attrs.emplace_back();
                ba = OrbisNpMatching2BinAttr{};
                ba.id = static_cast<OrbisNpMatching2AttributeId>(src.attr_id());
                ba.data = buf.empty() ? nullptr : buf.data();
                ba.dataSize = buf.size();
                if (!first) {
                    first = &ba;
                }
            }
            dst.userBinAttr = first;
            dst.userBinAttrNum = static_cast<u64>(u.user_bin_attrs_size());
        }
    }

    p.user_info_list_response = std::make_unique<OrbisNpMatching2GetUserInfoListResponse>();
    auto& out = *p.user_info_list_response;
    out = OrbisNpMatching2GetUserInfoListResponse{};
    out.userInfo = p.user_info.empty() ? nullptr : p.user_info.data();
    out.userInfoNum = static_cast<u64>(p.user_info.size());

    p.request_data = p.user_info_list_response.get();
    return p.request_data;
}

void* BuildGetUserInfoListPayloadA(ContextObject& ctx, CallbackPayload& p,
                                   const shadnet::GetUserInfoListReply& resp) {
    p.Reset();

    size_t total_bin_attrs = 0;
    for (int i = 0; i < resp.users_size(); ++i) {
        total_bin_attrs += static_cast<size_t>(resp.users(i).user_bin_attrs_size());
    }
    p.user_bin_attrs.reserve(total_bin_attrs);
    p.bin_buffers.reserve(total_bin_attrs);

    const int user_count = resp.users_size();
    p.user_info_a.resize(user_count);
    for (int i = 0; i < user_count; ++i) {
        const auto& u = resp.users(i);
        auto& dst = p.user_info_a[i];
        dst = OrbisNpMatching2UserInfoA{};
        dst.next = (i + 1 < user_count) ? &p.user_info_a[i + 1] : nullptr;
        SetNpOnlineId(dst.userOnlineId, u.npid());
        dst.user.accountId = static_cast<Libraries::Np::OrbisNpAccountId>(u.account_id());
        dst.user.platform = static_cast<Libraries::Np::OrbisNpPlatformType>(u.platform());

        if (u.user_bin_attrs_size() > 0) {
            OrbisNpMatching2BinAttr* first = nullptr;
            for (int a = 0; a < u.user_bin_attrs_size(); ++a) {
                const auto& src = u.user_bin_attrs(a);
                p.bin_buffers.emplace_back(src.data().begin(), src.data().end());
                auto& buf = p.bin_buffers.back();
                auto& ba = p.user_bin_attrs.emplace_back();
                ba = OrbisNpMatching2BinAttr{};
                ba.id = static_cast<OrbisNpMatching2AttributeId>(src.attr_id());
                ba.data = buf.empty() ? nullptr : buf.data();
                ba.dataSize = buf.size();
                if (!first) {
                    first = &ba;
                }
            }
            dst.userBinAttr = first;
            dst.userBinAttrNum = static_cast<u64>(u.user_bin_attrs_size());
        }
    }

    p.user_info_list_response_a = std::make_unique<OrbisNpMatching2GetUserInfoListResponseA>();
    auto& out = *p.user_info_list_response_a;
    out = OrbisNpMatching2GetUserInfoListResponseA{};
    out.userInfo = p.user_info_a.empty() ? nullptr : p.user_info_a.data();
    out.userInfoNum = static_cast<u64>(p.user_info_a.size());

    p.request_data = p.user_info_list_response_a.get();
    return p.request_data;
}

void* BuildGetRoomDataInternalPayload(ContextObject& ctx, CallbackPayload& p,
                                      OrbisNpMatching2RoomId room_id) {
    auto cache = NpHandler::GetInstance().LockMatching2Cache(ctx.ctx_id);
    const auto rc_it = cache->rooms.find(room_id);
    if (rc_it == cache->rooms.end()) {
        return nullptr;
    }
    const RoomCache& rc = rc_it->second;

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

void* BuildGetRoomMemberDataInternalPayload(
    ContextObject& ctx, CallbackPayload& p,
    const OrbisNpMatching2GetRoomMemberDataInternalRequest& request) {
    auto cache = NpHandler::GetInstance().LockMatching2Cache(ctx.ctx_id);
    const auto room_it = cache->rooms.find(request.roomId);
    if (room_it == cache->rooms.end()) {
        return nullptr;
    }
    const RoomCache& room = room_it->second;
    const auto member_it = room.members.find(request.memberId);
    if (member_it == room.members.end()) {
        return nullptr;
    }
    const MemberCache& member = member_it->second;

    p.Reset();
    if (member.group_id != 0) {
        const auto group_it = room.groups.find(member.group_id);
        if (group_it != room.groups.end()) {
            p.room_groups.push_back(group_it->second);
        }
    }

    std::vector<OrbisNpMatching2AttributeId> ids;
    if (request.attrIdNum == 0) {
        for (const auto& entry : member.bins) {
            ids.push_back(entry.first);
        }
    } else {
        ids.assign(request.attrId, request.attrId + request.attrIdNum);
    }
    p.member_bin_attrs.reserve(ids.size());
    p.bin_buffers.reserve(ids.size());
    for (const OrbisNpMatching2AttributeId id : ids) {
        const auto bin_it = member.bins.find(id);
        auto& dst = p.member_bin_attrs.emplace_back();
        dst = {};
        dst.binAttr.id = id;
        if (bin_it == member.bins.end()) {
            p.bin_buffers.emplace_back(64, u8{0});
            dst.binAttr.data = p.bin_buffers.back().data();
            dst.binAttr.dataSize = 0;
            continue;
        }
        p.bin_buffers.push_back(bin_it->second.data);
        auto& data = p.bin_buffers.back();
        dst.lastUpdate.tick = bin_it->second.update_date;
        dst.binAttr.data = data.empty() ? nullptr : data.data();
        dst.binAttr.dataSize = data.size();
    }

    if (ctx.a_variant) {
        p.member_data_a.resize(1);
        auto& out_member = p.member_data_a.front();
        out_member = {};
        out_member.joinDateTicks.tick = member.join_date;
        out_member.user.accountId = member.account_id;
        out_member.user.platform = member.platform;
        out_member.onlineId = member.np_id.handle;
        out_member.memberId = member.member_id;
        out_member.teamId = member.team_id;
        out_member.natType = member.nat_type;
        out_member.flags = member.flag_attr;
        out_member.roomGroup = p.room_groups.empty() ? nullptr : p.room_groups.data();
        out_member.roomMemberInternalBinAttr =
            p.member_bin_attrs.empty() ? nullptr : p.member_bin_attrs.data();
        out_member.roomMemberInternalBinAttrs = p.member_bin_attrs.size();

        p.room_member_data_internal_response_a =
            std::make_unique<OrbisNpMatching2GetRoomMemberDataInternalResponseA>();
        p.room_member_data_internal_response_a->roomMemberDataInternal = &out_member;
        p.request_data = p.room_member_data_internal_response_a.get();
        return p.request_data;
    }

    p.member_data.resize(1);
    auto& out_member = p.member_data.front();
    out_member = {};
    out_member.joinDate = member.join_date;
    out_member.npId = member.np_id;
    out_member.memberId = member.member_id;
    out_member.teamId = member.team_id;
    out_member.natType = member.nat_type;
    out_member.flagAttr = member.flag_attr;
    out_member.roomGroup = p.room_groups.empty() ? nullptr : p.room_groups.data();
    out_member.roomMemberBinAttrInternal =
        p.member_bin_attrs.empty() ? nullptr : p.member_bin_attrs.data();
    out_member.roomMemberBinAttrInternalNum = p.member_bin_attrs.size();

    p.room_member_data_internal_response =
        std::make_unique<OrbisNpMatching2GetRoomMemberDataInternalResponse>();
    p.room_member_data_internal_response->roomMemberDataInternal = &out_member;
    p.request_data = p.room_member_data_internal_response.get();
    return p.request_data;
}

void* BuildRoomMessagePayload(CallbackPayload& p, bool a_variant, OrbisNpMatching2CastType castType,
                              const std::vector<OrbisNpMatching2RoomMemberId>& dstMembers,
                              const MemberCache* srcMember, const std::vector<u8>& msg) {
    p.Reset();

    p.room_message_dst = std::make_unique<OrbisNpMatching2RoomMessageDestination>();
    *p.room_message_dst = {};
    if (castType == ORBIS_NP_MATCHING2_CASTTYPE_UNICAST && !dstMembers.empty()) {
        p.room_message_dst->unicastTarget = dstMembers.front();
    } else if (castType == ORBIS_NP_MATCHING2_CASTTYPE_MULTICAST && !dstMembers.empty()) {
        p.room_message_multicast_members = dstMembers;
        p.room_message_dst->multicastTarget.memberId = p.room_message_multicast_members.data();
        p.room_message_dst->multicastTarget.memberIdNum = p.room_message_multicast_members.size();
    }

    p.room_message_data = msg;
    void* msg_ptr = p.room_message_data.empty() ? nullptr : p.room_message_data.data();

    LOG_INFO(Lib_NpMatching2,
             "variant={} cast={} dstN={} srcMember={} srcMid={} "
             "srcAccount={} srcPlatform={} msg={} msgLen={} preview=[{}]",
             a_variant ? "A" : "regular", castType, dstMembers.size(), fmt::ptr(srcMember),
             srcMember ? srcMember->member_id : 0, srcMember ? srcMember->account_id : 0,
             srcMember ? static_cast<s32>(srcMember->platform) : 0, fmt::ptr(msg_ptr), msg.size(),
             HexPreview(msg.data(), msg.size(), 32));

    if (a_variant) {
        p.room_message_info_a = std::make_unique<OrbisNpMatching2RoomMessageInfoA>();
        auto& info = *p.room_message_info_a;
        info = {};
        info.filtered = false;
        info.castType = castType;
        if (srcMember) {
            info.srcMember.accountId = srcMember->account_id;
            info.srcMember.platform = srcMember->platform;
            info.srcMemberOnlineId = srcMember->np_id.handle;
        }
        info.dst = p.room_message_dst.get();
        info.msg = msg_ptr;
        info.msgLen = p.room_message_data.size();
        p.room_message_callback_data = p.room_message_info_a.get();
        LOG_INFO(Lib_NpMatching2,
                 "RoomMessageInfoA: info={} dst={} srcMember={} msg={} "
                 "msgLen={} account={} platform={} onlineId='{}'",
                 fmt::ptr(&info), fmt::ptr(info.dst), fmt::ptr(&info.srcMember), fmt::ptr(info.msg),
                 info.msgLen, info.srcMember.accountId, static_cast<s32>(info.srcMember.platform),
                 srcMember ? std::string_view(srcMember->np_id.handle.data,
                                              strnlen(srcMember->np_id.handle.data,
                                                      Libraries::Np::ORBIS_NP_ONLINEID_MAX_LENGTH))
                           : std::string_view{});
        return p.room_message_callback_data;
    }

    p.room_message_src_npid = std::make_unique<Libraries::Np::OrbisNpId>();
    *p.room_message_src_npid = {};
    if (srcMember) {
        *p.room_message_src_npid = srcMember->np_id;
    }

    p.room_message_info = std::make_unique<OrbisNpMatching2RoomMessageInfo>();
    auto& info = *p.room_message_info;
    info = {};
    info.filtered = false;
    info.castType = castType;
    info.dst = p.room_message_dst.get();
    info.srcMember = p.room_message_src_npid.get();
    info.msg = msg_ptr;
    info.msgLen = static_cast<u32>(p.room_message_data.size());
    p.room_message_callback_data = p.room_message_info.get();
    LOG_DEBUG(Lib_NpMatching2, "Payload: info={} dst={} srcNpId={} msg={} msgLen={} onlineId='{}'",
              fmt::ptr(&info), fmt::ptr(info.dst), fmt::ptr(info.srcMember), fmt::ptr(info.msg),
              info.msgLen,
              srcMember ? std::string_view(srcMember->np_id.handle.data,
                                           strnlen(srcMember->np_id.handle.data,
                                                   Libraries::Np::ORBIS_NP_ONLINEID_MAX_LENGTH))
                        : std::string_view{});
    return p.room_message_callback_data;
}

std::shared_ptr<ContextObject> ContextManager::GetLocked(OrbisNpMatching2ContextId ctx_id) {
    if (ctx_id == 0 || ctx_id > kMaxContexts) {
        return {};
    }
    return m_contexts[ctx_id];
}

s32 ContextManager::CreateContext(const OrbisNpId* owner_np_id, OrbisNpServiceLabel service_label,
                                  OrbisNpMatching2ContextId* out_ctx_id, bool a_variant) {
    if (!out_ctx_id) {
        return ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT;
    }

    std::lock_guard lock(m_mutex);

    OrbisNpMatching2ContextId id = 0;
    for (u32 i = 0; i < kMaxContexts; ++i) {
        const OrbisNpMatching2ContextId candidate =
            static_cast<OrbisNpMatching2ContextId>(((m_next_id - 1 + i) % kMaxContexts) + 1);
        if (!m_contexts[candidate]) {
            id = candidate;
            break;
        }
    }
    if (id == 0) {
        return ORBIS_NP_MATCHING2_ERROR_CONTEXT_MAX;
    }
    m_next_id = static_cast<OrbisNpMatching2ContextId>((id % kMaxContexts) + 1);

    auto ctx = std::make_shared<ContextObject>();
    NpHandler::GetInstance().ResetMatching2Cache(id);
    ctx->ctx_id = id;
    ctx->a_variant = a_variant;
    ctx->service_label = service_label;
    NpHandler::GetInstance().LockMatching2Cache(id)->Reset();
    if (owner_np_id) {
        ctx->owner_np_id = *owner_np_id;
        ctx->online_id = owner_np_id->handle;
    }
    ctx->context_callback = m_pending_context_callback;
    ctx->context_callback_arg = m_pending_context_callback_arg;
    m_contexts[id] = ctx;

    *out_ctx_id = id;
    LOG_INFO(Lib_NpMatching2, "context{} created: id={} online_id={} serviceLabel={:#x}",
             a_variant ? "A" : "", id, ctx->online_id.data, service_label);
    return ORBIS_OK;
}

std::shared_ptr<ContextObject> ContextManager::Get(OrbisNpMatching2ContextId ctx_id) {
    std::lock_guard lock(m_mutex);
    return GetLocked(ctx_id);
}

bool ContextManager::Destroy(OrbisNpMatching2ContextId ctx_id) {
    std::lock_guard lock(m_mutex);
    auto ctx = GetLocked(ctx_id);
    if (!ctx) {
        return false;
    }
    if (ctx->stop_pending) {
        ctx->destroy_pending = true;
        LOG_INFO(Lib_NpMatching2, "context destroy deferred until stop callback: id={}", ctx_id);
        return true;
    }
    NpHandler::GetInstance().ResetMatching2Cache(ctx_id);
    m_contexts[ctx_id].reset();
    LOG_INFO(Lib_NpMatching2, "context destroyed: id={}", ctx_id);
    return true;
}

void ContextManager::CompleteStop(OrbisNpMatching2ContextId ctx_id,
                                  const std::shared_ptr<ContextObject>& expected_context) {
    std::lock_guard lock(m_mutex);
    auto ctx = GetLocked(ctx_id);
    if (!ctx || ctx != expected_context) {
        return;
    }
    ctx->stop_pending = false;
    if (!ctx->destroy_pending) {
        return;
    }
    NpHandler::GetInstance().ResetMatching2Cache(ctx_id);
    m_contexts[ctx_id].reset();
    LOG_INFO(Lib_NpMatching2, "context destroyed after stop callback: id={}", ctx_id);
}

s32 ContextManager::Start(OrbisNpMatching2ContextId ctx_id) {
    std::lock_guard lock(m_mutex);
    auto ctx = GetLocked(ctx_id);
    if (!ctx) {
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }
    if (ctx->started) {
        return ORBIS_NP_MATCHING2_ERROR_CONTEXT_ALREADY_STARTED;
    }
    ctx->started = true;
    ctx->stop_pending = false;
    ctx->destroy_pending = false;
    LOG_INFO(Lib_NpMatching2, "context started: id={}", ctx_id);
    return ORBIS_OK;
}

s32 ContextManager::Stop(OrbisNpMatching2ContextId ctx_id) {
    std::lock_guard lock(m_mutex);
    auto ctx = GetLocked(ctx_id);
    if (!ctx) {
        return ORBIS_NP_MATCHING2_ERROR_INVALID_CONTEXT_ID;
    }
    if (!ctx->started) {
        return ORBIS_NP_MATCHING2_ERROR_CONTEXT_NOT_STARTED;
    }
    ctx->started = false;
    ctx->stop_pending = true;
    LOG_INFO(Lib_NpMatching2, "context stopped: id={}", ctx_id);
    return ORBIS_OK;
}

void ContextManager::ApplyContextCallback(OrbisNpMatching2ContextCallback callback, void* arg) {
    std::lock_guard lock(m_mutex);
    m_pending_context_callback = callback;
    m_pending_context_callback_arg = arg;
    for (u32 id = 1; id <= kMaxContexts; ++id) {
        if (m_contexts[id]) {
            m_contexts[id]->context_callback = callback;
            m_contexts[id]->context_callback_arg = arg;
        }
    }
}

void ContextManager::Reset() {
    std::lock_guard lock(m_mutex);
    m_contexts = {};
    NpHandler::GetInstance().ResetMatching2Caches();
    m_next_id = 1;
    m_pending_context_callback = nullptr;
    m_pending_context_callback_arg = nullptr;
}

OrbisNpMatching2RequestId AllocRequestId() {
    auto& state = NpHandler::GetInstance().GetMatching2State();
    std::lock_guard lock(state.mutex);
    const OrbisNpMatching2RequestId id = state.next_request_id++;
    if (state.next_request_id == 0) {
        state.next_request_id = 1;
    }
    return id;
}

bool IsInitialized() {
    return NpHandler::GetInstance().GetMatching2State().initialized.load();
}

void SetInitialized(bool initialized) {
    NpHandler::GetInstance().GetMatching2State().initialized.store(initialized);
}

void StoreRequestCallback(const std::shared_ptr<ContextObject>& ctx,
                          const OrbisNpMatching2RequestOptParam* requestOpt) {
    ctx->per_request_callback = nullptr;
    ctx->per_request_callback_arg = nullptr;
    if (requestOpt && requestOpt->callback) {
        ctx->per_request_callback = requestOpt->callback;
        ctx->per_request_callback_arg = requestOpt->arg;
    }
}

RequestCallbackInfo ConsumeRequestCallback(const std::shared_ptr<ContextObject>& ctx) {
    RequestCallbackInfo cb{};
    if (ctx->per_request_callback) {
        cb.callback = ctx->per_request_callback;
        cb.arg = ctx->per_request_callback_arg;
        ctx->per_request_callback = nullptr;
        ctx->per_request_callback_arg = nullptr;
        return cb;
    }

    cb.callback = ctx->default_request_callback;
    cb.arg = ctx->default_request_callback_arg;
    return cb;
}

namespace {

std::string CallbackOnlineId(const Libraries::Np::OrbisNpId& npid) {
    return std::string(npid.handle.data, strnlen(npid.handle.data, ORBIS_NP_ONLINEID_MAX_LENGTH));
}

template <typename T>
void LogCallbackObject(std::string_view label, const T* object) {
    if (!object) {
        return;
    }
    LOG_INFO(Lib_NpMatching2, "Callback payload {}: ptr={} size={:#x} raw={}", label,
             fmt::ptr(object), sizeof(T), HexPreview(object, sizeof(T), sizeof(T)));
}

bool IsCreateJoinRequestEvent(OrbisNpMatching2Event event) {
    return event == ORBIS_NP_MATCHING2_REQUEST_EVENT_CREATE_JOIN_ROOM ||
           event == ORBIS_NP_MATCHING2_REQUEST_EVENT_JOIN_ROOM ||
           event == ORBIS_NP_MATCHING2_REQUEST_EVENT_CREATE_JOIN_ROOM_A ||
           event == ORBIS_NP_MATCHING2_REQUEST_EVENT_JOIN_ROOM_A;
}

bool IsMemberJoinedEvent(OrbisNpMatching2Event event) {
    return event == ORBIS_NP_MATCHING2_ROOM_EVENT_MEMBER_JOINED ||
           event == ORBIS_NP_MATCHING2_ROOM_EVENT_MEMBER_JOINED_A;
}

void LogCreateJoinCallbackPayload(const PendingEvent& ev) {
    if (!IsCreateJoinRequestEvent(ev.req_event) || !ev.payload_owner) {
        return;
    }

    const CallbackPayload& payload = *ev.payload_owner;
    LOG_DEBUG(Lib_NpMatching2,
              "Callback create/join payload begin: ctx={} reqId={} event={:#x} data={} "
              "normal_members={} a_members={}",
              ev.ctx_id, ev.req_id, static_cast<u16>(ev.req_event), fmt::ptr(ev.request_data),
              payload.member_data.size(), payload.member_data_a.size());
    LogCallbackObject("create_join_response", payload.create_join_response.get());
    LogCallbackObject("create_join_response_a", payload.create_join_response_a.get());
    LogCallbackObject("room_data", payload.room_data.get());
    for (size_t i = 0; i < payload.member_data.size(); ++i) {
        const auto& member = payload.member_data[i];
        LOG_DEBUG(Lib_NpMatching2,
                  "Callback payload member[{}]: ptr={} size={:#x} npid='{}' npid_raw={} raw={}", i,
                  fmt::ptr(&member), sizeof(member), CallbackOnlineId(member.npId),
                  HexPreview(&member.npId, sizeof(member.npId), sizeof(member.npId)),
                  HexPreview(&member, sizeof(member), sizeof(member)));
    }
    for (size_t i = 0; i < payload.member_data_a.size(); ++i) {
        const auto& member = payload.member_data_a[i];
        LOG_DEBUG(Lib_NpMatching2, "Callback payload member_a[{}]: ptr={} size={:#x} raw={}", i,
                  fmt::ptr(&member), sizeof(member),
                  HexPreview(&member, sizeof(member), sizeof(member)));
    }
    LOG_DEBUG(Lib_NpMatching2, "Callback create/join payload end: ctx={} reqId={} event={:#x}",
              ev.ctx_id, ev.req_id, static_cast<u16>(ev.req_event));
}

void LogMemberJoinedCallbackPayload(const PendingEvent& ev) {
    if (!IsMemberJoinedEvent(ev.room_event) || !ev.payload_owner) {
        return;
    }

    const CallbackPayload& payload = *ev.payload_owner;
    LOG_DEBUG(Lib_NpMatching2,
              "Callback member-joined payload begin: ctx={} room={} event={:#x} data={}", ev.ctx_id,
              ev.room_id, static_cast<u16>(ev.room_event), fmt::ptr(ev.room_event_data));
    LogCallbackObject("room_member_update", payload.room_member_update.get());
    LogCallbackObject("room_member_update_a", payload.room_member_update_a.get());
    if (payload.event_member) {
        const auto& member = *payload.event_member;
        LOG_DEBUG(Lib_NpMatching2,
                  "Callback payload joined_member: ptr={} size={:#x} npid='{}' npid_raw={} raw={}",
                  fmt::ptr(&member), sizeof(member), CallbackOnlineId(member.npId),
                  HexPreview(&member.npId, sizeof(member.npId), sizeof(member.npId)),
                  HexPreview(&member, sizeof(member), sizeof(member)));
    }
    LogCallbackObject("joined_member_a", payload.event_member_a.get());
    LOG_DEBUG(Lib_NpMatching2, "Callback member-joined payload end: ctx={} room={} event={:#x}",
              ev.ctx_id, ev.room_id, static_cast<u16>(ev.room_event));
}

void FireEvent(const PendingEvent& ev) {
    const auto& ctx = ev.context_owner;
    if (!ctx) {
        LOG_WARNING(Lib_NpMatching2, "type={} dropped: ctx={} not found", static_cast<int>(ev.type),
                    ev.ctx_id);
        return;
    }
    switch (ev.type) {
    case PendingEvent::CONTEXT_CB:
        if (ctx->context_callback) {
            LOG_INFO(Lib_NpMatching2, "callback CONTEXT ctx={} event={:#x} cause={} err={:#x}",
                     ev.ctx_id, static_cast<u16>(ev.ctx_event), static_cast<u8>(ev.ctx_event_cause),
                     ev.error_code);
            ctx->context_callback(ev.ctx_id, ev.ctx_event, ev.ctx_event_cause, ev.error_code,
                                  ctx->context_callback_arg);
        } else {
            LOG_WARNING(Lib_NpMatching2, "callback CONTEXT ctx={} event={:#x} SKIPPED: no callback",
                        ev.ctx_id, static_cast<u16>(ev.ctx_event));
        }
        if (ev.ctx_event == ORBIS_NP_MATCHING2_CONTEXT_EVENT_STOPPED) {
            NpHandler::GetInstance().GetMatching2ContextManager().CompleteStop(ev.ctx_id, ctx);
        }
        break;
    case PendingEvent::REQUEST_CB:
        if (ev.request_cb) {
            LOG_INFO(Lib_NpMatching2,
                     "callback REQUEST ctx={} reqId={} event={:#x} err={:#x} data={}", ev.ctx_id,
                     ev.req_id, static_cast<u16>(ev.req_event), ev.error_code,
                     fmt::ptr(ev.request_data));
            LogCreateJoinCallbackPayload(ev);
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
            LOG_INFO(Lib_NpMatching2,
                     "callback SIGNALING ctx={} room={} member={} event={:#x} err={:#x}", ev.ctx_id,
                     ev.room_id, ev.member_id, static_cast<u16>(ev.sig_event), ev.error_code);
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
            LOG_INFO(Lib_NpMatching2, "callback ROOM_EVENT ctx={} room={} event={:#x} data={}",
                     ev.ctx_id, ev.room_id, static_cast<u16>(ev.room_event),
                     fmt::ptr(ev.room_event_data));
            LogMemberJoinedCallbackPayload(ev);
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
            LOG_INFO(Lib_NpMatching2, "callback LOBBY_EVENT ctx={} lobby={} event={:#x} data={}",
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
            LOG_INFO(Lib_NpMatching2, "callback LOBBY_MESSAGE ctx={} lobby={} src={} event={:#x}",
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
            LOG_INFO(Lib_NpMatching2,
                     "callback ROOM_MESSAGE ctx={} room={} src={} event={:#x} cb={} data={} "
                     "userdata={} aVariant={}",
                     ev.ctx_id, ev.room_id, ev.src_member_id, static_cast<u16>(ev.msg_event),
                     fmt::ptr(ctx->room_message_callback), fmt::ptr(ev.message_data),
                     fmt::ptr(ctx->room_message_callback_arg), ctx->a_variant);
            if (ctx->a_variant && ev.message_data) {
                const auto* info =
                    static_cast<const OrbisNpMatching2RoomMessageInfoA*>(ev.message_data);
                LOG_DEBUG(Lib_NpMatching2,
                          "callback ROOM_MESSAGE_A data: filtered={} cast={} dst={} srcMember={} "
                          "msg={} msgLen={}",
                          info->filtered, info->castType, fmt::ptr(info->dst),
                          fmt::ptr(&info->srcMember), fmt::ptr(info->msg), info->msgLen);
            } else if (ev.message_data) {
                const auto* info =
                    static_cast<const OrbisNpMatching2RoomMessageInfo*>(ev.message_data);
                LOG_DEBUG(Lib_NpMatching2,
                          "callback ROOM_MESSAGE data: filtered={} cast={} dst={} srcNpId={} "
                          "msg={} msgLen={}",
                          info->filtered, info->castType, fmt::ptr(info->dst),
                          fmt::ptr(info->srcMember), fmt::ptr(info->msg), info->msgLen);
            }
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
    auto& state = NpHandler::GetInstance().GetMatching2State();
    std::unique_lock lock(state.queue_mutex);
    while (state.dispatch_running.load()) {
        const auto now = std::chrono::steady_clock::now();

        std::vector<PendingEvent> ready;
        auto nearest = std::chrono::steady_clock::time_point::max();
        for (auto it = state.pending_events.begin(); it != state.pending_events.end();) {
            if (it->fire_at <= now) {
                ready.push_back(std::move(*it));
                it = state.pending_events.erase(it);
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
            state.queue_cv.wait(lock);
        } else {
            state.queue_cv.wait_until(lock, nearest);
        }
    }
    return nullptr;
}

void InitEventDispatcher() {
    auto& state = NpHandler::GetInstance().GetMatching2State();
    if (state.dispatch_running.exchange(true)) {
        return;
    }
    const s32 rc = NpCommon::sceNpCreateThread(
        &state.dispatch_thread, EventDispatcherThreadMain, nullptr,
        Libraries::Kernel::ORBIS_KERNEL_PRIO_FIFO_DEFAULT,
        ORBIS_NP_MATCHING2_THREAD_STACK_SIZE_DEFAULT, 0, "SceNpMatching2Ex");
    if (rc < 0) {
        LOG_ERROR(Lib_NpMatching2, "failed to create event dispatcher thread: {:#x}", rc);
        state.dispatch_running.store(false);
        state.dispatch_thread = nullptr;
        return;
    }
    LOG_INFO(Lib_NpMatching2, "event dispatcher thread created");
}

void TermEventDispatcher() {
    auto& state = NpHandler::GetInstance().GetMatching2State();
    if (!state.dispatch_running.exchange(false)) {
        return;
    }
    state.queue_cv.notify_all();
    if (state.dispatch_thread) {
        NpCommon::sceNpJoinThread(state.dispatch_thread, nullptr);
        state.dispatch_thread = nullptr;
    }
    std::lock_guard lock(state.queue_mutex);
    state.pending_events.clear();
}

void ScheduleEvent(PendingEvent ev) {
    if (!ev.context_owner) {
        LOG_WARNING(Lib_NpMatching2, "type={} dropped: ctx={} not found", static_cast<int>(ev.type),
                    ev.ctx_id);
        return;
    }
    auto& state = NpHandler::GetInstance().GetMatching2State();
    {
        std::lock_guard lock(state.queue_mutex);
        state.pending_events.push_back(std::move(ev));
    }
    state.queue_cv.notify_all();
}

} // namespace Libraries::Np::NpMatching2
