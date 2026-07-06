// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/np/np_types.h"
#include "core/libraries/rtc/rtc.h"
#include "core/libraries/system/userservice.h"

namespace Libraries::Np::NpMatching2 {

// Constants for matching functions and structures
constexpr u32 ORBIS_NP_MATCHING2_SESSION_PASSWORD_SIZE = 8;
constexpr u32 ORBIS_NP_MATCHING2_GROUP_LABEL_SIZE = 8;

// Default initialization values
enum OrbisNpMatching2InitDefault : u64 {
    ORBIS_NP_MATCHING2_THREAD_STACK_SIZE_DEFAULT = 32768,
    ORBIS_NP_MATCHING2_POOLSIZE_DEFAULT = 131072,
    ORBIS_NP_MATCHING2_SSL_POOLSIZE_DEFAULT = 196608,
};

using OrbisNpMatching2AttributeId = u16;
using OrbisNpMatching2BlockKickFlag = u8;
using OrbisNpMatching2ContextId = u16;
using OrbisNpMatching2FlagAttr = u32;
using OrbisNpMatching2Flags = u32;
using OrbisNpMatching2LobbyId = u64;
using OrbisNpMatching2LobbyMemberId = u16;
using OrbisNpMatching2NatType = u8;
using OrbisNpMatching2Operator = u8;
using OrbisNpMatching2RequestId = u32;
using OrbisNpMatching2RoomGroupId = u8;
using OrbisNpMatching2RoomId = u64;
using OrbisNpMatching2RoomMemberId = u16;
using OrbisNpMatching2RoomPasswordSlotMask = u64;
using OrbisNpMatching2ServerId = u16;
using OrbisNpMatching2SessionType = u8;
using OrbisNpMatching2SignalingFlag = u8;
using OrbisNpMatching2SignalingRequestId = u32;
using OrbisNpMatching2TeamId = u8;
using OrbisNpMatching2WorldId = u32;

// Event of request/room/signaling/context functions
enum OrbisNpMatching2Event : u16 {
    ORBIS_NP_MATCHING2_REQUEST_EVENT_GET_WORLD_INFO_LIST = 0x0002,
    ORBIS_NP_MATCHING2_REQUEST_EVENT_GET_ROOM_MEMBER_DATA_EXTERNAL_LIST = 0x0003,
    ORBIS_NP_MATCHING2_REQUEST_EVENT_SET_ROOM_DATA_EXTERNAL = 0x0004,
    ORBIS_NP_MATCHING2_REQUEST_EVENT_GET_ROOM_DATA_EXTERNAL_LIST = 0x0005,
    ORBIS_NP_MATCHING2_REQUEST_EVENT_SET_USER_INFO = 0x0007,
    ORBIS_NP_MATCHING2_REQUEST_EVENT_GET_USER_INFO_LIST = 0x0008,
    ORBIS_NP_MATCHING2_REQUEST_EVENT_CREATE_JOIN_ROOM = 0x0101,
    ORBIS_NP_MATCHING2_REQUEST_EVENT_JOIN_ROOM = 0x0102,
    ORBIS_NP_MATCHING2_REQUEST_EVENT_LEAVE_ROOM = 0x0103,
    ORBIS_NP_MATCHING2_REQUEST_EVENT_KICKOUT_ROOM_MEMBER = 0x0104,
    ORBIS_NP_MATCHING2_REQUEST_EVENT_GRANT_ROOM_OWNER = 0x0105,
    ORBIS_NP_MATCHING2_REQUEST_EVENT_SEARCH_ROOM = 0x0106,
    ORBIS_NP_MATCHING2_REQUEST_EVENT_SEND_ROOM_MESSAGE = 0x0108,
    ORBIS_NP_MATCHING2_REQUEST_EVENT_SET_ROOM_DATA_INTERNAL = 0x0109,
    ORBIS_NP_MATCHING2_REQUEST_EVENT_GET_ROOM_DATA_INTERNAL = 0x010A,
    ORBIS_NP_MATCHING2_REQUEST_EVENT_JOIN_LOBBY = 0x0201,
    ORBIS_NP_MATCHING2_REQUEST_EVENT_SIGNALING_GET_PING_INFO = 0x0E01,
    ORBIS_NP_MATCHING2_ROOM_EVENT_MEMBER_JOINED = 0x1101,
    ORBIS_NP_MATCHING2_ROOM_EVENT_MEMBER_LEFT = 0x1102,
    ORBIS_NP_MATCHING2_ROOM_EVENT_KICKEDOUT = 0x1103,
    ORBIS_NP_MATCHING2_ROOM_EVENT_ROOM_DESTROYED = 0x1104,
    ORBIS_NP_MATCHING2_ROOM_EVENT_ROOM_OWNER_CHANGED = 0x1105,
    ORBIS_NP_MATCHING2_ROOM_EVENT_UPDATED_ROOM_DATA_INTERNAL = 0x1106,
    ORBIS_NP_MATCHING2_ROOM_EVENT_UPDATED_ROOM_MEMBER_DATA_INTERNAL = 0x1107,
    ORBIS_NP_MATCHING2_ROOM_EVENT_UPDATED_SIGNALING_OPT_PARAM = 0x1108,
    ORBIS_NP_MATCHING2_SIGNALING_EVENT_DEAD = 0x5101,
    ORBIS_NP_MATCHING2_SIGNALING_EVENT_ESTABLISHED = 0x5102,
    ORBIS_NP_MATCHING2_SIGNALING_EVENT_NETINFO_ERROR = 0x5103,
    ORBIS_NP_MATCHING2_CONTEXT_EVENT_START_OVER = 0x6F01,
    ORBIS_NP_MATCHING2_CONTEXT_EVENT_STARTED = 0x6F02,
    ORBIS_NP_MATCHING2_CONTEXT_EVENT_STOPPED = 0x6F03,
    ORBIS_NP_MATCHING2_REQUEST_EVENT_CREATE_JOIN_ROOM_A = 0x7101,
    ORBIS_NP_MATCHING2_REQUEST_EVENT_JOIN_ROOM_A = 0x7102,
    ORBIS_NP_MATCHING2_REQUEST_EVENT_GET_ROOM_MEMBER_DATA_EXTERNAL_LIST_A = 0x7003,
    ORBIS_NP_MATCHING2_REQUEST_EVENT_GET_ROOM_DATA_EXTERNAL_LIST_A = 0x7005,
    ORBIS_NP_MATCHING2_REQUEST_EVENT_SEARCH_ROOM_A = 0x7106,
    ORBIS_NP_MATCHING2_ROOM_EVENT_MEMBER_JOINED_A = 0x8101,
    ORBIS_NP_MATCHING2_ROOM_EVENT_MEMBER_LEFT_A = 0x8102,
    ORBIS_NP_MATCHING2_ROOM_EVENT_UPDATED_ROOM_MEMBER_DATA_INTERNAL_A = 0x8107,
    ORBIS_NP_MATCHING2_ROOM_MSG_EVENT_MESSAGE_A = 0x9102,
};

// Event cause
enum OrbisNpMatching2EventCause : u8 {
    ORBIS_NP_MATCHING2_EVENT_CAUSE_LEAVE_ACTION = 1,
    ORBIS_NP_MATCHING2_EVENT_CAUSE_KICKOUT_ACTION = 2,
    ORBIS_NP_MATCHING2_EVENT_CAUSE_GRANT_OWNER_ACTION = 3,
    ORBIS_NP_MATCHING2_EVENT_CAUSE_SERVER_OPERATION = 4,
    ORBIS_NP_MATCHING2_EVENT_CAUSE_MEMBER_DISAPPEARED = 5,
    ORBIS_NP_MATCHING2_EVENT_CAUSE_SERVER_INTERNAL = 6,
    ORBIS_NP_MATCHING2_EVENT_CAUSE_CONNECTION_ERROR = 7,
    ORBIS_NP_MATCHING2_EVENT_CAUSE_NP_SIGNED_OUT = 8,
    ORBIS_NP_MATCHING2_EVENT_CAUSE_SYSTEM_ERROR = 9,
    ORBIS_NP_MATCHING2_EVENT_CAUSE_CONTEXT_ERROR = 10,
    ORBIS_NP_MATCHING2_EVENT_CAUSE_CONTEXT_ACTION = 11,
};

// Signaling type
enum OrbisNpMatching2SignalingType : u8 {
    ORBIS_NP_MATCHING2_SIGNALING_TYPE_NONE = 0,
    ORBIS_NP_MATCHING2_SIGNALING_TYPE_MESH = 1,
    ORBIS_NP_MATCHING2_SIGNALING_TYPE_STAR = 2,
};

// Type of connection information to acquire
enum OrbisNpMatching2SignalingConnectionInfoType : s32 {
    ORBIS_NP_MATCHING2_SIGNALING_CONN_INFO_RTT = 1,
    ORBIS_NP_MATCHING2_SIGNALING_CONN_INFO_BANDWIDTH = 2,
    ORBIS_NP_MATCHING2_SIGNALING_CONN_INFO_PEER_NP_ID = 3,
    ORBIS_NP_MATCHING2_SIGNALING_CONN_INFO_PEER_ADDR = 4,
    ORBIS_NP_MATCHING2_SIGNALING_CONN_INFO_MAPPED_ADDR = 5,
    ORBIS_NP_MATCHING2_SIGNALING_CONN_INFO_PACKET_LOSS = 6,
    ORBIS_NP_MATCHING2_SIGNALING_CONN_INFO_PEER_NPID = 7,
    ORBIS_NP_MATCHING2_SIGNALING_CONN_INFO_PEER_ADDRESS_A = 7,
};

// Attribute IDs of external/internal room and room member binary attributes
enum OrbisNpMatching2AttrId : OrbisNpMatching2AttributeId {
    ORBIS_NP_MATCHING2_ROOM_SEARCHABLE_INT_ATTR_EXTERNAL_1_ID = 0x004C,
    ORBIS_NP_MATCHING2_ROOM_SEARCHABLE_INT_ATTR_EXTERNAL_2_ID = 0x004D,
    ORBIS_NP_MATCHING2_ROOM_SEARCHABLE_INT_ATTR_EXTERNAL_3_ID = 0x004E,
    ORBIS_NP_MATCHING2_ROOM_SEARCHABLE_INT_ATTR_EXTERNAL_4_ID = 0x004F,
    ORBIS_NP_MATCHING2_ROOM_SEARCHABLE_INT_ATTR_EXTERNAL_5_ID = 0x0050,
    ORBIS_NP_MATCHING2_ROOM_SEARCHABLE_INT_ATTR_EXTERNAL_6_ID = 0x0051,
    ORBIS_NP_MATCHING2_ROOM_SEARCHABLE_INT_ATTR_EXTERNAL_7_ID = 0x0052,
    ORBIS_NP_MATCHING2_ROOM_SEARCHABLE_INT_ATTR_EXTERNAL_8_ID = 0x0053,
    ORBIS_NP_MATCHING2_ROOM_SEARCHABLE_BIN_ATTR_EXTERNAL_1_ID = 0x0054,
    ORBIS_NP_MATCHING2_ROOM_BIN_ATTR_EXTERNAL_1_ID = 0x0055,
    ORBIS_NP_MATCHING2_ROOM_BIN_ATTR_EXTERNAL_2_ID = 0x0056,
    ORBIS_NP_MATCHING2_ROOM_BIN_ATTR_INTERNAL_1_ID = 0x0057,
    ORBIS_NP_MATCHING2_ROOM_BIN_ATTR_INTERNAL_2_ID = 0x0058,
    ORBIS_NP_MATCHING2_ROOMMEMBER_BIN_ATTR_INTERNAL_1_ID = 0x0059,
};

// Flag-type room attribute
enum OrbisNpMatching2RoomFlagAttr : OrbisNpMatching2Flags {
    ORBIS_NP_MATCHING2_ROOM_FLAG_ATTR_OWNER_AUTO_GRANT = 0x80000000,
    ORBIS_NP_MATCHING2_ROOM_FLAG_ATTR_CLOSED = 0x40000000,
    ORBIS_NP_MATCHING2_ROOM_FLAG_ATTR_FULL = 0x20000000,
    ORBIS_NP_MATCHING2_ROOM_FLAG_ATTR_HIDDEN = 0x10000000,
    ORBIS_NP_MATCHING2_ROOM_FLAG_ATTR_NAT_TYPE_RESTRICTION = 0x04000000,
    ORBIS_NP_MATCHING2_ROOM_FLAG_ATTR_PROHIBITIVE_MODE = 0x02000000,
};

// Flag-type room member attribute
enum OrbisNpMatching2RoomMemberFlagAttr : OrbisNpMatching2Flags {
    ORBIS_NP_MATCHING2_ROOMMEMBER_FLAG_ATTR_OWNER = 0x80000000,
};

// Member role
enum OrbisNpMatching2Role : u8 {
    ORBIS_NP_MATCHING2_ROLE_MEMBER = 1,
    ORBIS_NP_MATCHING2_ROLE_OWNER = 2,
};

// Context callback function
using OrbisNpMatching2ContextCallback = PS4_SYSV_ABI void (*)(OrbisNpMatching2ContextId contextId,
                                                              OrbisNpMatching2Event event,
                                                              OrbisNpMatching2EventCause cause,
                                                              int errorCode, void* userdata);

// Lobby event callback function
using OrbisNpMatching2LobbyEventCallback =
    PS4_SYSV_ABI void (*)(OrbisNpMatching2ContextId contextId, OrbisNpMatching2LobbyId lobbyId,
                          OrbisNpMatching2Event event, const void* data, void* userdata);

// Lobby message callback function
using OrbisNpMatching2LobbyMessageCallback =
    PS4_SYSV_ABI void (*)(OrbisNpMatching2ContextId contextId, OrbisNpMatching2LobbyId lobbyId,
                          OrbisNpMatching2LobbyMemberId srcMemberId, OrbisNpMatching2Event event,
                          const void* data, void* userdata);

// Request callback function
using OrbisNpMatching2RequestCallback = PS4_SYSV_ABI void (*)(OrbisNpMatching2ContextId,
                                                              OrbisNpMatching2RequestId,
                                                              OrbisNpMatching2Event, int,
                                                              const void*, void*);

// Room event callback function
using OrbisNpMatching2RoomEventCallback = PS4_SYSV_ABI void (*)(OrbisNpMatching2ContextId contextId,
                                                                OrbisNpMatching2RoomId roomId,
                                                                OrbisNpMatching2Event event,
                                                                const void* data, void* userdata);

// Room message callback function
using OrbisNpMatching2RoomMessageCallback =
    PS4_SYSV_ABI void (*)(OrbisNpMatching2ContextId contextId, OrbisNpMatching2RoomId roomId,
                          OrbisNpMatching2RoomMemberId srcMemberId, OrbisNpMatching2Event event,
                          const void* data, void* userdata);

// Signaling callback function
using OrbisNpMatching2SignalingCallback =
    PS4_SYSV_ABI void (*)(OrbisNpMatching2ContextId contextId, OrbisNpMatching2RoomId roomId,
                          OrbisNpMatching2RoomMemberId roomMemberId, OrbisNpMatching2Event event,
                          int errorCode, void* userdata);

struct OrbisNpMatching2RoomGroup;
struct OrbisNpMatching2RoomGroupInfo;
struct OrbisNpMatching2RoomBinAttrInternal;
struct OrbisNpMatching2RoomMemberBinAttrInternal;

// Session password
struct OrbisNpMatching2SessionPassword {
    u8 data[8];
};

// Group label
struct OrbisNpMatching2GroupLabel {
    u8 data[8];
};

// Optional presence data
struct OrbisNpMatching2PresenceOptionData {
    u8 data[16];
    u64 len;
};

// Integer-type attribute
struct OrbisNpMatching2IntAttr {
    OrbisNpMatching2AttributeId id;
    u8 padding[2];
    u32 num;
};

// Binary-type attribute
struct OrbisNpMatching2BinAttr {
    OrbisNpMatching2AttributeId id;
    u8 pad[6];
    u8* data;
    u64 dataSize;
};

// Range filter
struct OrbisNpMatching2RangeFilter {
    u32 start;
    u32 max;
};

// Integer-type search condition
struct OrbisNpMatching2IntSearchFilter {
    OrbisNpMatching2Operator searchOperator;
    u8 padding[7];
    OrbisNpMatching2IntAttr attr;
};

// Binary-type search condition
struct OrbisNpMatching2BinSearchFilter {
    OrbisNpMatching2Operator searchOperator;
    u8 padding[7];
    OrbisNpMatching2BinAttr attr;
};

// Range of result
struct OrbisNpMatching2Range {
    u32 start;
    u32 total;
    u32 results;
    u8 pad[4];
};

// Signaling optional parameter
struct OrbisNpMatching2SignalingOptParam {
    OrbisNpMatching2SignalingType type;
    OrbisNpMatching2SignalingFlag flag;
    OrbisNpMatching2RoomMemberId memberId;
    u8 padding[4];
};

// Set groups in a room
struct OrbisNpMatching2RoomGroupConfig {
    u32 slots;
    bool hasLabel;
    OrbisNpMatching2GroupLabel label;
    bool hasPassword;
    u8 pad[2];
};

// Set group password
struct OrbisNpMatching2RoomGroupPasswordConfig {
    OrbisNpMatching2RoomGroupId groupId;
    bool hasPassword;
    u8 pad[1];
};

// Group (of slots in a room)
struct OrbisNpMatching2RoomGroup {
    OrbisNpMatching2RoomGroupId id;
    bool hasPasswd;
    bool hasLabel;
    u8 pad;
    OrbisNpMatching2GroupLabel label;
    u32 slots;
    u32 groupMembers;
};

// Group information
struct OrbisNpMatching2RoomGroupInfo {
    OrbisNpMatching2RoomGroupId id;
    bool hasPasswd;
    u8 pad[2];
    u32 slots;
    u32 groupMembers;
};

// Room-internal binary attribute
struct OrbisNpMatching2RoomBinAttrInternal {
    Libraries::Rtc::OrbisRtcTick lastUpdate;
    OrbisNpMatching2RoomMemberId memberId;
    u8 pad[6];
    OrbisNpMatching2BinAttr binAttr;
};

// Room member internal binary attribute
struct OrbisNpMatching2RoomMemberBinAttrInternal {
    Libraries::Rtc::OrbisRtcTick lastUpdate;
    OrbisNpMatching2BinAttr binAttr;
};

// Server
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

// Room-internal room information
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

// Room-external room information
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

// Room-external room information (account-id variant)
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

// Room-internal room member information
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

// Room-internal room member information (account-id variant)
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

// Room member ID list
struct OrbisNpMatching2RoomMemberDataInternalList {
    OrbisNpMatching2RoomMemberDataInternal* members;
    u64 membersNum;
    OrbisNpMatching2RoomMemberDataInternal* me;
    OrbisNpMatching2RoomMemberDataInternal* owner;
};

// Room member ID list (account-id variant)
struct OrbisNpMatching2RoomMemberDataInternalListA {
    OrbisNpMatching2RoomMemberDataInternalA* members;
    u64 membersNum;
    OrbisNpMatching2RoomMemberDataInternalA* me;
    OrbisNpMatching2RoomMemberDataInternalA* owner;
};

// Room-external room member information
struct OrbisNpMatching2RoomMemberDataExternal {
    OrbisNpMatching2RoomMemberDataExternal* next;
    Libraries::Np::OrbisNpId npId;
    Libraries::Rtc::OrbisRtcTick joinDate;
    OrbisNpMatching2Role role;
    u8 padding[7];
};

// Room-external room member information (account-id variant)
struct OrbisNpMatching2RoomMemberDataExternalA {
    OrbisNpMatching2RoomMemberDataExternalA* next;
    Libraries::Np::OrbisNpPeerAddressA user;
    Libraries::Np::OrbisNpOnlineId onlineId;
    Libraries::Rtc::OrbisRtcTick joinDate;
    OrbisNpMatching2Role role;
    u8 padding[7];
};

// Initialization parameter
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

// Create context parameter
struct OrbisNpMatching2CreateContextParameter {
    Libraries::Np::OrbisNpId* npId;
    void* npCommunicationId;
    void* npPassphrase;
    Libraries::Np::OrbisNpServiceLabel serviceLabel;
    u64 size;
};
static_assert(sizeof(OrbisNpMatching2CreateContextParameter) == 0x28);

// Create context parameter (account-id variant)
struct OrbisNpMatching2CreateContextParameterA {
    Libraries::UserService::OrbisUserServiceUserId userId;
    Libraries::Np::OrbisNpServiceLabel serviceLabel;
    u64 size;
};
static_assert(sizeof(OrbisNpMatching2CreateContextParameterA) == 16);

// Request optional parameter
struct OrbisNpMatching2RequestOptParam {
    OrbisNpMatching2RequestCallback callback;
    void* arg;
    u32 timeout;
    u16 appId;
    u8 padding[2];
};

// GetWorldInfoList request
struct OrbisNpMatching2GetWorldInfoListRequest {
    OrbisNpMatching2ServerId serverId;
};

// GetWorldInfoList response
struct OrbisNpMatching2GetWorldInfoListResponse {
    OrbisNpMatching2World* world;
    u64 worldNum;
};

// CreateJoinRoom request
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

// CreateJoinRoom request (account-id variant)
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

// CreateJoinRoom response
struct OrbisNpMatching2CreateJoinRoomResponse {
    const OrbisNpMatching2RoomDataInternal* roomData;
    OrbisNpMatching2RoomMemberDataInternalList members;
};

// CreateJoinRoom response (account-id variant)
struct OrbisNpMatching2CreateJoinRoomResponseA {
    OrbisNpMatching2RoomDataInternal* roomData;
    OrbisNpMatching2RoomMemberDataInternalListA members;
};

// JoinRoom request
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

// JoinRoom request (account-id variant)
struct OrbisNpMatching2JoinRoomRequestA {
    OrbisNpMatching2RoomId roomId;
    OrbisNpMatching2SessionPassword* roomPasswd;
    OrbisNpMatching2GroupLabel* joinGroupLabel;
    OrbisNpMatching2BinAttr* roomMemberBinInternalAttr;
    u64 roomMemberBinInternalAttrNum;
    OrbisNpMatching2PresenceOptionData optData;
    OrbisNpMatching2TeamId teamId;
    u8 pad[3];
    OrbisNpMatching2Flags flags;
    Libraries::Np::OrbisNpAccountId* blockedUser;
    u64 blockedUsers;

