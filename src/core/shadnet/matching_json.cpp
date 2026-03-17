//  SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#include "matching_json.h"

#include "common/config.h"

#include "cppcodec/base64_rfc4648.hpp"

namespace nlohmann {
template <typename T>
struct adl_serializer<std::span<T>> {
    static void to_json(json& j, const std::span<T>& s) {
        if (s.empty()) {
            j = nullptr;
        } else {
            j = json::array();
            for (const auto& v : s) {
                j.push_back(v);
            }
        }
    }
};
} // namespace nlohmann

namespace Libraries::Rtc {
using json = nlohmann::json;

void from_json(const json& j, OrbisRtcTick& tick) {
    auto t = j.get<u64>();
    tick.tick = t;
}

} // namespace Libraries::Rtc

namespace Libraries::Np {

using json = nlohmann::json;

void to_json(json& j, const OrbisNpOnlineId& npId) {
    j = json{std::string{npId.data}};
}

void from_json(const json& j, OrbisNpOnlineId& npId) {
    auto id = j.get<std::string>();
    strncpy(npId.data, id.c_str(), sizeof(npId.data));
}

NLOHMANN_JSON_SERIALIZE_ENUM(OrbisNpPlatformType, {
                                                      {OrbisNpPlatformType::NONE, "NONE"},
                                                      {OrbisNpPlatformType::PS3, "PS3"},
                                                      {OrbisNpPlatformType::VITA, "VITA"},
                                                      {OrbisNpPlatformType::PS4, "PS4"},
                                                  })

} // namespace Libraries::Np

