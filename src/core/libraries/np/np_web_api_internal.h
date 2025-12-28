// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include <map>
#include <mutex>
#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/np/np_web_api.h"
#include "core/libraries/np/np_web_api_error.h"

namespace Libraries::Np::NpWebApi {

// Most structs reference each other, so declare them before their contents.
struct OrbisNpWebApiContext;
struct OrbisNpWebApiUserContext;

struct OrbisNpWebApiContext {
    s32 type;
    s32 userCount;
    s32 libCtxId;
    s32 libHttpCtxId;
    std::map<s32, OrbisNpWebApiUserContext*> userContexts;
    std::recursive_mutex contextLock;
    char name[0x20];
    bool terminated;
};

struct OrbisNpWebApiUserContext {
    OrbisNpWebApiContext* parentContext;
    s32 userCount;
    s32 userCtxId;
    Libraries::UserService::OrbisUserServiceUserId userId;
    bool deleted;
};

// Library context functions
s32 createLibraryContext(s32 libHttpCtxId, u64 poolSize, const char* name,
                         s32 type);                                   // FUN_01006970
OrbisNpWebApiContext* findAndValidateContext(s32 libCtxId, s32 flag); // FUN_01006860
void releaseContext(OrbisNpWebApiContext* context);                   // FUN_01006fc0
bool isContextTerminated(OrbisNpWebApiContext* context);              // FUN_01006910
bool isContextBusy(OrbisNpWebApiContext* context);                    // FUN_01008a50
void lockContext(OrbisNpWebApiContext* context);                      // FUN_010072e0
void unlockContext(OrbisNpWebApiContext* context);                    // FUN_010072f0
void markContextAsTerminated(OrbisNpWebApiContext* context);          // FUN_01008bf0
s32 deleteContext(s32 libCtxId);                                      // FUN_01006c70
s32 terminateContext(s32 libCtxId);                                   // FUN_010014b0

// User context functions
OrbisNpWebApiUserContext* findUserContextByUserId(
    OrbisNpWebApiContext* context,
    Libraries::UserService::OrbisUserServiceUserId userId); // FUN_010075c0
OrbisNpWebApiUserContext* findUserContext(OrbisNpWebApiContext* context,
                                          s32 userCtxId);                   // FUN_01007530
s32 createUserContextWithOnlineId(s32 libCtxId, OrbisNpOnlineId* onlineId); // FUN_010016a0
s32 createUserContext(s32 libCtxId,
                      Libraries::UserService::OrbisUserServiceUserId userId); // FUN_010015c0
bool isUserContextBusy(OrbisNpWebApiUserContext* userContext);                 // FUN_0100ea40
s32 deleteUserContext(s32 userCtxId);                                         // FUN_01001710

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
void validateHandleForContext(OrbisNpWebApiContext* context, int32_t handleId); // FUN_01007fd0

s32 createHandleInternal(OrbisNpWebApiContext* context); // FUN_01007730
s32 registerExtdPushEventCallbackInternalA(s32 userCtxId, s32 filterId,
                                           OrbisNpWebApiExtdPushEventCallbackA cbFunc,
                                           void* pUserArg); // FUN_01003240
s32 registerExtdPushEventCallbackInternal(s32 userCtxId, s32 filterId,
                                          OrbisNpWebApiExtdPushEventCallback cbFunc,
                                          OrbisNpWebApiExtdPushEventCallbackA cbFuncA,
                                          void* pUserArg); // FUN_01003250

}; // namespace Libraries::Np::NpWebApi