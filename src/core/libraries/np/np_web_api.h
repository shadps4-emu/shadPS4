// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/np/np_common.h"
#include "core/libraries/np/np_types.h"
#include "core/libraries/system/userservice.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Np::NpWebApi {

#define ORBIS_NP_WEBAPI_DEFAULT_CONNECTION_NUM 1
#define ORBIS_NP_WEBAPI_MAX_CONNECTION_NUM 16
#define ORBIS_NP_WEBAPI_PUSH_EVENT_DATA_TYPE_LEN_MAX 64
#define ORBIS_NP_WEBAPI_EXTD_PUSH_EVENT_EXTD_DATA_KEY_LEN_MAX 32

struct OrbisNpWebApiPushEventDataType {
    char val[ORBIS_NP_WEBAPI_PUSH_EVENT_DATA_TYPE_LEN_MAX + 1];
};

struct OrbisNpWebApiExtdPushEventExtdDataKey {
    char val[ORBIS_NP_WEBAPI_EXTD_PUSH_EVENT_EXTD_DATA_KEY_LEN_MAX + 1];
};

struct OrbisNpWebApiExtdPushEventFilterParameter {
    OrbisNpWebApiPushEventDataType dataType;
    OrbisNpWebApiExtdPushEventExtdDataKey* pExtdDataKey;
    u64 extdDataKeyNum;
};

struct SceNpWebApiExtdPushEventExtdData {
    OrbisNpWebApiExtdPushEventExtdDataKey extdDataKey;
    char* pData;
    u64 dataLen;
};

struct OrbisNpWebApiHttpHeader {
    char* pName;
    char* pValue;
};

struct OrbisNpWebApiMultipartPartParameter {
    OrbisNpWebApiHttpHeader* pHeaders;
    u64 headerNum;
    u64 contentLength;
};

enum OrbisNpWebApiHttpMethod {
    ORBIS_NP_WEBAPI_HTTP_METHOD_GET,
    ORBIS_NP_WEBAPI_HTTP_METHOD_POST,
    ORBIS_NP_WEBAPI_HTTP_METHOD_PUT,
    ORBIS_NP_WEBAPI_HTTP_METHOD_DELETE
};

struct OrbisNpWebApiContentParameter {
    u64 contentLength;
    const char* pContentType;
    u8 reserved[16];
};

struct OrbisNpWebApiResponseInformationOption {
    s32 httpStatus;
    char* pErrorObject;
    u64 errorObjectSize;
    u64 responseDataSize;
};

struct OrbisNpWebApiMemoryPoolStats {
    u64 poolSize;
    u64 maxInuseSize;
    u64 currentInuseSize;
    s32 reserved;
};

struct OrbisNpWebApiConnectionStats {
    u32 max;
    u32 used;
    u32 unused;
    u32 keepAlive;
    u64 reserved;
};

using OrbisNpWebApiExtdPushEventCallbackA = PS4_SYSV_ABI void (*)(
    s32 userCtxId, s32 callbackId, const char* pNpServiceName,
    Libraries::Np::NpCommon::OrbisNpServiceLabel npServiceLabel,
    const Libraries::Np::NpCommon::OrbisNpPeerAddressA* pTo,
    const Libraries::Np::OrbisNpOnlineId* pToOnlineId,
    const Libraries::Np::NpCommon::OrbisNpPeerAddressA* pFrom,
    const Libraries::Np::OrbisNpOnlineId* pFromOnlineId,
    const OrbisNpWebApiPushEventDataType* pDataType, const char* pData, size_t dataLen,
    const SceNpWebApiExtdPushEventExtdData* pExtdData, size_t extdDataNum, void* pUserArg);

s32 PS4_SYSV_ABI sceNpWebApiCreateContext(s32 libCtxId,
                                          Libraries::UserService::OrbisUserServiceUserId userId);
s32 PS4_SYSV_ABI sceNpWebApiCreatePushEventFilter();
s32 PS4_SYSV_ABI sceNpWebApiCreateServicePushEventFilter();
s32 PS4_SYSV_ABI sceNpWebApiDeletePushEventFilter();
s32 PS4_SYSV_ABI sceNpWebApiDeleteServicePushEventFilter();
s32 PS4_SYSV_ABI sceNpWebApiRegisterExtdPushEventCallback();
s32 PS4_SYSV_ABI sceNpWebApiRegisterNotificationCallback();
s32 PS4_SYSV_ABI sceNpWebApiRegisterPushEventCallback();
s32 PS4_SYSV_ABI sceNpWebApiRegisterServicePushEventCallback();
s32 PS4_SYSV_ABI sceNpWebApiUnregisterNotificationCallback();
s32 PS4_SYSV_ABI sceNpWebApiUnregisterPushEventCallback();
s32 PS4_SYSV_ABI sceNpWebApiUnregisterServicePushEventCallback();
s32 PS4_SYSV_ABI sceNpWebApiAbortHandle(s32 libCtxId, s32 handleId);
s32 PS4_SYSV_ABI sceNpWebApiAbortRequest(s64 requestId);
s32 PS4_SYSV_ABI sceNpWebApiAddHttpRequestHeader(s64 requestId, const char* pFieldName,
                                                 const char* pValue);