namespace Libraries::Np::NpMatching2 {

using base64 = cppcodec::base64_rfc4648;
using json = nlohmann::json;

std::string request_tag(const OrbisNpMatching2SearchRoomRequest&) {
    return "search_room";
}

std::string request_tag(const OrbisNpMatching2CreateJoinRoomRequest&) {
    return "create_join_room";
}

std::string request_tag(const OrbisNpMatching2CreateJoinRoomRequestA&) {
    return "create_join_room_a";
}

std::string request_tag(const OrbisNpMatching2JoinRoomRequest&) {
    return "join_room";
}

std::string request_tag(const OrbisNpMatching2LeaveRoomRequest&) {
    return "leave_room";
}

std::string request_tag(const OrbisNpMatching2SignalingGetPingInfoRequest&) {
    return "signaling_get_ping_info";
}

void to_json(json& j, const OrbisNpMatching2SignalingParam& p) {
    j["type"] = p.type;
    j["flag"] = p.flag;
    j["hubMemberId"] = p.mainMember;
}

void to_json(json& j, const OrbisNpMatching2BinAttr& attr) {
    j["attrId"] = attr.id;
    j["data"] = base64::encode(attr.data, attr.dataSize);
}

void from_json(const json& j, OrbisNpMatching2BinAttrOwned& attr) {
    j.at("attrId").get_to(attr.id);
    auto data = j.at("data").get<std::string>();
    attr.data = base64::decode(data);
}

void to_json(json& j, const OrbisNpMatching2IntAttr& attr) {
    j["attrId"] = attr.id;
    j["value"] = attr.attr;
}

void from_json(const json& j, OrbisNpMatching2IntAttr& attr) {
    j.at("attrId").get_to(attr.id);
    j.at("value").get_to(attr.attr);
}

void to_json(json& j, const OrbisNpMatching2SessionPassword& pw) {
    j = json{{"data", pw.data}};
}

void to_json(json& j, const OrbisNpMatching2RoomPassword& pw) {
    j = json{{"data", pw.data}};
}

void from_json(const json& j, OrbisNpMatching2PresenceOptionData& optData) {
    j.at("data").get_to(optData.data);
    j.at("len").get_to(optData.len);
}

void to_json(json& j, const OrbisNpMatching2PresenceOptionData& optData) {
    j["data"] = optData.data;
    j["len"] = optData.len;
}

void to_json(json& j, const OrbisNpMatching2GroupLabel& label) {
    j = json{{"data", label.data}};
}

void from_json(const json& j, OrbisNpMatching2GroupLabel& label) {
    j.at("data").get_to(label.data);
}

void to_json(json& j, const OrbisNpMatching2RoomGroupConfig& config) {
    j["slots"] = config.slots;
    if (config.hasLabel) {
        j["label"] = config.label;
    }
    j["hasPassword"] = config.hasPassword;
}

template <typename T>
void to_json_a(json& j, const OrbisNpMatching2CreateJoinRoomRequest_<T>& req) {
    j["maxSlot"] = req.maxSlot;
    j["teamId"] = req.teamId;
    j["flags"] = req.flags;
    j["worldId"] = req.worldId;
    j["lobbyId"] = req.lobbyId;
    if (req.roomPasswd) {
        j["roomPasswd"] = *req.roomPasswd;
    }
    if (req.passwdSlotMask) {
        j["passwdSlotMask"] = *req.passwdSlotMask;
    }
    if (req.groupConfig && req.groupConfigs > 0) {
        j["groupConfig"] = std::span(req.groupConfig, req.groupConfigs);
    }
    if (req.joinGroupLabel) {
        j["joinGroupLabel"] = *req.joinGroupLabel;
    }
    if (req.allowedUser && req.allowedUsers > 0) {
        j["allowedUsers"] = std::span(req.allowedUser, req.allowedUsers);
    }
    if (req.blockedUser && req.blockedUsers > 0) {
        j["blockedUsers"] = std::span(req.blockedUser, req.blockedUsers);
    }
    if (req.internalBinAttr && req.internalBinAttrs > 0) {
        j["roomBinAttrInternal"] = std::span(req.internalBinAttr, req.internalBinAttrs);
    }
    if (req.externalSearchIntAttr && req.externalSearchIntAttrs > 0) {
        j["roomSearchableIntAttrExternal"] =
            std::span(req.externalSearchIntAttr, req.externalSearchIntAttrs);
    }
    if (req.externalSearchBinAttr && req.externalSearchBinAttrs > 0) {
        j["roomSearchableBinAttrExternal"] =
            std::span(req.externalSearchBinAttr, req.externalSearchBinAttrs);
    }
    if (req.externalBinAttr && req.externalBinAttrs > 0) {
        j["roomBinAttrExternal"] = std::span(req.externalBinAttr, req.externalBinAttrs);
    }
    if (req.memberInternalBinAttr && req.memberInternalBinAttrs > 0) {
        j["roomMemberBinAttrInternal"] =
            std::span(req.memberInternalBinAttr, req.memberInternalBinAttrs);
    }
    if (req.signalingParam) {
        j["signaling"] = *req.signalingParam;
    }
}

void to_json(json& j, const OrbisNpMatching2CreateJoinRoomRequest& req) {
    to_json_a(j, req);
}

void to_json(json& j, const OrbisNpMatching2CreateJoinRoomRequestA& req) {
    to_json_a(j, req);
}

void to_json(json& j, const OrbisNpMatching2RangeFilter& f) {
    j["startIndex"] = f.start;
    j["max"] = f.max;
}

void to_json(json& j, const OrbisNpMatching2IntFilter& f) {
    j["operator"] = f.op;
    j["attr"] = f.attr;
}

void to_json(json& j, const OrbisNpMatching2BinFilter& f) {
    j["operator"] = f.op;
    j["attr"] = f.attr;
}

void to_json(json& j, const OrbisNpMatching2JoinRoomRequest& req) {
    j["roomId"] = req.roomId;
    if (req.roomPasswd) {
        j["roomPasswd"] = *req.roomPasswd;
    }
    if (req.joinGroupLabel) {
        j["joinGroupLabel"] = *req.joinGroupLabel;
    }
    if (req.roomMemberBinInternalAttr && req.roomMemberBinInternalAttrNum > 0) {
        j["roomMemberBinAttrInternal"] =
            std::span(req.roomMemberBinInternalAttr, req.roomMemberBinInternalAttrNum);
    }
    j["optData"] = req.optData;
    j["teamId"] = req.teamId;
    j["flags"] = req.flags;
    if (req.blockedUser && req.blockedUsers > 0) {
        j["blockedUser"] = std::span(req.blockedUser, req.blockedUsers);
    }
}

void to_json(json& j, const OrbisNpMatching2LeaveRoomRequest& req) {
    j["roomId"] = req.roomId;
    j["optData"] = req.optData;
}

void to_json(json& j, const OrbisNpMatching2SearchRoomRequest& req) {
    j["option"] = req.option;
    j["worldId"] = req.worldId;
    j["lobbyId"] = req.lobbyId;
    j["rangeFilter"] = req.rangeFilter;
    j["flagFilter"] = req.flagFilter;
    j["flagAttr"] = req.flagAttrs;
    if (req.intFilter && req.intFilters > 0) {
        j["intFilter"] = std::span(req.intFilter, req.intFilters);
    }
    if (req.binFilter && req.binFilters > 0) {
        j["binFilter"] = std::span(req.binFilter, req.binFilters);
    }
    if (req.attr && req.attrs > 0) {
        j["attrIds"] = std::span(req.attr, req.attrs);
    }
}

void to_json(json& j, const OrbisNpMatching2SignalingGetPingInfoRequest& req) {
    j["roomId"] = req.roomId;
}

void from_json(const json& j, OrbisNpMatching2RoomGroup& group) {
    j.at("id").get_to(group.id);
    j.at("hasPasswd").get_to(group.hasPasswd);
    j.at("hasLabel").get_to(group.hasLabel);
    j.at("label").get_to(group.label);
    j.at("slots").get_to(group.slots);
    j.at("groupMembers").get_to(group.groupMembers);
}

void from_json(const json& j, OrbisNpMatching2RoomGroupInfo& group) {
    j.at("id").get_to(group.id);
    j.at("hasPasswd").get_to(group.hasPasswd);
    j.at("slots").get_to(group.slots);
    j.at("groupMembers").get_to(group.groupMembers);
}

void from_json(const json& j, OrbisNpMatching2RoomBinAttrInternalOwned& res) {
    j.at("lastUpdate").get_to(res.lastUpdate);
    j.at("memberId").get_to(res.memberId);
    j.at("binAttr").get_to(res.binAttr);
}

void from_json(const json& j, OrbisNpMatching2RoomMemberBinAttrInternalOwned& res) {
    j.at("lastUpdate").get_to(res.lastUpdate);
    j.at("binAttr").get_to(res.binAttr);
}

void from_json(const json& j, OrbisNpMatching2RoomDataInternalOwned& res) {
    j.at("publicSlotNum").get_to(res.publicSlots);
    j.at("privateSlotNum").get_to(res.privateSlots);
    j.at("openPublicSlotNum").get_to(res.openPublicSlots);
    j.at("openPrivateSlotNum").get_to(res.openPrivateSlots);
    j.at("maxSlot").get_to(res.maxSlot);
    j.at("serverId").get_to(res.serverId);
    j.at("worldId").get_to(res.worldId);
    j.at("lobbyId").get_to(res.lobbyId);
    j.at("roomId").get_to(res.roomId);
    j.at("passwordSlotMask").get_to(res.passwdSlotMask);
    j.at("joinedSlotMask").get_to(res.joinedSlotMask);
    if (j.contains("roomGroup")) {
        res.roomGroup = j.at("roomGroup").get<std::vector<OrbisNpMatching2RoomGroup>>();
    }
    j.at("flags").get_to(res.flags);
    if (j.contains("roomBinAttrInternal")) {
        j.at("roomBinAttrInternal").get_to(res.internalBinAttr);
    }
}

void from_json(const json& j, OrbisNpPeerAddressOwned& addr) {
    auto npId = j.at("onlineId").get<std::string>();
    strncpy(addr.npId.handle.data, npId.c_str(), sizeof(addr.npId.handle.data));
    j.at("platform").get_to(addr.platform);
}

void from_json(const json& j, OrbisNpMatching2RoomMemberDataInternalOwned& res) {
    j.at("joinDate").get_to(res.joinDateTicks);
    j.at("user").get_to(res.user);
    j.at("onlineId").get_to(res.onlineId);
    j.at("memberId").get_to(res.memberId);
    j.at("teamId").get_to(res.teamId);
    j.at("natType").get_to(res.natType);
    j.at("flags").get_to(res.flags);
    if (j.contains("roomGroup")) {
        res.roomGroup = j.at("roomGroup").get<OrbisNpMatching2RoomGroup>();
    }
    if (j.contains("roomMemberBinAttrInternal")) {
        j.at("roomMemberBinAttrInternal").get_to(res.roomMemberInternalBinAttr);
    }
}

void from_json(const json& j, OrbisNpMatching2CreateJoinRoomResponseOwned& res) {
    j.at("roomDataInternal").get_to(res.roomData);
    j.at("members").get_to(res.members);
}

OrbisNpMatching2RoomDataInternal OrbisNpMatching2RoomDataInternalOwned::view() {
    for (auto& attr : this->internalBinAttr) {
        OrbisNpMatching2RoomBinAttrInternal a{
            attr.lastUpdate,
            attr.memberId,
            {},
            {attr.binAttr.id, {}, attr.binAttr.data.data(), attr.binAttr.data.size()}};
        this->internalRoomBinAttrView.push_back(a);
    }
    return {
        .publicSlots = this->publicSlots,
        .privateSlots = this->privateSlots,
        .openPublicSlots = this->openPublicSlots,
        .openPrivateSlots = this->openPrivateSlots,
        .maxSlot = this->maxSlot,
        .serverId = this->serverId,
        .worldId = this->worldId,
        .lobbyId = this->lobbyId,
        .roomId = this->roomId,
        .passwdSlotMask = this->passwdSlotMask,
        .joinedSlotMask = this->joinedSlotMask,
        .roomGroup = this->roomGroup.data(),
        .roomGroups = this->roomGroup.size(),
        .flags = this->flags,
        .roomBinAttrInternal = this->internalRoomBinAttrView.data(),
        .roomBinAttrInternalNum = this->internalRoomBinAttrView.size(),
    };
}

OrbisNpMatching2CreateJoinRoomResponse OrbisNpMatching2CreateJoinRoomResponseOwned::view() {
    OrbisNpMatching2RoomMemberDataInternal* me = nullptr;
    OrbisNpMatching2RoomMemberDataInternal* owner = nullptr;
    this->roomDataView = roomData.view();
    this->membersView.reserve(this->members.size());

    for (auto& member : this->members) {
        std::vector<OrbisNpMatching2RoomMemberBinAttrInternal> attrVec;
        for (auto& attr : member.roomMemberInternalBinAttr) {
            OrbisNpMatching2RoomMemberBinAttrInternal a{
                attr.lastUpdate,
                {attr.binAttr.id, {}, attr.binAttr.data.data(), attr.binAttr.data.size()}};
            attrVec.push_back(a);
        }
        this->membersBinAttrs.push_back(attrVec);

        OrbisNpMatching2RoomMemberDataInternal m = {
            .next = nullptr,
            .joinDateTicks = member.joinDateTicks,
            .user = {&member.user.npId, member.user.platform},
            .onlineId = member.onlineId,
            .memberId = member.memberId,
            .teamId = member.teamId,
            .natType = member.natType,
            .flags = member.flags,
            .roomGroup = member.roomGroup ? &*member.roomGroup : nullptr,
            .roomMemberInternalBinAttr = this->membersBinAttrs.back().data(),
            .roomMemberInternalBinAttrs = this->membersBinAttrs.back().size(),
        };
        this->membersView.push_back(m);

        if ((member.flags & ORBIS_NP_MATCHING2_ROOM_MEMBER_FLAG_ATTR_OWNER) != 0) {
            owner = &this->membersView.back();
        }
        if (strncmp(member.onlineId.data, Config::getUserName().c_str(),
                    sizeof(member.onlineId.data))) {
            me = &this->membersView.back();
        }
    }

    OrbisNpMatching2RoomMemberDataInternal* next = nullptr;
    for (auto& x : this->membersView | std::views::reverse) {
        x.next = next;
        next = &x;
    }

    return {&this->roomDataView,
            {
                .members = this->membersView.data(),
                .membersNum = this->membersView.size(),
                .me = me,
                .owner = owner,
            }};
}

void from_json(const json& j, OrbisNpMatching2Range& range) {
    j.at("startIndex").get_to(range.start);
    j.at("total").get_to(range.total);
    j.at("resultCount").get_to(range.results);
}

void from_json(const json& j, OrbisNpMatching2RoomDataExternalOwned& data) {
    j.at("maxSlot").get_to(data.maxSlot);
    j.at("currentMemberNum").get_to(data.curMembers);
    j.at("flagAttr").get_to(data.flags);
    j.at("serverId").get_to(data.serverId);
    j.at("worldId").get_to(data.worldId);
    j.at("lobbyId").get_to(data.lobbyId);
    j.at("roomId").get_to(data.roomId);
    if (j.contains("passwdSlotMask")) {
        j.at("passwdSlotMask").get_to(data.passwdSlotMask);
    }
    j.at("joinedSlotMask").get_to(data.joinedSlotMask);
    j.at("publicSlotNum").get_to(data.publicSlots);
    j.at("privateSlotNum").get_to(data.privateSlots);
    j.at("openPublicSlotNum").get_to(data.openPublicSlots);
    j.at("openPrivateSlotNum").get_to(data.openPrivateSlots);
    j.at("owner").get_to(data.owner);
    j.at("ownerOnlineId").get_to(data.ownerOnlineId);
    if (j.contains("roomGroup")) {
        j.at("roomGroup").get_to(data.roomGroup);
    }
    j.at("roomSearchableIntAttrExternal").get_to(data.externalSearchIntAttr);
    j.at("roomSearchableBinAttrExternal").get_to(data.externalSearchBinAttr);
    j.at("roomBinAttrExternal").get_to(data.externalBinAttr);
}

void from_json(const json& j, OrbisNpMatching2SearchRoomResponseOwned& res) {
    j.at("range").get_to(res.range);
    j.at("rooms").get_to(res.roomDataExt);
}

OrbisNpMatching2RoomDataExternal OrbisNpMatching2RoomDataExternalOwned::view() {
    for (auto& attr : this->externalSearchBinAttr) {
        OrbisNpMatching2BinAttr a{attr.id, {}, attr.data.data(), attr.data.size()};
        this->externalSearchBinAttrView.push_back(a);
    }

    for (auto& attr : this->externalBinAttr) {
        OrbisNpMatching2BinAttr a{attr.id, {}, attr.data.data(), attr.data.size()};
        this->externalBinAttrView.push_back(a);
    }

    return {
        .next = nullptr,
        .maxSlot = this->maxSlot,
        .curMembers = this->curMembers,
        .flags = this->flags,
        .serverId = this->serverId,
        .worldId = this->worldId,
        .lobbyId = this->lobbyId,
        .roomId = this->roomId,
        .passwdSlotMask = this->passwdSlotMask,
        .joinedSlotMask = this->joinedSlotMask,
        .publicSlots = this->publicSlots,
        .privateSlots = this->privateSlots,
        .openPublicSlots = this->openPublicSlots,
        .openPrivateSlots = this->openPrivateSlots,
        .unk = 0xCAFEDEAD,
        .roomGroup = this->roomGroup.data(),
        .roomGroups = this->roomGroup.size(),
        .externalSearchIntAttr = this->externalSearchIntAttr.data(),
        .externalSearchIntAttrs = this->externalSearchIntAttr.size(),
        .externalSearchBinAttr = this->externalSearchBinAttrView.data(),
        .externalSearchBinAttrs = this->externalSearchBinAttrView.size(),
        .externalBinAttr = this->externalBinAttrView.data(),
        .externalBinAttrs = this->externalBinAttrView.size(),
    };
}

OrbisNpMatching2SearchRoomResponse OrbisNpMatching2SearchRoomResponseOwned::view() {
    for (auto& roomData : this->roomDataExt) {
        auto view = roomData.view();
        this->roomDataExtView.push_back(view);
    }

    OrbisNpMatching2RoomDataExternal* next = nullptr;
    for (auto& x : this->roomDataExtView | std::views::reverse) {
        x.next = next;
        next = &x;
    }

    return {this->range, this->roomDataExtView.data()};
}

OrbisNpMatching2RoomMemberUpdateInfo OrbisNpMatching2RoomMemberUpdateInfoOwned::view() {
    for (auto& attr : roomMemberDataInternal.roomMemberInternalBinAttr) {
        OrbisNpMatching2RoomMemberBinAttrInternal a{
            attr.lastUpdate,
            {attr.binAttr.id, {}, attr.binAttr.data.data(), attr.binAttr.data.size()}};
        this->memberBinAttrs.push_back(a);
    }

    OrbisNpMatching2RoomMemberDataInternal m = {
        .next = nullptr,
        .joinDateTicks = roomMemberDataInternal.joinDateTicks,
        .user = {&roomMemberDataInternal.user.npId, roomMemberDataInternal.user.platform},
        .onlineId = roomMemberDataInternal.onlineId,
        .memberId = roomMemberDataInternal.memberId,
        .teamId = roomMemberDataInternal.teamId,
        .natType = roomMemberDataInternal.natType,
        .flags = roomMemberDataInternal.flags,
        .roomGroup =
            roomMemberDataInternal.roomGroup ? &*roomMemberDataInternal.roomGroup : nullptr,
        .roomMemberInternalBinAttr = this->memberBinAttrs.data(),
        .roomMemberInternalBinAttrs = this->memberBinAttrs.size(),
    };
    this->roomMemberDataInternalView = m;

    return {&this->roomMemberDataInternalView, this->eventCause, {}, this->optData};
}

void from_json(const json& j, OrbisNpMatching2RoomMemberUpdateInfoOwned& res) {
    j.at("roomMemberDataInternal").get_to(res.roomMemberDataInternal);
    j.at("eventCause").get_to(res.eventCause);
    j.at("optData").get_to(res.optData);
    j.at("roomId").get_to(res.roomId);
}

void from_json(const json& j, SignalingEstablishedInfo& res) {
    j.at("roomMemberId").get_to(res.roomMemberId);
    j.at("roomId").get_to(res.roomId);
}

void from_json(const json& j, OrbisNpMatching2LeaveRoomResponse& res) {
    j.at("roomId").get_to(res.roomId);
}

} // namespace Libraries::Np::NpMatching2
