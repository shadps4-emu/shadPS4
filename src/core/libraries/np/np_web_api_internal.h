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

// Structs reference each other, so declare them before their contents.
struct OrbisNpWebApiContext;
struct OrbisNpWebApiUserContext;
struct OrbisNpWebApiRequest;
struct OrbisNpWebApiHandle;
struct OrbisNpWebApiTimerHandle;
struct OrbisNpWebApiPushEventFilter;
struct OrbisNpWebApiServicePushEventFilter;
struct OrbisNpWebApiExtendedPushEventFilter;
struct OrbisNpWebApiRegisteredPushEventCallback;
struct OrbisNpWebApiRegisteredServicePushEventCallback;
struct OrbisNpWebApiRegisteredExtendedPushEventCallback;

struct OrbisNpWebApiContext {
    s32 type;
    s32 userCount;
    s32 libCtxId;
    s32 libHttpCtxId;
    std::recursive_mutex contextLock;
    std::map<s32, OrbisNpWebApiUserContext*> userContexts;
    std::map<s32, OrbisNpWebApiHandle*> handles;
    std::map<s32, OrbisNpWebApiTimerHandle*> timerHandles;
    std::map<s32, OrbisNpWebApiPushEventFilter*> pushEventFilters;
    std::map<s32, OrbisNpWebApiServicePushEventFilter*> servicePushEventFilters;
    std::map<s32, OrbisNpWebApiExtendedPushEventFilter*> extendedPushEventFilters;
    std::string name;
    bool terminated;
};

struct OrbisNpWebApiUserContext {
    OrbisNpWebApiContext* parentContext;
    s32 userCount;
    s32 userCtxId;
    Libraries::UserService::OrbisUserServiceUserId userId;
    std::map<s64, OrbisNpWebApiRequest*> requests;
    std::map<s32, OrbisNpWebApiRegisteredPushEventCallback*> pushEventCallbacks;
    std::map<s32, OrbisNpWebApiRegisteredServicePushEventCallback*> servicePushEventCallbacks;
    std::map<s32, OrbisNpWebApiRegisteredExtendedPushEventCallback*> extendedPushEventCallbacks;
    bool deleted;
    OrbisNpWebApiNotificationCallback notificationCallbackFunction;
    void* pNotificationCallbackUserArgs;
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
    // not sure Stephen
    u8 requestState;
    u64 remainingData;
    u32 readOffset;
    char data[64];
};

struct OrbisNpWebApiHandle {
    s32 handleId;
    bool aborted;
    bool deleted;
    s32 userCount;
};

struct OrbisNpWebApiTimerHandle {
    s32 handleId;
    u32 handleTimeout;
    u64 handleEndTime;
    bool timedOut;
};

struct OrbisNpWebApiPushEventFilter {
    s32 filterId;
    std::vector<OrbisNpWebApiPushEventFilterParameter> filterParams;
    OrbisNpWebApiContext* parentContext;
};

struct OrbisNpWebApiServicePushEventFilter {
    s32 filterId;
    bool internal;
    std::vector<OrbisNpWebApiServicePushEventFilterParameter> filterParams;
    std::string npServiceName;
    OrbisNpServiceLabel npServiceLabel;
    OrbisNpWebApiContext* parentContext;
};

struct OrbisNpWebApiExtendedPushEventFilter {
    s32 filterId;
    bool internal;
    std::vector<OrbisNpWebApiExtdPushEventFilterParameter> filterParams;
    std::string npServiceName;
    OrbisNpServiceLabel npServiceLabel;
    OrbisNpWebApiContext* parentContext;
};

struct OrbisNpWebApiRegisteredPushEventCallback {
    s32 callbackId;
    s32 filterId;
    OrbisNpWebApiPushEventCallback cbFunc;
    void* pUserArg;
};

struct OrbisNpWebApiRegisteredServicePushEventCallback {
    s32 callbackId;
    s32 filterId;
    OrbisNpWebApiServicePushEventCallback cbFunc;
    OrbisNpWebApiInternalServicePushEventCallback internalCbFunc;
    // Note: real struct stores both internal callbacks in one field
    OrbisNpWebApiInternalServicePushEventCallbackA internalCbFuncA;
    void* pUserArg;
};

