// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>

#include "common/types.h"
#include "core/libraries/np/np_types2.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Np::NpSignaling {

s32 PS4_SYSV_ABI sceNpSignalingInitialize(s64 memorySize, s32 threadPriority, s32 cpuAffinityMask,
                                          s64 threadStackSize);
s32 PS4_SYSV_ABI sceNpSignalingTerminate();

s32 PS4_SYSV_ABI sceNpSignalingCreateContext(const void* npId, void* callback, void* callbackArg,
                                             OrbisNpSignalingContextId* outContextId);
s32 PS4_SYSV_ABI sceNpSignalingCreateContextA(s32 userId, void* callback, void* callbackArg,
                                              OrbisNpSignalingContextId* outContextId);
s32 PS4_SYSV_ABI sceNpSignalingDeleteContext(OrbisNpSignalingContextId ctxId);

s32 PS4_SYSV_ABI sceNpSignalingActivateConnection(OrbisNpSignalingContextId ctxId,
                                                  const void* peerNpId,
                                                  OrbisNpSignalingConnectionId* outConnId);
s32 PS4_SYSV_ABI sceNpSignalingActivateConnectionA(
    OrbisNpSignalingContextId ctxId, const OrbisNpSignalingAccountPlatformPair* peerAddr,
    OrbisNpSignalingConnectionId* outConnId);
s32 PS4_SYSV_ABI sceNpSignalingDeactivateConnection(OrbisNpSignalingContextId ctxId,
                                                    OrbisNpSignalingConnectionId connId);
s32 PS4_SYSV_ABI sceNpSignalingTerminateConnection(OrbisNpSignalingContextId ctxId,
                                                   OrbisNpSignalingConnectionId connId);

s32 PS4_SYSV_ABI sceNpSignalingGetConnectionStatus(OrbisNpSignalingContextId ctxId,
                                                   OrbisNpSignalingConnectionId connId,
                                                   u32* outStatus, u32* outPeerAddr,
                                                   u16* outPeerPort);
s32 PS4_SYSV_ABI sceNpSignalingGetConnectionInfo(OrbisNpSignalingContextId ctxId,
                                                 OrbisNpSignalingConnectionId connId, s32 infoCode,
                                                 void* outInfo);
s32 PS4_SYSV_ABI sceNpSignalingGetConnectionInfoA(OrbisNpSignalingContextId ctxId,
                                                  OrbisNpSignalingConnectionId connId, s32 infoCode,
                                                  void* outInfo);

s32 PS4_SYSV_ABI sceNpSignalingGetConnectionFromNpId(OrbisNpSignalingContextId ctxId,
                                                     const void* peerNpId,
                                                     OrbisNpSignalingConnectionId* outConnId);
s32 PS4_SYSV_ABI
sceNpSignalingGetConnectionFromPeerAddress(OrbisNpSignalingContextId ctxId, u32 peerAddr,
                                           u16 peerPort, OrbisNpSignalingConnectionId* outConnId);
s32 PS4_SYSV_ABI sceNpSignalingGetConnectionFromPeerAddressA(
    OrbisNpSignalingContextId ctxId, const OrbisNpSignalingAccountPlatformPair* peerAddr,
    OrbisNpSignalingConnectionId* outConnId);

s32 PS4_SYSV_ABI sceNpSignalingSetContextOption(OrbisNpSignalingContextId ctxId, s32 optionId,
                                                s32 optionValue);
s32 PS4_SYSV_ABI sceNpSignalingGetContextOption(OrbisNpSignalingContextId ctxId, s32 optionId,
                                                s32* outOptionValue);

s32 PS4_SYSV_ABI sceNpSignalingGetLocalNetInfo(OrbisNpSignalingContextId ctxId,
                                               OrbisNpSignalingNetInfo* info);

s32 PS4_SYSV_ABI sceNpSignalingGetPeerNetInfo(OrbisNpSignalingContextId ctxId, const void* peerNpId,
                                              OrbisNpSignalingRequestId* outReqId);
s32 PS4_SYSV_ABI sceNpSignalingGetPeerNetInfoA(OrbisNpSignalingContextId ctxId,
                                               const void* peerAccountPayload,
                                               OrbisNpSignalingRequestId* outReqId);
s32 PS4_SYSV_ABI sceNpSignalingCancelPeerNetInfo(OrbisNpSignalingContextId ctxId, s32 reqOrConnId);
s32 PS4_SYSV_ABI sceNpSignalingGetPeerNetInfoResult(OrbisNpSignalingContextId ctxId,
                                                    OrbisNpSignalingConnectionId connId,
                                                    OrbisNpSignalingNetInfo* peerNetInfo);

s32 PS4_SYSV_ABI sceNpSignalingGetMemoryInfo(OrbisNpSignalingMemoryInfo* info);
s32 PS4_SYSV_ABI sceNpSignalingGetConnectionStatistics(OrbisNpSignalingConnectionStatistics* stats);

s32 GetActiveConnectionIdForPeer(std::string_view online_id);

s32 GetConnectionStatusForPeer(std::string_view online_id, s32* out_conn_id);

bool GetPeerAddress(std::string_view online_id, u32* out_addr, u16* out_port);

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Np::NpSignaling
