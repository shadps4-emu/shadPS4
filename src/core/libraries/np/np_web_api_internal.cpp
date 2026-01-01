// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/config.h"
#include "common/elf_info.h"
#include "core/libraries/kernel/process.h"
#include "core/libraries/kernel/time.h"
#include "core/libraries/network/http.h"
#include "np_web_api_internal.h"

#include <magic_enum/magic_enum.hpp>

namespace Libraries::Np::NpWebApi {

static std::mutex g_global_mutex;
static std::map<s32, OrbisNpWebApiContext*> g_contexts;
static s32 g_context_count = 0;
static s32 g_user_context_count = 0;
static s64 g_request_count = 0;
static s32 g_sdk_ver = 0;

s32 initializeLibrary() {
    return Kernel::sceKernelGetCompiledSdkVersion(&g_sdk_ver);
}

s32 getCompiledSdkVersion() {
    return g_sdk_ver;
}

s32 createLibraryContext(s32 libHttpCtxId, u64 poolSize, const char* name, s32 type) {
    std::scoped_lock lk{g_global_mutex};
    s32 ctx_id = 1;
    while (g_contexts.contains(ctx_id)) {
        ctx_id++;
    }
    if (ctx_id >= 0x8000) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_MAX;
    }

    // Create new context
    g_contexts[ctx_id] = new OrbisNpWebApiContext{};
    auto& new_context = g_contexts.at(ctx_id);
    new_context->libCtxId = ctx_id;
    new_context->libHttpCtxId = libHttpCtxId;
    new_context->type = type;
    new_context->userCount = 0;
    new_context->terminated = false;
    if (name != nullptr) {
        strncpy(new_context->name, name, 0x20);
    }

    g_context_count++;
    return ctx_id;
}

OrbisNpWebApiContext* findAndValidateContext(s32 libCtxId, s32 flag) {
    std::scoped_lock lk{g_global_mutex};
    if (libCtxId < 1 || libCtxId >= 0x8000) {
        return nullptr;
    }
    auto& context = g_contexts[libCtxId];
    std::scoped_lock lk2{context->contextLock};
    if (flag == 0 && context->terminated) {
        return nullptr;
    }
    context->userCount++;
    return context;
}

void releaseContext(OrbisNpWebApiContext* context) {
    std::scoped_lock lk{context->contextLock};
    context->userCount--;
}

bool isContextTerminated(OrbisNpWebApiContext* context) {
    std::scoped_lock lk{context->contextLock};
    return context->terminated;
}

bool isContextBusy(OrbisNpWebApiContext* context) {
    std::scoped_lock lk{context->contextLock};
    return context->userCount > 1;
}

void lockContext(OrbisNpWebApiContext* context) {
    context->contextLock.lock();
}

void unlockContext(OrbisNpWebApiContext* context) {
    context->contextLock.unlock();
}

void markContextAsTerminated(OrbisNpWebApiContext* context) {
    std::scoped_lock lk{context->contextLock};
    context->terminated = true;
}

s32 deleteContext(s32 libCtxId) {
    std::scoped_lock lk{g_global_mutex};
    if (!g_contexts.contains(libCtxId)) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    free(g_contexts[libCtxId]);
    g_contexts.erase(libCtxId);
    g_context_count--;
    return ORBIS_OK;
}