struct OrbisNpWebApiRegisteredExtendedPushEventCallback {
    s32 callbackId;
    s32 filterId;
    OrbisNpWebApiExtdPushEventCallback cbFunc;
    // Note: real struct stores both callbacks in one field
    OrbisNpWebApiExtdPushEventCallbackA cbFuncA;
    void* pUserArg;
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
bool areContextHandlesBusy(OrbisNpWebApiContext* context);                // FUN_01008c20
void lockContext(OrbisNpWebApiContext* context);                          // FUN_010072e0
void unlockContext(OrbisNpWebApiContext* context);                        // FUN_010072f0
void markContextAsTerminated(OrbisNpWebApiContext* context);              // FUN_01008bf0
void checkContextTimeout(OrbisNpWebApiContext* context);                  // FUN_01008ad0
void checkTimeout();                                                      // FUN_01003700
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
s32 registerNotificationCallback(s32 titleUserCtxId, OrbisNpWebApiNotificationCallback cbFunc,
                                 void* pUserArg);                       // FUN_01003770
s32 unregisterNotificationCallback(s32 titleUserCtxId);                 // FUN_01003800
bool isUserContextBusy(OrbisNpWebApiUserContext* userContext);          // FUN_0100ea40
bool areUserContextRequestsBusy(OrbisNpWebApiUserContext* userContext); // FUN_0100d1f0
void releaseUserContext(OrbisNpWebApiUserContext* userContext);         // FUN_0100caa0
void checkUserContextTimeout(OrbisNpWebApiUserContext* userContext);    // FUN_0100ea90
s32 deleteUserContext(s32 userCtxId);                                   // FUN_01001710

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
s32 setRequestTimeout(s64 requestId, u32 timeout);           // FUN_01003610
void startRequestTimer(OrbisNpWebApiRequest* request);       // FUN_0100c0d0
void checkRequestTimeout(OrbisNpWebApiRequest* request);     // FUN_0100c130
s32 sendRequest(
    s64 requestId, s32 partIndex, const void* data, u64 dataSize, s8 flag,
    const OrbisNpWebApiResponseInformationOption* pResponseInformationOption); // FUN_01001c50
s32 abortRequestInternal(OrbisNpWebApiContext* context, OrbisNpWebApiUserContext* userContext,
                         OrbisNpWebApiRequest* request); // FUN_01001b70
s32 abortRequest(s64 requestId);                         // FUN_01002c70
void releaseRequest(OrbisNpWebApiRequest* request);      // FUN_01009fb0
s32 deleteRequest(s64 requestId);                        // FUN_010019a0

// Handle functions
s32 createHandleInternal(OrbisNpWebApiContext* context); // FUN_01007730
s32 createHandle(s32 libCtxId);                          // FUN_01002ee0
s32 setHandleTimeoutInternal(OrbisNpWebApiContext* context, s32 handleId,
                             u32 timeout);                                      // FUN_01007ed0
s32 setHandleTimeout(s32 libCtxId, s32 handleId, u32 timeout);                  // FUN_010036b0
void startHandleTimer(OrbisNpWebApiContext* context, s32 handleId);             // FUN_01007fd0
void releaseHandle(OrbisNpWebApiContext* context, OrbisNpWebApiHandle* handle); // FUN_01007ea0
s32 getHandle(OrbisNpWebApiContext* context, s32 handleId,
              OrbisNpWebApiHandle** handleOut);                        // FUN_01007e20
s32 abortHandle(s32 libCtxId, s32 handleId);                           // FUN_01003390
s32 deleteHandleInternal(OrbisNpWebApiContext* context, s32 handleId); // FUN_01007a00
s32 deleteHandle(s32 libCtxId, s32 handleId);                          // FUN_01002f20

// Push event filter functions
s32 createPushEventFilterInternal(OrbisNpWebApiContext* context,
                                  const OrbisNpWebApiPushEventFilterParameter* pFilterParam,
                                  u64 filterParamNum); // FUN_01008040
s32 createPushEventFilter(s32 libCtxId, const OrbisNpWebApiPushEventFilterParameter* pFilterParam,
                          u64 filterParamNum);                                  // FUN_01002d10
s32 deletePushEventFilterInternal(OrbisNpWebApiContext* context, s32 filterId); // FUN_01008180
s32 deletePushEventFilter(s32 libCtxId, s32 filterId);                          // FUN_01002d60

// Push event callback functions
s32 registerPushEventCallbackInternal(OrbisNpWebApiUserContext* userContext, s32 filterId,
                                      OrbisNpWebApiPushEventCallback cbFunc,
                                      void* userArg); // FUN_0100d450
s32 registerPushEventCallback(s32 titleUserCtxId, s32 filterId,
                              OrbisNpWebApiPushEventCallback cbFunc,
                              void* pUserArg);                       // FUN_01002da0
s32 unregisterPushEventCallback(s32 titleUserCtxId, s32 callbackId); // FUN_01002e50

// Service push event filter functions
s32 createServicePushEventFilterInternal(
    OrbisNpWebApiContext* context, s32 handleId, const char* pNpServiceName,
    OrbisNpServiceLabel npServiceLabel,
    const OrbisNpWebApiServicePushEventFilterParameter* pFilterParam,
    u64 filterParamNum); // FUN_010082f0
s32 createServicePushEventFilter(s32 libCtxId, s32 handleId, const char* pNpServiceName,
                                 OrbisNpServiceLabel npServiceLabel,
                                 const OrbisNpWebApiServicePushEventFilterParameter* pFilterParam,
                                 u64 filterParamNum); // FUN_01002f60
s32 deleteServicePushEventFilterInternal(OrbisNpWebApiContext* context,
                                         s32 filterId);       // FUN_010084f0
s32 deleteServicePushEventFilter(s32 libCtxId, s32 filterId); // FUN_01002fe0

// Service push event callback functions
s32 registerServicePushEventCallbackInternal(
    OrbisNpWebApiUserContext* userContext, s32 filterId,
    OrbisNpWebApiServicePushEventCallback cbFunc,
    OrbisNpWebApiInternalServicePushEventCallback intCbFunc,
    OrbisNpWebApiInternalServicePushEventCallbackA intCbFuncA, void* pUserArg); // FUN_0100d8c0
s32 registerServicePushEventCallback(s32 titleUserCtxId, s32 filterId,
                                     OrbisNpWebApiServicePushEventCallback cbFunc,
                                     OrbisNpWebApiInternalServicePushEventCallback intCbFunc,
                                     OrbisNpWebApiInternalServicePushEventCallbackA intCbFuncA,
                                     void* pUserArg);                       // FUN_01003030
s32 unregisterServicePushEventCallback(s32 titleUserCtxId, s32 callbackId); // FUN_010030f0

// Extended push event filter functions
s32 createExtendedPushEventFilterInternal(
    OrbisNpWebApiContext* context, s32 handleId, const char* pNpServiceName,
    OrbisNpServiceLabel npServiceLabel,
    const OrbisNpWebApiExtdPushEventFilterParameter* pFilterParam, u64 filterParamNum,
    bool internal); // FUN_01008680
s32 createExtendedPushEventFilter(s32 libCtxId, s32 handleId, const char* pNpServiceName,
                                  OrbisNpServiceLabel npServiceLabel,
                                  const OrbisNpWebApiExtdPushEventFilterParameter* pFilterParam,
                                  u64 filterParamNum, bool internal); // FUN_01003180
s32 deleteExtendedPushEventFilterInternal(OrbisNpWebApiContext* context,
                                          s32 filterId);       // FUN_01008880
s32 deleteExtendedPushEventFilter(s32 libCtxId, s32 filterId); // FUN_01003200

// Extended push event callback functions
s32 registerExtdPushEventCallbackInternal(OrbisNpWebApiUserContext* userContext, s32 filterId,
                                          OrbisNpWebApiExtdPushEventCallback cbFunc,
                                          OrbisNpWebApiExtdPushEventCallbackA cbFuncA,
                                          void* pUserArg); // FUN_0100df60
s32 registerExtdPushEventCallback(s32 userCtxId, s32 filterId,
                                  OrbisNpWebApiExtdPushEventCallback cbFunc,
                                  OrbisNpWebApiExtdPushEventCallbackA cbFuncA,
                                  void* pUserArg); // FUN_01003250
s32 registerExtdPushEventCallbackA(s32 userCtxId, s32 filterId,
                                   OrbisNpWebApiExtdPushEventCallbackA cbFunc,
                                   void* pUserArg);                      // FUN_01003240
s32 unregisterExtdPushEventCallback(s32 titleUserCtxId, s32 callbackId); // FUN_01003300

s32 PS4_SYSV_ABI getHttpStatusCodeInternal(s64 requestId, s32* out_status_code);
s32 PS4_SYSV_ABI getHttpRequestIdFromRequest(OrbisNpWebApiRequest* request);
s32 PS4_SYSV_ABI readDataInternal(s64 requestId, void* pData, u64 size);
void PS4_SYSV_ABI setRequestEndTime(OrbisNpWebApiRequest* request);
void PS4_SYSV_ABI clearRequestEndTime(OrbisNpWebApiRequest* req);
bool PS4_SYSV_ABI hasRequestTimedOut(OrbisNpWebApiRequest* request);
void PS4_SYSV_ABI setRequestState(OrbisNpWebApiRequest* request, u8 state);
u64 PS4_SYSV_ABI copyRequestData(OrbisNpWebApiRequest* request, void* data, u64 size);

}; // namespace Libraries::Np::NpWebApi