s32 PS4_SYSV_ABI sceNpWebApiAddMultipartPart(s64 requestId,
                                             const OrbisNpWebApiMultipartPartParameter* pParam,
                                             s32* pIndex);
void PS4_SYSV_ABI sceNpWebApiCheckTimeout();
s32 PS4_SYSV_ABI sceNpWebApiClearAllUnusedConnection(s32 userCtxId,
                                                     bool bRemainKeepAliveConnection);
s32 PS4_SYSV_ABI sceNpWebApiClearUnusedConnection(s32 userCtxId, const char* pApiGroup,
                                                  bool bRemainKeepAliveConnection);
s32 PS4_SYSV_ABI sceNpWebApiCreateContextA(s32 libCtxId,
                                           Libraries::UserService::OrbisUserServiceUserId userId);
s32 PS4_SYSV_ABI sceNpWebApiCreateExtdPushEventFilter(
    s32 libCtxId, s32 handleId, const char* pNpServiceName,
    Libraries::Np::NpCommon::OrbisNpServiceLabel npServiceLabel,
    const OrbisNpWebApiExtdPushEventFilterParameter* pFilterParam, u64 filterParamNum);
s32 PS4_SYSV_ABI sceNpWebApiCreateHandle(s32 libCtxId);
s32 PS4_SYSV_ABI sceNpWebApiCreateMultipartRequest(s32 titleUserCtxId, const char* pApiGroup,
                                                   const char* pPath,
                                                   OrbisNpWebApiHttpMethod method, s64* pRequestId);
s32 PS4_SYSV_ABI sceNpWebApiCreateRequest(s32 titleUserCtxId, const char* pApiGroup,
                                          const char* pPath, OrbisNpWebApiHttpMethod method,
                                          const OrbisNpWebApiContentParameter* pContentParameter,
                                          s64* pRequestId);
s32 PS4_SYSV_ABI sceNpWebApiDeleteContext(s32 titleUserCtxId);
s32 PS4_SYSV_ABI sceNpWebApiDeleteExtdPushEventFilter(s32 libCtxId, s32 filterId);
s32 PS4_SYSV_ABI sceNpWebApiDeleteHandle(s32 libCtxId, s32 handleId);
s32 PS4_SYSV_ABI sceNpWebApiDeleteRequest(s64 requestId);
s32 PS4_SYSV_ABI sceNpWebApiGetConnectionStats(s32 userCtxId, const char* pApiGroup,
                                               OrbisNpWebApiConnectionStats* pStats);
s32 PS4_SYSV_ABI sceNpWebApiGetErrorCode();
s32 PS4_SYSV_ABI sceNpWebApiGetHttpResponseHeaderValue(s64 requestId, const char* pFieldName,
                                                       char* pValue, u64 valueSize);
s32 PS4_SYSV_ABI sceNpWebApiGetHttpResponseHeaderValueLength(s64 requestId, const char* pFieldName,
                                                             u64* pValueLength);
s32 PS4_SYSV_ABI sceNpWebApiGetHttpStatusCode();
s32 PS4_SYSV_ABI sceNpWebApiGetMemoryPoolStats(s32 libCtxId,
                                               OrbisNpWebApiMemoryPoolStats* pCurrentStat);
s32 PS4_SYSV_ABI sceNpWebApiInitialize(int libHttpCtxId, u64 poolSize);
s32 PS4_SYSV_ABI sceNpWebApiInitializeForPresence();
s32 PS4_SYSV_ABI sceNpWebApiIntCreateCtxIndExtdPushEventFilter();
s32 PS4_SYSV_ABI sceNpWebApiIntCreateRequest();
s32 PS4_SYSV_ABI sceNpWebApiIntCreateServicePushEventFilter();
s32 PS4_SYSV_ABI sceNpWebApiIntInitialize();
s32 PS4_SYSV_ABI sceNpWebApiIntRegisterServicePushEventCallback();
s32 PS4_SYSV_ABI sceNpWebApiIntRegisterServicePushEventCallbackA();
s32 PS4_SYSV_ABI sceNpWebApiReadData(s64 requestId, void* pData, u64 size);
s32 PS4_SYSV_ABI sceNpWebApiRegisterExtdPushEventCallbackA(
    s32 userCtxId, s32 filterId, OrbisNpWebApiExtdPushEventCallbackA cbFunc, void* pUserArg);
s32 PS4_SYSV_ABI sceNpWebApiSendMultipartRequest();
s32 PS4_SYSV_ABI
sceNpWebApiSendMultipartRequest2(s64 requestId, s32 partIndex, const void* pData, u64 dataSize,
                                 OrbisNpWebApiResponseInformationOption* pRespInfoOption);
