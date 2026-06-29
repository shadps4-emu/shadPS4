// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/np/np_web_api2/np_web_api2.h"
#include "core/libraries/system/userservice.h"

namespace Libraries::Np::NpWebApi2 {

s32 createLibraryContext(s32 http_ctx_id, s32 type, u64 pool_size, const char* name);
s32 getMemoryPoolStats(s32 lib_ctx_id, OrbisNpWebApi2MemoryPoolStats* stats);
s32 createUserContext(s32 lib_ctx_id, Libraries::UserService::OrbisUserServiceUserId user_id);
s32 createRequest(s32 user_ctx_id, const char* api_group, const char* path, const char* method,
                  const OrbisNpWebApi2ContentParameter* content_parameter, bool multipart,
                  s64* request_id);
s32 addHttpRequestHeader(s64 request_id, const char* field_name, const char* field_value);
s32 sendRequest(s64 request_id, s32 part_index, void* data, u64 data_size,
                OrbisNpWebApi2ResponseInformationOption* resp_info_option);
s32 abortRequest(s64 request_id);
s32 deleteRequest(s64 request_id);

}; // namespace Libraries::Np::NpWebApi2