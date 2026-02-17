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
static s32 g_library_context_count = 0;
static s32 g_user_context_count = 0;
static s32 g_handle_count = 0;
static s32 g_push_event_filter_count = 0;
static s32 g_service_push_event_filter_count = 0;
static s32 g_extended_push_event_filter_count = 0;
static s32 g_registered_callback_count = 0;
static s64 g_request_count = 0;
static u64 g_last_timeout_check = 0;
static s32 g_sdk_ver = 0;

s32 initializeLibrary() {
    return Kernel::sceKernelGetCompiledSdkVersion(&g_sdk_ver);
}

s32 getCompiledSdkVersion() {
    return g_sdk_ver;
}

s32 createLibraryContext(s32 libHttpCtxId, u64 poolSize, const char* name, s32 type) {
    std::scoped_lock lk{g_global_mutex};

    g_library_context_count++;
    if (g_library_context_count >= 0x8000) {
        g_library_context_count = 1;
    }
    s32 ctx_id = g_library_context_count;
    while (g_contexts.contains(ctx_id)) {
        ctx_id--;
    }
    if (ctx_id <= 0) {
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
        new_context->name = std::string(name);
    }

    return ctx_id;
}

OrbisNpWebApiContext* findAndValidateContext(s32 libCtxId, s32 flag) {
    std::scoped_lock lk{g_global_mutex};
    if (libCtxId < 1 || libCtxId >= 0x8000 || !g_contexts.contains(libCtxId)) {
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

bool areContextHandlesBusy(OrbisNpWebApiContext* context) {
    std::scoped_lock lk{context->contextLock};
    bool is_busy = false;
    for (auto& handle : context->handles) {
        if (handle.second->userCount > 0) {
            return true;
        }
    }
    return false;
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

void checkContextTimeout(OrbisNpWebApiContext* context) {
    u64 time = Kernel::sceKernelGetProcessTime();
    std::scoped_lock lk{context->contextLock};

    for (auto& user_context : context->userContexts) {
        checkUserContextTimeout(user_context.second);
    }

    for (auto& value : context->timerHandles) {
        auto& timer_handle = value.second;
        if (!timer_handle->timedOut && timer_handle->handleTimeout != 0 &&
            timer_handle->handleEndTime < time) {
            timer_handle->timedOut = true;
            abortHandle(context->libCtxId, timer_handle->handleId);
        }
    }
}

void checkTimeout() {
    u64 time = Kernel::sceKernelGetProcessTime();
    if (time < g_last_timeout_check + 1000) {
        return;
    }
    g_last_timeout_check = time;
    std::scoped_lock lk{g_global_mutex};

    for (auto& context : g_contexts) {
        checkContextTimeout(context.second);
    }
}

s32 deleteContext(s32 libCtxId) {
    std::scoped_lock lk{g_global_mutex};
    if (!g_contexts.contains(libCtxId)) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    auto& context = g_contexts[libCtxId];
    context->handles.clear();
    context->timerHandles.clear();
    context->pushEventFilters.clear();
    context->servicePushEventFilters.clear();
    context->extendedPushEventFilters.clear();

    g_contexts.erase(libCtxId);
    return ORBIS_OK;
}

s32 terminateContext(s32 libCtxId) {
    OrbisNpWebApiContext* ctx = findAndValidateContext(libCtxId);
    if (ctx == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    if (g_sdk_ver < Common::ElfInfo::FW_40 && isContextBusy(ctx)) {
        releaseContext(ctx);
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_BUSY;
    }

    std::vector<s32> user_context_ids;
    for (auto& user_context : ctx->userContexts) {
        user_context_ids.emplace_back(user_context.first);
    }
    for (s32 user_context_id : user_context_ids) {
        s32 result = deleteUserContext(user_context_id);
        if (result != ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND ||
            g_sdk_ver < Common::ElfInfo::FW_40) {
            return result;
        }
    }

    lockContext(ctx);
    if (g_sdk_ver >= Common::ElfInfo::FW_40) {
        for (auto& handle : ctx->handles) {
            abortHandle(libCtxId, handle.first);
        }
        if (isContextTerminated(ctx)) {
            unlockContext(ctx);
            releaseContext(ctx);
            return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
        }
        markContextAsTerminated(ctx);
        while (isContextBusy(ctx) || areContextHandlesBusy(ctx)) {
            unlockContext(ctx);
            Kernel::sceKernelUsleep(50000);
            lockContext(ctx);
        }
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
    LOG_WARNING(Lib_NpWebApi, "called libCtxId = {}", libCtxId);

    Libraries::UserService::OrbisUserServiceUserId user_id = 0;
    Libraries::UserService::sceUserServiceGetInitialUser(&user_id);
    return createUserContext(libCtxId, user_id);
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
    g_user_context_count++;
    if (g_user_context_count >= 0x10000) {
        g_user_context_count = 1;
    }
    s32 user_ctx_id = (libCtxId << 0x10) | g_user_context_count;
    while (context->userContexts.contains(user_ctx_id)) {
        user_ctx_id--;
    }
    if (user_ctx_id <= (libCtxId << 0x10)) {
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

s32 registerNotificationCallback(s32 titleUserCtxId, OrbisNpWebApiNotificationCallback cbFunc,
                                 void* pUserArg) {
    OrbisNpWebApiContext* context = findAndValidateContext(titleUserCtxId >> 0x10);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiUserContext* user_context = findUserContext(context, titleUserCtxId);
    if (user_context == nullptr) {
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND;
    }

    lockContext(context);
    user_context->notificationCallbackFunction = cbFunc;
    user_context->pNotificationCallbackUserArgs = pUserArg;
    unlockContext(context);
    releaseUserContext(user_context);
    releaseContext(context);
    return ORBIS_OK;
}

s32 unregisterNotificationCallback(s32 titleUserCtxId) {
    OrbisNpWebApiContext* context = findAndValidateContext(titleUserCtxId >> 0x10);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiUserContext* user_context = findUserContext(context, titleUserCtxId);
    if (user_context == nullptr) {
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND;
    }

    lockContext(context);
    user_context->notificationCallbackFunction = nullptr;
    user_context->pNotificationCallbackUserArgs = nullptr;
    unlockContext(context);
    releaseUserContext(user_context);
    releaseContext(context);
    return ORBIS_OK;
}

bool isUserContextBusy(OrbisNpWebApiUserContext* userContext) {
    std::scoped_lock lk{userContext->parentContext->contextLock};
    return userContext->userCount > 1;
}

bool areUserContextRequestsBusy(OrbisNpWebApiUserContext* userContext) {
    std::scoped_lock lk{userContext->parentContext->contextLock};
    bool is_busy = false;
    for (auto& request : userContext->requests) {
        request.second->userCount++;
        bool req_busy = isRequestBusy(request.second);
        request.second->userCount--;
        if (req_busy) {
            return true;
        }
    }
    return false;
}

void releaseUserContext(OrbisNpWebApiUserContext* userContext) {
    std::scoped_lock lk{userContext->parentContext->contextLock};
    userContext->userCount--;
}

void checkUserContextTimeout(OrbisNpWebApiUserContext* userContext) {
    std::scoped_lock lk{userContext->parentContext->contextLock};
    for (auto& request : userContext->requests) {
        checkRequestTimeout(request.second);
    }
}

s32 deleteUserContext(s32 titleUserCtxId) {
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
    }

    if (g_sdk_ver < Common::ElfInfo::FW_40) {
        if (isUserContextBusy(user_context)) {
            releaseUserContext(user_context);
            unlockContext(context);
            releaseContext(context);
            return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_BUSY;
        }

        if (areUserContextRequestsBusy(user_context)) {
            releaseUserContext(user_context);
            unlockContext(context);
            releaseContext(context);
            return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_BUSY;
        }
    } else {
        for (auto& request : user_context->requests) {
            abortRequestInternal(context, user_context, request.second);
        }

        if (user_context->deleted) {
            releaseUserContext(user_context);
            unlockContext(context);
            releaseContext(context);
            return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND;
        }

        user_context->deleted = true;
        while (isUserContextBusy(user_context) || areUserContextRequestsBusy(user_context)) {
            unlockContext(context);
            Kernel::sceKernelUsleep(50000);
            lockContext(context);
        }
    }

    user_context->extendedPushEventCallbacks.clear();
    user_context->servicePushEventCallbacks.clear();
    user_context->pushEventCallbacks.clear();
    user_context->requests.clear();
    context->userContexts.erase(titleUserCtxId);

    unlockContext(context);
    releaseContext(context);
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

    s64 user_ctx_id = static_cast<s64>(titleUserCtxId);
    s32 request_id = (user_ctx_id << 0x20) | g_request_count;
    while (user_context->requests.contains(request_id)) {
        request_id--;
    }
    // Real library would hang if this assert fails.
    ASSERT_MSG(request_id <= (user_ctx_id << 0x20), "Too many requests!");
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

s32 setRequestTimeout(s64 requestId, u32 timeout) {
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

    request->requestTimeout = timeout;

    releaseRequest(request);
    releaseUserContext(user_context);
    releaseContext(context);
    return ORBIS_OK;
}

void startRequestTimer(OrbisNpWebApiRequest* request) {
    if (request->requestTimeout != 0 && request->requestEndTime == 0) {
        request->requestEndTime = Kernel::sceKernelGetProcessTime() + request->requestTimeout;
    }
}

void checkRequestTimeout(OrbisNpWebApiRequest* request) {
    u64 time = Kernel::sceKernelGetProcessTime();
    if (!request->timedOut && request->requestEndTime != 0 && request->requestEndTime < time) {
        request->timedOut = true;
        abortRequest(request->requestId);
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

    startRequestTimer(request);

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

    releaseUserContext(user_context);
    unlockContext(context);
    releaseContext(context);
    return ORBIS_OK;
}

s32 createHandleInternal(OrbisNpWebApiContext* context) {
    g_handle_count++;
    if (g_handle_count >= 0xf0000000) {
        g_handle_count = 1;
    }

    std::scoped_lock lk{context->contextLock};

    s32 handle_id = g_handle_count;
    context->handles[handle_id] = new OrbisNpWebApiHandle{};
    auto& handle = context->handles[handle_id];
    handle->handleId = handle_id;
    handle->userCount = 0;
    handle->aborted = false;
    handle->deleted = false;

    if (g_sdk_ver >= Common::ElfInfo::FW_30) {
        context->timerHandles[handle_id] = new OrbisNpWebApiTimerHandle{};
        auto& timer_handle = context->timerHandles[handle_id];
        timer_handle->handleId = handle_id;
        timer_handle->timedOut = false;
        timer_handle->handleTimeout = 0;
        timer_handle->handleEndTime = 0;
    }

    return handle_id;
}

s32 createHandle(s32 libCtxId) {
    OrbisNpWebApiContext* context = findAndValidateContext(libCtxId);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    s32 result = createHandleInternal(context);
    releaseContext(context);
    return result;
}

s32 setHandleTimeoutInternal(OrbisNpWebApiContext* context, s32 handleId, u32 timeout) {
    std::scoped_lock lk{context->contextLock};
    if (!context->timerHandles.contains(handleId)) {
        return ORBIS_NP_WEBAPI_ERROR_HANDLE_NOT_FOUND;
    }
    auto& handle = context->handles[handleId];
    handle->userCount++;

    auto& timer_handle = context->timerHandles[handleId];
    timer_handle->handleTimeout = timeout;

    handle->userCount--;
    return ORBIS_OK;
}

s32 setHandleTimeout(s32 libCtxId, s32 handleId, u32 timeout) {
    OrbisNpWebApiContext* context = findAndValidateContext(libCtxId);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    s32 result = setHandleTimeoutInternal(context, handleId, timeout);
    releaseContext(context);
    return result;
}

void startHandleTimer(OrbisNpWebApiContext* context, s32 handleId) {
    std::scoped_lock lk{context->contextLock};
    if (!context->timerHandles.contains(handleId)) {
        return;
    }
    auto& timer_handle = context->timerHandles[handleId];
    if (timer_handle->handleTimeout == 0) {
        return;
    }
    timer_handle->handleEndTime = Kernel::sceKernelGetProcessTime() + timer_handle->handleTimeout;
}

void releaseHandle(OrbisNpWebApiContext* context, OrbisNpWebApiHandle* handle) {
    if (handle != nullptr) {
        std::scoped_lock lk{context->contextLock};
        handle->userCount--;
    }
}

s32 getHandle(OrbisNpWebApiContext* context, s32 handleId, OrbisNpWebApiHandle** handleOut) {
    std::scoped_lock lk{context->contextLock};
    if (!context->handles.contains(handleId)) {
        return ORBIS_NP_WEBAPI_ERROR_HANDLE_NOT_FOUND;
    }
    auto& handle = context->handles[handleId];
    handle->userCount++;
    if (handleOut != nullptr) {
        *handleOut = handle;
    }
    return ORBIS_OK;
}

s32 abortHandle(s32 libCtxId, s32 handleId) {
    OrbisNpWebApiContext* context = findAndValidateContext(libCtxId);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiHandle* handle;
    s32 result = getHandle(context, handleId, &handle);
    if (result == ORBIS_OK) {
        std::scoped_lock lk{context->contextLock};
        handle->aborted = true;
        // TODO: sceNpAsmClientAbortRequest call
        releaseHandle(context, handle);
    }

    releaseContext(context);
    return result;
}

s32 deleteHandleInternal(OrbisNpWebApiContext* context, s32 handleId) {
    lockContext(context);
    if (!context->handles.contains(handleId)) {
        return ORBIS_NP_WEBAPI_ERROR_HANDLE_NOT_FOUND;
    }

    auto& handle = context->handles[handleId];
    if (g_sdk_ver >= Common::ElfInfo::FW_40) {
        if (handle->deleted) {
            unlockContext(context);
            return ORBIS_NP_WEBAPI_ERROR_HANDLE_NOT_FOUND;
        }
        handle->deleted = true;
        unlockContext(context);
        abortHandle(context->libCtxId, handleId);
        lockContext(context);
        handle->userCount++;
        while (handle->userCount > 1) {
            handle->userCount--;
            unlockContext(context);
            Kernel::sceKernelUsleep(50000);
            lockContext(context);
            handle->userCount++;
        }
        handle->userCount--;
    } else if (handle->userCount > 0) {
        unlockContext(context);
        return ORBIS_NP_WEBAPI_ERROR_HANDLE_BUSY;
    }

    context->handles.erase(handleId);

    if (g_sdk_ver >= Common::ElfInfo::FW_30 && context->timerHandles.contains(handleId)) {
        auto& timer_handle = context->timerHandles[handleId];
        context->timerHandles.erase(handleId);
    }

    unlockContext(context);
    return ORBIS_OK;
}

s32 deleteHandle(s32 libCtxId, s32 handleId) {
    OrbisNpWebApiContext* context = findAndValidateContext(libCtxId);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    s32 result = deleteHandleInternal(context, handleId);
    releaseContext(context);
    return result;
}

s32 createPushEventFilterInternal(OrbisNpWebApiContext* context,
                                  const OrbisNpWebApiPushEventFilterParameter* pFilterParam,
                                  u64 filterParamNum) {
    std::scoped_lock lk{context->contextLock};
    g_push_event_filter_count++;
    if (g_push_event_filter_count >= 0xf0000000) {
        g_push_event_filter_count = 1;
    }
    s32 filterId = g_push_event_filter_count;

    context->pushEventFilters[filterId] = new OrbisNpWebApiPushEventFilter{};
    auto& filter = context->pushEventFilters[filterId];
    filter->parentContext = context;
    filter->filterId = filterId;

    if (pFilterParam != nullptr && filterParamNum != 0) {
        for (u64 param_idx = 0; param_idx < filterParamNum; param_idx++) {
            OrbisNpWebApiPushEventFilterParameter copy = OrbisNpWebApiPushEventFilterParameter{};
            memcpy(&copy, &pFilterParam[param_idx], sizeof(OrbisNpWebApiPushEventFilterParameter));
            filter->filterParams.emplace_back(copy);
        }
    }
    return filterId;
}

s32 createPushEventFilter(s32 libCtxId, const OrbisNpWebApiPushEventFilterParameter* pFilterParam,
                          u64 filterParamNum) {
    OrbisNpWebApiContext* context = findAndValidateContext(libCtxId);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    s32 result = createPushEventFilterInternal(context, pFilterParam, filterParamNum);
    releaseContext(context);
    return result;
}

s32 deletePushEventFilterInternal(OrbisNpWebApiContext* context, s32 filterId) {
    std::scoped_lock lk{context->contextLock};
    if (!context->pushEventFilters.contains(filterId)) {
        return ORBIS_NP_WEBAPI_ERROR_PUSH_EVENT_FILTER_NOT_FOUND;
    }

    context->pushEventFilters[filterId]->filterParams.clear();
    context->pushEventFilters.erase(filterId);
    return ORBIS_OK;
}

s32 deletePushEventFilter(s32 libCtxId, s32 filterId) {
    OrbisNpWebApiContext* context = findAndValidateContext(libCtxId);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    s32 result = deletePushEventFilterInternal(context, filterId);
    releaseContext(context);
    return result;
}

s32 registerPushEventCallbackInternal(OrbisNpWebApiUserContext* userContext, s32 filterId,
                                      OrbisNpWebApiPushEventCallback cbFunc, void* pUserArg) {
    std::scoped_lock lk{userContext->parentContext->contextLock};
    g_registered_callback_count++;
    if (g_registered_callback_count >= 0xf0000000) {
        g_registered_callback_count = 1;
    }
    s32 cbId = g_registered_callback_count;

    userContext->pushEventCallbacks[cbId] = new OrbisNpWebApiRegisteredPushEventCallback{};
    auto& cb = userContext->pushEventCallbacks[cbId];
    cb->callbackId = cbId;
    cb->filterId = filterId;
    cb->cbFunc = cbFunc;
    cb->pUserArg = pUserArg;

    return cbId;
}

s32 registerPushEventCallback(s32 titleUserCtxId, s32 filterId,
                              OrbisNpWebApiPushEventCallback cbFunc, void* pUserArg) {
    OrbisNpWebApiContext* context = findAndValidateContext(titleUserCtxId >> 0x10);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiUserContext* user_context = findUserContext(context, titleUserCtxId);
    if (user_context == nullptr) {
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND;
    }

    if (g_sdk_ver >= Common::ElfInfo::FW_25 && !context->pushEventFilters.contains(filterId)) {
        releaseUserContext(user_context);
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_PUSH_EVENT_FILTER_NOT_FOUND;
    }

    s32 result = registerPushEventCallbackInternal(user_context, filterId, cbFunc, pUserArg);
    releaseUserContext(user_context);
    releaseContext(context);
    return result;
}

s32 unregisterPushEventCallback(s32 titleUserCtxId, s32 callbackId) {
    OrbisNpWebApiContext* context = findAndValidateContext(titleUserCtxId >> 0x10);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiUserContext* user_context = findUserContext(context, titleUserCtxId);
    if (user_context == nullptr) {
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND;
    }

    if (!user_context->pushEventCallbacks.contains(callbackId)) {
        releaseUserContext(user_context);
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_PUSH_EVENT_CALLBACK_NOT_FOUND;
    }

    lockContext(context);
    user_context->pushEventCallbacks.erase(callbackId);
    unlockContext(context);
    releaseUserContext(user_context);
    releaseContext(context);
    return ORBIS_OK;
}

s32 createServicePushEventFilterInternal(
    OrbisNpWebApiContext* context, s32 handleId, const char* pNpServiceName,
    OrbisNpServiceLabel npServiceLabel,
    const OrbisNpWebApiServicePushEventFilterParameter* pFilterParam, u64 filterParamNum) {
    std::scoped_lock lk{context->contextLock};
    if (!context->handles.contains(handleId)) {
        return ORBIS_NP_WEBAPI_ERROR_HANDLE_NOT_FOUND;
    }
    auto& handle = context->handles[handleId];
    handle->userCount++;

    if (pNpServiceName != nullptr && !Config::getPSNSignedIn()) {
        // Seems sceNpManagerIntGetUserList fails?
        LOG_DEBUG(Lib_NpWebApi, "Cannot create service push event while PSN is disabled");
        handle->userCount--;
        return ORBIS_NP_WEBAPI_ERROR_SIGNED_IN_USER_NOT_FOUND;
    }

    g_service_push_event_filter_count++;
    if (g_service_push_event_filter_count >= 0xf0000000) {
        g_service_push_event_filter_count = 1;
    }
    s32 filterId = g_service_push_event_filter_count;

    context->servicePushEventFilters[filterId] = new OrbisNpWebApiServicePushEventFilter{};
    auto& filter = context->servicePushEventFilters[filterId];
    filter->parentContext = context;
    filter->filterId = filterId;

    if (pNpServiceName == nullptr) {
        filter->internal = true;
    } else {
        // TODO: if pNpServiceName is non-null, create an np request for this filter.
        LOG_ERROR(Lib_NpWebApi, "Np behavior not handled");
        filter->npServiceName = std::string(pNpServiceName);
    }

    filter->npServiceLabel = npServiceLabel;

    if (pFilterParam != nullptr && filterParamNum != 0) {
        for (u64 param_idx = 0; param_idx < filterParamNum; param_idx++) {
            OrbisNpWebApiServicePushEventFilterParameter copy =
                OrbisNpWebApiServicePushEventFilterParameter{};
            memcpy(&copy, &pFilterParam[param_idx],
                   sizeof(OrbisNpWebApiServicePushEventFilterParameter));
            filter->filterParams.emplace_back(copy);
        }
    }

    handle->userCount--;
    return filterId;
}

s32 createServicePushEventFilter(s32 libCtxId, s32 handleId, const char* pNpServiceName,
                                 OrbisNpServiceLabel npServiceLabel,
                                 const OrbisNpWebApiServicePushEventFilterParameter* pFilterParam,
                                 u64 filterParamNum) {
    OrbisNpWebApiContext* context = findAndValidateContext(libCtxId);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    startHandleTimer(context, handleId);
    s32 result = createServicePushEventFilterInternal(context, handleId, pNpServiceName,
                                                      npServiceLabel, pFilterParam, filterParamNum);
    releaseContext(context);
    return result;
}

s32 deleteServicePushEventFilterInternal(OrbisNpWebApiContext* context, s32 filterId) {
    std::scoped_lock lk{context->contextLock};
    if (!context->servicePushEventFilters.contains(filterId)) {
        return ORBIS_NP_WEBAPI_ERROR_SERVICE_PUSH_EVENT_FILTER_NOT_FOUND;
    }

    context->servicePushEventFilters[filterId]->filterParams.clear();
    context->servicePushEventFilters.erase(filterId);
    return ORBIS_OK;
}

s32 deleteServicePushEventFilter(s32 libCtxId, s32 filterId) {
    OrbisNpWebApiContext* context = findAndValidateContext(libCtxId);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    s32 result = deleteServicePushEventFilterInternal(context, filterId);
    releaseContext(context);
    return result;
}

s32 registerServicePushEventCallbackInternal(
    OrbisNpWebApiUserContext* userContext, s32 filterId,
    OrbisNpWebApiServicePushEventCallback cbFunc,
    OrbisNpWebApiInternalServicePushEventCallback intCbFunc,
    OrbisNpWebApiInternalServicePushEventCallbackA intCbFuncA, void* pUserArg) {
    std::scoped_lock lk{userContext->parentContext->contextLock};
    if (cbFunc == nullptr && intCbFunc == nullptr && intCbFuncA == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }

    g_registered_callback_count++;
    if (g_registered_callback_count >= 0xf0000000) {
        g_registered_callback_count = 1;
    }
    s32 cbId = g_registered_callback_count;

    userContext->servicePushEventCallbacks[cbId] =
        new OrbisNpWebApiRegisteredServicePushEventCallback{};
    auto& cb = userContext->servicePushEventCallbacks[cbId];
    cb->callbackId = cbId;
    cb->filterId = filterId;
    cb->cbFunc = cbFunc;
    cb->internalCbFunc = intCbFunc;
    cb->internalCbFuncA = intCbFuncA;
    cb->pUserArg = pUserArg;

    return cbId;
}

s32 registerServicePushEventCallback(s32 titleUserCtxId, s32 filterId,
                                     OrbisNpWebApiServicePushEventCallback cbFunc,
                                     OrbisNpWebApiInternalServicePushEventCallback intCbFunc,
                                     OrbisNpWebApiInternalServicePushEventCallbackA intCbFuncA,
                                     void* pUserArg) {
    OrbisNpWebApiContext* context = findAndValidateContext(titleUserCtxId >> 0x10);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiUserContext* user_context = findUserContext(context, titleUserCtxId);
    if (user_context == nullptr) {
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND;
    }

    if (g_sdk_ver >= Common::ElfInfo::FW_25 &&
        !context->servicePushEventFilters.contains(filterId)) {
        releaseUserContext(user_context);
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_SERVICE_PUSH_EVENT_FILTER_NOT_FOUND;
    }

    s32 result = registerServicePushEventCallbackInternal(user_context, filterId, cbFunc, intCbFunc,
                                                          intCbFuncA, pUserArg);
    releaseUserContext(user_context);
    releaseContext(context);
    return result;
}

s32 unregisterServicePushEventCallback(s32 titleUserCtxId, s32 callbackId) {
    OrbisNpWebApiContext* context = findAndValidateContext(titleUserCtxId >> 0x10);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiUserContext* user_context = findUserContext(context, titleUserCtxId);
    if (user_context == nullptr) {
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND;
    }

    if (!user_context->servicePushEventCallbacks.contains(callbackId)) {
        releaseUserContext(user_context);
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_SERVICE_PUSH_EVENT_CALLBACK_NOT_FOUND;
    }

    lockContext(context);
    user_context->servicePushEventCallbacks.erase(callbackId);
    unlockContext(context);
    releaseUserContext(user_context);
    releaseContext(context);
    return ORBIS_OK;
}

s32 createExtendedPushEventFilterInternal(
    OrbisNpWebApiContext* context, s32 handleId, const char* pNpServiceName,
    OrbisNpServiceLabel npServiceLabel,
    const OrbisNpWebApiExtdPushEventFilterParameter* pFilterParam, u64 filterParamNum,
    bool internal) {
    std::scoped_lock lk{context->contextLock};
    if (!context->handles.contains(handleId)) {
        return ORBIS_NP_WEBAPI_ERROR_HANDLE_NOT_FOUND;
    }
    auto& handle = context->handles[handleId];
    handle->userCount++;

    if (pNpServiceName != nullptr && !Config::getPSNSignedIn()) {
        // Seems sceNpManagerIntGetUserList fails?
        LOG_DEBUG(Lib_NpWebApi, "Cannot create extended push event while PSN is disabled");
        handle->userCount--;
        return ORBIS_NP_WEBAPI_ERROR_SIGNED_IN_USER_NOT_FOUND;
    }

    g_extended_push_event_filter_count++;
    if (g_extended_push_event_filter_count >= 0xf0000000) {
        g_extended_push_event_filter_count = 1;
    }
    s32 filterId = g_extended_push_event_filter_count;

    context->extendedPushEventFilters[filterId] = new OrbisNpWebApiExtendedPushEventFilter{};
    auto& filter = context->extendedPushEventFilters[filterId];
    filter->internal = internal;
    filter->parentContext = context;
    filter->filterId = filterId;

    if (pNpServiceName == nullptr) {
        npServiceLabel = ORBIS_NP_INVALID_SERVICE_LABEL;
    } else {
        // TODO: if pNpServiceName is non-null, create an np request for this filter.
        LOG_ERROR(Lib_NpWebApi, "Np behavior not handled");
        filter->npServiceName = std::string(pNpServiceName);
    }

    filter->npServiceLabel = npServiceLabel;

    if (pFilterParam != nullptr && filterParamNum != 0) {
        for (u64 param_idx = 0; param_idx < filterParamNum; param_idx++) {
            OrbisNpWebApiExtdPushEventFilterParameter copy =
                OrbisNpWebApiExtdPushEventFilterParameter{};
            memcpy(&copy, &pFilterParam[param_idx],
                   sizeof(OrbisNpWebApiExtdPushEventFilterParameter));
            filter->filterParams.emplace_back(copy);
            // TODO: Every parameter is registered with an extended data filter through
            // sceNpPushRegisterExtendedDataFilter
        }
    }

    handle->userCount--;
    return filterId;
}

s32 createExtendedPushEventFilter(s32 libCtxId, s32 handleId, const char* pNpServiceName,
                                  OrbisNpServiceLabel npServiceLabel,
                                  const OrbisNpWebApiExtdPushEventFilterParameter* pFilterParam,
                                  u64 filterParamNum, bool internal) {
    OrbisNpWebApiContext* context = findAndValidateContext(libCtxId);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    startHandleTimer(context, handleId);
    s32 result = createExtendedPushEventFilterInternal(
        context, handleId, pNpServiceName, npServiceLabel, pFilterParam, filterParamNum, internal);
    releaseContext(context);
    return result;
}

s32 deleteExtendedPushEventFilterInternal(OrbisNpWebApiContext* context, s32 filterId) {
    std::scoped_lock lk{context->contextLock};
    if (!context->extendedPushEventFilters.contains(filterId)) {
        return ORBIS_NP_WEBAPI_ERROR_EXTD_PUSH_EVENT_FILTER_NOT_FOUND;
    }

    context->extendedPushEventFilters[filterId]->filterParams.clear();
    context->extendedPushEventFilters.erase(filterId);
    return ORBIS_OK;
}

s32 deleteExtendedPushEventFilter(s32 libCtxId, s32 filterId) {
    OrbisNpWebApiContext* context = findAndValidateContext(libCtxId);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    s32 result = deleteExtendedPushEventFilterInternal(context, filterId);
    releaseContext(context);
    return result;
}

s32 registerExtdPushEventCallbackInternal(OrbisNpWebApiUserContext* userContext, s32 filterId,
                                          OrbisNpWebApiExtdPushEventCallback cbFunc,
                                          OrbisNpWebApiExtdPushEventCallbackA cbFuncA,
                                          void* pUserArg) {
    std::scoped_lock lk{userContext->parentContext->contextLock};

    g_registered_callback_count++;
    if (g_registered_callback_count >= 0xf0000000) {
        g_registered_callback_count = 1;
    }

    if (cbFunc == nullptr && cbFuncA == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }
    s32 cbId = g_registered_callback_count;

    userContext->extendedPushEventCallbacks[cbId] =
        new OrbisNpWebApiRegisteredExtendedPushEventCallback{};
    auto& cb = userContext->extendedPushEventCallbacks[cbId];
    cb->callbackId = cbId;
    cb->filterId = filterId;
    cb->cbFunc = cbFunc;
    cb->cbFuncA = cbFuncA;
    cb->pUserArg = pUserArg;

    return cbId;
}

s32 registerExtdPushEventCallback(s32 titleUserCtxId, s32 filterId,
                                  OrbisNpWebApiExtdPushEventCallback cbFunc,
                                  OrbisNpWebApiExtdPushEventCallbackA cbFuncA, void* pUserArg) {
    OrbisNpWebApiContext* context = findAndValidateContext(titleUserCtxId >> 0x10);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiUserContext* user_context = findUserContext(context, titleUserCtxId);
    if (user_context == nullptr) {
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND;
    }

    if (!context->extendedPushEventFilters.contains(filterId)) {
        releaseUserContext(user_context);
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_EXTD_PUSH_EVENT_FILTER_NOT_FOUND;
    }

    s32 result =
        registerExtdPushEventCallbackInternal(user_context, filterId, cbFunc, cbFuncA, pUserArg);
    releaseUserContext(user_context);
    releaseContext(context);
    return result;
}

s32 registerExtdPushEventCallbackA(s32 titleUserCtxId, s32 filterId,
                                   OrbisNpWebApiExtdPushEventCallbackA cbFunc, void* pUserArg) {
    return registerExtdPushEventCallback(titleUserCtxId, filterId, nullptr, cbFunc, pUserArg);
}

s32 unregisterExtdPushEventCallback(s32 titleUserCtxId, s32 callbackId) {
    OrbisNpWebApiContext* context = findAndValidateContext(titleUserCtxId >> 0x10);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiUserContext* user_context = findUserContext(context, titleUserCtxId);
    if (user_context == nullptr) {
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_USER_CONTEXT_NOT_FOUND;
    }

    if (!user_context->extendedPushEventCallbacks.contains(callbackId)) {
        releaseUserContext(user_context);
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_EXTD_PUSH_EVENT_CALLBACK_NOT_FOUND;
    }

    lockContext(context);
    user_context->extendedPushEventCallbacks.erase(callbackId);
    unlockContext(context);
    releaseUserContext(user_context);
    releaseContext(context);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI getHttpRequestIdFromRequest(OrbisNpWebApiRequest* request)

{
    return request->requestId;
}

s32 PS4_SYSV_ABI getHttpStatusCodeInternal(s64 requestId, s32* out_status_code) {
    s32 status_code;
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

    // Query HTTP layer
    {
        int32_t httpReqId = getHttpRequestIdFromRequest(request);
        s32 err = Libraries::Http::sceHttpGetStatusCode(httpReqId, &status_code);

        if (out_status_code != nullptr)
            *out_status_code = status_code;

        releaseRequest(request);
        releaseUserContext(user_context);
        releaseContext(context);

        return err;
    }
}

void PS4_SYSV_ABI setRequestEndTime(OrbisNpWebApiRequest* request) {
    u64 time;
    if ((request->requestTimeout != 0) && (request->requestEndTime == 0)) {
        time = Libraries::Kernel::sceKernelGetProcessTime();
        request->requestEndTime = (u64)request->requestTimeout + time;
    }
}

void PS4_SYSV_ABI clearRequestEndTime(OrbisNpWebApiRequest* req) {
    req->requestEndTime = 0;
    return;
}

bool PS4_SYSV_ABI hasRequestTimedOut(OrbisNpWebApiRequest* request) {
    return request->timedOut;
}

bool PS4_SYSV_ABI isRequestAborted(OrbisNpWebApiRequest* request) {
    return request->aborted;
}

void PS4_SYSV_ABI setRequestState(OrbisNpWebApiRequest* request, u8 state) {
    request->requestState = state;
}

u64 PS4_SYSV_ABI copyRequestData(OrbisNpWebApiRequest* request, void* data, u64 size) {
    u64 readSize = 0;

    if (request->remainingData != 0) {
        u64 remainingSize = request->remainingData - request->readOffset;

        if (remainingSize != 0) {
            if (remainingSize < size) {
                size = remainingSize;
            }
            memcpy(data, request->data + request->readOffset, size);
            request->readOffset += static_cast<u32>(size);
            readSize = size;
        }
    }
    return readSize;
}

s32 PS4_SYSV_ABI readDataInternal(s64 requestId, void* pData, u64 size) {
    u32 offset;
    s32 result;
    u64 remainingSize;
    u64 bytesCopied;

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

    setRequestEndTime(request);

    bytesCopied = copyRequestData(request, pData, size);
    offset = (u32)bytesCopied;
    remainingSize = size - offset;

    // If caller wants more data than buffered
    if (remainingSize != 0) {
        lockContext(context);
        setRequestState(request, 5); // TODO add request states?

        if (!hasRequestTimedOut(request) && isRequestAborted(request)) {
            unlockContext(context);
            offset = ORBIS_NP_WEBAPI_ERROR_ABORTED;
        } else {
            unlockContext(context);

            int32_t httpReqId = getHttpRequestIdFromRequest(request);
            int32_t httpRead =
                Libraries::Http::sceHttpReadData(httpReqId, (u8*)pData + offset, remainingSize);

            if (httpRead < 0)
                httpRead = 0;

            offset += httpRead;
        }
    }

    // Final state resolution
    lockContext(context);
    setRequestState(request, 0);

    if (hasRequestTimedOut(request)) {
        result = ORBIS_NP_WEBAPI_ERROR_TIMEOUT;
    } else if (isRequestAborted(request)) {
        result = ORBIS_NP_WEBAPI_ERROR_ABORTED;
    } else {
        result = offset;
    }

    unlockContext(context);

    // Cleanup
    clearRequestEndTime(request);
    releaseRequest(request);
    releaseUserContext(user_context);
    releaseContext(context);

    return result;
}

}; // namespace Libraries::Np::NpWebApi