    int Validate() {
        return 0;
    }
};
static_assert(sizeof(OrbisNpMatching2JoinRoomRequestA) == 0x58);

// LeaveRoom request
struct OrbisNpMatching2LeaveRoomRequest {
    OrbisNpMatching2RoomId roomId;
    OrbisNpMatching2PresenceOptionData optData;
};

// LeaveRoom response
struct OrbisNpMatching2LeaveRoomResponse {
    OrbisNpMatching2RoomId roomId;
};

// KickoutRoomMember request
struct OrbisNpMatching2KickoutRoomMemberRequest {
    OrbisNpMatching2RoomId roomId;
    OrbisNpMatching2RoomMemberId memberId;
    OrbisNpMatching2BlockKickFlag blockKickFlag;
    u8 padding[5];
    OrbisNpMatching2PresenceOptionData optData;
};
static_assert(sizeof(OrbisNpMatching2KickoutRoomMemberRequest) == 0x28);

// SearchRoom request
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

// SearchRoom response
struct OrbisNpMatching2SearchRoomResponse {
    OrbisNpMatching2Range range;
    OrbisNpMatching2RoomDataExternal* roomDataExt;
};

// SearchRoom response (account-id variant)
struct OrbisNpMatching2SearchRoomResponseA {
    OrbisNpMatching2Range range;
    OrbisNpMatching2RoomDataExternalA* roomDataExt;
};

// GetRoomDataExternalList request
struct OrbisNpMatching2GetRoomDataExternalListRequest {
    OrbisNpMatching2RoomId* roomId;
    u64 roomIdNum;
    const OrbisNpMatching2AttributeId* attrId;
    u64 attrIdNum;
};

// GetRoomDataExternalList response
struct OrbisNpMatching2GetRoomDataExternalListResponse {
    OrbisNpMatching2RoomDataExternal* roomDataExternal;
    u64 roomDataExternalNum;
};

// GetRoomDataExternalList response (account-id variant)
struct OrbisNpMatching2GetRoomDataExternalListResponseA {
    OrbisNpMatching2RoomDataExternalA* roomDataExternal;
    u64 roomDataExternalNum;
};

// GetRoomMemberDataExternalList request
struct OrbisNpMatching2GetRoomMemberDataExternalListRequest {
    OrbisNpMatching2RoomId roomId;
};

// GetRoomMemberDataExternalList response
struct OrbisNpMatching2GetRoomMemberDataExternalListResponse {
    OrbisNpMatching2RoomMemberDataExternal* roomMemberDataExternal;
    u64 roomMemberDataExternalNum;
};

// GetRoomMemberDataExternalList response (account-id variant)
struct OrbisNpMatching2GetRoomMemberDataExternalListResponseA {
    OrbisNpMatching2RoomMemberDataExternalA* roomMemberDataExternal;
    u64 roomMemberDataExternalNum;
};

// SetRoomDataExternal request
struct OrbisNpMatching2SetRoomDataExternalRequest {
    OrbisNpMatching2RoomId roomId;
    const OrbisNpMatching2IntAttr* roomSearchableIntAttrExternal;
    u64 roomSearchableIntAttrExternalNum;
    const OrbisNpMatching2BinAttr* roomSearchableBinAttrExternal;
    u64 roomSearchableBinAttrExternalNum;
    const OrbisNpMatching2BinAttr* roomBinAttrExternal;
    u64 roomBinAttrExternalNum;
};

// SetRoomDataInternal request
struct OrbisNpMatching2SetRoomDataInternalRequest {
    OrbisNpMatching2RoomId roomId;
    OrbisNpMatching2Flags flagFilter;
    OrbisNpMatching2Flags flagAttr;
    const OrbisNpMatching2BinAttr* roomBinAttrInternal;
    u64 roomBinAttrInternalNum;
    const OrbisNpMatching2RoomGroupPasswordConfig* passwordConfig;
    u64 passwordConfigNum;
    const OrbisNpMatching2RoomPasswordSlotMask* passwordSlotMask;
    const OrbisNpMatching2RoomMemberId* ownerPrivilegeRank;
    u64 ownerPrivilegeRankNum;
};

// GetRoomDataInternal request
struct OrbisNpMatching2GetRoomDataInternalRequest {
    OrbisNpMatching2RoomId roomId;
    const OrbisNpMatching2AttributeId* attrId;
    u64 attrIdNum;
};

// SetUserInfo request
struct OrbisNpMatching2SetUserInfoRequest {
    OrbisNpMatching2ServerId serverId;
    u8 padding[6];
    OrbisNpMatching2BinAttr* userBinAttr;
    u64 userBinAttrs;
};

// Session a user is currently joined to
struct OrbisNpMatching2JoinedSessionInfo {
    OrbisNpMatching2SessionType sessionType;
    u8 padding[1];
    OrbisNpMatching2ServerId serverId;
    OrbisNpMatching2WorldId worldId;
    OrbisNpMatching2LobbyId lobbyId;
    OrbisNpMatching2RoomId roomId;
    Libraries::Rtc::OrbisRtcTick joinDate;
};

// User information
struct OrbisNpMatching2UserInfo {
    OrbisNpMatching2UserInfo* next;
    Libraries::Np::OrbisNpId npId;
    OrbisNpMatching2BinAttr* userBinAttr;
    u64 userBinAttrNum;
    OrbisNpMatching2JoinedSessionInfo joinedSessionInfo;
    u64 joinedSessionInfoNum;
};

// User information (account-id variant)
struct OrbisNpMatching2UserInfoA {
    OrbisNpMatching2UserInfoA* next;
    Libraries::Np::OrbisNpPeerAddressA user;
    Libraries::Np::OrbisNpOnlineId userOnlineId;
    OrbisNpMatching2BinAttr* userBinAttr;
    u64 userBinAttrNum;
    OrbisNpMatching2JoinedSessionInfo joinedSessionInfo;
    u64 joinedSessionInfoNum;
};

// GetUserInfoList request
struct OrbisNpMatching2GetUserInfoListRequest {
    OrbisNpMatching2ServerId serverId;
    u8 padding[6];
    Libraries::Np::OrbisNpId* npId;
    u64 npIdNum;
    const OrbisNpMatching2AttributeId* attrId;
    u64 attrIdNum;
    s32 option;
};

// GetUserInfoList response
struct OrbisNpMatching2GetUserInfoListResponse {
    OrbisNpMatching2UserInfo* userInfo;
    u64 userInfoNum;
};

// GetUserInfoList response (account-id variant)
struct OrbisNpMatching2GetUserInfoListResponseA {
    OrbisNpMatching2UserInfoA* userInfo;
    u64 userInfoNum;
};

// Room data internal update information
struct OrbisNpMatching2RoomDataInternalUpdate {
    OrbisNpMatching2RoomDataInternal* chgRoomDataInternal;
    OrbisNpMatching2FlagAttr* chgFlagAttr;
    OrbisNpMatching2FlagAttr* prevFlagAttr;
    OrbisNpMatching2RoomPasswordSlotMask* chgRoomPasswordSlotMask;
    OrbisNpMatching2RoomPasswordSlotMask* prevRoomPasswordSlotMask;
    OrbisNpMatching2RoomGroup** chgRoomGroup;
    u64 chgRoomGroupNum;
    OrbisNpMatching2RoomBinAttrInternal** chgRoomBinAttrInternal;
    u64 chgRoomBinAttrInternalNum;
};

// Room update information
struct OrbisNpMatching2RoomUpdate {
    OrbisNpMatching2EventCause eventCause;
    u8 padding[3];
    s32 errorCode;
    OrbisNpMatching2PresenceOptionData optData;
};

// Room member update information
struct OrbisNpMatching2RoomMemberUpdate {
    OrbisNpMatching2RoomMemberDataInternal* roomMemberDataInternal;
    OrbisNpMatching2EventCause eventCause;
    u8 padding[7];
    OrbisNpMatching2PresenceOptionData optData;
};

// Room member update information (account-id variant)
struct OrbisNpMatching2RoomMemberUpdateA {
    OrbisNpMatching2RoomMemberDataInternalA* roomMemberDataInternal;
    OrbisNpMatching2EventCause eventCause;
    u8 padding[7];
    OrbisNpMatching2PresenceOptionData optData;
};

// Signaling connection information address
struct OrbisNpMatching2SignalingConnectionInfoAddr {
    u32 addr;
    u16 port;
    u8 pad[2];
};

// Signaling connection information
union OrbisNpMatching2SignalingConnectionInfo {
    u32 rtt;
    u32 bandwidth;
    Libraries::Np::OrbisNpId npId;
    OrbisNpMatching2SignalingConnectionInfoAddr address;
    u32 packetLoss;
};
static_assert(sizeof(OrbisNpMatching2SignalingConnectionInfo) == 0x24);

// Signaling connection information (account-id variant)
union OrbisNpMatching2SignalingConnectionInfoA {
    u32 rtt;
    u32 bandwidth;
    Libraries::Np::OrbisNpPeerAddressA peerAddrA;
    OrbisNpMatching2SignalingConnectionInfoAddr address;
    u32 packetLoss;
};
static_assert(sizeof(OrbisNpMatching2SignalingConnectionInfoA) == 0x10);

// SignalingGetPingInfo request
struct OrbisNpMatching2SignalingGetPingInfoRequest {
    OrbisNpMatching2RoomId roomId;
    u8 pad[16];

