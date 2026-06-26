// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Np::NpMatching2 {

using OrbisNpMatching2AttributeId = u16;
using OrbisNpMatching2ContextId = u16;
using OrbisNpMatching2Event = u16;
using OrbisNpMatching2EventCause = u8;
using OrbisNpMatching2Flags = u32;
using OrbisNpMatching2LobbyId = u64;
using OrbisNpMatching2NatType = u8;
using OrbisNpMatching2RequestId = u32;
using OrbisNpMatching2RoomId = u64;
using OrbisNpMatching2RoomMemberId = u16;
using OrbisNpMatching2RoomPasswordSlotMask = u64;
using OrbisNpMatching2ServerId = u16;
using OrbisNpMatching2SignalingFlag = u8;
using OrbisNpMatching2SignalingRequestId = u32;
using OrbisNpMatching2SignalingType = u8;
using OrbisNpMatching2TeamId = u8;
using OrbisNpMatching2WorldId = u32;

constexpr OrbisNpMatching2Event ORBIS_NP_MATCHING2_REQUEST_EVENT_GET_WORLD_INFO_LIST = 0x0002;
constexpr OrbisNpMatching2Event ORBIS_NP_MATCHING2_REQUEST_EVENT_SET_ROOM_DATA_EXTERNAL = 0x0004;
constexpr OrbisNpMatching2Event ORBIS_NP_MATCHING2_REQUEST_EVENT_SET_USER_INFO = 0x0007;
constexpr OrbisNpMatching2Event ORBIS_NP_MATCHING2_REQUEST_EVENT_CREATE_JOIN_ROOM = 0x0101;
constexpr OrbisNpMatching2Event ORBIS_NP_MATCHING2_REQUEST_EVENT_JOIN_ROOM = 0x0102;
constexpr OrbisNpMatching2Event ORBIS_NP_MATCHING2_REQUEST_EVENT_LEAVE_ROOM = 0x0103;
constexpr OrbisNpMatching2Event ORBIS_NP_MATCHING2_REQUEST_EVENT_KICKOUT_ROOM_MEMBER = 0x0104;
constexpr OrbisNpMatching2Event ORBIS_NP_MATCHING2_REQUEST_EVENT_GRANT_ROOM_OWNER = 0x0105;
constexpr OrbisNpMatching2Event ORBIS_NP_MATCHING2_REQUEST_EVENT_SEARCH_ROOM = 0x0106;
constexpr OrbisNpMatching2Event ORBIS_NP_MATCHING2_REQUEST_EVENT_SEND_ROOM_MESSAGE = 0x0108;
constexpr OrbisNpMatching2Event ORBIS_NP_MATCHING2_REQUEST_EVENT_SET_ROOM_DATA_INTERNAL = 0x0109;
constexpr OrbisNpMatching2Event ORBIS_NP_MATCHING2_REQUEST_EVENT_GET_ROOM_DATA_INTERNAL = 0x010A;
constexpr OrbisNpMatching2Event ORBIS_NP_MATCHING2_REQUEST_EVENT_JOIN_LOBBY = 0x0201;
constexpr OrbisNpMatching2Event ORBIS_NP_MATCHING2_REQUEST_EVENT_SIGNALING_GET_PING_INFO = 0x0E01;
constexpr OrbisNpMatching2Event ORBIS_NP_MATCHING2_ROOM_EVENT_MEMBER_JOINED = 0x1101;
constexpr OrbisNpMatching2Event ORBIS_NP_MATCHING2_ROOM_EVENT_MEMBER_LEFT = 0x1102;
constexpr OrbisNpMatching2Event ORBIS_NP_MATCHING2_ROOM_EVENT_KICKEDOUT = 0x1103;
constexpr OrbisNpMatching2Event ORBIS_NP_MATCHING2_ROOM_EVENT_ROOM_DESTROYED = 0x1104;
constexpr OrbisNpMatching2Event ORBIS_NP_MATCHING2_ROOM_EVENT_ROOM_OWNER_CHANGED = 0x1105;
constexpr OrbisNpMatching2Event ORBIS_NP_MATCHING2_ROOM_EVENT_UPDATED_ROOM_DATA_INTERNAL = 0x1106;
constexpr OrbisNpMatching2Event ORBIS_NP_MATCHING2_ROOM_EVENT_UPDATED_ROOM_MEMBER_DATA_INTERNAL =
    0x1107;
