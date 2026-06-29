// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/np/np_matching2/np_matching2.h"
#include "core/libraries/np/np_types.h"
#include "core/libraries/rtc/rtc.h"
#include "core/libraries/system/userservice.h"

namespace Libraries::Np::NpMatching2 {

struct OrbisNpMatching2RoomGroup;
struct OrbisNpMatching2RoomGroupInfo;
struct OrbisNpMatching2RoomBinAttrInternal;
struct OrbisNpMatching2RoomMemberBinAttrInternal;

using OrbisNpMatching2ContextCallback = PS4_SYSV_ABI void (*)(OrbisNpMatching2ContextId contextId,
                                                              OrbisNpMatching2Event event,
                                                              OrbisNpMatching2EventCause cause,
                                                              int errorCode, void* userdata);

using OrbisNpMatching2LobbyEventCallback =
    PS4_SYSV_ABI void (*)(OrbisNpMatching2ContextId contextId, OrbisNpMatching2LobbyId lobbyId,
                          OrbisNpMatching2Event event, const void* data, void* userdata);

using OrbisNpMatching2LobbyMessageCallback =
    PS4_SYSV_ABI void (*)(OrbisNpMatching2ContextId contextId, OrbisNpMatching2LobbyId lobbyId,
                          OrbisNpMatching2LobbyMemberId srcMemberId, OrbisNpMatching2Event event,
                          const void* data, void* userdata);

using OrbisNpMatching2RequestCallback = PS4_SYSV_ABI void (*)(OrbisNpMatching2ContextId,
                                                              OrbisNpMatching2RequestId,
                                                              OrbisNpMatching2Event, int,
                                                              const void*, void*);

using OrbisNpMatching2RoomEventCallback = PS4_SYSV_ABI void (*)(OrbisNpMatching2ContextId contextId,
                                                                OrbisNpMatching2RoomId roomId,
                                                                OrbisNpMatching2Event event,
                                                                const void* data, void* userdata);

using OrbisNpMatching2RoomMessageCallback =
    PS4_SYSV_ABI void (*)(OrbisNpMatching2ContextId contextId, OrbisNpMatching2RoomId roomId,
                          OrbisNpMatching2RoomMemberId srcMemberId, OrbisNpMatching2Event event,
                          const void* data, void* userdata);

using OrbisNpMatching2SignalingCallback =
    PS4_SYSV_ABI void (*)(OrbisNpMatching2ContextId contextId, OrbisNpMatching2RoomId roomId,
                          OrbisNpMatching2RoomMemberId roomMemberId, OrbisNpMatching2Event event,
                          int errorCode, void* userdata);

// internal - to be removed.
struct NpMatching2ContextEvent {
    OrbisNpMatching2ContextId contextId;
    OrbisNpMatching2Event event;
    OrbisNpMatching2EventCause cause;
    int errorCode;
};

// internal - to be removed.
struct NpMatching2LobbyEvent {
    OrbisNpMatching2ContextId contextId;
    OrbisNpMatching2LobbyId lobbyId;
    OrbisNpMatching2Event event;
    void* data;
};

// internal - to be removed.
struct NpMatching2RoomEvent {
    OrbisNpMatching2ContextId contextId;
    OrbisNpMatching2RoomId roomId;
    OrbisNpMatching2Event event;
    void* data;
};

struct OrbisNpMatching2BinAttr {
    OrbisNpMatching2AttributeId id;
    u8 pad[6];
    u8* data;
    u64 dataSize;
};

struct OrbisNpMatching2CreateContextParameter {
    Libraries::Np::OrbisNpId* npId;
    void* npCommunicationId;
    void* npPassphrase;
    Libraries::Np::OrbisNpServiceLabel serviceLabel;
    u64 size;
};
static_assert(sizeof(OrbisNpMatching2CreateContextParameter) == 0x28);

struct OrbisNpMatching2CreateContextParameterA {
    Libraries::UserService::OrbisUserServiceUserId userId;
    Libraries::Np::OrbisNpServiceLabel serviceLabel;
    u64 size;
};
static_assert(sizeof(OrbisNpMatching2CreateContextParameterA) == 16);

struct OrbisNpMatching2IntAttr {
    OrbisNpMatching2AttributeId id;
    u8 padding[2];
    u32 num;
};

struct OrbisNpMatching2SignalingOptParam {
    OrbisNpMatching2SignalingType type;
    OrbisNpMatching2SignalingFlag flag;
    OrbisNpMatching2RoomMemberId memberId;
    u8 padding[4];
};

struct OrbisNpMatching2CreateJoinRoomRequest {
    u16 maxSlot;
    OrbisNpMatching2TeamId teamId;
    u8 pad[5];
    OrbisNpMatching2Flags flags;
    OrbisNpMatching2WorldId worldId;
    OrbisNpMatching2LobbyId lobbyId;
    void* roomPasswd;
    OrbisNpMatching2RoomPasswordSlotMask* passwdSlotMask;
    void* groupConfig;
    u64 groupConfigs;
    void* joinGroupLabel;
    Libraries::Np::OrbisNpOnlineId* allowedUser;
    u64 allowedUsers;
    Libraries::Np::OrbisNpOnlineId* blockedUser;
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
    OrbisNpMatching2SignalingOptParam* signalingParam;
};
static_assert(sizeof(OrbisNpMatching2CreateJoinRoomRequest) == 184);

struct OrbisNpMatching2CreateJoinRoomRequestA {
    u16 maxSlot;
    OrbisNpMatching2TeamId teamId;
    u8 pad[5];
    OrbisNpMatching2Flags flags;
    OrbisNpMatching2WorldId worldId;
    OrbisNpMatching2LobbyId lobbyId;
    void* roomPasswd;
    OrbisNpMatching2RoomPasswordSlotMask* passwdSlotMask;
    void* groupConfig;
    u64 groupConfigs;
    void* joinGroupLabel;
    Libraries::Np::OrbisNpAccountId* allowedUser;
    u64 allowedUsers;
    Libraries::Np::OrbisNpAccountId* blockedUser;
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
    OrbisNpMatching2SignalingOptParam* signalingParam;
};
static_assert(sizeof(OrbisNpMatching2CreateJoinRoomRequestA) == 184);

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
    OrbisNpMatching2RoomPasswordSlotMask passwdSlotMask;
    u64 joinedSlotMask;
    OrbisNpMatching2RoomGroup* roomGroup;
    u64 roomGroups;
    OrbisNpMatching2Flags flags;
    u8 pad[4];
    OrbisNpMatching2RoomBinAttrInternal* internalBinAttr;
    u64 internalBinAttrs;
};

