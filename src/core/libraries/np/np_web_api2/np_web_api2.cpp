// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/emulator_settings.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_manager.h"
#include "core/libraries/np/np_types.h"
#include "core/libraries/np/np_web_api2/np_web_api2.h"
#include "core/libraries/np/np_web_api2/np_web_api2_internal.h"
#include "core/libraries/system/userservice.h"

namespace Libraries::Np::NpWebApi2 {

s32 PS4_SYSV_ABI sceNpWebApi2AbortRequest(s64 request_id) {
    LOG_INFO(Lib_NpWebApi2, "called, request_id = {:#x}", request_id);
    return abortRequest(request_id);
}

s32 PS4_SYSV_ABI sceNpWebApi2AddHttpRequestHeader(s64 request_id, const char* field_name,
                                                  const char* field_value) {
    if (!field_name || !field_value) {
        LOG_ERROR(Lib_NpWebApi2, "Invalid parameters");
        return ORBIS_NP_WEBAPI2_ERROR_INVALID_ARGUMENT;
    }
    LOG_INFO(Lib_NpWebApi2, "called, request_id = {:#x}, field_name = {}, field_value = {}",
             request_id, field_name, field_value);
    return addHttpRequestHeader(request_id, field_name, field_value);
}

s32 PS4_SYSV_ABI sceNpWebApi2AddMultipartPart() {
    LOG_ERROR(Lib_NpWebApi2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApi2AddWebTraceTag(s64 request_id, const char* value) {
    if (!value) {
        LOG_ERROR(Lib_NpWebApi2, "Invalid parameters");
        return ORBIS_NP_WEBAPI2_ERROR_INVALID_ARGUMENT;
    }
    LOG_INFO(Lib_NpWebApi2, "called, request_id = {:#x}, value = {}", request_id, value);
    return addHttpRequestHeader(request_id, "X-Psn-WebTrace-Tag", value);
}

void PS4_SYSV_ABI sceNpWebApi2CheckTimeout() {
    LOG_TRACE(Lib_NpWebApi2, "called");
    return checkTimeout();
}

s32 PS4_SYSV_ABI sceNpWebApi2CreateMultipartRequest(s32 user_ctx_id, const char* api_group,
                                                    const char* path, const char* method,
                                                    s64* request_id) {
    if (!api_group || !path || !method) {
        LOG_ERROR(Lib_NpWebApi2, "Invalid parameters");
        return ORBIS_NP_WEBAPI2_ERROR_INVALID_ARGUMENT;
    }

    LOG_INFO(Lib_NpWebApi2, "called, user_ctx_id = {:#x}, api_group = {}, path = {}, method = {}",
             user_ctx_id, api_group, path, method);
    s64 temp_request_id = 0;
    s32 result =
        createRequest(user_ctx_id, api_group, path, method, nullptr, true, &temp_request_id);
    if (result == 0) {
        LOG_INFO(Lib_NpWebApi2, "created request_id = {:#x}", temp_request_id);
        if (request_id) {
            *request_id = temp_request_id;
        }
    }
    return result;
}

s32 PS4_SYSV_ABI sceNpWebApi2CreateRequest(s32 user_ctx_id, const char* api_group, const char* path,
                                           const char* method,
                                           const OrbisNpWebApi2ContentParameter* content_parameter,
                                           s64* request_id) {
    if (!api_group || !path || !method) {
        LOG_ERROR(Lib_NpWebApi2, "Invalid parameters");
        return ORBIS_NP_WEBAPI2_ERROR_INVALID_ARGUMENT;
    }
    if (content_parameter && content_parameter->content_length != 0 &&
        !content_parameter->content_type) {
        LOG_ERROR(Lib_NpWebApi2, "Invalid content parameter");
        return ORBIS_NP_WEBAPI2_ERROR_INVALID_CONTENT_PARAMETER;
    }

    LOG_INFO(Lib_NpWebApi2, "called, user_ctx_id = {:#x}, api_group = {}, path = {}, method = {}",
             user_ctx_id, api_group, path, method);
    s64 temp_request_id = 0;
    s32 result = createRequest(user_ctx_id, api_group, path, method, content_parameter, false,
                               &temp_request_id);
    if (result == 0) {
        LOG_INFO(Lib_NpWebApi2, "created request_id = {:#x}", temp_request_id);
        if (request_id) {
            *request_id = temp_request_id;
        }
    }
    return result;
}

s32 PS4_SYSV_ABI sceNpWebApi2CreateUserContext(s32 lib_ctx_id,
                                               UserService::OrbisUserServiceUserId user_id) {
    if (lib_ctx_id >= 0x8000) {
        LOG_ERROR(Lib_NpWebApi2, "Invalid library context id {:#x}", lib_ctx_id);
        return ORBIS_NP_WEBAPI2_ERROR_INVALID_LIB_CONTEXT_ID;
    }
    if (user_id == UserService::ORBIS_USER_SERVICE_USER_ID_INVALID) {
        LOG_ERROR(Lib_NpWebApi2, "Invalid user id");
        return ORBIS_NP_WEBAPI2_ERROR_INVALID_ARGUMENT;
    }
    LOG_DEBUG(Lib_NpWebApi2, "called, lib_ctx_id = {:#x}, user_id = {}", lib_ctx_id, user_id);
    s32 user_ctx_id = createUserContext(lib_ctx_id, user_id);
    if (user_ctx_id > 0) {
        LOG_INFO(Lib_NpWebApi2, "created user_ctx_id = {:#x}", user_ctx_id);
    }
    return user_ctx_id;
}

s32 PS4_SYSV_ABI sceNpWebApi2DeleteRequest(s64 request_id) {
    LOG_INFO(Lib_NpWebApi2, "called, request_id = {:#x}", request_id);
    return deleteRequest(request_id);
}

s32 PS4_SYSV_ABI sceNpWebApi2DeleteUserContext(s32 user_ctx_id) {
    LOG_INFO(Lib_NpWebApi2, "called, user_ctx_id = {:#x}", user_ctx_id);
    return deleteUserContext(user_ctx_id);
}

s32 PS4_SYSV_ABI sceNpWebApi2GetHttpResponseHeaderValue(s64 request_id, const char* field_name,
                                                        u64* field_value_length) {
    if (!field_name || !field_value_length) {
        LOG_ERROR(Lib_NpWebApi2, "Invalid parameters");
        return ORBIS_NP_WEBAPI2_ERROR_INVALID_ARGUMENT;
    }
    s32 result = getHttpResponseHeaderData(request_id, field_name, nullptr, 0, field_value_length);
    if (result >= 0) {
        LOG_INFO(Lib_NpWebApi2,
                 "on request_id = {:#x}, field_name = {} returned field_value_length = {:#x}",
                 request_id, field_name, *field_value_length);
    }
    return result;
}

s32 PS4_SYSV_ABI sceNpWebApi2GetHttpResponseHeaderValueLength(s64 request_id,
                                                              const char* field_name, char* value,
                                                              u64 value_size) {
    if (!field_name || !value || value_size == 0) {
        LOG_ERROR(Lib_NpWebApi2, "Invalid parameters");
        return ORBIS_NP_WEBAPI2_ERROR_INVALID_ARGUMENT;
    }
    LOG_INFO(Lib_NpWebApi2, "called, request_id = {:#x}, field_name = {}", request_id, field_name);
    return getHttpResponseHeaderData(request_id, field_name, value, value_size, nullptr);
}

s32 PS4_SYSV_ABI sceNpWebApi2GetMemoryPoolStats(s32 lib_ctx_id,
                                                OrbisNpWebApi2MemoryPoolStats* stats) {
    LOG_INFO(Lib_NpWebApi2, "called, lib_ctx_id = {:#x}", lib_ctx_id);
    return getMemoryPoolStats(lib_ctx_id, stats);
}

s32 PS4_SYSV_ABI sceNpWebApi2Initialize(s32 lib_http_ctx_id, u64 pool_size) {
    LOG_DEBUG(Lib_NpWebApi2, "called, lib_http_ctx_id = {:#x}, pool_size = {:#x}", lib_http_ctx_id,
              pool_size);

    // Uses a sceLncUtilGetAppStatus check to enable debug mode. For now, default to normal.
    s32 ctx_id = createLibraryContext(lib_http_ctx_id, 1, pool_size, nullptr);
    if (ctx_id > 0) {
        LOG_INFO(Lib_NpWebApi2, "created lib_ctx_id = {:#x}", ctx_id);
    }
    return ctx_id;
}

s32 PS4_SYSV_ABI sceNpWebApi2InitializeForPresence(s32 lib_http_ctx_id, u64 pool_size) {
    LOG_DEBUG(Lib_NpWebApi2, "called, lib_http_ctx_id = {:#x}, pool_size = {:#x}", lib_http_ctx_id,
              pool_size);

    s32 ctx_id = createLibraryContext(lib_http_ctx_id, 3, pool_size, nullptr);
    if (ctx_id > 0) {
        LOG_INFO(Lib_NpWebApi2, "created lib_ctx_id = {:#x}", ctx_id);
    }
    return ctx_id;
}

s32 PS4_SYSV_ABI sceNpWebApi2IntCreateRequest() {
    LOG_ERROR(Lib_NpWebApi2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApi2IntInitialize(const OrbisNpWebApi2IntInitializeArgs* args) {
    if (args == nullptr || args->struct_size != sizeof(OrbisNpWebApi2IntInitializeArgs)) {
        LOG_ERROR(Lib_NpWebApi2, "Invalid arguments");
        return ORBIS_NP_WEBAPI2_ERROR_INVALID_ARGUMENT;
    }
    LOG_DEBUG(Lib_NpWebApi2, "called, lib_http_ctx_id = {:#x}, pool_size = {:#x}, name = {}",
              args->lib_http_ctx_id, args->pool_size, args->name ? args->name : "(null)");

    s32 ctx_id = createLibraryContext(args->lib_http_ctx_id, 2, args->pool_size, args->name);
    if (ctx_id > 0) {
        LOG_INFO(Lib_NpWebApi2, "created lib_ctx_id = {:#x}", ctx_id);
    }
    return ctx_id;
}

s32 PS4_SYSV_ABI sceNpWebApi2IntInitialize2(const OrbisNpWebApi2IntInitialize2Args* args) {

    if (args == nullptr || args->struct_size != sizeof(OrbisNpWebApi2IntInitialize2Args)) {
        LOG_ERROR(Lib_NpWebApi2, "Invalid arguments");
        return ORBIS_NP_WEBAPI2_ERROR_INVALID_ARGUMENT;
    }
    LOG_DEBUG(
        Lib_NpWebApi2,
        "called, lib_http_ctx_id = {:#x}, pool_size = {:#x}, name = {}, push_config_group = {:#x}",
        args->lib_http_ctx_id, args->pool_size, args->name ? args->name : "(null)",
        args->push_config_group);

    s32 ctx_id = createLibraryContext(args->lib_http_ctx_id, 2, args->pool_size, args->name);
    if (ctx_id > 0) {
        LOG_INFO(Lib_NpWebApi2, "created lib_ctx_id = {:#x}", ctx_id);
    }
    return ctx_id;
}

s32 PS4_SYSV_ABI sceNpWebApi2IntPushEventCreateCtxIndFilter() {
    LOG_ERROR(Lib_NpWebApi2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApi2PushEventAbortHandle(s32 lib_ctx_id, s32 handle_id) {
    LOG_INFO(Lib_NpWebApi2, "called, lib_ctx_id = {:#x}, handle_id = {:#x}", lib_ctx_id, handle_id);
    return abortPushEventHandle(lib_ctx_id, handle_id);
}

s32 PS4_SYSV_ABI sceNpWebApi2PushEventCreateFilter(
    s32 lib_ctx_id, s32 handle_id, const char* np_service_name,
    OrbisNpServiceLabel np_service_label,
    const OrbisNpWebApi2PushEventFilterParameter* filter_param, u64 filter_param_num) {
    if ((np_service_name && np_service_label == ORBIS_NP_INVALID_SERVICE_LABEL) || !filter_param ||
        filter_param_num == 0) {
        LOG_ERROR(Lib_NpWebApi2, "Invalid parameters");
        return ORBIS_NP_WEBAPI2_ERROR_INVALID_ARGUMENT;
    }
    LOG_DEBUG(Lib_NpWebApi2,
              "called, lib_ctx_id = {:#x}, handle_id = {:#x}, np_service_name = {}, "
              "np_service_label = {:#x}, filter_param_num = {:#x}",
              lib_ctx_id, handle_id, np_service_name ? np_service_name : "(null)",
              static_cast<u32>(np_service_label), filter_param_num);
    for (u64 i = 0; i < filter_param_num; i++) {
        LOG_DEBUG(Lib_NpWebApi2, "filter_param[{}].data_type.val = {}", i,
                  filter_param[i].data_type.val);
        LOG_DEBUG(Lib_NpWebApi2, "filter_param[{}].extd_data_key_num = {}", i,
                  filter_param[i].extd_data_key_num);
        for (u64 j = 0; j < filter_param[i].extd_data_key_num && filter_param[i].extd_data_key;
             j++) {
            LOG_DEBUG(Lib_NpWebApi2, "filter_param[{}].extd_data_key[{}].val = {}", i, j,
                      filter_param[i].extd_data_key[j].val);
        }
    }
    s32 filter_id = createPushEventFilter(lib_ctx_id, handle_id, np_service_name, np_service_label,
                                          filter_param, filter_param_num, false);
    if (filter_id > 0) {
        LOG_INFO(Lib_NpWebApi2, "created filter_id = {:#x}", filter_id);
    }
    return filter_id;
}

s32 PS4_SYSV_ABI sceNpWebApi2PushEventCreateHandle(s32 lib_ctx_id) {
    LOG_DEBUG(Lib_NpWebApi2, "called, lib_ctx_id = {:#x}", lib_ctx_id);
    s32 handle_id = createPushEventHandle(lib_ctx_id);
    if (handle_id > 0) {
        LOG_INFO(Lib_NpWebApi2, "created handle_id = {:#x}", handle_id);
    }
    return handle_id;
}

s32 PS4_SYSV_ABI sceNpWebApi2PushEventCreatePushContext(
    s32 user_ctx_id, OrbisNpWebApi2PushEventPushContextId* push_ctx_id) {
    if (!push_ctx_id) {
        LOG_ERROR(Lib_NpWebApi2, "Invalid parameters");
        return ORBIS_NP_WEBAPI2_ERROR_INVALID_ARGUMENT;
    }
    LOG_DEBUG(Lib_NpWebApi2, "called, user_ctx_id = {:#x}", user_ctx_id);
    s32 result = createPushContext(user_ctx_id, push_ctx_id);
    if (result == 0) {
        s64 raw_id{};
        std::memcpy(&raw_id, push_ctx_id, sizeof(s64));
        LOG_INFO(Lib_NpWebApi2, "created push_ctx_id = {:#x}", raw_id);
    }
    return result;
}

s32 PS4_SYSV_ABI sceNpWebApi2PushEventDeleteFilter(s32 lib_ctx_id, s32 filter_id) {
    LOG_INFO(Lib_NpWebApi2, "called, lib_ctx_id = {:#x}, filter_id = {:#x}", lib_ctx_id, filter_id);
    return deletePushEventFilter(lib_ctx_id, filter_id);
}

s32 PS4_SYSV_ABI sceNpWebApi2PushEventDeleteHandle(s32 lib_ctx_id, s32 handle_id) {
    LOG_INFO(Lib_NpWebApi2, "called, lib_ctx_id = {:#x}, handle_id = {:#x}", lib_ctx_id, handle_id);
    return deletePushEventHandle(lib_ctx_id, handle_id);
}

s32 PS4_SYSV_ABI sceNpWebApi2PushEventDeletePushContext(
    s32 user_ctx_id, OrbisNpWebApi2PushEventPushContextId* push_ctx_id) {
    if (!push_ctx_id) {
        LOG_ERROR(Lib_NpWebApi2, "Invalid parameters");
        return ORBIS_NP_WEBAPI2_ERROR_INVALID_ARGUMENT;
    }
    s64 raw_id{};
    std::memcpy(&raw_id, push_ctx_id, sizeof(s64));
    LOG_INFO(Lib_NpWebApi2, "called, user_ctx_id = {:#x}, push_ctx_id = {:#x}", user_ctx_id,
             raw_id);
    return deletePushContext(user_ctx_id, push_ctx_id);
}

s32 PS4_SYSV_ABI sceNpWebApi2PushEventRegisterCallback(s32 user_ctx_id, s32 filter_id,
                                                       OrbisNpWebApi2PushEventCallback cb_func,
                                                       void* user_arg) {
    LOG_DEBUG(Lib_NpWebApi2, "called, user_ctx_id = {:#x}, filter_id = {:#x}", user_ctx_id,
              filter_id);
    if (!cb_func) {
        LOG_ERROR(Lib_NpWebApi2, "Invalid parameters");
        return ORBIS_NP_WEBAPI2_ERROR_INVALID_ARGUMENT;
    }
    s32 callback_id = registerPushEventCallback(user_ctx_id, filter_id, cb_func, user_arg);
    if (callback_id > 0) {
        LOG_INFO(Lib_NpWebApi2, "created callback_id = {:#x}", callback_id);
    }
    return callback_id;
}

s32 PS4_SYSV_ABI sceNpWebApi2PushEventRegisterPushContextCallback(
    s32 user_ctx_id, s32 filter_id, OrbisNpWebApi2PushEventPushContextCallback cb_func,
    void* user_arg) {
    LOG_DEBUG(Lib_NpWebApi2, "called, user_ctx_id = {:#x}, filter_id = {:#x}", user_ctx_id,
              filter_id);
    if (!cb_func) {
        LOG_ERROR(Lib_NpWebApi2, "Invalid parameters");
        return ORBIS_NP_WEBAPI2_ERROR_INVALID_ARGUMENT;
    }
    s32 callback_id = registerPushContextCallback(user_ctx_id, filter_id, cb_func, user_arg);
    if (callback_id > 0) {
        LOG_INFO(Lib_NpWebApi2, "created callback_id = {:#x}", callback_id);
    }
    return callback_id;
}

s32 PS4_SYSV_ABI sceNpWebApi2PushEventSetHandleTimeout(s32 lib_ctx_id, s32 handle_id, u32 timeout) {
    LOG_INFO(Lib_NpWebApi2, "called, lib_ctx_id = {:#x}, handle_id = {:#x}, timeout = {}",
             lib_ctx_id, handle_id, timeout);
    return setHandleTimeout(lib_ctx_id, handle_id, timeout);
}

s32 PS4_SYSV_ABI sceNpWebApi2PushEventStartPushContextCallback(
    s32 user_ctx_id, const OrbisNpWebApi2PushEventPushContextId* push_ctx_id) {
    if (!push_ctx_id) {
        LOG_ERROR(Lib_NpWebApi2, "Invalid parameters");
        return ORBIS_NP_WEBAPI2_ERROR_INVALID_ARGUMENT;
    }
    s64 raw_id{};
    std::memcpy(&raw_id, push_ctx_id, sizeof(s64));
    LOG_INFO(Lib_NpWebApi2, "called, user_ctx_id = {:#x}, push_ctx_id = {:#x}", user_ctx_id,
             raw_id);
    return startPushContextCallback(user_ctx_id, push_ctx_id);
}

s32 PS4_SYSV_ABI sceNpWebApi2PushEventUnregisterCallback(s32 user_ctx_id, s32 callback_id) {
    LOG_INFO(Lib_NpWebApi2, "called, user_ctx_id = {:#x}, callback_id = {:#x}", user_ctx_id,
             callback_id);
    return unregisterPushEventCallback(user_ctx_id, callback_id);
}

s32 PS4_SYSV_ABI sceNpWebApi2PushEventUnregisterPushContextCallback(s32 user_ctx_id,
                                                                    s32 callback_id) {
    LOG_INFO(Lib_NpWebApi2, "called, user_ctx_id = {:#x}, callback_id = {:#x}", user_ctx_id,
             callback_id);
    return unregisterPushContextCallback(user_ctx_id, callback_id);
}

s32 PS4_SYSV_ABI sceNpWebApi2ReadData(s64 request_id, void* data, u64 size) {
    if (!data || size == 0) {
        LOG_ERROR(Lib_NpWebApi2, "Invalid parameters");
        return ORBIS_NP_WEBAPI2_ERROR_INVALID_ARGUMENT;
    }
    LOG_INFO(Lib_NpWebApi2, "called, request_id = {:#x}, size = {:#x}", request_id, size);
    return readData(request_id, data, size);
}

s32 PS4_SYSV_ABI
sceNpWebApi2SendMultipartRequest(s64 request_id, s32 part_index, void* data, u64 data_size,
                                 OrbisNpWebApi2ResponseInformationOption* resp_info_option) {
    if (part_index <= 0 || !data || data_size == 0) {
        LOG_ERROR(Lib_NpWebApi2, "Invalid parameters");
        return ORBIS_NP_WEBAPI2_ERROR_INVALID_ARGUMENT;
    }

    LOG_WARNING(Lib_NpWebApi2, "called, request_id = {:#x}, part_index = {}, data_size = {:#x}",
                request_id, part_index, data_size);
    return sendRequest(request_id, part_index, data, data_size, resp_info_option);
}

s32 PS4_SYSV_ABI
sceNpWebApi2SendRequest(s64 request_id, void* data, u64 data_size,
                        OrbisNpWebApi2ResponseInformationOption* resp_info_option) {
    LOG_INFO(Lib_NpWebApi2, "called, request_id = {:#x}, data_size = {:#x}", request_id, data_size);
    return sendRequest(request_id, 0, data, data_size, resp_info_option);
}

s32 PS4_SYSV_ABI sceNpWebApi2SetMultipartContentType() {
    LOG_ERROR(Lib_NpWebApi2, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWebApi2SetRequestTimeout(s64 request_id, u32 timeout) {
    LOG_INFO(Lib_NpWebApi2, "called, request_id = {:#x}, timeout = {}", request_id, timeout);
    return setRequestTimeout(request_id, timeout);
}

s32 PS4_SYSV_ABI sceNpWebApi2Terminate(s32 lib_ctx_id) {
    LOG_INFO(Lib_NpWebApi2, "called, lib_ctx_id = {:#x}", lib_ctx_id);
    return terminateLibraryContext(lib_ctx_id);
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
    Libraries::Np::NpManager::RegisterNpCallback("npwebapi2_push", processPushEvents);

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