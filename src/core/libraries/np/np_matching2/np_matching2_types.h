// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/np/np_matching2/np_matching2.h"
#include "core/libraries/np/np_types.h"
#include "core/libraries/rtc/rtc.h"
#include "core/libraries/system/userservice.h"

namespace Libraries::Np::NpMatching2 {

using OrbisNpMatching2ContextCallback = PS4_SYSV_ABI void (*)(OrbisNpMatching2ContextId contextId,
                                                              OrbisNpMatching2Event event,
                                                              OrbisNpMatching2EventCause cause,
                                                              int errorCode, void* userdata);

using OrbisNpMatching2LobbyEventCallback =
    PS4_SYSV_ABI void (*)(OrbisNpMatching2ContextId contextId, OrbisNpMatching2LobbyId lobbyId,
                          OrbisNpMatching2Event event, void* data, void* userdata);

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
    void* roomGroup;
    u64 roomGroups;
    OrbisNpMatching2Flags flags;
    u8 pad[4];
    void* internalBinAttr;
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
    void* roomGroup;
    void* roomMemberInternalBinAttr;
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

struct OrbisNpMatching2LeaveRoomRequest {
    OrbisNpMatching2RoomId roomId;
    OrbisNpMatching2PresenceOptionData optData;
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
    void* roomGroup;
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
    void* intFilter;
    u64 intFilters;
    void* binFilter;
    u64 binFilters;
    OrbisNpMatching2AttributeId* attr;
    u64 attrs;
};

struct OrbisNpMatching2SearchRoomResponseA {
    OrbisNpMatching2Range range;
    OrbisNpMatching2RoomDataExternalA* roomDataExt;
};

struct OrbisNpMatching2SetUserInfoRequest {
    OrbisNpMatching2ServerId serverId;
    u8 padding[6];
    OrbisNpMatching2BinAttr* userBinAttr;
    u64 userBinAttrs;
};

// internal - to be removed.
struct OrbisNpMatching2SignalingEvent {
    OrbisNpMatching2ContextId contextId;
    OrbisNpMatching2RoomId roomId;
    OrbisNpMatching2RoomMemberId roomMemberId;
    OrbisNpMatching2Event event;
    int errorCode;
};

} // namespace Libraries::Np::NpMatching2
