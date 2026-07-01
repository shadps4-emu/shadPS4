// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include <string_view>
#include <vector>

#include "common/types.h"
#include "core/libraries/np/np_matching2/np_matching2.h"
#include "core/libraries/np/np_matching2/np_matching2_types.h"

namespace ShadNet {
class ShadNetClient;
enum class CommandType : u16;
enum class ErrorType : u8;
} // namespace ShadNet

namespace Libraries::Np::NpMatching2 {

enum class MmCommand : u16 {
    ContextStart = 12,
    CreateRoom = 13,
    JoinRoom = 14,
    LeaveRoom = 15,
    SearchRoom = 16,
    RequestSignalingInfos = 17,
    ContextStop = 18,
    SetRoomDataInternal = 20,
    SetRoomDataExternal = 21,
    KickoutRoomMember = 22,
    GetWorldInfoList = 23,
};

void SetMmShadNetClient(std::shared_ptr<ShadNet::ShadNetClient> client,
                        std::string_view server_host, u16 tcp_port);
void ClearMmShadNetClient();
bool IsMmClientRunning();

void OnMatchingReply(ShadNet::CommandType cmd, u64 pkt_id, ShadNet::ErrorType error,
                     const std::vector<u8>& body);

void MmContextStart(OrbisNpMatching2ContextId ctx_id);
void MmContextStop(OrbisNpMatching2ContextId ctx_id);

s32 MmSubmitRequest(OrbisNpMatching2ContextId ctx_id, OrbisNpMatching2RequestId req_id,
                    OrbisNpMatching2Event req_event, MmCommand cmd, const std::vector<u8>& payload);

s32 MmCreateJoinRoom(OrbisNpMatching2ContextId ctx_id, OrbisNpMatching2RequestId req_id,
                     const OrbisNpMatching2CreateJoinRoomRequest& request);
s32 MmJoinRoom(OrbisNpMatching2ContextId ctx_id, OrbisNpMatching2RequestId req_id,
               const OrbisNpMatching2JoinRoomRequest& request);
s32 MmLeaveRoom(OrbisNpMatching2ContextId ctx_id, OrbisNpMatching2RequestId req_id,
                const OrbisNpMatching2LeaveRoomRequest& request);
s32 MmGetWorldInfoList(OrbisNpMatching2ContextId ctx_id, OrbisNpMatching2RequestId req_id,
                       const OrbisNpMatching2GetWorldInfoListRequest& request);
s32 MmSearchRoom(OrbisNpMatching2ContextId ctx_id, OrbisNpMatching2RequestId req_id,
                 const OrbisNpMatching2SearchRoomRequest& request);
s32 MmSetRoomDataInternal(OrbisNpMatching2ContextId ctx_id, OrbisNpMatching2RequestId req_id,
                          const OrbisNpMatching2SetRoomDataInternalRequest& request);
s32 MmSetRoomDataExternal(OrbisNpMatching2ContextId ctx_id, OrbisNpMatching2RequestId req_id,
                          const OrbisNpMatching2SetRoomDataExternalRequest& request);
s32 MmKickoutRoomMember(OrbisNpMatching2ContextId ctx_id, OrbisNpMatching2RequestId req_id,
                        const OrbisNpMatching2KickoutRoomMemberRequest& request);

u32 GetMmServerAddr();
u16 GetMmServerUdpPort();

bool RequestSignalingInfos(std::string_view target_online_id, u32* out_addr, u16* out_port);

} // namespace Libraries::Np::NpMatching2
