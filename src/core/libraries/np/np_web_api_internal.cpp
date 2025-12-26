// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
#include "np_web_api_internal.h"

namespace Libraries::Np::NpWebApi {

s32 createExtendedPushEventFilterInternal(
    s32 libCtxId, s32 handleId, const char* pNpServiceName,
    Libraries::Np::NpCommon::OrbisNpServiceLabel npServiceLabel,
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
                                      Libraries::Np::NpCommon::OrbisNpServiceLabel npServiceLabel,
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
void releaseContext(OrbisNpWebApiContext* context) {
    LOG_ERROR(Lib_NpWebApi, "called (STUBBED)");
    // TODO: implement
}
OrbisNpWebApiContext* findAndValidateContext(int32_t libCtxId, int flag) {
    LOG_ERROR(Lib_NpWebApi,
              "called (STUBBED) : libCtxId = {}, "
              "flag = {}",
              libCtxId, flag);
    return nullptr; // TODO: implement
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