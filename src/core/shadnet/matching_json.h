//  SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/libraries/np/np_matching2_requests.h"
#include "nlohmann/json.hpp"

namespace Libraries::Np::NpMatching2 {

using json = nlohmann::json;

struct OrbisNpPeerAddressOwned {
    OrbisNpId npId;
    OrbisNpPlatformType platform;
};

struct OrbisNpMatching2BinAttrOwned {
    OrbisNpMatching2AttributeId id;
    std::vector<u8> data;
};

struct OrbisNpMatching2RoomBinAttrInternalOwned {
    Libraries::Rtc::OrbisRtcTick lastUpdate;
    OrbisNpMatching2RoomMemberId memberId;
    u8 pad[6];
    OrbisNpMatching2BinAttrOwned binAttr;
};

struct OrbisNpMatching2RoomMemberBinAttrInternalOwned {
    Libraries::Rtc::OrbisRtcTick lastUpdate;
    OrbisNpMatching2BinAttrOwned binAttr;
};

struct OrbisNpMatching2RoomMemberDataInternalOwned {
    u64 joinDateTicks;
    OrbisNpPeerAddressOwned user;
    Libraries::Np::OrbisNpOnlineId onlineId;
    u8 pad[4];
    OrbisNpMatching2RoomMemberId memberId;
    OrbisNpMatching2TeamId teamId;
    OrbisNpMatching2NatType natType;
    OrbisNpMatching2Flags flags;
    std::optional<OrbisNpMatching2RoomGroup> roomGroup;
    std::vector<OrbisNpMatching2RoomMemberBinAttrInternalOwned> roomMemberInternalBinAttr;
};

struct OrbisNpMatching2RoomDataInternalOwned {
    u16 publicSlots;
    u16 privateSlots;
    u16 openPublicSlots;
    u16 openPrivateSlots;
    u16 maxSlot;
    OrbisNpMatching2ServerId serverId;
    OrbisNpMatching2WorldId worldId;
    OrbisNpMatching2LobbyId lobbyId;
    OrbisNpMatching2RoomId roomId;
    u64 passwdSlotMask;
    u64 joinedSlotMask;
    std::vector<OrbisNpMatching2RoomGroup> roomGroup;
    OrbisNpMatching2Flags flags;
    u8 pad[4];
    std::vector<OrbisNpMatching2RoomBinAttrInternalOwned> internalBinAttr;

    std::vector<OrbisNpMatching2RoomBinAttrInternal> internalRoomBinAttrView;
    OrbisNpMatching2RoomDataInternal view();
};

struct OrbisNpMatching2CreateJoinRoomResponseOwned {
    OrbisNpMatching2RoomDataInternalOwned roomData;
    std::vector<OrbisNpMatching2RoomMemberDataInternalOwned> members;

    OrbisNpMatching2RoomDataInternal roomDataView;
    std::vector<OrbisNpMatching2RoomMemberDataInternal> membersView;
    std::vector<std::vector<OrbisNpMatching2RoomMemberBinAttrInternal>> membersBinAttrs;
    OrbisNpMatching2CreateJoinRoomResponse view();
};

struct OrbisNpMatching2RoomDataExternalOwned {
    u16 maxSlot;
    u16 curMembers;
    OrbisNpMatching2Flags flags;
    OrbisNpMatching2ServerId serverId;
    OrbisNpMatching2WorldId worldId;
    OrbisNpMatching2LobbyId lobbyId;
    OrbisNpMatching2RoomId roomId;
    u64 passwdSlotMask;
    u64 joinedSlotMask;
    u16 publicSlots;
    u16 privateSlots;
    u16 openPublicSlots;
    u16 openPrivateSlots;
    OrbisNpPeerAddressOwned owner;
    OrbisNpOnlineId ownerOnlineId;
    std::vector<OrbisNpMatching2RoomGroupInfo> roomGroup;
    std::vector<OrbisNpMatching2IntAttr> externalSearchIntAttr;
    std::vector<OrbisNpMatching2BinAttrOwned> externalSearchBinAttr;
    std::vector<OrbisNpMatching2BinAttrOwned> externalBinAttr;

    std::vector<OrbisNpMatching2BinAttr> externalSearchBinAttrView;
    std::vector<OrbisNpMatching2BinAttr> externalBinAttrView;

    OrbisNpMatching2RoomDataExternal view();
};

struct OrbisNpMatching2SearchRoomResponseOwned {
    OrbisNpMatching2Range range;
    std::vector<OrbisNpMatching2RoomDataExternalOwned> roomDataExt;

    std::vector<OrbisNpMatching2RoomDataExternal> roomDataExtView;
    OrbisNpMatching2SearchRoomResponse view();
};

struct OrbisNpMatching2RoomMemberUpdateInfoOwned {
    OrbisNpMatching2RoomMemberDataInternalOwned roomMemberDataInternal;
    OrbisNpMatching2EventCause eventCause;
    u8 pad[7];
    OrbisNpMatching2PresenceOptionData optData;
    OrbisNpMatching2RoomId roomId;

    std::vector<OrbisNpMatching2RoomMemberBinAttrInternal> memberBinAttrs;
    OrbisNpMatching2RoomMemberDataInternal roomMemberDataInternalView;

    OrbisNpMatching2RoomMemberUpdateInfo view();
};

struct SignalingEstablishedInfo {
    OrbisNpMatching2RoomId roomId;
    OrbisNpMatching2RoomMemberId roomMemberId;
};

std::string request_tag(const OrbisNpMatching2CreateJoinRoomRequest&);
std::string request_tag(const OrbisNpMatching2CreateJoinRoomRequestA&);
std::string request_tag(const OrbisNpMatching2JoinRoomRequest&);
std::string request_tag(const OrbisNpMatching2LeaveRoomRequest&);
std::string request_tag(const OrbisNpMatching2SearchRoomRequest&);
std::string request_tag(const OrbisNpMatching2SignalingGetPingInfoRequest&);

template <typename T>
std::string request_tag_t() {
    T t;
    return request_tag(t);
}

void to_json(json& j, const OrbisNpMatching2CreateJoinRoomRequest& req);
void to_json(json& j, const OrbisNpMatching2CreateJoinRoomRequestA& req);
void to_json(json& j, const OrbisNpMatching2JoinRoomRequest& req);
void to_json(json& j, const OrbisNpMatching2LeaveRoomRequest& req);
void to_json(json& j, const OrbisNpMatching2SearchRoomRequest& req);
void to_json(json& j, const OrbisNpMatching2SignalingGetPingInfoRequest& req);

void from_json(const json& j, OrbisNpMatching2CreateJoinRoomResponseOwned& res);
void from_json(const json& j, OrbisNpMatching2LeaveRoomResponse& res);
void from_json(const json& j, OrbisNpMatching2SearchRoomResponseOwned& res);
void from_json(const json& j, OrbisNpMatching2RoomMemberUpdateInfoOwned& res);
void from_json(const json& j, SignalingEstablishedInfo& res);

} // namespace Libraries::Np::NpMatching2
