// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/config.h"
#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/np/np_web_api2.h"
#include "core/libraries/np/np_web_api2_error.h"
#include "core/libraries/system/userservice.h"

namespace Libraries::Np::NpWebApi2 {

s32 PS4_SYSV_ABI sceNpWebApi2AbortRequest(s64 request_id) {
    LOG_ERROR(Lib_NpWebApi2, "(STUBBED) called, request_id = {:#x}", request_id);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApi2AddHttpRequestHeader() {
    LOG_ERROR(Lib_NpWebApi2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApi2AddMultipartPart() {
    LOG_ERROR(Lib_NpWebApi2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApi2AddWebTraceTag() {
    LOG_ERROR(Lib_NpWebApi2, "(STUBBED) called");
    return ORBIS_OK;
}

void PS4_SYSV_ABI sceNpWebApi2CheckTimeout() {
    LOG_ERROR(Lib_NpWebApi2, "(STUBBED) called");
}

s32 PS4_SYSV_ABI sceNpWebApi2CreateMultipartRequest() {
    LOG_ERROR(Lib_NpWebApi2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApi2CreateRequest() {
    LOG_ERROR(Lib_NpWebApi2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApi2CreateUserContext(s32 lib_ctx_id,
                                               UserService::OrbisUserServiceUserId user_id) {
    if (lib_ctx_id >= 0x8000) {
        return ORBIS_NP_WEBAPI2_ERROR_INVALID_LIB_CONTEXT_ID;
    }
    if (user_id == UserService::ORBIS_USER_SERVICE_USER_ID_INVALID) {
        return ORBIS_NP_WEBAPI2_ERROR_INVALID_ARGUMENT;
    }
    LOG_ERROR(Lib_NpWebApi2, "(STUBBED) called, lib_ctx_id = {:#x}, user_id = {:#x}", lib_ctx_id,
              user_id);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApi2DeleteRequest(s64 request_id) {
    LOG_ERROR(Lib_NpWebApi2, "(STUBBED) called, request_id = {:#x}", request_id);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApi2DeleteUserContext(s32 user_ctx_id) {
    LOG_ERROR(Lib_NpWebApi2, "(STUBBED) called, user_ctx_id = {:#x}", user_ctx_id);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApi2GetHttpResponseHeaderValue() {
    LOG_ERROR(Lib_NpWebApi2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApi2GetHttpResponseHeaderValueLength() {
    LOG_ERROR(Lib_NpWebApi2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApi2GetMemoryPoolStats(s32 lib_ctx_id,
                                                OrbisNpWebApi2MemoryPoolStats* stats) {
    LOG_ERROR(Lib_NpWebApi2, "(STUBBED) called, lib_ctx_id = {:#x}", lib_ctx_id);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApi2Initialize(s32 lib_http_ctx_id, u64 pool_size) {
    LOG_ERROR(Lib_NpWebApi2, "(STUBBED) called, lib_http_ctx_id = {:#x}, pool_size = {:#x}",
              lib_http_ctx_id, pool_size);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApi2InitializeForPresence(s32 lib_http_ctx_id, u64 pool_size) {
    LOG_ERROR(Lib_NpWebApi2, "(STUBBED) called, lib_http_ctx_id = {:#x}, pool_size = {:#x}",
              lib_http_ctx_id, pool_size);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApi2IntCreateRequest() {
    LOG_ERROR(Lib_NpWebApi2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApi2IntInitialize(const OrbisNpWebApi2IntInitializeArgs* args) {
    if (args == nullptr || args->struct_size != sizeof(OrbisNpWebApi2IntInitializeArgs)) {
        return ORBIS_NP_WEBAPI2_ERROR_INVALID_ARGUMENT;
    }
    LOG_ERROR(Lib_NpWebApi2,
              "(STUBBED) called, lib_http_ctx_id = {:#x}, pool_size = {:#x}, name = '{}'",
              args->lib_http_ctx_id, args->pool_size, args->name);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApi2IntInitialize2(const OrbisNpWebApi2IntInitialize2Args* args) {
    if (args == nullptr || args->struct_size != sizeof(OrbisNpWebApi2IntInitialize2Args)) {
        return ORBIS_NP_WEBAPI2_ERROR_INVALID_ARGUMENT;
    }
    LOG_ERROR(
        Lib_NpWebApi2,
        "(STUBBED) called, lib_http_ctx_id = {:#x}, pool_size = {:#x}, name = '{}', group = {:#x}",
        args->lib_http_ctx_id, args->pool_size, args->name, args->push_config_group);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApi2IntPushEventCreateCtxIndFilter() {
    LOG_ERROR(Lib_NpWebApi2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApi2PushEventAbortHandle() {
    LOG_ERROR(Lib_NpWebApi2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApi2PushEventCreateFilter() {
    LOG_ERROR(Lib_NpWebApi2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApi2PushEventCreateHandle() {
    LOG_ERROR(Lib_NpWebApi2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApi2PushEventCreatePushContext() {
    LOG_ERROR(Lib_NpWebApi2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApi2PushEventDeleteFilter() {
    LOG_ERROR(Lib_NpWebApi2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApi2PushEventDeleteHandle() {
    LOG_ERROR(Lib_NpWebApi2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApi2PushEventDeletePushContext() {
    LOG_ERROR(Lib_NpWebApi2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApi2PushEventRegisterCallback() {
    LOG_ERROR(Lib_NpWebApi2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApi2PushEventRegisterPushContextCallback() {
    LOG_ERROR(Lib_NpWebApi2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApi2PushEventSetHandleTimeout() {
    LOG_ERROR(Lib_NpWebApi2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApi2PushEventStartPushContextCallback() {
    LOG_ERROR(Lib_NpWebApi2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApi2PushEventUnregisterCallback() {
    LOG_ERROR(Lib_NpWebApi2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApi2PushEventUnregisterPushContextCallback() {
    LOG_ERROR(Lib_NpWebApi2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApi2ReadData(s64 request_id, void* data, u64 size) {
    if (data == nullptr || size == 0) {
        return ORBIS_NP_WEBAPI2_ERROR_INVALID_ARGUMENT;
    }
    LOG_ERROR(Lib_NpWebApi2, "(STUBBED) called, request_id = {:#x}, size = {:#x}", request_id,
              size);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApi2SendMultipartRequest() {
    LOG_ERROR(Lib_NpWebApi2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApi2SendRequest() {
    if (!Config::getPSNSignedIn()) {
        LOG_INFO(Lib_NpWebApi2, "called, returning PSN signed out.");
        return ORBIS_NP_WEBAPI2_ERROR_NOT_SIGNED_IN;
    }
    LOG_ERROR(Lib_NpWebApi2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApi2SetMultipartContentType() {
    LOG_ERROR(Lib_NpWebApi2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApi2SetRequestTimeout(s64 request_id, u32 timeout) {
    LOG_ERROR(Lib_NpWebApi2, "(STUBBED) called, request_id = {:#x}, timeout = {}", request_id,
              timeout);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApi2Terminate(s32 lib_ctx_id) {
    LOG_ERROR(Lib_NpWebApi2, "(STUBBED) called, lib_ctx_id = {:#x}", lib_ctx_id);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_A9A31C5F6FBA6620() {
    LOG_ERROR(Lib_NpWebApi2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_03D22863300D2B73() {
    LOG_ERROR(Lib_NpWebApi2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_97296F7578AAD541() {
    LOG_ERROR(Lib_NpWebApi2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_E0DF39A36F087DB9() {
    LOG_ERROR(Lib_NpWebApi2, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("zpiPsH7dbFQ", "libSceNpWebApi2", 1, "libSceNpWebApi2", sceNpWebApi2AbortRequest);
    LIB_FUNCTION("egOOvrnF6mI", "libSceNpWebApi2", 1, "libSceNpWebApi2",
                 sceNpWebApi2AddHttpRequestHeader);
    LIB_FUNCTION("Io7kh1LHDoM", "libSceNpWebApi2", 1, "libSceNpWebApi2",
                 sceNpWebApi2AddMultipartPart);
    LIB_FUNCTION("MgsTa76wlEk", "libSceNpWebApi2", 1, "libSceNpWebApi2",
                 sceNpWebApi2AddWebTraceTag);
    LIB_FUNCTION("3Tt9zL3tkoc", "libSceNpWebApi2", 1, "libSceNpWebApi2", sceNpWebApi2CheckTimeout);
    LIB_FUNCTION("+nz1Vq-NrDA", "libSceNpWebApi2", 1, "libSceNpWebApi2",
                 sceNpWebApi2CreateMultipartRequest);
    LIB_FUNCTION("3EI-OSJ65Xc", "libSceNpWebApi2", 1, "libSceNpWebApi2", sceNpWebApi2CreateRequest);
    LIB_FUNCTION("sk54bi6FtYM", "libSceNpWebApi2", 1, "libSceNpWebApi2",
                 sceNpWebApi2CreateUserContext);
    LIB_FUNCTION("vvzWO-DvG1s", "libSceNpWebApi2", 1, "libSceNpWebApi2", sceNpWebApi2DeleteRequest);
    LIB_FUNCTION("9X9+cneTGUU", "libSceNpWebApi2", 1, "libSceNpWebApi2",
                 sceNpWebApi2DeleteUserContext);
    LIB_FUNCTION("hksbskNToEA", "libSceNpWebApi2", 1, "libSceNpWebApi2",
                 sceNpWebApi2GetHttpResponseHeaderValue);
    LIB_FUNCTION("HwP3aM+c85c", "libSceNpWebApi2", 1, "libSceNpWebApi2",
                 sceNpWebApi2GetHttpResponseHeaderValueLength);
    LIB_FUNCTION("Xweb+naPZ8Y", "libSceNpWebApi2", 1, "libSceNpWebApi2",
                 sceNpWebApi2GetMemoryPoolStats);
    LIB_FUNCTION("+o9816YQhqQ", "libSceNpWebApi2", 1, "libSceNpWebApi2", sceNpWebApi2Initialize);
    LIB_FUNCTION("dowMWFgowXY", "libSceNpWebApi2", 1, "libSceNpWebApi2",
                 sceNpWebApi2InitializeForPresence);
    LIB_FUNCTION("qmINYLuqzaA", "libSceNpWebApi2", 1, "libSceNpWebApi2",
                 sceNpWebApi2IntCreateRequest);
    LIB_FUNCTION("zXaFo7euxsQ", "libSceNpWebApi2", 1, "libSceNpWebApi2", sceNpWebApi2IntInitialize);
    LIB_FUNCTION("9KSGFMRnp3k", "libSceNpWebApi2", 1, "libSceNpWebApi2",
                 sceNpWebApi2IntInitialize2);
    LIB_FUNCTION("2hlBNB96saE", "libSceNpWebApi2", 1, "libSceNpWebApi2",
                 sceNpWebApi2IntPushEventCreateCtxIndFilter);
    LIB_FUNCTION("1OLgvahaSco", "libSceNpWebApi2", 1, "libSceNpWebApi2",
                 sceNpWebApi2PushEventAbortHandle);
    LIB_FUNCTION("MsaFhR+lPE4", "libSceNpWebApi2", 1, "libSceNpWebApi2",
                 sceNpWebApi2PushEventCreateFilter);
    LIB_FUNCTION("WV1GwM32NgY", "libSceNpWebApi2", 1, "libSceNpWebApi2",
                 sceNpWebApi2PushEventCreateHandle);
    LIB_FUNCTION("NNVf18SlbT8", "libSceNpWebApi2", 1, "libSceNpWebApi2",
                 sceNpWebApi2PushEventCreatePushContext);
    LIB_FUNCTION("KJdPcOGmK58", "libSceNpWebApi2", 1, "libSceNpWebApi2",
                 sceNpWebApi2PushEventDeleteFilter);
    LIB_FUNCTION("fIATVMo4Y1w", "libSceNpWebApi2", 1, "libSceNpWebApi2",
                 sceNpWebApi2PushEventDeleteHandle);
    LIB_FUNCTION("QafxeZM3WK4", "libSceNpWebApi2", 1, "libSceNpWebApi2",
                 sceNpWebApi2PushEventDeletePushContext);
    LIB_FUNCTION("fY3QqeNkF8k", "libSceNpWebApi2", 1, "libSceNpWebApi2",
                 sceNpWebApi2PushEventRegisterCallback);
    LIB_FUNCTION("lxtHJMwBsaU", "libSceNpWebApi2", 1, "libSceNpWebApi2",
                 sceNpWebApi2PushEventRegisterPushContextCallback);
    LIB_FUNCTION("KWkc6Q3tjXc", "libSceNpWebApi2", 1, "libSceNpWebApi2",
                 sceNpWebApi2PushEventSetHandleTimeout);
    LIB_FUNCTION("AAj9X+4aGYA", "libSceNpWebApi2", 1, "libSceNpWebApi2",
                 sceNpWebApi2PushEventStartPushContextCallback);
    LIB_FUNCTION("hOnIlcGrO6g", "libSceNpWebApi2", 1, "libSceNpWebApi2",
                 sceNpWebApi2PushEventUnregisterCallback);
    LIB_FUNCTION("PmyrbbJSFz0", "libSceNpWebApi2", 1, "libSceNpWebApi2",
                 sceNpWebApi2PushEventUnregisterPushContextCallback);
    LIB_FUNCTION("OOY9+ObfKec", "libSceNpWebApi2", 1, "libSceNpWebApi2", sceNpWebApi2ReadData);
    LIB_FUNCTION("NKCwS8+5Fx8", "libSceNpWebApi2", 1, "libSceNpWebApi2",
                 sceNpWebApi2SendMultipartRequest);
    LIB_FUNCTION("lQOCF84lvzw", "libSceNpWebApi2", 1, "libSceNpWebApi2", sceNpWebApi2SendRequest);
    LIB_FUNCTION("bltDCAskmfE", "libSceNpWebApi2", 1, "libSceNpWebApi2",
                 sceNpWebApi2SetMultipartContentType);
    LIB_FUNCTION("TjAutbrkr60", "libSceNpWebApi2", 1, "libSceNpWebApi2",
                 sceNpWebApi2SetRequestTimeout);
    LIB_FUNCTION("bEvXpcEk200", "libSceNpWebApi2", 1, "libSceNpWebApi2", sceNpWebApi2Terminate);
    LIB_FUNCTION("qaMcX2+6ZiA", "libSceNpWebApi2", 1, "libSceNpWebApi2", Func_A9A31C5F6FBA6620);
    LIB_FUNCTION("A9IoYzANK3M", "libSceNpWebApi2AsyncRestricted", 1, "libSceNpWebApi2",
                 Func_03D22863300D2B73);
    LIB_FUNCTION("lylvdXiq1UE", "libSceNpWebApi2AsyncRestricted", 1, "libSceNpWebApi2",
                 Func_97296F7578AAD541);
    LIB_FUNCTION("4N85o28Ifbk", "libSceNpWebApi2AsyncRestricted", 1, "libSceNpWebApi2",
                 Func_E0DF39A36F087DB9);
};

} // namespace Libraries::Np::NpWebApi2