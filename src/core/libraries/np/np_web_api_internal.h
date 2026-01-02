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
struct OrbisNpWebApiRequest;
struct OrbisNpWebApiHandle;
struct OrbisNpWebApiTimerHandle;

struct OrbisNpWebApiContext {
    s32 type;
    s32 userCount;
    s32 libCtxId;
    s32 libHttpCtxId;
    std::recursive_mutex contextLock;
    std::map<s32, OrbisNpWebApiUserContext*> userContexts;
    std::map<s32, OrbisNpWebApiHandle*> handles;
    std::map<s32, OrbisNpWebApiTimerHandle*> timerHandles;
    std::string name;
    bool terminated;
};

struct OrbisNpWebApiUserContext {
    OrbisNpWebApiContext* parentContext;
    s32 userCount;
    s32 userCtxId;
    Libraries::UserService::OrbisUserServiceUserId userId;
    std::map<s64, OrbisNpWebApiRequest*> requests;
    bool deleted;
};

struct OrbisNpWebApiRequest {
    OrbisNpWebApiContext* parentContext;
    s32 userCount;
    s64 requestId;
    std::string userApiGroup;
    std::string userPath;
    OrbisNpWebApiHttpMethod userMethod;
    u64 userContentLength;
    std::string userContentType;
    bool multipart;
    bool aborted;
    bool sent;
    u32 requestTimeout;
    u64 requestEndTime;
    bool timedOut;
};

struct OrbisNpWebApiHandle {
    s32 handleId;
    bool aborted;
    bool deleted;
    s32 userCount;
};

struct OrbisNpWebApiTimerHandle {
    s32 handleId;
    u32 timeout;
    u64 useTime;
    bool timedOut;
};

// General functions
s32 initializeLibrary();     // FUN_01001450
s32 getCompiledSdkVersion(); // FUN_01001440

// Library context functions
s32 createLibraryContext(s32 libHttpCtxId, u64 poolSize, const char* name,
                         s32 type);                                       // FUN_01006970
OrbisNpWebApiContext* findAndValidateContext(s32 libCtxId, s32 flag = 0); // FUN_01006860
void releaseContext(OrbisNpWebApiContext* context);                       // FUN_01006fc0
bool isContextTerminated(OrbisNpWebApiContext* context);                  // FUN_01006910
bool isContextBusy(OrbisNpWebApiContext* context);                        // FUN_01008a50
void lockContext(OrbisNpWebApiContext* context);                          // FUN_010072e0
void unlockContext(OrbisNpWebApiContext* context);                        // FUN_010072f0
void markContextAsTerminated(OrbisNpWebApiContext* context);              // FUN_01008bf0
s32 deleteContext(s32 libCtxId);                                          // FUN_01006c70
s32 terminateContext(s32 libCtxId);                                       // FUN_010014b0

// User context functions
OrbisNpWebApiUserContext* findUserContextByUserId(
    OrbisNpWebApiContext* context,
    Libraries::UserService::OrbisUserServiceUserId userId); // FUN_010075c0
OrbisNpWebApiUserContext* findUserContext(OrbisNpWebApiContext* context,
                                          s32 userCtxId);                   // FUN_01007530
s32 createUserContextWithOnlineId(s32 libCtxId, OrbisNpOnlineId* onlineId); // FUN_010016a0
s32 createUserContext(s32 libCtxId,
                      Libraries::UserService::OrbisUserServiceUserId userId); // FUN_010015c0
bool isUserContextBusy(OrbisNpWebApiUserContext* userContext);                // FUN_0100ea40
bool areUserContextRequestsBusy(OrbisNpWebApiUserContext* userContext);       // FUN_0100d1f0
void releaseUserContext(OrbisNpWebApiUserContext* userContext);               // FUN_0100caa0
s32 deleteUserContext(s32 userCtxId);                                         // FUN_01001710

// Request functions
s32 createRequest(s32 titleUserCtxId, const char* pApiGroup, const char* pPath,
                  OrbisNpWebApiHttpMethod method,
                  const OrbisNpWebApiContentParameter* pContentParameter,
                  const OrbisNpWebApiIntCreateRequestExtraArgs* pInternalArgs, s64* pRequestId,
                  bool isMultipart); // FUN_01001850
OrbisNpWebApiRequest* findRequest(OrbisNpWebApiUserContext* userContext,
                                  s64 requestId); // FUN_0100d3a0
OrbisNpWebApiRequest* findRequestAndMarkBusy(OrbisNpWebApiUserContext* userContext,
                                             s64 requestId); // FUN_0100d330
bool isRequestBusy(OrbisNpWebApiRequest* request);           // FUN_0100c1b0
void setRequestEndTime(OrbisNpWebApiRequest* request);       // FUN_0100c0d0
s32 sendRequest(
    s64 requestId, s32 partIndex, const void* data, u64 dataSize, s8 flag,
    const OrbisNpWebApiResponseInformationOption* pResponseInformationOption); // FUN_01001c50
s32 abortRequestInternal(OrbisNpWebApiContext* context, OrbisNpWebApiUserContext* userContext,
                         OrbisNpWebApiRequest* request); // FUN_01001b70
s32 abortRequest(s64 requestId);                         // FUN_01002c70
void releaseRequest(OrbisNpWebApiRequest* request);      // FUN_01009fb0
s32 deleteRequest(s64 requestId);                        // FUN_010019a0

// Handle functions
s32 createHandleInternal(OrbisNpWebApiContext* context);                        // FUN_01007730
s32 createHandle(s32 libCtxId);                                                 // FUN_01002ee0
void checkTimerHandle(OrbisNpWebApiContext* context, s32 handleId);             // FUN_01007fd0
void releaseHandle(OrbisNpWebApiContext* context, OrbisNpWebApiHandle* handle); // FUN_01007ea0
s32 getHandle(OrbisNpWebApiContext* context, s32 handleId,
              OrbisNpWebApiHandle** handleOut);                        // FUN_01007e20
s32 abortHandle(s32 libCtxId, s32 handleId);                           // FUN_01003390
s32 deleteHandleInternal(OrbisNpWebApiContext* context, s32 handleId); // FUN_01007a00
s32 deleteHandle(s32 libCtxId, s32 handleId);                          // FUN_01002f20

s32 createExtendedPushEventFilterInternal(
    s32 libCtxId, s32 handleId, const char* pNpServiceName, OrbisNpServiceLabel npServiceLabel,
    const OrbisNpWebApiExtdPushEventFilterParameter* pFilterParam, u64 filterParamNum,
    int additionalParam); // FUN_01003180
s32 createExtendedPushEventFilterImpl(OrbisNpWebApiContext* context, s32 handleId,
                                      const char* pNpServiceName,
                                      OrbisNpServiceLabel npServiceLabel,
                                      const OrbisNpWebApiExtdPushEventFilterParameter* pFilterParam,
                                      u64 filterParamNum,
                                      int additionalParam); // FUN_01008680

s32 registerExtdPushEventCallbackInternalA(s32 userCtxId, s32 filterId,
                                           OrbisNpWebApiExtdPushEventCallbackA cbFunc,
                                           void* pUserArg); // FUN_01003240
s32 registerExtdPushEventCallbackInternal(s32 userCtxId, s32 filterId,
                                          OrbisNpWebApiExtdPushEventCallback cbFunc,
                                          OrbisNpWebApiExtdPushEventCallbackA cbFuncA,
                                          void* pUserArg); // FUN_01003250

}; // namespace Libraries::Np::NpWebApi