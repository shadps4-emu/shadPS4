// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/np/np_web_api.h"
#include "core/libraries/np/np_web_api_error.h"

namespace Libraries::Np::NpWebApi {

struct OrbisNpWebApiContext {};

s32 createExtendedPushEventFilterInternal(
    s32 libCtxId, s32 handleId, const char* pNpServiceName, OrbisNpServiceLabel npServiceLabel,
    const OrbisNpWebApiExtdPushEventFilterParameter* pFilterParam, u64 filterParamNum,
    int additionalParam); // FUN_01003180
s32 createExtendedPushEventFilterImpl(OrbisNpWebApiContext* context, s32 handleId,
                                      const char* pNpServiceName,
                                      OrbisNpServiceLabel npServiceLabel,
                                      const OrbisNpWebApiExtdPushEventFilterParameter* pFilterParam,
                                      u64 filterParamNum,
                                      int additionalParam);                     // FUN_01008680
void releaseContext(OrbisNpWebApiContext* context);                             // FUN_01006fc0
OrbisNpWebApiContext* findAndValidateContext(int32_t libCtxId, int flag);       // FUN_01006860
void validateHandleForContext(OrbisNpWebApiContext* context, int32_t handleId); // FUN_01007fd0
s32 createContextForUser(int32_t libCtxId, int32_t userId);                     // FUN_010015c0
s32 createHandleInternal(OrbisNpWebApiContext* context);                        // FUN_01007730
s32 registerExtdPushEventCallbackInternalA(s32 userCtxId, s32 filterId,
                                           OrbisNpWebApiExtdPushEventCallbackA cbFunc,
                                           void* pUserArg); // FUN_01003240
s32 registerExtdPushEventCallbackInternal(s32 userCtxId, s32 filterId,
                                          OrbisNpWebApiExtdPushEventCallback cbFunc,
                                          OrbisNpWebApiExtdPushEventCallbackA cbFuncA,
                                          void* pUserArg); // FUN_01003250

}; // namespace Libraries::Np::NpWebApi