s32 PS4_SYSV_ABI sceNpWebApiSendRequest();
s32 PS4_SYSV_ABI sceNpWebApiSendRequest2(s64 requestId, const void* pData, u64 dataSize,
                                         OrbisNpWebApiResponseInformationOption* pRespInfoOption);
s32 PS4_SYSV_ABI sceNpWebApiSetHandleTimeout(s32 libCtxId, s32 handleId, u32 timeout);
s32 PS4_SYSV_ABI sceNpWebApiSetMaxConnection(s32 libCtxId, s32 maxConnection);
s32 PS4_SYSV_ABI sceNpWebApiSetMultipartContentType(s64 requestId, const char* pTypeName,
                                                    const char* pBoundary);
s32 PS4_SYSV_ABI sceNpWebApiSetRequestTimeout(s64 requestId, u32 timeout);
s32 PS4_SYSV_ABI sceNpWebApiTerminate(s32 libCtxId);
s32 PS4_SYSV_ABI sceNpWebApiUnregisterExtdPushEventCallback(s32 userCtxId, s32 callbackId);
s32 PS4_SYSV_ABI sceNpWebApiUtilityParseNpId(const char* pJsonNpId,
                                             Libraries::Np::OrbisNpId* pNpId);
s32 PS4_SYSV_ABI sceNpWebApiVshInitialize();
s32 PS4_SYSV_ABI Func_064C4ED1EDBEB9E8();
s32 PS4_SYSV_ABI Func_0783955D4E9563DA();
s32 PS4_SYSV_ABI Func_1A6D77F3FD8323A8();
s32 PS4_SYSV_ABI Func_1E0693A26FE0F954();
s32 PS4_SYSV_ABI Func_24A9B5F1D77000CF();
s32 PS4_SYSV_ABI Func_24AAA6F50E4C2361();
s32 PS4_SYSV_ABI Func_24D8853D6B47FC79();
s32 PS4_SYSV_ABI Func_279B3E9C7C4A9DC5();
s32 PS4_SYSV_ABI Func_28461E29E9F8D697();
s32 PS4_SYSV_ABI Func_3C29624704FAB9E0();
s32 PS4_SYSV_ABI Func_3F027804ED2EC11E();
s32 PS4_SYSV_ABI Func_4066C94E782997CD();
s32 PS4_SYSV_ABI Func_47C85356815DBE90();
s32 PS4_SYSV_ABI Func_4FCE8065437E3B87();
s32 PS4_SYSV_ABI Func_536280BE3DABB521();
s32 PS4_SYSV_ABI Func_57A0E1BC724219F3();
s32 PS4_SYSV_ABI Func_5819749C040B6637();
s32 PS4_SYSV_ABI Func_6198D0C825E86319();
s32 PS4_SYSV_ABI Func_61F2B9E8AB093743();
s32 PS4_SYSV_ABI Func_6BC388E6113F0D44();
s32 PS4_SYSV_ABI Func_7500F0C4F8DC2D16();
s32 PS4_SYSV_ABI Func_75A03814C7E9039F();
s32 PS4_SYSV_ABI Func_789D6026C521416E();
s32 PS4_SYSV_ABI Func_7DED63D06399EFFF();
s32 PS4_SYSV_ABI Func_7E55A2DCC03D395A();
s32 PS4_SYSV_ABI Func_7E6C8F9FB86967F4();
s32 PS4_SYSV_ABI Func_7F04B7D4A7D41E80();
s32 PS4_SYSV_ABI Func_8E167252DFA5C957();
s32 PS4_SYSV_ABI Func_95D0046E504E3B09();
s32 PS4_SYSV_ABI Func_97284BFDA4F18FDF();
s32 PS4_SYSV_ABI Func_99E32C1F4737EAB4();
s32 PS4_SYSV_ABI Func_9CFF661EA0BCBF83();
s32 PS4_SYSV_ABI Func_9EB0E1F467AC3B29();
s32 PS4_SYSV_ABI Func_A2318FE6FBABFAA3();
s32 PS4_SYSV_ABI Func_BA07A2E1BF7B3971();
s32 PS4_SYSV_ABI Func_BD0803EEE0CC29A0();
s32 PS4_SYSV_ABI Func_BE6F4E5524BB135F();
s32 PS4_SYSV_ABI Func_C0D490EB481EA4D0();
s32 PS4_SYSV_ABI Func_C175D392CA6D084A();
s32 PS4_SYSV_ABI Func_CD0136AF165D2F2F();
s32 PS4_SYSV_ABI Func_D1C0ADB7B52FEAB5();
s32 PS4_SYSV_ABI Func_E324765D18EE4D12();
s32 PS4_SYSV_ABI Func_E789F980D907B653();
s32 PS4_SYSV_ABI Func_F9A32E8685627436();

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Np::NpWebApi