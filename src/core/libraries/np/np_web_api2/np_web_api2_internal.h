// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/np/np_web_api2/np_web_api2.h"
#include "core/libraries/system/userservice.h"

namespace Libraries::Np::NpWebApi2 {

s32 createLibraryContext(s32 http_ctx_id, s32 type, u64 pool_size, const char* name);
s32 getMemoryPoolStats(s32 lib_ctx_id, OrbisNpWebApi2MemoryPoolStats* stats);
s32 createPushEventHandle(s32 lib_ctx_id);
s32 setHandleTimeout(s32 lib_ctx_id, s32 handle_id, u32 timeout);
s32 abortPushEventHandle(s32 lib_ctx_id, s32 handle_id);
s32 deletePushEventHandle(s32 lib_ctx_id, s32 handle_id);
s32 createPushEventFilter(s32 lib_ctx_id, s32 handle_id, const char* np_service_name,
                          OrbisNpServiceLabel np_service_label,
                          const OrbisNpWebApi2PushEventFilterParameter* filter_param,
                          u64 filter_param_num, bool internal);
s32 deletePushEventFilter(s32 lib_ctx_id, s32 filter_id);
s32 terminateLibraryContext(s32 lib_ctx_id);
s32 createUserContext(s32 lib_ctx_id, Libraries::UserService::OrbisUserServiceUserId user_id);
s32 deleteUserContext(s32 user_ctx_id);
s32 registerPushEventCallback(s32 user_ctx_id, s32 filter_id,
                              OrbisNpWebApi2PushEventCallback cb_func, void* user_arg);
s32 unregisterPushEventCallback(s32 user_ctx_id, s32 callback_id);
s32 createPushContext(s32 user_ctx_id, OrbisNpWebApi2PushEventPushContextId* push_ctx_id);
s32 startPushContextCallback(s32 user_ctx_id,
                             const OrbisNpWebApi2PushEventPushContextId* push_ctx_id);
s32 deletePushContext(s32 user_ctx_id, const OrbisNpWebApi2PushEventPushContextId* push_ctx_id);
s32 registerPushContextCallback(s32 user_ctx_id, s32 filter_id,
                                OrbisNpWebApi2PushEventPushContextCallback cb_func, void* user_arg);
s32 unregisterPushContextCallback(s32 user_ctx_id, s32 callback_id);
void processPushEvents();
s32 createRequest(s32 user_ctx_id, const char* api_group, const char* path, const char* method,
                  const OrbisNpWebApi2ContentParameter* content_parameter, bool multipart,
                  s64* request_id);
s32 addHttpRequestHeader(s64 request_id, const char* field_name, const char* field_value);
s32 setRequestTimeout(s64 request_id, u32 timeout);
s32 sendRequest(s64 request_id, s32 part_index, void* data, u64 data_size,
                OrbisNpWebApi2ResponseInformationOption* resp_info_option);
s32 getHttpResponseHeaderData(s64 request_id, const char* field_name, char* value, u64 value_size,
                              u64* value_size_out);
s32 readData(s64 request_id, void* data, u64 size);
s32 abortRequest(s64 request_id);
s32 deleteRequest(s64 request_id);
void checkTimeout();

}; // namespace Libraries::Np::NpWebApi2