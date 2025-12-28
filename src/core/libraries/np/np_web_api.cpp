// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/np/np_web_api.h"
#include "core/libraries/np/np_web_api_error.h"
#include "core/libraries/np/np_web_api_internal.h"

namespace Libraries::Np::NpWebApi {

static bool g_is_initialized = false;
static s32 g_active_library_contexts = 0;

s32 PS4_SYSV_ABI sceNpWebApiCreateContext(s32 libCtxId, OrbisNpOnlineId* onlineId) {
    LOG_ERROR(Lib_NpWebApi, "called (STUBBED): libCtxId = {}", libCtxId);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApiCreatePushEventFilter() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApiCreateServicePushEventFilter() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApiDeletePushEventFilter() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApiDeleteServicePushEventFilter() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApiRegisterExtdPushEventCallback() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApiRegisterNotificationCallback() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApiRegisterPushEventCallback() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApiRegisterServicePushEventCallback() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApiUnregisterNotificationCallback() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApiUnregisterPushEventCallback() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApiUnregisterServicePushEventCallback() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApiAbortHandle(s32 libCtxId, s32 handleId) {
    LOG_ERROR(Lib_NpWebApi, "called (STUBBED) : libCtxId = {}, handleId = {}", libCtxId, handleId);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApiAbortRequest(s64 requestId) {
    LOG_ERROR(Lib_NpWebApi, "called (STUBBED) : requestId = {:#x}", requestId);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApiAddHttpRequestHeader(s64 requestId, const char* pFieldName,
                                                 const char* pValue) {
    LOG_ERROR(Lib_NpWebApi,
              "called (STUBBED) : requestId = {:#x}, "
              "pFieldName = '{}', pValue = '{}'",
              requestId, (pFieldName ? pFieldName : "null"), (pValue ? pValue : "null"));
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApiAddMultipartPart(s64 requestId,
                                             const OrbisNpWebApiMultipartPartParameter* pParam,
                                             s32* pIndex) {
    LOG_INFO(Lib_NpWebApi,
             "called (STUBBED) : requestId = {:#x}, "
             "pParam = {}, pIndex = {}",
             requestId, fmt::ptr(pParam), fmt::ptr(pIndex));
    if (pParam) {
        LOG_ERROR(Lib_NpWebApi, "  Part params: headerNum = {}, contentLength = {}",
                  pParam->headerNum, pParam->contentLength);
    }
    return ORBIS_OK;
}

void PS4_SYSV_ABI sceNpWebApiCheckTimeout() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
}

s32 PS4_SYSV_ABI sceNpWebApiClearAllUnusedConnection(s32 userCtxId,
                                                     bool bRemainKeepAliveConnection) {
    LOG_ERROR(Lib_NpWebApi,
              "called (STUBBED) : userCtxId = {}, "
              "bRemainKeepAliveConnection = {}",
              userCtxId, bRemainKeepAliveConnection);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApiClearUnusedConnection(s32 userCtxId, const char* pApiGroup,
                                                  bool bRemainKeepAliveConnection) {
    LOG_ERROR(Lib_NpWebApi,
              "called (STUBBED) : userCtxId = {}, "
              "pApiGroup = '{}', bRemainKeepAliveConnection = {}",
              userCtxId, (pApiGroup ? pApiGroup : "null"), bRemainKeepAliveConnection);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApiCreateContextA(s32 libCtxId,
                                           Libraries::UserService::OrbisUserServiceUserId userId) {
    s32 result;

    result = ORBIS_NP_WEBAPI_ERROR_INVALID_LIB_CONTEXT_ID;

    if ((u32)(libCtxId - 1) < 0x7FFF) {
        if (userId != -1) {
            return createContextForUser(libCtxId, userId);
        }
        result = ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }
    LOG_ERROR(Lib_NpWebApi,
              "sceNpWebApiCreateContextA : "
              "libCtxId = {}, userId = {} error{:#x}",
              libCtxId, userId, result);
    return result;
}

s32 PS4_SYSV_ABI sceNpWebApiCreateExtdPushEventFilter(
    s32 libCtxId, s32 handleId, const char* pNpServiceName, OrbisNpServiceLabel npServiceLabel,
    const OrbisNpWebApiExtdPushEventFilterParameter* pFilterParam, u64 filterParamNum) {
    s32 result;
    // Validate parameters
    if (((pNpServiceName == NULL || npServiceLabel != ORBIS_NP_INVALID_SERVICE_LABEL) &&
         (pFilterParam != NULL)) &&
        (filterParamNum != 0)) {
        result = createExtendedPushEventFilterInternal(
            libCtxId, handleId, pNpServiceName, npServiceLabel, pFilterParam, filterParamNum, 0);
        return result;
    }
    LOG_ERROR(Lib_NpWebApi,
              "sceNpWebApiCreateExtdPushEventFilter invalid argument: "
              "libCtxId = {}, handleId = {}, pNpServiceName = '{}', npServiceLabel = {}, "
              "pFilterParam = {}, filterParamNum = {}",
              libCtxId, handleId, (pNpServiceName ? pNpServiceName : "null"), npServiceLabel,
              fmt::ptr(pFilterParam), filterParamNum);
    return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
}

s32 PS4_SYSV_ABI sceNpWebApiCreateHandle(s32 libCtxId) {
    s32 result;
    OrbisNpWebApiContext* context;

    context = findAndValidateContext(libCtxId, 0);
    if (context == nullptr) {
        result = ORBIS_NP_WEBAPI_ERROR_LIB_CONTEXT_NOT_FOUND;
        LOG_ERROR(Lib_NpWebApi,
                  " sceNpWebApiCreateHandle: "
                  "lib context not found: libCtxId = {}",
                  libCtxId);
    } else {
        result = createHandleInternal(context);
        releaseContext(context);
    }
    return result;
}

s32 PS4_SYSV_ABI sceNpWebApiCreateMultipartRequest(s32 titleUserCtxId, const char* pApiGroup,
                                                   const char* pPath,
                                                   OrbisNpWebApiHttpMethod method,
                                                   s64* pRequestId) {
    LOG_ERROR(Lib_NpWebApi,
              "called (STUBBED) : titleUserCtxId = {}, "
              "pApiGroup = '{}', pPath = '{}', method = {}, pRequestId = {}",
              titleUserCtxId, (pApiGroup ? pApiGroup : "null"), (pPath ? pPath : "null"),
              (s32)method, fmt::ptr(pRequestId));
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApiCreateRequest(s32 titleUserCtxId, const char* pApiGroup,
                                          const char* pPath, OrbisNpWebApiHttpMethod method,
                                          const OrbisNpWebApiContentParameter* pContentParameter,
                                          s64* pRequestId) {
    LOG_ERROR(
        Lib_NpWebApi,
        "called (STUBBED) : titleUserCtxId = {}, "
        "pApiGroup = '{}', pPath = '{}', method = {}, pContentParameter = {}, pRequestId = {}",
        titleUserCtxId, (pApiGroup ? pApiGroup : "null"), (pPath ? pPath : "null"), (s32)method,
        fmt::ptr(pContentParameter), fmt::ptr(pRequestId));
    if (pContentParameter) {
        LOG_ERROR(Lib_NpWebApi, "  Content params: contentLength = {}, pContentType = '{}'",
                  pContentParameter->contentLength,
                  (pContentParameter->pContentType ? pContentParameter->pContentType : "null"));
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApiDeleteContext(s32 titleUserCtxId) {
    LOG_ERROR(Lib_NpWebApi, "called (STUBBED) : titleUserCtxId = {}", titleUserCtxId);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApiDeleteExtdPushEventFilter(s32 libCtxId, s32 filterId) {
    LOG_ERROR(Lib_NpWebApi, "called (STUBBED) : libCtxId = {}, filterId = {}", libCtxId, filterId);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApiDeleteHandle(s32 libCtxId, s32 handleId) {
    LOG_ERROR(Lib_NpWebApi, "called (STUBBED) : libCtxId = {}, handleId = {}", libCtxId, handleId);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApiDeleteRequest(s64 requestId) {
    LOG_ERROR(Lib_NpWebApi, "called (STUBBED) : requestId = {:#x}", requestId);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApiGetConnectionStats(s32 userCtxId, const char* pApiGroup,
                                               OrbisNpWebApiConnectionStats* pStats) {
    LOG_ERROR(Lib_NpWebApi,
              "called (STUBBED) : userCtxId = {}, "
              "pApiGroup = '{}', pStats = {}",
              userCtxId, (pApiGroup ? pApiGroup : "null"), fmt::ptr(pStats));
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApiGetErrorCode() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApiGetHttpResponseHeaderValue(s64 requestId, const char* pFieldName,
                                                       char* pValue, u64 valueSize) {
    LOG_ERROR(Lib_NpWebApi,
              "called (STUBBED) : requestId = {:#x}, "
              "pFieldName = '{}', pValue = {}, valueSize = {}",
              requestId, (pFieldName ? pFieldName : "null"), fmt::ptr(pValue), valueSize);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApiGetHttpResponseHeaderValueLength(s64 requestId, const char* pFieldName,
                                                             u64* pValueLength) {
    LOG_ERROR(Lib_NpWebApi,
              "called (STUBBED) : requestId = {:#x}, "
              "pFieldName = '{}', pValueLength = {}",
              requestId, (pFieldName ? pFieldName : "null"), fmt::ptr(pValueLength));
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApiGetHttpStatusCode() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApiGetMemoryPoolStats(s32 libCtxId,
                                               OrbisNpWebApiMemoryPoolStats* pCurrentStat) {
    LOG_ERROR(Lib_NpWebApi, "called (STUBBED) : libCtxId = {}, pCurrentStat = {}", libCtxId,
              fmt::ptr(pCurrentStat));
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApiInitialize(s32 libHttpCtxId, u64 poolSize) {
    LOG_INFO(Lib_NpWebApi, "called libHttpCtxId = {}, poolSize = {} bytes", libHttpCtxId, poolSize);
    g_is_initialized = true;
    s32 result = createLibraryContext(libHttpCtxId, poolSize, nullptr, 0);
    if (result >= ORBIS_OK) {
        g_active_library_contexts++;
    }
    return result;
}

s32 PS4_SYSV_ABI sceNpWebApiInitializeForPresence(s32 libHttpCtxId, u64 poolSize) {
    LOG_INFO(Lib_NpWebApi, "called libHttpCtxId = {}, poolSize = {} bytes", libHttpCtxId, poolSize);
    g_is_initialized = true;
    s32 result = createLibraryContext(libHttpCtxId, poolSize, nullptr, 3);
    if (result >= ORBIS_OK) {
        g_active_library_contexts++;
    }
    return result;
}

s32 PS4_SYSV_ABI sceNpWebApiIntCreateCtxIndExtdPushEventFilter() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApiIntCreateRequest() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApiIntCreateServicePushEventFilter() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApiIntInitialize(const OrbisNpWebApiIntInitializeArgs* args) {
    LOG_INFO(Lib_NpWebApi, "called");
    if (args == nullptr || args->structSize != sizeof(OrbisNpWebApiIntInitializeArgs)) {
        return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
    }
    g_is_initialized = true;
    s32 result = createLibraryContext(args->libHttpCtxId, args->poolSize, args->name, 2);
    if (result >= ORBIS_OK) {
        g_active_library_contexts++;
    }
    return result;
}

s32 PS4_SYSV_ABI sceNpWebApiIntRegisterServicePushEventCallback() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApiIntRegisterServicePushEventCallbackA() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApiReadData(s64 requestId, void* pData, u64 size) {
    LOG_ERROR(Lib_NpWebApi, "called (STUBBED) : requestId = {:#x}, pData = {}, size = {}",
              requestId, fmt::ptr(pData), size);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApiRegisterExtdPushEventCallbackA(
    s32 userCtxId, s32 filterId, OrbisNpWebApiExtdPushEventCallbackA cbFunc, void* pUserArg) {
    if (cbFunc != nullptr) {
        return registerExtdPushEventCallbackInternalA(userCtxId, filterId, cbFunc, pUserArg);
    }
    LOG_ERROR(Lib_NpWebApi,
              "sceNpWebApiRegisterExtdPushEventCallbackA invalid argument: "
              "userCtxId = {}, filterId = {}, cbFunc = {}, pUserArg = {}",
              userCtxId, filterId, fmt::ptr(cbFunc), fmt::ptr(pUserArg));
    return ORBIS_NP_WEBAPI_ERROR_INVALID_ARGUMENT;
}

s32 PS4_SYSV_ABI sceNpWebApiSendMultipartRequest() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
sceNpWebApiSendMultipartRequest2(s64 requestId, s32 partIndex, const void* pData, u64 dataSize,
                                 OrbisNpWebApiResponseInformationOption* pRespInfoOption) {
    LOG_ERROR(Lib_NpWebApi,
              "called (STUBBED) : requestId = {:#x}, "
              "partIndex = {}, pData = {}, dataSize = {}, pRespInfoOption = {}",
              requestId, partIndex, fmt::ptr(pData), dataSize, fmt::ptr(pRespInfoOption));
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApiSendRequest() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApiSendRequest2(s64 requestId, const void* pData, u64 dataSize,
                                         OrbisNpWebApiResponseInformationOption* pRespInfoOption) {
    LOG_ERROR(Lib_NpWebApi,
              "called (STUBBED) : requestId = {:#x}, "
              "pData = {}, dataSize = {}, pRespInfoOption = {}",
              requestId, fmt::ptr(pData), dataSize, fmt::ptr(pRespInfoOption));
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApiSetHandleTimeout(s32 libCtxId, s32 handleId, u32 timeout) {
    LOG_ERROR(Lib_NpWebApi, "called (STUBBED) : libCtxId = {}, handleId = {}, timeout = {} ms",
              libCtxId, handleId, timeout);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApiSetMaxConnection(s32 libCtxId, s32 maxConnection) {
    LOG_ERROR(Lib_NpWebApi, "called (STUBBED) : libCtxId = {}, maxConnection = {}", libCtxId,
              maxConnection);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApiSetMultipartContentType(s64 requestId, const char* pTypeName,
                                                    const char* pBoundary) {
    LOG_ERROR(Lib_NpWebApi,
              "called (STUBBED) : requestId = {:#x}, "
              "pTypeName = '{}', pBoundary = '{}'",
              requestId, (pTypeName ? pTypeName : "null"), (pBoundary ? pBoundary : "null"));
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApiSetRequestTimeout(s64 requestId, u32 timeout) {
    LOG_ERROR(Lib_NpWebApi, "called (STUBBED) : requestId = {:#x}, timeout = {} ms", requestId,
              timeout);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApiTerminate(s32 libCtxId) {
    LOG_INFO(Lib_NpWebApi, "called libCtxId = {}", libCtxId);
    s32 result = terminateContext(libCtxId);
    if (result != ORBIS_OK) {
        return result;
    }

    g_active_library_contexts--;
    if (g_active_library_contexts == 0) {
        g_is_initialized = false;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApiUnregisterExtdPushEventCallback(s32 userCtxId, s32 callbackId) {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApiUtilityParseNpId(const char* pJsonNpId,
                                             Libraries::Np::OrbisNpId* pNpId) {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApiVshInitialize() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_064C4ED1EDBEB9E8() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_0783955D4E9563DA() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_1A6D77F3FD8323A8() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_1E0693A26FE0F954() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_24A9B5F1D77000CF() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_24AAA6F50E4C2361() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_24D8853D6B47FC79() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_279B3E9C7C4A9DC5() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_28461E29E9F8D697() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_3C29624704FAB9E0() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_3F027804ED2EC11E() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_4066C94E782997CD() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_47C85356815DBE90() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_4FCE8065437E3B87() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_536280BE3DABB521() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_57A0E1BC724219F3() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_5819749C040B6637() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_6198D0C825E86319() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_61F2B9E8AB093743() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_6BC388E6113F0D44() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_7500F0C4F8DC2D16() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_75A03814C7E9039F() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_789D6026C521416E() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_7DED63D06399EFFF() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_7E55A2DCC03D395A() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_7E6C8F9FB86967F4() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_7F04B7D4A7D41E80() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_8E167252DFA5C957() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_95D0046E504E3B09() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_97284BFDA4F18FDF() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_99E32C1F4737EAB4() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_9CFF661EA0BCBF83() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_9EB0E1F467AC3B29() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_A2318FE6FBABFAA3() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_BA07A2E1BF7B3971() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_BD0803EEE0CC29A0() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_BE6F4E5524BB135F() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_C0D490EB481EA4D0() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_C175D392CA6D084A() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_CD0136AF165D2F2F() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_D1C0ADB7B52FEAB5() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_E324765D18EE4D12() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_E789F980D907B653() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_F9A32E8685627436() {
    LOG_ERROR(Lib_NpWebApi, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("x1Y7yiYSk7c", "libSceNpWebApiCompat", 1, "libSceNpWebApi",
                 sceNpWebApiCreateContext);
    LIB_FUNCTION("y5Ta5JCzQHY", "libSceNpWebApiCompat", 1, "libSceNpWebApi",
                 sceNpWebApiCreatePushEventFilter);
    LIB_FUNCTION("sIFx734+xys", "libSceNpWebApiCompat", 1, "libSceNpWebApi",
                 sceNpWebApiCreateServicePushEventFilter);
    LIB_FUNCTION("zE+R6Rcx3W0", "libSceNpWebApiCompat", 1, "libSceNpWebApi",
                 sceNpWebApiDeletePushEventFilter);
    LIB_FUNCTION("PfQ+f6ws764", "libSceNpWebApiCompat", 1, "libSceNpWebApi",
                 sceNpWebApiDeleteServicePushEventFilter);
    LIB_FUNCTION("vrM02A5Gy1M", "libSceNpWebApiCompat", 1, "libSceNpWebApi",
                 sceNpWebApiRegisterExtdPushEventCallback);
    LIB_FUNCTION("HVgWmGIOKdk", "libSceNpWebApiCompat", 1, "libSceNpWebApi",
                 sceNpWebApiRegisterNotificationCallback);
    LIB_FUNCTION("PfSTDCgNMgc", "libSceNpWebApiCompat", 1, "libSceNpWebApi",
                 sceNpWebApiRegisterPushEventCallback);
    LIB_FUNCTION("kJQJE0uKm5w", "libSceNpWebApiCompat", 1, "libSceNpWebApi",
                 sceNpWebApiRegisterServicePushEventCallback);
    LIB_FUNCTION("wjYEvo4xbcA", "libSceNpWebApiCompat", 1, "libSceNpWebApi",
                 sceNpWebApiUnregisterNotificationCallback);
    LIB_FUNCTION("qK4o2656W4w", "libSceNpWebApiCompat", 1, "libSceNpWebApi",
                 sceNpWebApiUnregisterPushEventCallback);
    LIB_FUNCTION("2edrkr0c-wg", "libSceNpWebApiCompat", 1, "libSceNpWebApi",
                 sceNpWebApiUnregisterServicePushEventCallback);
    LIB_FUNCTION("WKcm4PeyJww", "libSceNpWebApi", 1, "libSceNpWebApi", sceNpWebApiAbortHandle);
    LIB_FUNCTION("JzhYTP2fG18", "libSceNpWebApi", 1, "libSceNpWebApi", sceNpWebApiAbortRequest);
    LIB_FUNCTION("joRjtRXTFoc", "libSceNpWebApi", 1, "libSceNpWebApi",
                 sceNpWebApiAddHttpRequestHeader);
    LIB_FUNCTION("19KgfJXgM+U", "libSceNpWebApi", 1, "libSceNpWebApi", sceNpWebApiAddMultipartPart);
    LIB_FUNCTION("gVNNyxf-1Sg", "libSceNpWebApi", 1, "libSceNpWebApi", sceNpWebApiCheckTimeout);
    LIB_FUNCTION("KQIkDGf80PQ", "libSceNpWebApi", 1, "libSceNpWebApi",
                 sceNpWebApiClearAllUnusedConnection);
    LIB_FUNCTION("f-pgaNSd1zc", "libSceNpWebApi", 1, "libSceNpWebApi",
                 sceNpWebApiClearUnusedConnection);
    LIB_FUNCTION("x1Y7yiYSk7c", "libSceNpWebApi", 1, "libSceNpWebApi", sceNpWebApiCreateContext);
    LIB_FUNCTION("zk6c65xoyO0", "libSceNpWebApi", 1, "libSceNpWebApi", sceNpWebApiCreateContextA);
    LIB_FUNCTION("M2BUB+DNEGE", "libSceNpWebApi", 1, "libSceNpWebApi",
                 sceNpWebApiCreateExtdPushEventFilter);
    LIB_FUNCTION("79M-JqvvGo0", "libSceNpWebApi", 1, "libSceNpWebApi", sceNpWebApiCreateHandle);
    LIB_FUNCTION("KBxgeNpoRIQ", "libSceNpWebApi", 1, "libSceNpWebApi",
                 sceNpWebApiCreateMultipartRequest);
    LIB_FUNCTION("y5Ta5JCzQHY", "libSceNpWebApi", 1, "libSceNpWebApi",
                 sceNpWebApiCreatePushEventFilter);
    LIB_FUNCTION("rdgs5Z1MyFw", "libSceNpWebApi", 1, "libSceNpWebApi", sceNpWebApiCreateRequest);
    LIB_FUNCTION("sIFx734+xys", "libSceNpWebApi", 1, "libSceNpWebApi",
                 sceNpWebApiCreateServicePushEventFilter);
    LIB_FUNCTION("XUjdsSTTZ3U", "libSceNpWebApi", 1, "libSceNpWebApi", sceNpWebApiDeleteContext);
    LIB_FUNCTION("pfaJtb7SQ80", "libSceNpWebApi", 1, "libSceNpWebApi",
                 sceNpWebApiDeleteExtdPushEventFilter);
    LIB_FUNCTION("5Mn7TYwpl30", "libSceNpWebApi", 1, "libSceNpWebApi", sceNpWebApiDeleteHandle);
    LIB_FUNCTION("zE+R6Rcx3W0", "libSceNpWebApi", 1, "libSceNpWebApi",
                 sceNpWebApiDeletePushEventFilter);
    LIB_FUNCTION("noQgleu+KLE", "libSceNpWebApi", 1, "libSceNpWebApi", sceNpWebApiDeleteRequest);
    LIB_FUNCTION("PfQ+f6ws764", "libSceNpWebApi", 1, "libSceNpWebApi",
                 sceNpWebApiDeleteServicePushEventFilter);
    LIB_FUNCTION("UJ8H+7kVQUE", "libSceNpWebApi", 1, "libSceNpWebApi",
                 sceNpWebApiGetConnectionStats);
    LIB_FUNCTION("2qSZ0DgwTsc", "libSceNpWebApi", 1, "libSceNpWebApi", sceNpWebApiGetErrorCode);
    LIB_FUNCTION("VwJ5L0Higg0", "libSceNpWebApi", 1, "libSceNpWebApi",
                 sceNpWebApiGetHttpResponseHeaderValue);
    LIB_FUNCTION("743ZzEBzlV8", "libSceNpWebApi", 1, "libSceNpWebApi",
                 sceNpWebApiGetHttpResponseHeaderValueLength);
    LIB_FUNCTION("k210oKgP80Y", "libSceNpWebApi", 1, "libSceNpWebApi",
                 sceNpWebApiGetHttpStatusCode);
    LIB_FUNCTION("3OnubUs02UM", "libSceNpWebApi", 1, "libSceNpWebApi",
                 sceNpWebApiGetMemoryPoolStats);
    LIB_FUNCTION("G3AnLNdRBjE", "libSceNpWebApi", 1, "libSceNpWebApi", sceNpWebApiInitialize);
    LIB_FUNCTION("FkuwsD64zoQ", "libSceNpWebApi", 1, "libSceNpWebApi",
                 sceNpWebApiInitializeForPresence);
    LIB_FUNCTION("c1pKoztonB8", "libSceNpWebApi", 1, "libSceNpWebApi",
                 sceNpWebApiIntCreateCtxIndExtdPushEventFilter);
    LIB_FUNCTION("N2Jbx4tIaQ4", "libSceNpWebApi", 1, "libSceNpWebApi", sceNpWebApiIntCreateRequest);
    LIB_FUNCTION("TZSep4xB4EY", "libSceNpWebApi", 1, "libSceNpWebApi",
                 sceNpWebApiIntCreateServicePushEventFilter);
    LIB_FUNCTION("8Vjplhyyc44", "libSceNpWebApi", 1, "libSceNpWebApi", sceNpWebApiIntInitialize);
    LIB_FUNCTION("VjVukb2EWPc", "libSceNpWebApi", 1, "libSceNpWebApi",
                 sceNpWebApiIntRegisterServicePushEventCallback);
    LIB_FUNCTION("sfq23ZVHVEw", "libSceNpWebApi", 1, "libSceNpWebApi",
                 sceNpWebApiIntRegisterServicePushEventCallbackA);
    LIB_FUNCTION("CQtPRSF6Ds8", "libSceNpWebApi", 1, "libSceNpWebApi", sceNpWebApiReadData);
    LIB_FUNCTION("vrM02A5Gy1M", "libSceNpWebApi", 1, "libSceNpWebApi",
                 sceNpWebApiRegisterExtdPushEventCallback);
    LIB_FUNCTION("jhXKGQJ4egI", "libSceNpWebApi", 1, "libSceNpWebApi",
                 sceNpWebApiRegisterExtdPushEventCallbackA);
    LIB_FUNCTION("HVgWmGIOKdk", "libSceNpWebApi", 1, "libSceNpWebApi",
                 sceNpWebApiRegisterNotificationCallback);
    LIB_FUNCTION("PfSTDCgNMgc", "libSceNpWebApi", 1, "libSceNpWebApi",
                 sceNpWebApiRegisterPushEventCallback);
    LIB_FUNCTION("kJQJE0uKm5w", "libSceNpWebApi", 1, "libSceNpWebApi",
                 sceNpWebApiRegisterServicePushEventCallback);
    LIB_FUNCTION("KCItz6QkeGs", "libSceNpWebApi", 1, "libSceNpWebApi",
                 sceNpWebApiSendMultipartRequest);
    LIB_FUNCTION("DsPOTEvSe7M", "libSceNpWebApi", 1, "libSceNpWebApi",
                 sceNpWebApiSendMultipartRequest2);
    LIB_FUNCTION("kVbL4hL3K7w", "libSceNpWebApi", 1, "libSceNpWebApi", sceNpWebApiSendRequest);
    LIB_FUNCTION("KjNeZ-29ysQ", "libSceNpWebApi", 1, "libSceNpWebApi", sceNpWebApiSendRequest2);
    LIB_FUNCTION("6g6q-g1i4XU", "libSceNpWebApi", 1, "libSceNpWebApi", sceNpWebApiSetHandleTimeout);
    LIB_FUNCTION("gRiilVCvfAI", "libSceNpWebApi", 1, "libSceNpWebApi", sceNpWebApiSetMaxConnection);
    LIB_FUNCTION("i0dr6grIZyc", "libSceNpWebApi", 1, "libSceNpWebApi",
                 sceNpWebApiSetMultipartContentType);
    LIB_FUNCTION("qWcbJkBj1Lg", "libSceNpWebApi", 1, "libSceNpWebApi",
                 sceNpWebApiSetRequestTimeout);
    LIB_FUNCTION("asz3TtIqGF8", "libSceNpWebApi", 1, "libSceNpWebApi", sceNpWebApiTerminate);
    LIB_FUNCTION("PqCY25FMzPs", "libSceNpWebApi", 1, "libSceNpWebApi",
                 sceNpWebApiUnregisterExtdPushEventCallback);
    LIB_FUNCTION("wjYEvo4xbcA", "libSceNpWebApi", 1, "libSceNpWebApi",
                 sceNpWebApiUnregisterNotificationCallback);
    LIB_FUNCTION("qK4o2656W4w", "libSceNpWebApi", 1, "libSceNpWebApi",
                 sceNpWebApiUnregisterPushEventCallback);
    LIB_FUNCTION("2edrkr0c-wg", "libSceNpWebApi", 1, "libSceNpWebApi",
                 sceNpWebApiUnregisterServicePushEventCallback);
    LIB_FUNCTION("or0e885BlXo", "libSceNpWebApi", 1, "libSceNpWebApi", sceNpWebApiUtilityParseNpId);
    LIB_FUNCTION("uRsskUhAfnM", "libSceNpWebApi", 1, "libSceNpWebApi", sceNpWebApiVshInitialize);
    LIB_FUNCTION("BkxO0e2+ueg", "libSceNpWebApi", 1, "libSceNpWebApi", Func_064C4ED1EDBEB9E8);
    LIB_FUNCTION("B4OVXU6VY9o", "libSceNpWebApi", 1, "libSceNpWebApi", Func_0783955D4E9563DA);
    LIB_FUNCTION("Gm138-2DI6g", "libSceNpWebApi", 1, "libSceNpWebApi", Func_1A6D77F3FD8323A8);
    LIB_FUNCTION("HgaTom-g+VQ", "libSceNpWebApi", 1, "libSceNpWebApi", Func_1E0693A26FE0F954);
    LIB_FUNCTION("JKm18ddwAM8", "libSceNpWebApi", 1, "libSceNpWebApi", Func_24A9B5F1D77000CF);
    LIB_FUNCTION("JKqm9Q5MI2E", "libSceNpWebApi", 1, "libSceNpWebApi", Func_24AAA6F50E4C2361);
    LIB_FUNCTION("JNiFPWtH-Hk", "libSceNpWebApi", 1, "libSceNpWebApi", Func_24D8853D6B47FC79);
    LIB_FUNCTION("J5s+nHxKncU", "libSceNpWebApi", 1, "libSceNpWebApi", Func_279B3E9C7C4A9DC5);
    LIB_FUNCTION("KEYeKen41pc", "libSceNpWebApi", 1, "libSceNpWebApi", Func_28461E29E9F8D697);
    LIB_FUNCTION("PCliRwT6ueA", "libSceNpWebApi", 1, "libSceNpWebApi", Func_3C29624704FAB9E0);
    LIB_FUNCTION("PwJ4BO0uwR4", "libSceNpWebApi", 1, "libSceNpWebApi", Func_3F027804ED2EC11E);
    LIB_FUNCTION("QGbJTngpl80", "libSceNpWebApi", 1, "libSceNpWebApi", Func_4066C94E782997CD);
    LIB_FUNCTION("R8hTVoFdvpA", "libSceNpWebApi", 1, "libSceNpWebApi", Func_47C85356815DBE90);
    LIB_FUNCTION("T86AZUN+O4c", "libSceNpWebApi", 1, "libSceNpWebApi", Func_4FCE8065437E3B87);
    LIB_FUNCTION("U2KAvj2rtSE", "libSceNpWebApi", 1, "libSceNpWebApi", Func_536280BE3DABB521);
    LIB_FUNCTION("V6DhvHJCGfM", "libSceNpWebApi", 1, "libSceNpWebApi", Func_57A0E1BC724219F3);
    LIB_FUNCTION("WBl0nAQLZjc", "libSceNpWebApi", 1, "libSceNpWebApi", Func_5819749C040B6637);
    LIB_FUNCTION("YZjQyCXoYxk", "libSceNpWebApi", 1, "libSceNpWebApi", Func_6198D0C825E86319);
    LIB_FUNCTION("YfK56KsJN0M", "libSceNpWebApi", 1, "libSceNpWebApi", Func_61F2B9E8AB093743);
    LIB_FUNCTION("a8OI5hE-DUQ", "libSceNpWebApi", 1, "libSceNpWebApi", Func_6BC388E6113F0D44);
    LIB_FUNCTION("dQDwxPjcLRY", "libSceNpWebApi", 1, "libSceNpWebApi", Func_7500F0C4F8DC2D16);
    LIB_FUNCTION("daA4FMfpA58", "libSceNpWebApi", 1, "libSceNpWebApi", Func_75A03814C7E9039F);
    LIB_FUNCTION("eJ1gJsUhQW4", "libSceNpWebApi", 1, "libSceNpWebApi", Func_789D6026C521416E);
    LIB_FUNCTION("fe1j0GOZ7-8", "libSceNpWebApi", 1, "libSceNpWebApi", Func_7DED63D06399EFFF);
    LIB_FUNCTION("flWi3MA9OVo", "libSceNpWebApi", 1, "libSceNpWebApi", Func_7E55A2DCC03D395A);
    LIB_FUNCTION("fmyPn7hpZ-Q", "libSceNpWebApi", 1, "libSceNpWebApi", Func_7E6C8F9FB86967F4);
    LIB_FUNCTION("fwS31KfUHoA", "libSceNpWebApi", 1, "libSceNpWebApi", Func_7F04B7D4A7D41E80);
    LIB_FUNCTION("jhZyUt+lyVc", "libSceNpWebApi", 1, "libSceNpWebApi", Func_8E167252DFA5C957);
    LIB_FUNCTION("ldAEblBOOwk", "libSceNpWebApi", 1, "libSceNpWebApi", Func_95D0046E504E3B09);
    LIB_FUNCTION("lyhL-aTxj98", "libSceNpWebApi", 1, "libSceNpWebApi", Func_97284BFDA4F18FDF);
    LIB_FUNCTION("meMsH0c36rQ", "libSceNpWebApi", 1, "libSceNpWebApi", Func_99E32C1F4737EAB4);
    LIB_FUNCTION("nP9mHqC8v4M", "libSceNpWebApi", 1, "libSceNpWebApi", Func_9CFF661EA0BCBF83);
    LIB_FUNCTION("nrDh9GesOyk", "libSceNpWebApi", 1, "libSceNpWebApi", Func_9EB0E1F467AC3B29);
    LIB_FUNCTION("ojGP5vur+qM", "libSceNpWebApi", 1, "libSceNpWebApi", Func_A2318FE6FBABFAA3);
    LIB_FUNCTION("ugei4b97OXE", "libSceNpWebApi", 1, "libSceNpWebApi", Func_BA07A2E1BF7B3971);
    LIB_FUNCTION("vQgD7uDMKaA", "libSceNpWebApi", 1, "libSceNpWebApi", Func_BD0803EEE0CC29A0);
    LIB_FUNCTION("vm9OVSS7E18", "libSceNpWebApi", 1, "libSceNpWebApi", Func_BE6F4E5524BB135F);
    LIB_FUNCTION("wNSQ60gepNA", "libSceNpWebApi", 1, "libSceNpWebApi", Func_C0D490EB481EA4D0);
    LIB_FUNCTION("wXXTksptCEo", "libSceNpWebApi", 1, "libSceNpWebApi", Func_C175D392CA6D084A);
    LIB_FUNCTION("zQE2rxZdLy8", "libSceNpWebApi", 1, "libSceNpWebApi", Func_CD0136AF165D2F2F);
    LIB_FUNCTION("0cCtt7Uv6rU", "libSceNpWebApi", 1, "libSceNpWebApi", Func_D1C0ADB7B52FEAB5);
    LIB_FUNCTION("4yR2XRjuTRI", "libSceNpWebApi", 1, "libSceNpWebApi", Func_E324765D18EE4D12);
    LIB_FUNCTION("54n5gNkHtlM", "libSceNpWebApi", 1, "libSceNpWebApi", Func_E789F980D907B653);
    LIB_FUNCTION("+aMuhoVidDY", "libSceNpWebApi", 1, "libSceNpWebApi", Func_F9A32E8685627436);
};

} // namespace Libraries::Np::NpWebApi