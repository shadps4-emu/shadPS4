// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
#include <map>
#include "core/libraries/kernel/time.h"
#include "np_web_api_internal.h"

namespace Libraries::Np::NpWebApi {

static std::mutex g_global_mutex;
static std::map<s32, OrbisNpWebApiContext*> g_contexts;
static s32 g_context_count = 0;

s32 createLibraryContext(s32 libHttpCtxId, u64 poolSize, const char* name, s32 type) {
    std::scoped_lock lk{g_global_mutex};
    s32 ctx_id = 0;
    while (g_contexts.contains(ctx_id)) {
        ctx_id++;
    }
    if (ctx_id >= 0x7fff) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_MAX;
    }

    // Create new context
    g_contexts[ctx_id] = new OrbisNpWebApiContext{};
    auto& new_context = g_contexts.at(ctx_id);
    new_context->libCtxId = ctx_id + 1;
    new_context->libHttpCtxId = libHttpCtxId;
    new_context->type = type;
    new_context->userCount = 0;
    new_context->terminated = false;
    if (name != nullptr) {
        strncpy(new_context->name, name, 0x20);
    }

    g_context_count++;
    return ctx_id + 1;
}

OrbisNpWebApiContext* findAndValidateContext(s32 libCtxId, s32 flag) {
    std::scoped_lock lk{g_global_mutex};
    if (libCtxId < 1 || libCtxId >= 0x8000) {
        return nullptr;
    }
    auto& context = g_contexts[libCtxId - 1];
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
    s32 ctx_id = libCtxId - 1;
    if (!g_contexts.contains(ctx_id)) {
        return ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    g_contexts.erase(ctx_id);
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

s32 createContextForUser(int32_t libCtxId, int32_t userId) {
    LOG_ERROR(Lib_NpWebApi,
              "called (STUBBED) : libCtxId = {}, "
              "userId = {}",
              libCtxId, userId);
    return ORBIS_OK; // TODO: implement
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