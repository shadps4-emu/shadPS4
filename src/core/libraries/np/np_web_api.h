// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
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

struct OrbisNpWebApiPushEventFilterParameter {
    OrbisNpWebApiPushEventDataType dataType;
};

struct OrbisNpWebApiServicePushEventFilterParameter {
    OrbisNpWebApiPushEventDataType dataType;
};

struct OrbisNpWebApiExtdPushEventFilterParameter {
    OrbisNpWebApiPushEventDataType dataType;
    OrbisNpWebApiExtdPushEventExtdDataKey* pExtdDataKey;
    u64 extdDataKeyNum;
};

struct OrbisNpWebApiExtdPushEventExtdData {
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

enum OrbisNpWebApiHttpMethod : s32 {
    ORBIS_NP_WEBAPI_HTTP_METHOD_GET,
    ORBIS_NP_WEBAPI_HTTP_METHOD_POST,
    ORBIS_NP_WEBAPI_HTTP_METHOD_PUT,
    ORBIS_NP_WEBAPI_HTTP_METHOD_DELETE,
    ORBIS_NP_WEBAPI_HTTP_METHOD_PATCH
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

struct OrbisNpWebApiIntInitializeArgs {
    u32 libHttpCtxId;
    u8 reserved[4];
    u64 poolSize;
    const char* name;
    u64 structSize;
};

struct OrbisNpWebApiIntCreateRequestExtraArgs {
    void* unk_0;
    void* unk_1;
    void* unk_2;
};

using OrbisNpWebApiPushEventCallback = PS4_SYSV_ABI void (*)(); // dummy

using OrbisNpWebApiExtdPushEventCallback = PS4_SYSV_ABI void (*)(); // dummy
using OrbisNpWebApiExtdPushEventCallbackA = PS4_SYSV_ABI void (*)(
    s32 userCtxId, s32 callbackId, const char* pNpServiceName, OrbisNpServiceLabel npServiceLabel,
    const OrbisNpPeerAddressA* pTo, const OrbisNpOnlineId* pToOnlineId,
    const OrbisNpPeerAddressA* pFrom, const OrbisNpOnlineId* pFromOnlineId,
    const OrbisNpWebApiPushEventDataType* pDataType, const char* pData, u64 dataLen,
    const OrbisNpWebApiExtdPushEventExtdData* pExtdData, u64 extdDataNum, void* pUserArg);

using OrbisNpWebApiServicePushEventCallback = PS4_SYSV_ABI void (*)();          // dummy
using OrbisNpWebApiInternalServicePushEventCallback = PS4_SYSV_ABI void (*)();  // dummy
using OrbisNpWebApiInternalServicePushEventCallbackA = PS4_SYSV_ABI void (*)(); // dummy

using OrbisNpWebApiNotificationCallback = PS4_SYSV_ABI void (*)(); // dummy

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Np::NpWebApi