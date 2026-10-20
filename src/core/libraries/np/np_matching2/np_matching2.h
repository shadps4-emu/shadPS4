// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/np/np_types2.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Np::NpMatching2 {

int PS4_SYSV_ABI sceNpMatching2CreateContext(const OrbisNpMatching2CreateContextParameter* param,
                                             OrbisNpMatching2ContextId* ctxId);
int PS4_SYSV_ABI sceNpMatching2CreateContextA(const OrbisNpMatching2CreateContextParameterA* param,
                                              OrbisNpMatching2ContextId* ctxId);
int PS4_SYSV_ABI sceNpMatching2CreateJoinRoom(OrbisNpMatching2ContextId ctxId,
                                              OrbisNpMatching2CreateJoinRoomRequest* request,
                                              OrbisNpMatching2RequestOptParam* requestOpt,
                                              OrbisNpMatching2RequestId* requestId);
int PS4_SYSV_ABI sceNpMatching2JoinRoom(OrbisNpMatching2ContextId ctxId,
                                        OrbisNpMatching2JoinRoomRequest* request,
                                        OrbisNpMatching2RequestOptParam* requestOpt,
                                        OrbisNpMatching2RequestId* requestId);
int PS4_SYSV_ABI sceNpMatching2CreateJoinRoomA(OrbisNpMatching2ContextId ctxId,
                                               OrbisNpMatching2CreateJoinRoomRequestA* request,
                                               OrbisNpMatching2RequestOptParam* requestOpt,
                                               OrbisNpMatching2RequestId* requestId);
int PS4_SYSV_ABI sceNpMatching2RegisterContextCallback(OrbisNpMatching2ContextCallback callback,
                                                       void* userdata);
int PS4_SYSV_ABI sceNpMatching2RegisterLobbyEventCallback(
    OrbisNpMatching2ContextId ctxId, OrbisNpMatching2LobbyEventCallback callback, void* userdata);
int PS4_SYSV_ABI sceNpMatching2RegisterRoomEventCallback(OrbisNpMatching2ContextId ctxId,
                                                         OrbisNpMatching2RoomEventCallback callback,
                                                         void* userdata);
int PS4_SYSV_ABI sceNpMatching2RegisterSignalingCallback(OrbisNpMatching2ContextId ctxId,
                                                         OrbisNpMatching2SignalingCallback callback,
                                                         void* userdata);
int PS4_SYSV_ABI sceNpMatching2ContextStart(OrbisNpMatching2ContextId ctxId, u64 timeout);
int PS4_SYSV_ABI sceNpMatching2ContextStop(OrbisNpMatching2ContextId ctxId);
int PS4_SYSV_ABI sceNpMatching2Initialize(OrbisNpMatching2InitializeParameter* param);
int PS4_SYSV_ABI sceNpMatching2Terminate();
int PS4_SYSV_ABI sceNpMatching2SetDefaultRequestOptParam(
    OrbisNpMatching2ContextId ctxId, OrbisNpMatching2RequestOptParam* requestOpt);
int PS4_SYSV_ABI sceNpMatching2GetServerId(OrbisNpMatching2ContextId ctxId,
                                           OrbisNpMatching2ServerId* serverId);
int PS4_SYSV_ABI sceNpMatching2GetWorldInfoList(OrbisNpMatching2ContextId ctxId,
                                                OrbisNpMatching2GetWorldInfoListRequest* request,
                                                OrbisNpMatching2RequestOptParam* requestOpt,
                                                OrbisNpMatching2RequestId* requestId);
int PS4_SYSV_ABI sceNpMatching2LeaveRoom(OrbisNpMatching2ContextId ctxId,
                                         OrbisNpMatching2LeaveRoomRequest* request,
                                         OrbisNpMatching2RequestOptParam* requestOpt,
                                         OrbisNpMatching2RequestId* requestId);
int PS4_SYSV_ABI sceNpMatching2SearchRoom(OrbisNpMatching2ContextId ctxId,
                                          OrbisNpMatching2SearchRoomRequest* request,
                                          OrbisNpMatching2RequestOptParam* requestOpt,
                                          OrbisNpMatching2RequestId* requestId);
