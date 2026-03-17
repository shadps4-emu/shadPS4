// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/np/np_manager.h"
#include "core/libraries/np/np_matching2.h"
#include "core/libraries/rtc/rtc.h"

namespace Libraries::Np::NpMatching2 {

struct OrbisNpMatching2SignalingParam {
    int type;
    int flag;
    OrbisNpMatching2RoomMemberId mainMember;
    u8 pad[4];
};

struct OrbisNpMatching2SessionPassword {
    u8 data[8];
};

struct OrbisNpMatching2RoomPassword {
    u8 data[8];
};

struct OrbisNpMatching2GroupLabel {
    u8 data[8];
};

struct OrbisNpMatching2RoomGroupConfig {
    u32 slots;
    bool hasLabel;
    OrbisNpMatching2GroupLabel label;
    bool hasPassword;
    u8 pad[2];
};

struct OrbisNpMatching2BinAttr {
    OrbisNpMatching2AttributeId id;
    u8 pad[6];
    u8* data;
    u64 dataSize;
};

struct OrbisNpMatching2RoomBinAttrInternal {
    Libraries::Rtc::OrbisRtcTick lastUpdate;
    OrbisNpMatching2RoomMemberId memberId;
    u8 pad[6];
    OrbisNpMatching2BinAttr binAttr;
};

struct OrbisNpMatching2RoomMemberBinAttrInternal {
    Libraries::Rtc::OrbisRtcTick lastUpdate;
    OrbisNpMatching2BinAttr binAttr;
};

struct OrbisNpMatching2IntAttr {
    OrbisNpMatching2AttributeId id;
    u8 pad[2];
    u32 attr;
};

template <typename T>
struct OrbisNpMatching2CreateJoinRoomRequest_ {
    u16 maxSlot;
    OrbisNpMatching2TeamId teamId;
    u8 pad[5];
    OrbisNpMatching2Flags flags;
    OrbisNpMatching2WorldId worldId;
    OrbisNpMatching2LobbyId lobbyId;
    OrbisNpMatching2RoomPassword* roomPasswd;
    u64* passwdSlotMask;
    OrbisNpMatching2RoomGroupConfig* groupConfig;
    u64 groupConfigs;
    OrbisNpMatching2GroupLabel* joinGroupLabel;
    T* allowedUser;
    u64 allowedUsers;
    T* blockedUser;
    u64 blockedUsers;
    OrbisNpMatching2BinAttr* internalBinAttr;
    u64 internalBinAttrs;
    OrbisNpMatching2IntAttr* externalSearchIntAttr;
    u64 externalSearchIntAttrs;
    OrbisNpMatching2BinAttr* externalSearchBinAttr;
    u64 externalSearchBinAttrs;
    OrbisNpMatching2BinAttr* externalBinAttr;
    u64 externalBinAttrs;
    OrbisNpMatching2BinAttr* memberInternalBinAttr;
    u64 memberInternalBinAttrs;
    OrbisNpMatching2SignalingParam* signalingParam;

    int Validate() {
        return 0;
    }
};

using OrbisNpMatching2CreateJoinRoomRequest =
    OrbisNpMatching2CreateJoinRoomRequest_<Libraries::Np::OrbisNpOnlineId>;
using OrbisNpMatching2CreateJoinRoomRequestA =
    OrbisNpMatching2CreateJoinRoomRequest_<Libraries::Np::OrbisNpAccountId>;

static_assert(sizeof(OrbisNpMatching2CreateJoinRoomRequestA) == 184);

struct OrbisNpMatching2RoomGroup {
    OrbisNpMatching2RoomGroupId id;
    bool hasPasswd;
    bool hasLabel;
    u8 pad;
    OrbisNpMatching2GroupLabel label;
    u32 slots;
    u32 groupMembers;
};

struct OrbisNpMatching2RoomGroupInfo {
    OrbisNpMatching2RoomGroupId id;
    bool hasPasswd;
    u8 pad[2];
    u32 slots;
    u32 groupMembers;
};

struct OrbisNpMatching2RangeFilter {
    u32 start;
    u32 max;
};

enum class OrbisNpMatching2Operator : u8 { Eq = 1, Ne = 2, Lt = 3, Le = 4, Gt = 5, Ge = 6 };

struct OrbisNpMatching2IntFilter {
    OrbisNpMatching2Operator op;
    u8 pad[7];
    OrbisNpMatching2IntAttr attr;
};

struct OrbisNpMatching2BinFilter {
    OrbisNpMatching2Operator op;
    u8 pad[7];
    OrbisNpMatching2BinAttr attr;
};

struct OrbisNpMatching2SearchRoomRequest {
    int option;
    OrbisNpMatching2WorldId worldId;
    OrbisNpMatching2LobbyId lobbyId;
    OrbisNpMatching2RangeFilter rangeFilter;
    OrbisNpMatching2Flags flagFilter;
    OrbisNpMatching2Flags flagAttrs;
    OrbisNpMatching2IntFilter* intFilter;
    u64 intFilters;
    OrbisNpMatching2BinFilter* binFilter;
    u64 binFilters;
    OrbisNpMatching2AttributeId* attr;
    u64 attrs;

