// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <deque>
#include <string>
#include <string_view>
#include <magic_enum/magic_enum.hpp>
#include "common/elf_info.h"
#include "core/emulator_settings.h"
#include "core/libraries/kernel/process.h"
#include "core/libraries/kernel/time.h"
#include "core/libraries/network/http.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_handler.h"
#include "np_web_api_internal.h"

namespace Libraries::Np::NpWebApi {

static std::recursive_mutex g_global_mutex;
static std::map<s32, OrbisNpWebApiContext*> g_contexts;
static s32 g_library_context_count = 0;

// Last WebApi error code parsed from an error response body
static thread_local s32 g_last_webapi_error = ORBIS_OK;

static s32 captureWebApiError(const char* body, u64 len);
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

    if (g_sdk_ver < Common::ElfInfo::FW_400 && isContextBusy(ctx)) {
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
            g_sdk_ver < Common::ElfInfo::FW_400) {
            return result;
        }
    }

    lockContext(ctx);
    if (g_sdk_ver >= Common::ElfInfo::FW_400) {
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

    if (g_sdk_ver < Common::ElfInfo::FW_400) {
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
    if (g_sdk_ver >= Common::ElfInfo::FW_400 && user_context->deleted) {
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
    s64 request_id = (user_ctx_id << 0x20) | g_request_count;
    while (user_context->requests.contains(request_id)) {
        request_id--;
    }
    // Real library would hang if this assert fails.
    ASSERT_MSG(request_id > (user_ctx_id << 0x20), "Too many requests!");
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

s32 setMultipartContentType(s64 requestId, const char* pTypeName, const char* pBoundary) {
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
    request->multipartContentType = (pTypeName != nullptr) ? pTypeName : "";
    request->multipartBoundary = (pBoundary != nullptr) ? pBoundary : "";
    releaseRequest(request);
    releaseUserContext(user_context);
    releaseContext(context);
    return ORBIS_OK;
}

s32 addMultipartPart(s64 requestId, const OrbisNpWebApiMultipartPartParameter* pParam,
                     s32* pIndex) {
    if (pParam == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }
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

    // The library supplies Content-Length itself (from contentLength), so skip an app-provided one.
    const auto isContentLength = [](const char* name) {
        const char* t = "content-length";
        for (; *name != '\0' && *t != '\0'; ++name, ++t) {
            char c = *name;
            if (c >= 'A' && c <= 'Z') {
                c = static_cast<char>(c - 'A' + 'a');
            }
            if (c != *t) {
                return false;
            }
        }
        return *name == '\0' && *t == '\0';
    };

    OrbisNpWebApiRequest::MultipartPart part;
    part.contentLength = pParam->contentLength;
    std::string headers;
    for (u64 i = 0; i < pParam->headerNum; ++i) {
        const OrbisNpWebApiHttpHeader& h = pParam->pHeaders[i];
        if (h.pName == nullptr || h.pValue == nullptr || isContentLength(h.pName)) {
            continue;
        }
        headers += h.pName;
        headers += ": ";
        headers += h.pValue;
        headers += "\r\n";
    }
    headers += "Content-Length: ";
    headers += std::to_string(pParam->contentLength);
    headers += "\r\n\r\n";
    part.rawHeaders = std::move(headers);

    request->multipartParts.push_back(std::move(part));
    if (pIndex != nullptr) {
        *pIndex = static_cast<s32>(request->multipartParts.size()); // 1-based part index
    }
    releaseRequest(request);
    releaseUserContext(user_context);
    releaseContext(context);
    return ORBIS_OK;
}

s32 sendRequest(s64 requestId, s32 partIndex, const void* pData, u64 dataSize, s8 flag,
                OrbisNpWebApiResponseInformationOption* pRespInfoOption) {
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

    // Multipart requests accumulate each part payload across sendMultipartRequest() calls,the
    // full multipart/mixed body is framed and transmitted once every declared part is complete.
    std::string multipartBody;
    const void* sendData = pData;
    u64 sendSize = dataSize;
    if (request->multipart) {
        if (partIndex < 1 || partIndex > static_cast<s32>(request->multipartParts.size())) {
            releaseRequest(request);
            releaseUserContext(user_context);
            releaseContext(context);
            return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
        }
        auto& part = request->multipartParts[partIndex - 1];
        if (pData != nullptr && dataSize > 0) {
            part.data.append(static_cast<const char*>(pData), dataSize);
        }
        part.sentSize += dataSize;

        bool allComplete = true;
        for (const auto& p : request->multipartParts) {
            if (p.sentSize < p.contentLength) {
                allComplete = false;
                break;
            }
        }
        if (!allComplete) {
            // Further part payloads still expected; nothing to transmit yet.
            releaseRequest(request);
            releaseUserContext(user_context);
            releaseContext(context);
            return ORBIS_OK;
        }

        std::string boundary = request->multipartBoundary;
        if (boundary.empty()) {
            // Fallback boundary derived from the unique request id.
            static const char kHex[] = "0123456789abcdef";
            boundary = "----shadPS4Boundary";
            const u64 v = static_cast<u64>(requestId);
            for (int shift = 60; shift >= 0; shift -= 4) {
                boundary += kHex[(v >> shift) & 0xf];
            }
        }
        const std::string typeName = request->multipartContentType.empty()
                                         ? std::string("multipart/mixed")
                                         : request->multipartContentType;
        for (const auto& p : request->multipartParts) {
            multipartBody += "--";
            multipartBody += boundary;
            multipartBody += "\r\n";
            multipartBody += p.rawHeaders; // header lines + blank line separating headers/body
            multipartBody += p.data;
            multipartBody += "\r\n";
        }
        multipartBody += "--";
        multipartBody += boundary;
        multipartBody += "--\r\n";

        request->userContentType = typeName + "; boundary=" + boundary;
        request->userContentLength = multipartBody.size();
        sendData = multipartBody.data();
        sendSize = multipartBody.size();
    }

    if (g_sdk_ver >= Common::ElfInfo::FW_250 && !request->sent) {
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
    if (!EmulatorSettings.IsShadNetEnabled()) {
        releaseRequest(request);
        releaseUserContext(user_context);
        releaseContext(context);
        return ORBIS_NP_WEBAPI_ERROR_NOT_SIGNED_IN;
    }

    if (request->http_request_id == 0) {
        std::string base_url = EmulatorSettings.GetShadNetWebApiServer();
        // sceHttpCreateConnectionWithURL expects a template id, not the raw libhttp
        // context id that NpWebApi was initialized with. Create a template from the
        // context first, then open the connection against it.
        const s32 tmpl_id = Libraries::Http::sceHttpCreateTemplate(
            context->libHttpCtxId, "libhttp", /*httpVer=*/2, /*isAutoProxyConf=*/0);
        if (tmpl_id < 0) {
            LOG_ERROR(Lib_NpWebApi, "sceHttpCreateTemplate failed: {:#x}", tmpl_id);
            releaseRequest(request);
            releaseUserContext(user_context);
            releaseContext(context);
            return tmpl_id;
        }
        request->http_template_id = tmpl_id;
        const int conn_id = Libraries::Http::sceHttpCreateConnectionWithURL(
            tmpl_id, base_url.c_str(), /*enableKeepalive=*/true);
        if (conn_id < 0) {
            LOG_ERROR(Lib_NpWebApi, "sceHttpCreateConnectionWithURL failed: {:#x}", conn_id);
            Libraries::Http::sceHttpDeleteTemplate(tmpl_id);
            request->http_template_id = 0;
            releaseRequest(request);
            releaseUserContext(user_context);
            releaseContext(context);
            return conn_id;
        }
        request->http_connection_id = conn_id;

        // Convert the WebAPI method enum to libhttp's SceHttpMethods
        // enum. The two enums have different orderings:
        //
        //   OrbisNpWebApiHttpMethod: GET=0, POST=1, PUT=2, DELETE=3, PATCH=4
        //   SceHttpMethods:          GET=0, POST=1, HEAD=2, OPTIONS=3,
        //                            PUT=4, DELETE=5, TRACE=6, CONNECT=7
        s32 sceMethod;
        switch (request->userMethod) {
        case ORBIS_NP_WEBAPI_HTTP_METHOD_GET:
            sceMethod = 0;
            break;
        case ORBIS_NP_WEBAPI_HTTP_METHOD_POST:
            sceMethod = 1;
            break;
        case ORBIS_NP_WEBAPI_HTTP_METHOD_PUT:
            sceMethod = 4;
            break;
        case ORBIS_NP_WEBAPI_HTTP_METHOD_DELETE:
            sceMethod = 5;
            break;
        case ORBIS_NP_WEBAPI_HTTP_METHOD_PATCH:
            sceMethod = 8; // out-of-band PATCH marker recognised by libhttp
            break;
        default:
            LOG_ERROR(Lib_NpWebApi, "unknown method enum value {}",
                      static_cast<int>(request->userMethod));
            releaseRequest(request);
            releaseUserContext(user_context);
            releaseContext(context);
            return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
        }
        const std::string full_url = base_url + request->userPath;
        const int req_id = Libraries::Http::sceHttpCreateRequestWithURL(
            conn_id, sceMethod, full_url.c_str(), request->userContentLength);
        if (req_id < 0) {
            LOG_ERROR(Lib_NpWebApi, "sceHttpCreateRequestWithURL failed: {:#x}", req_id);
            Libraries::Http::sceHttpDeleteConnection(conn_id);
            request->http_connection_id = 0;
            Libraries::Http::sceHttpDeleteTemplate(tmpl_id);
            request->http_template_id = 0;
            releaseRequest(request);
            releaseUserContext(user_context);
            releaseContext(context);
            return req_id;
        }
        request->http_request_id = req_id;

        // Add Content-Type if the caller specified one.
        // adds Accept-Encoding, User-Agent, OAuth Authorization , etc ....
        // TODO check if we need to add them
        if (!request->userContentType.empty()) {
            Libraries::Http::sceHttpAddRequestHeader(req_id, "Content-Type",
                                                     request->userContentType.c_str(), /*mode=*/0);
        }

        const std::string bearer = NpHandler::GetInstance().GetBearerToken(user_context->userId);
        if (!bearer.empty()) {
            const std::string auth_value = "Bearer " + bearer;
            Libraries::Http::sceHttpAddRequestHeader(req_id, "Authorization", auth_value.c_str(),
                                                     /*mode=*/0);
        } else {
            LOG_WARNING(Lib_NpWebApi,
                        "no bearer token for user_id={}; request to '{}' will "
                        "be unauthenticated (expect 401 from server)",
                        user_context->userId, request->userPath);
        }

        // Replay app-supplied headers. Content-Type is already emitted above from the
        // ContentParameter (userContentType),skip a duplicate app-supplied Content-Type so the
        // request never carries two
        const auto isContentType = [](const std::string& name) {
            constexpr std::string_view target = "content-type";
            if (name.size() != target.size()) {
                return false;
            }
            for (size_t i = 0; i < name.size(); ++i) {
                char c = name[i];
                if (c >= 'A' && c <= 'Z') {
                    c = static_cast<char>(c - 'A' + 'a');
                }
                if (c != target[i]) {
                    return false;
                }
            }
            return true;
        };
        const bool haveContentType = !request->userContentType.empty();
        for (const auto& [hname, hvalue] : request->userHeaders) {
            if (haveContentType && isContentType(hname)) {
                continue;
            }
            Libraries::Http::sceHttpAddRequestHeader(req_id, hname.c_str(), hvalue.c_str(),
                                                     /*mode=*/0);
        }
    }

    setRequestState(request, 4);

    const s32 send_err =
        Libraries::Http::sceHttpSendRequest(request->http_request_id, sendData, sendSize);
    if (send_err < 0) {
        LOG_ERROR(Lib_NpWebApi, "sceHttpSendRequest failed: {:#x}", send_err);
        releaseRequest(request);
        releaseUserContext(user_context);
        releaseContext(context);
        return send_err;
    }

    LOG_INFO(Lib_NpWebApi, "requestId={:#x} apiGroup='{}' path='{}' method={} httpReqId={}",
             requestId, request->userApiGroup, request->userPath,
             magic_enum::enum_name(request->userMethod), request->http_request_id);

    s32 sendResult = ORBIS_OK;
    if (flag != 0) {
        s32 status = 0;
        if (Libraries::Http::sceHttpGetStatusCode(request->http_request_id, &status) >= 0) {
            if (pRespInfoOption != nullptr) {
                pRespInfoOption->httpStatus = status;
            }
            if (status >= 400) {
                std::string errBody;
                char buf[256];
                for (;;) {
                    const s32 n = Libraries::Http::sceHttpReadData(request->http_request_id, buf,
                                                                   sizeof(buf));
                    if (n <= 0)
                        break;
                    errBody.append(buf, static_cast<size_t>(n));
                    if (errBody.size() > 64u * 1024u)
                        break; // sanity cap on a runaway error body
                }
                if (pRespInfoOption != nullptr) {
                    pRespInfoOption->responseDataSize = errBody.size();
                    char* const dst = pRespInfoOption->pErrorObject;
                    const u64 cap = pRespInfoOption->errorObjectSize;
                    if (dst != nullptr && cap > 0) {
                        const u64 n = std::min<u64>(errBody.size(), cap - 1);
                        if (n > 0)
                            std::memcpy(dst, errBody.data(), n);
                        dst[n] = '\0';
                    }
                }
                const s32 npErr = captureWebApiError(errBody.data(), errBody.size());
                if (npErr > 0 && npErr < 0xc00000) {
                    sendResult = static_cast<s32>(0x82000000u | static_cast<u32>(npErr));
                } else if (status >= 100 && status < 600) {
                    sendResult = static_cast<s32>(0x82f00000u | static_cast<u32>(status));
                } else {
                    sendResult = static_cast<s32>(0x82ffffffu);
                }
            }
        }
    }

    releaseRequest(request);
    releaseUserContext(user_context);
    releaseContext(context);
    return sendResult;
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

    if (g_sdk_ver < Common::ElfInfo::FW_400 && isRequestBusy(request)) {
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
    if (request->http_request_id != 0) {
        Libraries::Http::sceHttpDeleteRequest(request->http_request_id);
        request->http_request_id = 0;
    }
    if (request->http_connection_id != 0) {
        Libraries::Http::sceHttpDeleteConnection(request->http_connection_id);
        request->http_connection_id = 0;
    }
    if (request->http_template_id != 0) {
        Libraries::Http::sceHttpDeleteTemplate(request->http_template_id);
        request->http_template_id = 0;
    }
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

    if (g_sdk_ver >= Common::ElfInfo::FW_300) {
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
    if (g_sdk_ver >= Common::ElfInfo::FW_400) {
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

    if (g_sdk_ver >= Common::ElfInfo::FW_300 && context->timerHandles.contains(handleId)) {
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

    LOG_INFO(Lib_NpWebApi, "filterId={} dataTypeParams={}", filterId, filterParamNum);
    if (pFilterParam != nullptr && filterParamNum != 0) {
        for (u64 param_idx = 0; param_idx < filterParamNum; param_idx++) {
            OrbisNpWebApiPushEventFilterParameter copy = OrbisNpWebApiPushEventFilterParameter{};
            memcpy(&copy, &pFilterParam[param_idx], sizeof(OrbisNpWebApiPushEventFilterParameter));
            LOG_INFO(Lib_NpWebApi, "  filterParam[{}] dataType='{}'", param_idx, copy.dataType.val);
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

    if (g_sdk_ver >= Common::ElfInfo::FW_250 && !context->pushEventFilters.contains(filterId)) {
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

    if (pNpServiceName != nullptr && !EmulatorSettings.IsShadNetEnabled()) {
        // Seems sceNpManagerIntGetUserList fails?
        LOG_DEBUG(Lib_NpWebApi, "Cannot create service push event while shadNet is disabled");
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
        filter->npServiceName = std::string(pNpServiceName);
    }

    filter->npServiceLabel = npServiceLabel;
    LOG_INFO(Lib_NpWebApi, "filterId={} dataTypeParams={}", filterId, filterParamNum);
    if (pFilterParam != nullptr && filterParamNum != 0) {
        for (u64 param_idx = 0; param_idx < filterParamNum; param_idx++) {
            OrbisNpWebApiServicePushEventFilterParameter copy =
                OrbisNpWebApiServicePushEventFilterParameter{};
            memcpy(&copy, &pFilterParam[param_idx],
                   sizeof(OrbisNpWebApiServicePushEventFilterParameter));
            LOG_INFO(Lib_NpWebApi, "  filterParam[{}] data_type='{}'", param_idx,
                     copy.dataType.val);
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

    if (g_sdk_ver >= Common::ElfInfo::FW_250 &&
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

    if (pNpServiceName != nullptr && !EmulatorSettings.IsShadNetEnabled()) {
        // Seems sceNpManagerIntGetUserList fails?
        LOG_DEBUG(Lib_NpWebApi, "Cannot create extended push event while shadNet is disabled");
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
        filter->npServiceName = std::string(pNpServiceName);
    }

    filter->npServiceLabel = npServiceLabel;

    LOG_INFO(Lib_NpWebApi, "filterId={} service='{}' label={:#x} dataTypeParams={}", filterId,
             pNpServiceName ? pNpServiceName : "null", npServiceLabel, filterParamNum);
    if (pFilterParam != nullptr && filterParamNum != 0) {
        for (u64 param_idx = 0; param_idx < filterParamNum; param_idx++) {
            OrbisNpWebApiExtdPushEventFilterParameter copy =
                OrbisNpWebApiExtdPushEventFilterParameter{};
            memcpy(&copy, &pFilterParam[param_idx],
                   sizeof(OrbisNpWebApiExtdPushEventFilterParameter));
            LOG_INFO(Lib_NpWebApi, "  filterParam[{}] dataType='{}' extdKeys={}", param_idx,
                     copy.dataType.val, copy.extdDataKeyNum); // debug
            if (copy.pExtdDataKey != nullptr) {
                for (u64 k = 0; k < copy.extdDataKeyNum; k++) {
                    LOG_INFO(Lib_NpWebApi, "    extdDataKey[{}]='{}'", k,
                             copy.pExtdDataKey[k].val); // debug
                }
            }
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
    return request->http_request_id;
}

// Request/response header support

bool iEquals(const char* a, u64 aLen, const char* b) {
    u64 i = 0;
    for (; i < aLen && b[i]; ++i) {
        if (std::tolower((unsigned char)a[i]) != std::tolower((unsigned char)b[i]))
            return false;
    }
    return i == aLen && b[i] == '\0';
}

// Locate `field`'s value in a raw HTTP response header block (CRLF-separated
// "Name: Value" lines, possibly led by a status line). Returns false if absent.
bool findHeaderValue(const char* block, u64 blockLen, const char* field, std::string& outValue) {
    u64 i = 0;
    while (i < blockLen) {
        u64 lineStart = i;
        while (i < blockLen && block[i] != '\n')
            ++i;
        u64 lineEnd = i; // points at '\n' or end
        if (i < blockLen)
            ++i; // step past '\n'
        // Trim a trailing '\r'.
        if (lineEnd > lineStart && block[lineEnd - 1] == '\r')
            --lineEnd;
        // Split on the first ':'.
        u64 colon = lineStart;
        while (colon < lineEnd && block[colon] != ':')
            ++colon;
        if (colon >= lineEnd)
            continue; // no colon (status line / blank)
        u64 nameLen = colon - lineStart;
        if (!iEquals(block + lineStart, nameLen, field))
            continue;
        u64 vs = colon + 1;
        while (vs < lineEnd && (block[vs] == ' ' || block[vs] == '\t'))
            ++vs;
        outValue.assign(block + vs, lineEnd - vs);
        return true;
    }
    return false;
}

// extraction of the numeric WebApi error code from an error JSON body
// (e.g. {"error":{"code":2240512,...}} or {"code":...}). Returns true on success.
bool parseErrorCode(const char* body, u64 len, s32& out) {
    std::string_view sv(body, len);
    size_t k = sv.find("\"code\"");
    if (k == std::string_view::npos)
        return false;
    k += 6;
    while (k < sv.size() && (sv[k] == ' ' || sv[k] == ':' || sv[k] == '"'))
        ++k;
    bool neg = false;
    if (k < sv.size() && (sv[k] == '-' || sv[k] == '+')) {
        neg = sv[k] == '-';
        ++k;
    }
    if (k >= sv.size() || !std::isdigit((unsigned char)sv[k]))
        return false;
    long long v = 0;
    while (k < sv.size() && std::isdigit((unsigned char)sv[k])) {
        v = v * 10 + (sv[k] - '0');
        ++k;
    }
    out = static_cast<s32>(neg ? -v : v);
    return true;
}

static s32 captureWebApiError(const char* body, u64 len) {
    s32 code = 0;
    if (body != nullptr && len > 0 && parseErrorCode(body, len, code)) {
        g_last_webapi_error = code;
        return code;
    }
    return 0;
}

s32 getLastWebApiError() {
    return g_last_webapi_error;
}

s32 addHttpRequestHeaderInternal(s64 requestId, const char* pFieldName, const char* pValue) {
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
    request->userHeaders.emplace_back(pFieldName, pValue);
    releaseRequest(request);
    releaseUserContext(user_context);
    releaseContext(context);
    return ORBIS_OK;
}

s32 getHttpResponseHeaderValueInternal(s64 requestId, const char* pFieldName, char* pValue,
                                       u64 valueSize, u64* pValueLength) {
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

    char* block = nullptr;
    u64 blockSize = 0;
    const s32 httpReqId = getHttpRequestIdFromRequest(request);
    const s32 err = Libraries::Http::sceHttpGetAllResponseHeaders(httpReqId, &block, &blockSize);

    s32 result = ORBIS_OK;
    if (err < 0) {
        result = err;
    } else {
        std::string value;
        const bool found = block != nullptr && findHeaderValue(block, blockSize, pFieldName, value);
        if (pValueLength != nullptr)
            *pValueLength = found ? value.size() : 0;
        if (pValue != nullptr && valueSize > 0) {
            const u64 n = found ? std::min<u64>(value.size(), valueSize - 1) : 0;
            if (n > 0)
                std::memcpy(pValue, value.data(), n);
            pValue[n] = '\0';
        }
    }

    releaseRequest(request);
    releaseUserContext(user_context);
    releaseContext(context);
    return result;
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

    OrbisNpWebApiRequest* request = findRequestAndMarkBusy(user_context, requestId);
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

    OrbisNpWebApiRequest* request = findRequestAndMarkBusy(user_context, requestId);
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
        // Surface the server error code for sceNpWebApiGetErrorCode() when the body
        // just read belongs to an error response
        if (offset > 0) {
            s32 sc = 0;
            if (Libraries::Http::sceHttpGetStatusCode(getHttpRequestIdFromRequest(request), &sc) >=
                    0 &&
                sc >= 400) {
                captureWebApiError(reinterpret_cast<const char*>(pData), offset);
            }
        }
    }

    if (result > 0) {
        LOG_INFO(Lib_NpWebApi, "reqId={:#x} -> {} bytes: {:.256s}", requestId, result,
                 std::string(reinterpret_cast<const char*>(pData),
                             std::min<u64>(result, 256))); // debug to be removed
    }

    unlockContext(context);

    // Cleanup
    clearRequestEndTime(request);
    releaseRequest(request);
    releaseUserContext(user_context);
    releaseContext(context);

    return result;
}

// --- Push-event delivery -----------------------------------------------------
//
// Mirrors the native lib: notificationCallbackFunc -> FUN_0100e220 (extended) and the
// service dispatch, with matching done by FUN_010049c0. Natively this is driven by the
// NP manager's push listener thread (mnp:usr:npweblis)

using ServiceCb = PS4_SYSV_ABI void (*)(s32, s32, const char*, OrbisNpServiceLabel,
                                        const OrbisNpWebApiPushEventDataType*, const char*, u64,
                                        void*);
struct PushPeerAddress {
    OrbisNpOnlineId onlineId;
    s32 platform;
};
using BasicCb = PS4_SYSV_ABI void (*)(s32, s32, const PushPeerAddress*, const PushPeerAddress*,
                                      const OrbisNpWebApiPushEventDataType*, const char*, u64,
                                      void*);
using ExtdCbA = PS4_SYSV_ABI void (*)(s32, s32, const char*, OrbisNpServiceLabel,
                                      const OrbisNpPeerAddressA*, const OrbisNpOnlineId*,
                                      const OrbisNpPeerAddressA*, const OrbisNpOnlineId*,
                                      const OrbisNpWebApiPushEventDataType*, const char*, u64,
                                      const OrbisNpWebApiExtdPushEventExtdData*, u64, void*);

std::mutex g_push_mutex;
std::deque<PushEventInput> g_push_queue;

template <typename Filter>
bool filterMatches(const Filter* flt, const std::string& evServiceName, bool evHasServiceName,
                   const std::string& evDataType) {
    const bool catch_all = flt->npServiceName.empty() && flt->npServiceLabel == 0xffffffffu;
    if (catch_all) {
        if (evHasServiceName) {
            return false;
        }
    } else if (!evHasServiceName || flt->npServiceName != evServiceName) {
        return false;
    }
    for (const auto& p : flt->filterParams) {
        if (evDataType == p.dataType.val) {
            return true;
        }
    }
    return false;
}

// Network-thread hand-off only,the event is dispatched later from DrainPushEvents.
void EnqueuePushEvent(const PushEventInput& ev) {
    std::scoped_lock lk{g_push_mutex};
    g_push_queue.push_back(ev);
}

// Runs on the game thread from sceNpCheckCallback
void DrainPushEvents() {
    std::deque<PushEventInput> local;
    {
        std::scoped_lock lk{g_push_mutex};
        if (g_push_queue.empty()) {
            return;
        }
        local.swap(g_push_queue);
    }

    for (const PushEventInput& ev : local) {
        const bool ev_has_service = !ev.npServiceName.empty();
        OrbisNpWebApiPushEventDataType dt{};
        std::snprintf(dt.val, sizeof(dt.val), "%s", ev.dataType.c_str());
        const OrbisNpOnlineId* from_p = ev.hasFrom ? &ev.fromOnlineId : nullptr;
        const OrbisNpOnlineId* to_p = ev.hasTo ? &ev.toOnlineId : nullptr;

        std::scoped_lock gl{g_global_mutex};
        for (auto& [libId, context] : g_contexts) {
            if (context == nullptr) {
                continue;
            }
            std::scoped_lock cl{context->contextLock};
            for (auto& [ucKey, uc] : context->userContexts) {
                if (uc == nullptr) {
                    continue;
                }
                if (uc->userId != ev.targetUserId) {
                    continue;
                }
                const s32 title_user_ctx_id = ucKey;

                for (auto& [cbId, cb] : uc->extendedPushEventCallbacks) {
                    if (cb == nullptr) {
                        continue;
                    }
                    void (*raw)() = cb->cbFuncA  ? reinterpret_cast<void (*)()>(cb->cbFuncA)
                                    : cb->cbFunc ? reinterpret_cast<void (*)()>(cb->cbFunc)
                                                 : nullptr;
                    if (raw == nullptr) {
                        continue;
                    }
                    auto fit = context->extendedPushEventFilters.find(cb->filterId);
                    if (fit == context->extendedPushEventFilters.end()) {
                        continue;
                    }
                    const OrbisNpWebApiExtendedPushEventFilter* flt = fit->second;
                    if (!filterMatches(flt, ev.npServiceName, ev_has_service, ev.dataType)) {
                        continue;
                    }
                    std::vector<OrbisNpWebApiExtdPushEventExtdData> exarr;
                    exarr.reserve(ev.extdData.size());
                    for (auto& [k, v] : ev.extdData) {
                        OrbisNpWebApiExtdPushEventExtdData e{};
                        std::snprintf(e.extdDataKey.val, sizeof(e.extdDataKey.val), "%s",
                                      k.c_str());
                        e.pData = const_cast<char*>(v.data());
                        e.dataLen = v.size();
                        exarr.push_back(e);
                    }
                    // Service name/label come from the FILTER
                    const char* svc =
                        flt->npServiceName.empty() ? nullptr : flt->npServiceName.c_str();
                    // pData/pExtdData are NULL (and the counts 0) when the event
                    // carries no payload / no matching extended data.
                    const char* ext_data = ev.data.empty() ? nullptr : ev.data.data();
                    const OrbisNpWebApiExtdPushEventExtdData* ext_arr =
                        exarr.empty() ? nullptr : exarr.data();

                    LOG_INFO(
                        Lib_NpWebApi, "invoking extd cb ctx={:#x} cbId={} dataType='{}'",
                        title_user_ctx_id, cbId,
                        ev.dataType); // debug confirm the listener callback fires. to be removed
                    reinterpret_cast<ExtdCbA>(raw)(
                        title_user_ctx_id, cbId, svc, flt->npServiceLabel, nullptr, to_p, nullptr,
                        from_p, &dt, ext_data, ev.data.size(), ext_arr, exarr.size(), cb->pUserArg);
                }

                // Service push
                for (auto& [cbId, cb] : uc->servicePushEventCallbacks) {
                    if (cb == nullptr || cb->cbFunc == nullptr) {
                        continue;
                    }
                    auto fit = context->servicePushEventFilters.find(cb->filterId);
                    if (fit == context->servicePushEventFilters.end()) {
                        continue;
                    }
                    const OrbisNpWebApiServicePushEventFilter* flt = fit->second;
                    if (!filterMatches(flt, ev.npServiceName, ev_has_service, ev.dataType)) {
                        continue;
                    }
                    const char* svc =
                        flt->npServiceName.empty() ? nullptr : flt->npServiceName.c_str();
                    const char* svc_data = ev.data.empty() ? nullptr : ev.data.data();
                    reinterpret_cast<ServiceCb>(reinterpret_cast<void (*)()>(cb->cbFunc))(
                        title_user_ctx_id, cbId, svc, flt->npServiceLabel, &dt, svc_data,
                        ev.data.size(), cb->pUserArg);
                }

                // Basic push
                if (!ev_has_service) {
                    for (auto& [cbId, cb] : uc->pushEventCallbacks) {
                        if (cb == nullptr || cb->cbFunc == nullptr) {
                            continue;
                        }
                        auto fit = context->pushEventFilters.find(cb->filterId);
                        if (fit == context->pushEventFilters.end()) {
                            continue;
                        }
                        const OrbisNpWebApiPushEventFilter* flt = fit->second;
                        bool matched = false;
                        for (const auto& p : flt->filterParams) {
                            if (ev.dataType == p.dataType.val) {
                                matched = true;
                                break;
                            }
                        }
                        if (!matched) {
                            continue;
                        }
                        LOG_INFO(Lib_NpWebApi,
                                 "invoking basic cb ctx={:#x} cbId={} "
                                 "dataType='{}'",
                                 title_user_ctx_id, cbId,
                                 ev.dataType); // debug confirm the listener callback fires. to be
                                               // removed
                        PushPeerAddress to_peer{};   // notified user (self)
                        PushPeerAddress from_peer{}; // user that caused the event
                        if (ev.hasTo) {
                            to_peer.onlineId = ev.toOnlineId;
                        }
                        if (ev.hasFrom) {
                            from_peer.onlineId = ev.fromOnlineId;
                        }
                        const char* p_data = ev.data.empty() ? nullptr : ev.data.data();
                        reinterpret_cast<BasicCb>(reinterpret_cast<void (*)()>(cb->cbFunc))(
                            title_user_ctx_id, cbId, &to_peer, &from_peer, &dt, p_data,
                            ev.data.size(), cb->pUserArg);
                    }
                }
            }
        }
    }
}
}; // namespace Libraries::Np::NpWebApi
