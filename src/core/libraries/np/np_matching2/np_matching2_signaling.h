// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/np/np_matching2/np_matching2.h"

namespace Libraries::Np::NpMatching2 {

struct ContextObject;
struct CallbackPayload;

void StartMatching2SignalingRuntime();
void StopMatching2SignalingRuntime();

bool SendMatching2StunPing(const ContextObject& ctx);

void QueueMatching2DeadForRoomPeers(ContextObject& ctx, OrbisNpMatching2RoomId room_id,
                                    s32 error_code);
void QueueMatching2SignalingEvent(ContextObject& ctx, OrbisNpMatching2RoomId room_id,
                                  OrbisNpMatching2RoomMemberId member_id,
                                  OrbisNpMatching2Event event, s32 error_code);

u32 GetRoomPingUs(const ContextObject& ctx, OrbisNpMatching2RoomId roomId);
void* BuildSignalingGetPingInfoPayload(ContextObject& ctx, CallbackPayload& payload,
                                       OrbisNpMatching2RoomId roomId);
s32 FillMatching2ConnectionInfo(const ContextObject& ctx, OrbisNpMatching2RoomId roomId,
                                OrbisNpMatching2RoomMemberId memberId, u32 infoType, void* connInfo,
                                bool a_variant);
s32 FillMatching2LocalNetInfo(void* info);

} // namespace Libraries::Np::NpMatching2
