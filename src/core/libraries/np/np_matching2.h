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
using OrbisNpMatching2RequestId = u16;
using OrbisNpMatching2RoomId = u64;
using OrbisNpMatching2RoomMemberId = u16;
using OrbisNpMatching2ServerId = u16;
using OrbisNpMatching2TeamId = u8;
using OrbisNpMatching2WorldId = u32;

constexpr int ORBIS_NP_MATCHING2_ERROR_NOT_INITIALIZED = 0x80550c01;
constexpr int ORBIS_NP_MATCHING2_ERROR_ALREADY_INITIALIZED = 0x80550c02;
constexpr int ORBIS_NP_MATCHING2_ERROR_INVALID_ARGUMENT = 0x80550c15;

constexpr OrbisNpMatching2Event ORBIS_NP_MATCHING2_REQUEST_EVENT_CREATE_JOIN_ROOM = 0x0101;
constexpr OrbisNpMatching2Event ORBIS_NP_MATCHING2_REQUEST_EVENT_CREATE_JOIN_ROOM_A = 0x7101;
constexpr OrbisNpMatching2Event ORBIS_NP_MATCHING2_REQUEST_EVENT_GET_WORLD_INFO_LIST = 0x0002;
constexpr OrbisNpMatching2Event ORBIS_NP_MATCHING2_REQUEST_EVENT_SEARCH_ROOM_A = 0x7106;
constexpr OrbisNpMatching2Event ORBIS_NP_MATCHING2_REQUEST_EVENT_LEAVE_ROOM = 0x0103;
constexpr OrbisNpMatching2Event ORBIS_NP_MATCHING2_REQUEST_EVENT_SEND_ROOM_MESSAGE = 0x0108;
constexpr OrbisNpMatching2Event ORBIS_NP_MATCHING2_REQUEST_EVENT_SET_ROOM_DATA_EXTERNAL = 0x0004;
constexpr OrbisNpMatching2Event ORBIS_NP_MATCHING2_REQUEST_EVENT_SET_ROOM_DATA_INTERNAL = 0x1106;
constexpr OrbisNpMatching2Event ORBIS_NP_MATCHING2_REQUEST_EVENT_SET_USER_INFO = 0x0007;
constexpr OrbisNpMatching2Event ORBIS_NP_MATCHING2_CONTEXT_EVENT_START_OVER = 0x6F01;
constexpr OrbisNpMatching2Event ORBIS_NP_MATCHING2_CONTEXT_EVENT_STARTED = 0x6F02;

constexpr OrbisNpMatching2EventCause ORBIS_NP_MATCHING2_EVENT_CAUSE_CONTEXT_ERROR = 10;
constexpr OrbisNpMatching2EventCause ORBIS_NP_MATCHING2_EVENT_CAUSE_CONTEXT_ACTION = 11;

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Np::NpMatching2