    int Validate() {
        return 0;
    }
};

struct OrbisNpMatching2RoomDataInternal {
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
    const OrbisNpMatching2RoomGroup* roomGroup;
    u64 roomGroups;
    OrbisNpMatching2Flags flags;
    u8 pad[4];
    const OrbisNpMatching2RoomBinAttrInternal* roomBinAttrInternal;
    u64 roomBinAttrInternalNum;
};

static_assert(offsetof(OrbisNpMatching2RoomDataInternal, roomBinAttrInternal) == 0x48);

template <typename T>
struct OrbisNpMatching2RoomMemberDataInternal_ {
    OrbisNpMatching2RoomMemberDataInternal_<T>* next;
    u64 joinDateTicks;
    T user;
    Libraries::Np::OrbisNpOnlineId onlineId;
    u8 pad[4];
    OrbisNpMatching2RoomMemberId memberId;
    OrbisNpMatching2TeamId teamId;
    OrbisNpMatching2NatType natType;
    OrbisNpMatching2Flags flags;
    OrbisNpMatching2RoomGroup* roomGroup;
    OrbisNpMatching2RoomMemberBinAttrInternal* roomMemberInternalBinAttr;
    u64 roomMemberInternalBinAttrs;
};

using OrbisNpMatching2RoomMemberDataInternal =
    OrbisNpMatching2RoomMemberDataInternal_<Libraries::Np::OrbisNpPeerAddress>;
using OrbisNpMatching2RoomMemberDataInternalA =
    OrbisNpMatching2RoomMemberDataInternal_<Libraries::Np::OrbisNpPeerAddressA>;

// static_assert(sizeof(OrbisNpMatching2RoomMemberDataInternal) == 0x60);

template <typename T>
struct OrbisNpMatching2RoomMemberDataInternalList_ {
    OrbisNpMatching2RoomMemberDataInternal_<T>* members;
    u64 membersNum;
    OrbisNpMatching2RoomMemberDataInternal_<T>* me;
    OrbisNpMatching2RoomMemberDataInternal_<T>* owner;
};

using OrbisNpMatching2RoomMemberDataInternalList =
    OrbisNpMatching2RoomMemberDataInternalList_<Libraries::Np::OrbisNpPeerAddress>;
using OrbisNpMatching2RoomMemberDataInternalListA =
    OrbisNpMatching2RoomMemberDataInternalList_<Libraries::Np::OrbisNpPeerAddressA>;

struct OrbisNpMatching2CreateJoinRoomResponse {
    const OrbisNpMatching2RoomDataInternal* roomData;
    OrbisNpMatching2RoomMemberDataInternalList members;
};

struct OrbisNpMatching2CreateJoinRoomResponseA {
    OrbisNpMatching2RoomDataInternal* roomData;
    OrbisNpMatching2RoomMemberDataInternalListA members;
};

struct OrbisNpMatching2RoomDataExternalA {
    OrbisNpMatching2RoomDataExternalA* next;
    u16 maxSlot;
    u16 curMembers;
    OrbisNpMatching2Flags flags;
    OrbisNpMatching2ServerId serverId;
    u8 pad[2];
    OrbisNpMatching2WorldId worldId;
    OrbisNpMatching2LobbyId lobbyId;
    OrbisNpMatching2RoomId roomId;
    u64 passwdSlotMask;
    u64 joinedSlotMask;
    u16 publicSlots;
    u16 privateSlots;
    u16 openPublicSlots;
    u16 openPrivateSlots;
    Np::OrbisNpPeerAddressA owner;
    OrbisNpOnlineId ownerOnlineId;
    OrbisNpMatching2RoomGroupInfo* roomGroup;
    u64 roomGroups;
    OrbisNpMatching2IntAttr* externalSearchIntAttr;
    u64 externalSearchIntAttrs;
    OrbisNpMatching2BinAttr* externalSearchBinAttr;
    u64 externalSearchBinAttrs;
    OrbisNpMatching2BinAttr* externalBinAttr;
    u64 externalBinAttrs;
};

struct OrbisNpMatching2RoomDataExternal {
    OrbisNpMatching2RoomDataExternal* next;
    u16 maxSlot;
    u16 curMembers;
    OrbisNpMatching2Flags flags;
    OrbisNpMatching2ServerId serverId;
    u8 pad[2];
    OrbisNpMatching2WorldId worldId;
    OrbisNpMatching2LobbyId lobbyId;
    OrbisNpMatching2RoomId roomId;
    u64 passwdSlotMask;
    u64 joinedSlotMask;
    u16 publicSlots;
    u16 privateSlots;
    u16 openPublicSlots;
    u16 openPrivateSlots;
    u64 unk;
    OrbisNpMatching2RoomGroupInfo* roomGroup;
    u64 roomGroups;
    OrbisNpMatching2IntAttr* externalSearchIntAttr;
    u64 externalSearchIntAttrs;
    OrbisNpMatching2BinAttr* externalSearchBinAttr;
    u64 externalSearchBinAttrs;
    OrbisNpMatching2BinAttr* externalBinAttr;
    u64 externalBinAttrs;
};

static_assert(sizeof(OrbisNpMatching2RoomDataExternal) == 0x88);

struct OrbisNpMatching2Range {
    u32 start;
    u32 total;
    u32 results;
    u8 pad[4];
};

struct OrbisNpMatching2SearchRoomResponseA {
    OrbisNpMatching2Range range;
    OrbisNpMatching2RoomDataExternalA* roomDataExt;
};

struct OrbisNpMatching2SearchRoomResponse {
    OrbisNpMatching2Range range;
    OrbisNpMatching2RoomDataExternal* roomDataExt;
};

struct OrbisNpMatching2SignalingGetPingInfoRequest {
    OrbisNpMatching2RoomId roomId;
    u8 pad[16];