constexpr OrbisNpMatching2Event ORBIS_NP_MATCHING2_ROOM_EVENT_UPDATED_SIGNALING_OPT_PARAM = 0x1108;
constexpr OrbisNpMatching2Event ORBIS_NP_MATCHING2_SIGNALING_EVENT_DEAD = 0x5101;
constexpr OrbisNpMatching2Event ORBIS_NP_MATCHING2_SIGNALING_EVENT_ESTABLISHED = 0x5102;
constexpr OrbisNpMatching2Event ORBIS_NP_MATCHING2_SIGNALING_EVENT_NETINFO_ERROR = 0x5103;
constexpr OrbisNpMatching2Event ORBIS_NP_MATCHING2_CONTEXT_EVENT_START_OVER = 0x6F01;
constexpr OrbisNpMatching2Event ORBIS_NP_MATCHING2_CONTEXT_EVENT_STARTED = 0x6F02;
constexpr OrbisNpMatching2Event ORBIS_NP_MATCHING2_CONTEXT_EVENT_STOPPED = 0x6F03;
constexpr OrbisNpMatching2Event ORBIS_NP_MATCHING2_REQUEST_EVENT_CREATE_JOIN_ROOM_A = 0x7101;
constexpr OrbisNpMatching2Event ORBIS_NP_MATCHING2_REQUEST_EVENT_SEARCH_ROOM_A = 0x7106;
constexpr OrbisNpMatching2Event ORBIS_NP_MATCHING2_ROOM_EVENT_MEMBER_JOINED_A = 0x8101;
constexpr OrbisNpMatching2Event ORBIS_NP_MATCHING2_ROOM_EVENT_MEMBER_LEFT_A = 0x8102;
constexpr OrbisNpMatching2Event ORBIS_NP_MATCHING2_ROOM_EVENT_UPDATED_ROOM_MEMBER_DATA_INTERNAL_A =
    0x8107;
constexpr OrbisNpMatching2Event ORBIS_NP_MATCHING2_ROOM_MSG_EVENT_MESSAGE_A = 0x9102;

constexpr OrbisNpMatching2EventCause ORBIS_NP_MATCHING2_EVENT_CAUSE_LEAVE_ACTION = 1;
constexpr OrbisNpMatching2EventCause ORBIS_NP_MATCHING2_EVENT_CAUSE_KICKOUT_ACTION = 2;
constexpr OrbisNpMatching2EventCause ORBIS_NP_MATCHING2_EVENT_CAUSE_GRANT_OWNER_ACTION = 3;
constexpr OrbisNpMatching2EventCause ORBIS_NP_MATCHING2_EVENT_CAUSE_MEMBER_DISAPPEARED = 5;
constexpr OrbisNpMatching2EventCause ORBIS_NP_MATCHING2_EVENT_CAUSE_NP_SIGNED_OUT = 8;
constexpr OrbisNpMatching2EventCause ORBIS_NP_MATCHING2_EVENT_CAUSE_CONTEXT_ERROR = 10;
constexpr OrbisNpMatching2EventCause ORBIS_NP_MATCHING2_EVENT_CAUSE_CONTEXT_ACTION = 11;

constexpr OrbisNpMatching2SignalingType ORBIS_NP_MATCHING2_SIGNALING_TYPE_NONE = 0;
constexpr OrbisNpMatching2SignalingType ORBIS_NP_MATCHING2_SIGNALING_TYPE_MESH = 1;
constexpr OrbisNpMatching2SignalingType ORBIS_NP_MATCHING2_SIGNALING_TYPE_STAR = 2;

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Np::NpMatching2