struct OrbisNpMatching2RoomMemberDataInternalA {
    OrbisNpMatching2RoomMemberDataInternalA* next;
    Libraries::Rtc::OrbisRtcTick joinDateTicks;
    Libraries::Np::OrbisNpPeerAddressA user;
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

struct OrbisNpMatching2RoomMemberDataInternalListA {
    OrbisNpMatching2RoomMemberDataInternalA* members;
    u64 membersNum;
    OrbisNpMatching2RoomMemberDataInternalA* me;
    OrbisNpMatching2RoomMemberDataInternalA* owner;
};

struct OrbisNpMatching2CreateJoinRoomResponseA {
    OrbisNpMatching2RoomDataInternal* roomData;
    OrbisNpMatching2RoomMemberDataInternalListA members;
};

struct OrbisNpMatching2GetWorldInfoListRequest {
    OrbisNpMatching2ServerId serverId;
};

struct OrbisNpMatching2World {
    OrbisNpMatching2World* next;
    OrbisNpMatching2WorldId worldId;
    u32 lobbiesNum;
    u32 maxLobbyMembersNum;
    u32 lobbyMembersNum;
    u32 roomsNum;
    u32 roomMembersNum;
    u8 pad[3];
};

struct OrbisNpMatching2GetWorldInfoListResponse {
    OrbisNpMatching2World* world;
    u64 worldNum;
};

struct OrbisNpMatching2GroupLabel {
    u8 data[8];
};

struct OrbisNpMatching2SessionPassword {
    u8 data[8];
};

struct OrbisNpMatching2InitializeParameter {
    u64 poolSize;
    u64 unknown;
    s32 priority;
    u8 padding[4];
    u64 stackSize;
    u64 size;
    u64 sslPoolSize;
};
static_assert(sizeof(OrbisNpMatching2InitializeParameter) == 0x30);

struct OrbisNpMatching2PresenceOptionData {
    u8 data[16];
    u64 len;
};

struct OrbisNpMatching2RoomMemberUpdateA {
    OrbisNpMatching2RoomMemberDataInternalA* roomMemberDataInternal;
    OrbisNpMatching2EventCause eventCause;
    u8 padding[7];
    OrbisNpMatching2PresenceOptionData optData;
};

struct OrbisNpMatching2RoomUpdate {
    OrbisNpMatching2EventCause eventCause;
    u8 padding[3];
    s32 errorCode;
    OrbisNpMatching2PresenceOptionData optData;
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
    Libraries::Np::OrbisNpOnlineId* blockedUser;
    u64 blockedUsers;