int PS4_SYSV_ABI sceNpMatching2SetUserInfo(OrbisNpMatching2ContextId ctxId,
                                           OrbisNpMatching2SetUserInfoRequest* request,
                                           OrbisNpMatching2RequestOptParam* requestOpt,
                                           OrbisNpMatching2RequestId* requestId);
int PS4_SYSV_ABI sceNpMatching2SendRoomMessage(OrbisNpMatching2ContextId ctxId, void* request,
                                               OrbisNpMatching2RequestOptParam* requestOpt,
                                               OrbisNpMatching2RequestId* requestId);
int PS4_SYSV_ABI sceNpMatching2SetRoomDataExternal(
    OrbisNpMatching2ContextId ctxId, OrbisNpMatching2SetRoomDataExternalRequest* request,
    OrbisNpMatching2RequestOptParam* requestOpt, OrbisNpMatching2RequestId* requestId);
int PS4_SYSV_ABI sceNpMatching2SetRoomDataInternal(
    OrbisNpMatching2ContextId ctxId, OrbisNpMatching2SetRoomDataInternalRequest* request,
    OrbisNpMatching2RequestOptParam* requestOpt, OrbisNpMatching2RequestId* requestId);
s32 PS4_SYSV_ABI sceNpMatching2SignalingGetConnectionStatus(OrbisNpMatching2ContextId ctxId,
                                                            OrbisNpMatching2RoomId roomId,
                                                            OrbisNpMatching2RoomMemberId memberId,
                                                            s32* connStatus, void* peerAddr,
                                                            u16* peerPort);
int PS4_SYSV_ABI sceNpMatching2AbortContextStart(OrbisNpMatching2ContextId ctxId);
int PS4_SYSV_ABI sceNpMatching2DestroyContext(OrbisNpMatching2ContextId ctxId);
int PS4_SYSV_ABI sceNpMatching2RegisterLobbyMessageCallback(
    OrbisNpMatching2ContextId ctxId, OrbisNpMatching2LobbyMessageCallback callback, void* userdata);
int PS4_SYSV_ABI sceNpMatching2RegisterRoomMessageCallback(
    OrbisNpMatching2ContextId ctxId, OrbisNpMatching2RoomMessageCallback callback, void* userdata);
int PS4_SYSV_ABI sceNpMatching2GetMemoryInfo();
int PS4_SYSV_ABI sceNpMatching2GetSslMemoryInfo();
int PS4_SYSV_ABI sceNpMatching2GetRoomMemberIdListLocal();
int PS4_SYSV_ABI sceNpMatching2GetRoomPasswordLocal();
int PS4_SYSV_ABI sceNpMatching2GetSignalingOptParamLocal();
int PS4_SYSV_ABI sceNpMatching2GetLobbyInfoList();
int PS4_SYSV_ABI sceNpMatching2GetLobbyMemberDataInternal();
int PS4_SYSV_ABI sceNpMatching2GetLobbyMemberDataInternalList();
int PS4_SYSV_ABI sceNpMatching2GetRoomDataExternalList(
    OrbisNpMatching2ContextId ctxId, OrbisNpMatching2GetRoomDataExternalListRequest* request,
    OrbisNpMatching2RequestOptParam* requestOpt, OrbisNpMatching2RequestId* requestId);
int PS4_SYSV_ABI sceNpMatching2GetRoomDataInternal(
    OrbisNpMatching2ContextId ctxId, OrbisNpMatching2GetRoomDataInternalRequest* request,
    OrbisNpMatching2RequestOptParam* requestOpt, OrbisNpMatching2RequestId* requestId);
int PS4_SYSV_ABI sceNpMatching2GetRoomMemberDataExternalList(
    OrbisNpMatching2ContextId ctxId, OrbisNpMatching2GetRoomMemberDataExternalListRequest* request,
    OrbisNpMatching2RequestOptParam* requestOpt, OrbisNpMatching2RequestId* requestId);