s32 terminateContext(s32 libCtxId) {
    OrbisNpWebApiContext* ctx = findAndValidateContext(libCtxId);
    if (ctx == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }
    // TODO: For compiled SDK version < FW 4.0, should return error instead of waiting.

    // TODO: destroy user context objects

    // TODO: destroy handle objects

    lockContext(ctx);
    if (isContextTerminated(ctx)) {
        unlockContext(ctx);
        releaseContext(ctx);
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    markContextAsTerminated(ctx);
    while (isContextBusy(ctx)) {
        // TODO: should also check if context handle objects are busy
        unlockContext(ctx);
        Kernel::sceKernelUsleep(50000);
        lockContext(ctx);
    }
    unlockContext(ctx);
    releaseContext(ctx);
    return deleteContext(libCtxId);
}

OrbisNpWebApiUserContext* findUserContextByUserId(
    OrbisNpWebApiContext* context, Libraries::UserService::OrbisUserServiceUserId userId) {
    if (userId == Libraries::UserService::ORBIS_USER_SERVICE_USER_ID_INVALID) {
        return nullptr;
    }

    std::scoped_lock lk{context->contextLock};
    for (auto& user_context : context->userContexts) {
        if (user_context.second->userId == userId) {
            user_context.second->userCount++;
            return user_context.second;
        }
    }
    return nullptr;
}

OrbisNpWebApiUserContext* findUserContext(OrbisNpWebApiContext* context, s32 titleUserCtxId) {
    std::scoped_lock lk{context->contextLock};
    if (!context->userContexts.contains(titleUserCtxId)) {
        return nullptr;
    }
    OrbisNpWebApiUserContext* user_context = context->userContexts[titleUserCtxId];
    if (user_context->deleted) {
        return nullptr;
    }
    user_context->userCount++;
    return user_context;
}

s32 createUserContextWithOnlineId(s32 libCtxId, OrbisNpOnlineId* onlineId) {
    LOG_ERROR(Lib_NpWebApi, "called (STUBBED) libCtxId = {}", libCtxId);
    return ORBIS_OK; // TODO: implement
}

s32 createUserContext(s32 libCtxId, Libraries::UserService::OrbisUserServiceUserId userId) {
    LOG_INFO(Lib_NpWebApi, "libCtxId = {}, userId = {}", libCtxId, userId);
    OrbisNpWebApiContext* context = findAndValidateContext(libCtxId);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiUserContext* user_context = findUserContextByUserId(context, userId);
    if (user_context != nullptr) {
        releaseUserContext(user_context);
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_ALREADY_EXIST;
    }

    std::scoped_lock lk{context->contextLock};

    // Create new user context
    s32 user_ctx_id = (libCtxId << 0x10) | 1;
    while (context->userContexts.contains(user_ctx_id)) {
        user_ctx_id++;
    }
    if (user_ctx_id >= (libCtxId << 0x10) + 0x10000) {
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_MAX;
    }

    context->userContexts[user_ctx_id] = new OrbisNpWebApiUserContext{};
    user_context = context->userContexts.at(user_ctx_id);
    user_context->userCount = 0;
    user_context->parentContext = context;
    user_context->userId = userId;
    user_context->userCtxId = user_ctx_id;
    user_context->deleted = false;

    // TODO: Internal structs related to libSceHttp use are initialized here.
    releaseContext(context);
    return user_ctx_id;
}

bool isUserContextBusy(OrbisNpWebApiUserContext* userContext) {
    std::scoped_lock lk{userContext->parentContext->contextLock};
    return userContext->userCount > 1;
}

void releaseUserContext(OrbisNpWebApiUserContext* userContext) {
    std::scoped_lock lk{userContext->parentContext->contextLock};
    userContext->userCount--;
}

s32 deleteUserContext(s32 titleUserCtxId) {
    LOG_INFO(Lib_NpWebApi, "userCtxId = {:#x}", titleUserCtxId);
    OrbisNpWebApiContext* context = findAndValidateContext(titleUserCtxId >> 0x10);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    lockContext(context);
    OrbisNpWebApiUserContext* user_context = findUserContext(context, titleUserCtxId);
    if (user_context == nullptr) {
        unlockContext(context);
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND;
    } else if (user_context->deleted) {
        unlockContext(context);
        releaseUserContext(user_context);
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND;
    }

    // TODO: Should abort all requests in user context

    // TODO: For compiled SDK version < FW 4.0, should return error instead of waiting.

    user_context->deleted = true;
    while (isUserContextBusy(user_context)) {
        // TODO: should also check for busy requests
        unlockContext(context);
        Kernel::sceKernelUsleep(50000);
        lockContext(context);
    }

    context->userContexts.erase(titleUserCtxId);
    free(user_context);

    unlockContext(context);
    releaseContext(context);
    g_user_context_count--;

    return ORBIS_OK;
}

s32 createRequest(s32 titleUserCtxId, const char* pApiGroup, const char* pPath,
                  OrbisNpWebApiHttpMethod method,
                  const OrbisNpWebApiContentParameter* pContentParameter,
                  const OrbisNpWebApiIntCreateRequestExtraArgs* pInternalArgs, s64* pRequestId,
                  bool isMultipart) {
    OrbisNpWebApiContext* context = findAndValidateContext(titleUserCtxId >> 0x10);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiUserContext* user_context = findUserContext(context, titleUserCtxId);
    if (user_context == nullptr) {
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND;
    }

    lockContext(user_context->parentContext);
    if (g_sdk_ver >= Common::ElfInfo::FW_40 && user_context->deleted) {
        unlockContext(user_context->parentContext);
        releaseUserContext(user_context);
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND;
    }

    g_request_count++;
    if (g_request_count >> 0x20 != 0) {
        g_request_count = 1;
    }
    s64 request_id = static_cast<s64>(user_context->userCtxId) << 0x20 | g_request_count;
    user_context->requests[request_id] = new OrbisNpWebApiRequest{};

    auto& request = user_context->requests[request_id];
    request->parentContext = context;
    request->userCount = 0;
    request->requestId = request_id;
    request->userMethod = method;
    request->multipart = isMultipart;
    request->aborted = false;

    if (pApiGroup != nullptr) {
        request->userApiGroup = std::string(pApiGroup);
    }

    if (pPath != nullptr) {
        request->userPath = std::string(pPath);
    }

    if (pContentParameter != nullptr) {
        request->userContentLength = pContentParameter->contentLength;
        if (pContentParameter->pContentType != nullptr) {
            request->userContentType = std::string(pContentParameter->pContentType);
        }
    }

    if (pInternalArgs != nullptr) {
        ASSERT_MSG(pInternalArgs->unk_0 == nullptr && pInternalArgs->unk_1 == nullptr &&
                       pInternalArgs->unk_2 == nullptr,
                   "Internal arguments for requests not supported");
    }

    unlockContext(user_context->parentContext);

    if (pRequestId != nullptr) {
        *pRequestId = request->requestId;
    }

    releaseUserContext(user_context);
    releaseContext(context);
    return ORBIS_OK;
}

OrbisNpWebApiRequest* findRequest(OrbisNpWebApiUserContext* userContext, s64 requestId) {
    std::scoped_lock lk{userContext->parentContext->contextLock};
    if (userContext->requests.contains(requestId)) {
        return userContext->requests[requestId];
    }

    return nullptr;
}

OrbisNpWebApiRequest* findRequestAndMarkBusy(OrbisNpWebApiUserContext* userContext, s64 requestId) {
    std::scoped_lock lk{userContext->parentContext->contextLock};
    if (userContext->requests.contains(requestId)) {
        auto& request = userContext->requests[requestId];
        request->userCount++;
        return request;
    }

    return nullptr;
}

bool isRequestBusy(OrbisNpWebApiRequest* request) {
    std::scoped_lock lk{request->parentContext->contextLock};
    return request->userCount > 1;
}

void setRequestEndTime(OrbisNpWebApiRequest* request) {
    if (request->requestTimeout != 0 && request->requestEndTime == 0) {
        request->requestEndTime = Kernel::sceKernelGetProcessTime() + request->requestTimeout;
    }
}

s32 sendRequest(s64 requestId, s32 partIndex, const void* pData, u64 dataSize, s8 flag,
                const OrbisNpWebApiResponseInformationOption* pRespInfoOption) {
    OrbisNpWebApiContext* context = findAndValidateContext(requestId >> 0x30);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiUserContext* user_context = findUserContext(context, requestId >> 0x20);
    if (user_context == nullptr) {
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiRequest* request = findRequestAndMarkBusy(user_context, requestId);
    if (request == nullptr) {
        releaseUserContext(user_context);
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_REQUEST_NOT_FOUND;
    }

    setRequestEndTime(request);

    // TODO: multipart logic

    if (g_sdk_ver >= Common::ElfInfo::FW_25 && !request->sent) {
        request->sent = true;
    }

    lockContext(context);
    if (!request->timedOut && request->aborted) {
        unlockContext(context);
        releaseRequest(request);
        releaseUserContext(user_context);
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_ABORTED;
    }

    unlockContext(context);

    // Stubbing sceNpManagerIntGetSigninState call with a config check.
    if (!Config::getPSNSignedIn()) {
        releaseRequest(request);
        releaseUserContext(user_context);
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
    }

    LOG_ERROR(Lib_NpWebApi,
              "(STUBBED) called, requestId = {:#x}, pApiGroup = '{}', pPath = '{}', pContentType = "
              "'{}', method = {}, multipart = {}",
              requestId, request->userApiGroup, request->userPath, request->userContentType,
              magic_enum::enum_name(request->userMethod), request->multipart);

    releaseRequest(request);
    releaseUserContext(user_context);
    releaseContext(context);
    return ORBIS_OK;
}

s32 abortRequestInternal(OrbisNpWebApiContext* context, OrbisNpWebApiUserContext* userContext,
                         OrbisNpWebApiRequest* request) {
    if (context == nullptr || userContext == nullptr || request == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }

    std::scoped_lock lk{context->contextLock};
    if (request->aborted) {
        return ORBIS_OK;
    }

    request->aborted = true;

    // TODO: Should also abort any Np requests and Http requests tied to this request.

    return ORBIS_OK;
}

s32 abortRequest(s64 requestId) {
    OrbisNpWebApiContext* context = findAndValidateContext(requestId >> 0x30);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiUserContext* user_context = findUserContext(context, requestId >> 0x20);
    if (user_context == nullptr) {
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiRequest* request = findRequest(user_context, requestId);
    if (request == nullptr) {
        releaseUserContext(user_context);
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_REQUEST_NOT_FOUND;
    }

    s32 result = abortRequestInternal(context, user_context, request);

    releaseUserContext(user_context);
    releaseContext(context);
    return result;
}

void releaseRequest(OrbisNpWebApiRequest* request) {
    std::scoped_lock lk{request->parentContext->contextLock};
    request->userCount--;
}

s32 deleteRequest(s64 requestId) {
    OrbisNpWebApiContext* context = findAndValidateContext(requestId >> 0x30);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    lockContext(context);
    OrbisNpWebApiUserContext* user_context = findUserContext(context, requestId >> 0x20);
    if (user_context == nullptr) {
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiRequest* request = findRequestAndMarkBusy(user_context, requestId);
    if (request == nullptr) {
        releaseUserContext(user_context);
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_REQUEST_NOT_FOUND;
    }

    if (g_sdk_ver < Common::ElfInfo::FW_40 && isRequestBusy(request)) {
        releaseRequest(request);
        releaseUserContext(user_context);
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_REQUEST_BUSY;
    }

    abortRequestInternal(context, user_context, request);
    while (isRequestBusy(request)) {
        unlockContext(context);
        Kernel::sceKernelUsleep(50000);
        lockContext(context);
    }

    releaseRequest(request);
    user_context->requests.erase(request->requestId);
    free(request);

    releaseUserContext(user_context);
    unlockContext(context);
    releaseContext(context);
    return ORBIS_OK;
}

s32 createExtendedPushEventFilterInternal(
    s32 libCtxId, s32 handleId, const char* pNpServiceName, OrbisNpServiceLabel npServiceLabel,
    const OrbisNpWebApiExtdPushEventFilterParameter* pFilterParam, u64 filterParamNum,
    int additionalParam) {
    LOG_ERROR(Lib_NpWebApi,
              "called (STUBBED) : libCtxId = {}, "
              "handleId = {}, pNpServiceName = '{}', npServiceLabel = {}, pFilterParam = {}, "
              "filterParamNum = {}",
              libCtxId, handleId, (pNpServiceName ? pNpServiceName : "null"), npServiceLabel,
              fmt::ptr(pFilterParam), filterParamNum);
    s32 result;
    OrbisNpWebApiContext* context;
    context = findAndValidateContext(libCtxId);
    if (context == nullptr) {
        LOG_ERROR(Lib_NpWebApi,
                  " createExtendedPushEventFilterInternal: "
                  "lib context not found: libCtxId = {}",
                  libCtxId);
        result = ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    } else {
        validateHandleForContext(context, handleId);
        result =
            createExtendedPushEventFilterImpl(context, handleId, pNpServiceName, npServiceLabel,
                                              pFilterParam, filterParamNum, additionalParam);
        releaseContext(context);
    }
    return result;
}

s32 createExtendedPushEventFilterImpl(OrbisNpWebApiContext* context, s32 handleId,
                                      const char* pNpServiceName,
                                      OrbisNpServiceLabel npServiceLabel,
                                      const OrbisNpWebApiExtdPushEventFilterParameter* pFilterParam,
                                      u64 filterParamNum, int additionalParam) {
    LOG_ERROR(Lib_NpWebApi,
              "called (STUBBED) : "
              "handleId = {}, pNpServiceName = '{}', npServiceLabel = {}, pFilterParam = {}, "
              "filterParamNum = {}, additionalParam = {}",
              handleId, (pNpServiceName ? pNpServiceName : "null"), npServiceLabel,
              fmt::ptr(pFilterParam), filterParamNum, additionalParam);
    return ORBIS_OK; // TODO: implement
}

void validateHandleForContext(OrbisNpWebApiContext* context, int32_t handleId) {
    LOG_ERROR(Lib_NpWebApi,
              "called (STUBBED) : context = {}, "
              "handleId = {}",
              fmt::ptr(context), handleId); // TODO: implement
}

s32 createHandleInternal(OrbisNpWebApiContext* context) {
    LOG_ERROR(Lib_NpWebApi, "called (STUBBED) : context = {}", fmt::ptr(context));
    return ORBIS_OK; // TODO: implement
}

s32 registerExtdPushEventCallbackInternalA(s32 titleUserCtxId, s32 filterId,
                                           OrbisNpWebApiExtdPushEventCallbackA cbFunc,
                                           void* pUserArg) {
    return registerExtdPushEventCallbackInternal(titleUserCtxId, filterId, nullptr, cbFunc,
                                                 pUserArg);
}

s32 registerExtdPushEventCallbackInternal(s32 titleUserCtxId, s32 filterId,
                                          OrbisNpWebApiExtdPushEventCallback cbFunc,
                                          OrbisNpWebApiExtdPushEventCallbackA cbFuncA,
                                          void* pUserArg) {
    LOG_ERROR(Lib_NpWebApi,
              "called (STUBBED) : userCtxId = {}, "
              "filterId = {}, cbFunc = {}, cbFuncA = {}, pUserArg = {}",
              titleUserCtxId, filterId, fmt::ptr(cbFunc), fmt::ptr(cbFuncA), fmt::ptr(pUserArg));
    return ORBIS_OK; // TODO: implement
}

}; // namespace Libraries::Np::NpWebApi