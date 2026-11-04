// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/np/np_matching2/np_matching2.h"
#include "core/libraries/np/np_types.h"
#include "core/libraries/np/np_types2.h"

namespace Libraries::Np::NpMatching2 {
struct ContextObject;
}

namespace Libraries::Np::SignalingHandler {

void Start();
void Stop();
void AddContextRef();
void ReleaseContextRef();

s32 ActivateSig1(NpSignaling::OrbisNpSignalingContextId ctx_id, const OrbisNpId& peer_npid,
                 const OrbisNpOnlineId& peer_online_id,
                 NpSignaling::OrbisNpSignalingConnectionId* out_conn_id);
bool FindMatching2MemberNpId(OrbisNpAccountId account_id, OrbisNpId* out_npid);
void DeactivateSig1(NpSignaling::OrbisNpSignalingConnectionId conn_id);
void TerminateSig1(NpSignaling::OrbisNpSignalingConnectionId conn_id);

void StartMatching2(NpMatching2::ContextObject& ctx);
void StopMatching2(NpMatching2::ContextObject& ctx, s32 error_code);
void StopMatching2(NpMatching2::ContextObject& ctx, NpMatching2::OrbisNpMatching2RoomId room_id,
                   s32 error_code);

} // namespace Libraries::Np::SignalingHandler