    int Validate() {
        return 0;
    }
};

struct OrbisNpMatching2SignalingGetPingInfoResponse {
    OrbisNpMatching2ServerId serverId;
    u8 pad[2];
    OrbisNpMatching2WorldId worldId;
    OrbisNpMatching2RoomId roomId;
    u32 pingUs;
    u8 reserved[20];
};

struct OrbisNpMatching2PresenceOptionData {
    u8 data[16];
    u64 len;
};

struct OrbisNpMatching2JoinRoomRequest {
    OrbisNpMatching2RoomId roomId;
    OrbisNpMatching2SessionPassword* roomPasswd;
    OrbisNpMatching2GroupLabel* joinGroupLabel;
    OrbisNpMatching2BinAttr* roomMemberBinInternalAttr;
    u64 roomMemberBinInternalAttrNum;
    OrbisNpMatching2PresenceOptionData optData;
    OrbisNpMatching2TeamId teamId;
    u8 pad[3];
    OrbisNpMatching2Flags flags;
    OrbisNpOnlineId* blockedUser;
    u64 blockedUsers;

    int Validate() {
        return 0;
    }
};

static_assert(sizeof(OrbisNpMatching2JoinRoomRequest) == 0x58);

struct OrbisNpMatching2LeaveRoomRequest {
    OrbisNpMatching2RoomId roomId;
    OrbisNpMatching2PresenceOptionData optData;

    int Validate() {
        return 0;
    }
};

struct OrbisNpMatching2LeaveRoomResponse {
    OrbisNpMatching2RoomId roomId;
};

struct OrbisNpMatching2RoomMemberUpdateInfo {
    OrbisNpMatching2RoomMemberDataInternal* roomMemberDataInternal;
    OrbisNpMatching2EventCause eventCause;
    u8 pad[7];
    OrbisNpMatching2PresenceOptionData optData;
};

using OrbisNpMatching2RequestCallback = PS4_SYSV_ABI void (*)(OrbisNpMatching2ContextId,
                                                              OrbisNpMatching2RequestId,
                                                              OrbisNpMatching2Event, int,
                                                              const void*, void*);
using OrbisNpMatching2RequestFn = PS4_SYSV_ABI void(OrbisNpMatching2ContextId,
                                                    OrbisNpMatching2RequestId,
                                                    OrbisNpMatching2Event, int, const void*, void*);

struct OrbisNpMatching2RequestOptParam {
    OrbisNpMatching2RequestCallback callback;
    void* arg;
    u32 timeout;
    u16 appId;
    u8 dummy[2];
};

} // namespace Libraries::Np::NpMatching2