    int Validate() {
        return 0;
    }
};
static_assert(sizeof(OrbisNpMatching2JoinRoomRequest) == 0x58);

struct OrbisNpMatching2KickoutRoomMemberRequest {
    OrbisNpMatching2RoomId roomId;
    OrbisNpMatching2RoomMemberId memberId;
    u8 pad0[6];
    u32 blockKickFlag;
    u8 pad1[12];
    u64 reserved;

    int Validate() {
        return 0;
    }
};
static_assert(sizeof(OrbisNpMatching2KickoutRoomMemberRequest) == 0x28);

struct OrbisNpMatching2LeaveRoomRequest {
    OrbisNpMatching2RoomId roomId;
    OrbisNpMatching2PresenceOptionData optData;
};

struct OrbisNpMatching2LeaveRoomResponse {
    OrbisNpMatching2RoomId roomId;
};

struct OrbisNpMatching2Range {
    u32 start;
    u32 total;
    u32 results;
    u8 pad[4];
};

struct OrbisNpMatching2RangeFilter {
    u32 start;
    u32 max;
};

using OrbisNpMatching2Operator = u8;

struct OrbisNpMatching2IntSearchFilter {
    OrbisNpMatching2Operator searchOperator;
    u8 padding[7];
    OrbisNpMatching2IntAttr attr;
};

struct OrbisNpMatching2BinSearchFilter {
    OrbisNpMatching2Operator searchOperator;
    u8 padding[7];
    OrbisNpMatching2BinAttr attr;
};

struct OrbisNpMatching2RequestOptParam {
    OrbisNpMatching2RequestCallback callback;
    void* arg;
    u32 timeout;
    u16 appId;
    u8 padding[2];
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
    OrbisNpMatching2RoomPasswordSlotMask passwdSlotMask;
    u64 joinedSlotMask;
    u16 publicSlots;
    u16 privateSlots;
    u16 openPublicSlots;
    u16 openPrivateSlots;
    Libraries::Np::OrbisNpPeerAddressA owner;
    Libraries::Np::OrbisNpOnlineId ownerOnlineId;
    OrbisNpMatching2RoomGroupInfo* roomGroup;
    u64 roomGroups;
    OrbisNpMatching2IntAttr* externalSearchIntAttr;
    u64 externalSearchIntAttrs;
    OrbisNpMatching2BinAttr* externalSearchBinAttr;
    u64 externalSearchBinAttrs;
    OrbisNpMatching2BinAttr* externalBinAttr;
    u64 externalBinAttrs;
};

struct OrbisNpMatching2RoomGroup {
    OrbisNpMatching2RoomGroupId id;
    bool hasPasswd;
    bool hasLabel;
    u8 pad;
    OrbisNpMatching2GroupLabel label;
    u32 slots;
    u32 groupMembers;
};

struct OrbisNpMatching2RoomGroupConfig {
    u32 slots;
    bool hasLabel;
    OrbisNpMatching2GroupLabel label;
    bool hasPassword;
    u8 pad[2];
};

struct OrbisNpMatching2RoomGroupInfo {
    OrbisNpMatching2RoomGroupId id;
    bool hasPasswd;
    u8 pad[2];
    u32 slots;
    u32 groupMembers;
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

struct OrbisNpMatching2RoomMemberDataInternal {
    OrbisNpMatching2RoomMemberDataInternal* next;
    u64 joinDate;
    Libraries::Np::OrbisNpId npId;
    u8 pad[4];
    OrbisNpMatching2RoomMemberId memberId;
    OrbisNpMatching2TeamId teamId;
    OrbisNpMatching2NatType natType;
    OrbisNpMatching2Flags flagAttr;
    OrbisNpMatching2RoomGroup* roomGroup;
    OrbisNpMatching2RoomMemberBinAttrInternal* roomMemberBinAttrInternal;
    u64 roomMemberBinAttrInternalNum;
};
static_assert(sizeof(OrbisNpMatching2RoomMemberDataInternal) == 0x58);

struct OrbisNpMatching2RoomMemberUpdate {
    OrbisNpMatching2RoomMemberDataInternal* roomMemberDataInternal;
    OrbisNpMatching2EventCause eventCause;
    u8 padding[7];
    OrbisNpMatching2PresenceOptionData optData;
};

struct OrbisNpMatching2RoomMemberDataInternalList {
    OrbisNpMatching2RoomMemberDataInternal* members;
    u64 membersNum;
    OrbisNpMatching2RoomMemberDataInternal* me;
    OrbisNpMatching2RoomMemberDataInternal* owner;
};

struct OrbisNpMatching2CreateJoinRoomResponse {
    const OrbisNpMatching2RoomDataInternal* roomData;
    OrbisNpMatching2RoomMemberDataInternalList members;
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
    Libraries::Np::OrbisNpId* ownerNpId;
    OrbisNpMatching2RoomGroupInfo* roomGroup;
    u64 roomGroups;
    OrbisNpMatching2IntAttr* externalSearchIntAttr;
    u64 externalSearchIntAttrs;
    OrbisNpMatching2BinAttr* externalSearchBinAttr;
    u64 externalSearchBinAttrs;
    OrbisNpMatching2BinAttr* externalBinAttr;
    u64 externalBinAttrs;
};

struct OrbisNpMatching2SearchRoomRequest {
    int option;
    OrbisNpMatching2WorldId worldId;
    OrbisNpMatching2LobbyId lobbyId;
    OrbisNpMatching2RangeFilter rangeFilter;
    OrbisNpMatching2Flags flags1;
    OrbisNpMatching2Flags flags2;
    OrbisNpMatching2IntSearchFilter* intFilter;
    u64 intFilters;
    OrbisNpMatching2BinSearchFilter* binFilter;
    u64 binFilters;
    OrbisNpMatching2AttributeId* attr;
    u64 attrs;
};

struct OrbisNpMatching2SearchRoomResponse {
    OrbisNpMatching2Range range;
    OrbisNpMatching2RoomDataExternal* roomDataExt;
};

struct OrbisNpMatching2SearchRoomResponseA {
    OrbisNpMatching2Range range;
    OrbisNpMatching2RoomDataExternalA* roomDataExt;
};

struct OrbisNpMatching2SetRoomDataExternalRequest {
    OrbisNpMatching2RoomId roomId;
    OrbisNpMatching2IntAttr* roomSearchableIntAttrExternal;
    u32 roomSearchableIntAttrExternalNum;
    u8 pad1[4];
    OrbisNpMatching2BinAttr* roomSearchableBinAttrExternal;
    u32 roomSearchableBinAttrExternalNum;
    u8 pad2[4];
    OrbisNpMatching2BinAttr* roomBinAttrExternal;
    u32 roomBinAttrExternalNum;
    u8 pad3[4];
};

struct OrbisNpMatching2SetRoomDataInternalRequest {
    OrbisNpMatching2RoomId roomId;
    OrbisNpMatching2Flags flagFilter;
    OrbisNpMatching2Flags flagAttr;
    OrbisNpMatching2BinAttr* roomBinAttrInternal;
    u32 roomBinAttrInternalNum;
    OrbisNpMatching2RoomGroupConfig* passwordConfig;
    u32 passwordConfigNum;
    u64* passwordSlotMask;
    OrbisNpMatching2RoomMemberId* ownerPrivilegeRank;
    u32 ownerPrivilegeRankNum;
    u8 padding[4];
};

struct OrbisNpMatching2SetUserInfoRequest {
    OrbisNpMatching2ServerId serverId;
    u8 padding[6];
    OrbisNpMatching2BinAttr* userBinAttr;
    u64 userBinAttrs;
};

struct OrbisNpMatching2SignalingConnectionInfoAddr {
    u32 addr;
    u16 port;
    u8 pad[2];
};

struct OrbisNpMatching2SignalingConnectionInfo {
    OrbisNpMatching2SignalingConnectionInfoAddr address;
};

// internal - to be removed.
struct OrbisNpMatching2SignalingEvent {
    OrbisNpMatching2ContextId contextId;
    OrbisNpMatching2RoomId roomId;
    OrbisNpMatching2RoomMemberId roomMemberId;
    OrbisNpMatching2Event event;
    int errorCode;
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

struct OrbisNpMatching2SignalingParam {
    int type;
    int flag;
    OrbisNpMatching2RoomMemberId mainMember;
    u8 pad[4];
};

} // namespace Libraries::Np::NpMatching2