int PS4_SYSV_ABI sceNpMatching2GetRoomMemberDataInternal(
    OrbisNpMatching2ContextId ctxId,
    const OrbisNpMatching2GetRoomMemberDataInternalRequest* request,
    const OrbisNpMatching2RequestOptParam* requestOpt, OrbisNpMatching2RequestId* requestId);
int PS4_SYSV_ABI sceNpMatching2GetUserInfoListA(OrbisNpMatching2ContextId ctxId,
                                                OrbisNpMatching2GetUserInfoListRequest* request,
                                                OrbisNpMatching2RequestOptParam* requestOpt,
                                                OrbisNpMatching2RequestId* requestId);
int PS4_SYSV_ABI sceNpMatching2GetUserInfoList(OrbisNpMatching2ContextId ctxId,
                                               OrbisNpMatching2GetUserInfoListRequest* request,
                                               OrbisNpMatching2RequestOptParam* requestOpt,
                                               OrbisNpMatching2RequestId* requestId);
int PS4_SYSV_ABI sceNpMatching2GrantRoomOwner(OrbisNpMatching2ContextId ctxId, void* reqParam,
                                              void* optParam, s32* reqId);
int PS4_SYSV_ABI sceNpMatching2JoinLobby(OrbisNpMatching2ContextId ctxId, void* reqParam,
                                         void* optParam, s32* reqId);
int PS4_SYSV_ABI sceNpMatching2JoinRoomA(OrbisNpMatching2ContextId ctxId,
                                         OrbisNpMatching2JoinRoomRequestA* request,
                                         OrbisNpMatching2RequestOptParam* requestOpt,
                                         OrbisNpMatching2RequestId* requestId);
int PS4_SYSV_ABI sceNpMatching2KickoutRoomMember(OrbisNpMatching2ContextId ctxId,
                                                 OrbisNpMatching2KickoutRoomMemberRequest* request,
                                                 OrbisNpMatching2RequestOptParam* requestOpt,
                                                 OrbisNpMatching2RequestId* requestId);
int PS4_SYSV_ABI sceNpMatching2LeaveLobby();
int PS4_SYSV_ABI sceNpMatching2SendLobbyChatMessage();
int PS4_SYSV_ABI sceNpMatching2SendRoomChatMessage(OrbisNpMatching2ContextId ctxId, void* reqParam,
                                                   void* optParam, s32* reqId);
int PS4_SYSV_ABI sceNpMatching2SetLobbyMemberDataInternal();
int PS4_SYSV_ABI sceNpMatching2SetRoomMemberDataInternal(
    OrbisNpMatching2ContextId ctxId,
    const OrbisNpMatching2SetRoomMemberDataInternalRequest* request,
    const OrbisNpMatching2RequestOptParam* requestOpt, OrbisNpMatching2RequestId* requestId);
int PS4_SYSV_ABI sceNpMatching2SetSignalingOptParam();
int PS4_SYSV_ABI sceNpMatching2SignalingGetPeerNetInfo();
int PS4_SYSV_ABI sceNpMatching2SignalingGetPingInfo(OrbisNpMatching2ContextId ctxId,
                                                    const void* reqParam, const void* optParam,
                                                    s32* reqId);
int PS4_SYSV_ABI sceNpMatching2SignalingCancelPeerNetInfo();
int PS4_SYSV_ABI sceNpMatching2SignalingGetConnectionInfoA(OrbisNpMatching2ContextId ctxId,
                                                           OrbisNpMatching2RoomId roomId,
                                                           OrbisNpMatching2RoomMemberId memberId,
                                                           u32 infoType, void* connInfo);
int PS4_SYSV_ABI sceNpMatching2SignalingGetConnectionInfo(OrbisNpMatching2ContextId ctxId,
                                                          OrbisNpMatching2RoomId roomId,
                                                          OrbisNpMatching2RoomMemberId memberId,
                                                          u32 infoType, void* connInfo);
int PS4_SYSV_ABI sceNpMatching2SignalingGetLocalNetInfo(void* info);
int PS4_SYSV_ABI sceNpMatching2SignalingGetPeerNetInfoResult();

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Np::NpMatching2
