// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
#include "core/libraries/kernel/time.h"
#include "np_web_api_internal.h"

namespace Libraries::Np::NpWebApi {

static std::mutex g_global_mutex;
static std::map<s32, OrbisNpWebApiContext*> g_contexts;
static s32 g_context_count = 0;
static s32 g_user_context_count = 0;

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
    OrbisNpWebApiContext* ctx = findAndValidateContext(libCtxId, 0);
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

OrbisNpWebApiUserContext* findUserContext(OrbisNpWebApiContext* context, s32 userCtxId) {
    std::scoped_lock lk{context->contextLock};
    if (!context->userContexts.contains(userCtxId)) {
        return nullptr;
    }
    OrbisNpWebApiUserContext* user_context = context->userContexts[userCtxId];
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
    OrbisNpWebApiContext* context = findAndValidateContext(libCtxId, 0);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    OrbisNpWebApiUserContext* user_context = findUserContextByUserId(context, userId);
    if (user_context != nullptr) {
        user_context->userCount--;
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

s32 deleteUserContext(s32 userCtxId) {
    LOG_INFO(Lib_NpWebApi, "userCtxId = {:#x}", userCtxId);
    OrbisNpWebApiContext* context = findAndValidateContext(userCtxId >> 0x10, 0);
    if (context == nullptr) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    lockContext(context);
    OrbisNpWebApiUserContext* user_context = findUserContext(context, userCtxId);
    if (user_context == nullptr || user_context->deleted) {
        unlockContext(context);
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

    context->userContexts.erase(userCtxId);
    free(user_context);

    unlockContext(context);
    releaseContext(context);
    g_user_context_count--;

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
    context = findAndValidateContext(libCtxId, 0);
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

s32 registerExtdPushEventCallbackInternalA(s32 userCtxId, s32 filterId,
                                           OrbisNpWebApiExtdPushEventCallbackA cbFunc,
                                           void* pUserArg) {
    return registerExtdPushEventCallbackInternal(userCtxId, filterId, nullptr, cbFunc, pUserArg);
}

s32 registerExtdPushEventCallbackInternal(s32 userCtxId, s32 filterId,
                                          OrbisNpWebApiExtdPushEventCallback cbFunc,
                                          OrbisNpWebApiExtdPushEventCallbackA cbFuncA,
                                          void* pUserArg) {
    LOG_ERROR(Lib_NpWebApi,
              "called (STUBBED) : userCtxId = {}, "
              "filterId = {}, cbFunc = {}, cbFuncA = {}, pUserArg = {}",
              userCtxId, filterId, fmt::ptr(cbFunc), fmt::ptr(cbFuncA), fmt::ptr(pUserArg));
    return ORBIS_OK; // TODO: implement
}

}; // namespace Libraries::Np::NpWebApi