    int Validate() {
        return 0;
    }
};

// SignalingGetPingInfo response
struct OrbisNpMatching2SignalingGetPingInfoResponse {
    OrbisNpMatching2ServerId serverId;
    u8 pad[2];
    OrbisNpMatching2WorldId worldId;
    OrbisNpMatching2RoomId roomId;
    u32 rtt;
    u8 reserved[20];
};
static_assert(sizeof(OrbisNpMatching2SignalingGetPingInfoResponse) == 0x28);

// Signaling parameter
struct OrbisNpMatching2SignalingParam {
    int type;
    int flag;
    OrbisNpMatching2RoomMemberId mainMember;
    u8 pad[4];
};

} // namespace Libraries::Np::NpMatching2

namespace Libraries::Np::NpSignaling {

using OrbisNpSignalingContextId = s32;
using OrbisNpSignalingConnectionId = s32;
using OrbisNpSignalingRequestId = u32;

// Signaling handler callback function
using OrbisNpSignalingHandler = PS4_SYSV_ABI void (*)(u32 ctxId, u32 connId, s32 event,
                                                      s32 errorCode, void* userArg);

// Signaling event
constexpr s32 ORBIS_NP_SIGNALING_EVENT_DEAD = 0;
constexpr s32 ORBIS_NP_SIGNALING_EVENT_ESTABLISHED = 1;
constexpr s32 ORBIS_NP_SIGNALING_EVENT_NETINFO_ERROR = 2;
constexpr s32 ORBIS_NP_SIGNALING_EVENT_NETINFO_RESULT = 3;
constexpr s32 ORBIS_NP_SIGNALING_EVENT_PEER_ACTIVATED = 10;
constexpr s32 ORBIS_NP_SIGNALING_EVENT_PEER_DEACTIVATED = 11;
constexpr s32 ORBIS_NP_SIGNALING_EVENT_MUTUAL_ACTIVATED = 12;

// Connection status
constexpr s32 ORBIS_NP_SIGNALING_CONN_STATUS_INACTIVE = 0;
constexpr s32 ORBIS_NP_SIGNALING_CONN_STATUS_PENDING = 1;
constexpr s32 ORBIS_NP_SIGNALING_CONN_STATUS_ACTIVE = 2;

// Type of connection information to acquire
constexpr s32 ORBIS_NP_SIGNALING_CONN_INFO_RTT = 1;
constexpr s32 ORBIS_NP_SIGNALING_CONN_INFO_BANDWIDTH = 2;
constexpr s32 ORBIS_NP_SIGNALING_CONN_INFO_PEER_NP_ID = 3;
constexpr s32 ORBIS_NP_SIGNALING_CONN_INFO_PEER_ADDR = 4;
constexpr s32 ORBIS_NP_SIGNALING_CONN_INFO_MAPPED_ADDR = 5;
constexpr s32 ORBIS_NP_SIGNALING_CONN_INFO_PACKET_LOSS = 6;
constexpr s32 ORBIS_NP_SIGNALING_CONN_INFO_PEER_ADDRESS_A = 7;

// Context option
constexpr s32 ORBIS_NP_SIGNALING_CONTEXT_OPTION_FLAG = 1;

// Network information
struct OrbisNpSignalingNetInfo {
    u64 size;
    u32 localAddr;
    u32 mappedAddr;
    s32 natStatus;
    u32 _pad_14;
};
static_assert(sizeof(OrbisNpSignalingNetInfo) == 0x18);

// Account-id / platform pair
struct OrbisNpSignalingAccountPlatformPair {
    u64 accountId;
    u32 platformType;
    u32 _pad_0c;
};
static_assert(sizeof(OrbisNpSignalingAccountPlatformPair) == 0x10);

// Memory information
struct OrbisNpSignalingMemoryInfo {
    u64 currentInUse;
    u64 peakInUse;
    u64 maxSystemSize;
};
static_assert(sizeof(OrbisNpSignalingMemoryInfo) == 0x18);

// Connection statistics
struct OrbisNpSignalingConnectionStatistics {
    u32 peakConnectionCount;
    u32 activeConnectionCount;
    u32 transientConnectionCount;
    u32 establishedConnectionCount;
};
static_assert(sizeof(OrbisNpSignalingConnectionStatistics) == 0x10);

} // namespace Libraries::Np::NpSignaling
