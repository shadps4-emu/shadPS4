// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/np/np_matching2/np_matching2.h"

namespace Libraries::Np::NpMatching2 {

struct ContextObject;

void StartMatching2HandshakeThread();
void StopMatching2HandshakeThread();

bool SendMatching2StunPing(const ContextObject& ctx);

void StartMatching2PeerHandshake(ContextObject& ctx, OrbisNpMatching2RoomId room_id,
                                 OrbisNpMatching2RoomMemberId member_id);
void StartMatching2SignalingForRoomPeers(ContextObject& ctx, OrbisNpMatching2RoomId room_id);
void QueueMatching2EstablishedForRoomPeers(ContextObject& ctx, OrbisNpMatching2RoomId room_id);
void QueueMatching2DeadForRoomPeers(ContextObject& ctx, OrbisNpMatching2RoomId room_id,
                                    s32 error_code);
void QueueMatching2SignalingEvent(ContextObject& ctx, OrbisNpMatching2RoomId room_id,
                                  OrbisNpMatching2RoomMemberId member_id,
                                  OrbisNpMatching2Event event, s32 error_code);

u32 GetRoomPingUs(const ContextObject& ctx, OrbisNpMatching2RoomId roomId);
void* BuildSignalingGetPingInfoPayload(ContextObject& ctx, OrbisNpMatching2RoomId roomId);
s32 FillMatching2ConnectionInfo(const ContextObject& ctx, OrbisNpMatching2RoomId roomId,
                                OrbisNpMatching2RoomMemberId memberId, u32 infoType, void* connInfo,
                                bool a_variant);

} // namespace Libraries::Np::NpMatching2
