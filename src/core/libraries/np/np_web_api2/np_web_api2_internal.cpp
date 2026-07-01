// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/emulator_settings.h"
#include "core/libraries/kernel/time.h"
#include "core/libraries/network/http2.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_web_api2/np_web_api2_context.h"
#include "core/libraries/np/np_web_api2/np_web_api2_internal.h"
#include "core/libraries/system/userservice.h"

#include <map>
#include <mutex>

namespace Libraries::Np::NpWebApi2 {

std::recursive_mutex g_mutex{};
s32 g_current_lib_context_id{};
std::map<s32, LibraryContext*> g_lib_contexts{};

s32 createLibraryContext(s32 http_ctx_id, s32 type, u64 pool_size, const char* name) {
    std::scoped_lock lk{g_mutex};

    if (g_lib_contexts.size() >= 0x8000) {
        LOG_ERROR(Lib_NpWebApi2, "Too many library contexts");
        return ORBIS_NP_WEBAPI2_ERROR_LIB_CONTEXT_MAX;
    }

    do {
        g_current_lib_context_id++;
        if (g_current_lib_context_id >= 0x8000) {
            g_current_lib_context_id = 1;
        }
    } while (g_lib_contexts.contains(g_current_lib_context_id));

    if (!name) {
        g_lib_contexts[g_current_lib_context_id] =
            new LibraryContext(g_current_lib_context_id, type, http_ctx_id, pool_size);
    } else {
        g_lib_contexts[g_current_lib_context_id] =
            new LibraryContext(g_current_lib_context_id, type, http_ctx_id, pool_size, name);
    }
    return g_current_lib_context_id;
}

LibraryContext* getLibraryContext(s32 lib_ctx_id) {
    std::scoped_lock lk{g_mutex};

    if (!g_lib_contexts.contains(lib_ctx_id)) {
        return nullptr;
    }
    LibraryContext* lib_ctx = g_lib_contexts.at(lib_ctx_id);
    lib_ctx->AddUser();
    return lib_ctx;
}

s32 getMemoryPoolStats(s32 lib_ctx_id, OrbisNpWebApi2MemoryPoolStats* stats) {
    LibraryContext* lib_ctx = getLibraryContext(lib_ctx_id);
    if (!lib_ctx) {
        LOG_ERROR(Lib_NpWebApi2, "No library context with id {:#x}", lib_ctx_id);
        return ORBIS_NP_WEBAPI2_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    if (stats) {
        memset(stats, 0, sizeof(*stats));
        stats->pool_size = lib_ctx->GetPoolSize();
    }
    return ORBIS_OK;
}

s32 createUserContext(s32 lib_ctx_id, Libraries::UserService::OrbisUserServiceUserId user_id) {
    LibraryContext* lib_ctx = getLibraryContext(lib_ctx_id);
    if (!lib_ctx) {
        LOG_ERROR(Lib_NpWebApi2, "No library context with id {:#x}", lib_ctx_id);
        return ORBIS_NP_WEBAPI2_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    UserContext* user_ctx = lib_ctx->GetUserContextByUserId(user_id);
    if (user_ctx) {
        LOG_ERROR(Lib_NpWebApi2, "User context already exists for user id {}", user_id);
        user_ctx->RemoveUser();
        lib_ctx->RemoveUser();
        return ORBIS_NP_WEBAPI2_ERROR_USER_CONTEXT_ALREADY_EXIST;
    }

    s32 user_ctx_id = lib_ctx->CreateUserContext(user_id);
    if (user_ctx_id < 0) {
        // user_ctx_id contains an error code, return it.
        return user_ctx_id;
    }

    user_ctx = lib_ctx->GetUserContext(user_ctx_id);
    user_ctx->RemoveUser();

    s32 result = user_ctx->Initialize();
    if (result < 0) {
        // Failed to initialize user_ctx, destroy it.
        lib_ctx->RemoveUserContext(user_ctx_id);
        delete user_ctx;
    }

    lib_ctx->RemoveUser();
    return user_ctx_id;
}

s32 deleteUserContext(s32 user_ctx_id) {
    LibraryContext* lib_ctx = getLibraryContext(user_ctx_id >> 0x10);
    if (!lib_ctx) {
        LOG_ERROR(Lib_NpWebApi2, "No library context for user context id {:#x}", user_ctx_id);
        return ORBIS_NP_WEBAPI2_ERROR_LIB_CONTEXT_NOT_FOUND;
    }
    lib_ctx->Lock();

    UserContext* user_ctx = lib_ctx->GetUserContext(user_ctx_id);
    if (!user_ctx) {
        LOG_ERROR(Lib_NpWebApi2, "No user context with id {:#x}", user_ctx_id);
        lib_ctx->Unlock();
        lib_ctx->RemoveUser();
        return ORBIS_NP_WEBAPI2_ERROR_USER_CONTEXT_NOT_FOUND;
    }

    user_ctx->AbortAllRequests();
    if (user_ctx->IsDeleted()) {
        LOG_ERROR(Lib_NpWebApi2, "User context with id {:#x} is already deleted", user_ctx_id);
        user_ctx->RemoveUser();
        lib_ctx->Unlock();
        lib_ctx->RemoveUser();
        return ORBIS_NP_WEBAPI2_ERROR_USER_CONTEXT_NOT_FOUND;
    }

    user_ctx->MarkForDeletion();
    while (user_ctx->IsBusy() || user_ctx->HasBusyRequests()) {
        lib_ctx->Unlock();
        Libraries::Kernel::sceKernelUsleep(50000);
        lib_ctx->Lock();
    }

    user_ctx->Delete();
    lib_ctx->Unlock();
    lib_ctx->RemoveUser();
    return ORBIS_OK;
}

s32 createRequest(s32 user_ctx_id, const char* api_group, const char* path, const char* method,
                  const OrbisNpWebApi2ContentParameter* content_parameter, bool multipart,
                  s64* request_id) {
    LibraryContext* lib_ctx = getLibraryContext(user_ctx_id >> 0x10);
    if (!lib_ctx) {
        LOG_ERROR(Lib_NpWebApi2, "No library context for user context id {:#x}", user_ctx_id);
        return ORBIS_NP_WEBAPI2_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    UserContext* user_ctx = lib_ctx->GetUserContext(user_ctx_id);
    if (!user_ctx) {
        LOG_ERROR(Lib_NpWebApi2, "No user context with id {:#x}", user_ctx_id);
        lib_ctx->RemoveUser();
        return ORBIS_NP_WEBAPI2_ERROR_USER_CONTEXT_NOT_FOUND;
    }

    if (std::strlen(api_group) >= 63) {
        LOG_ERROR(Lib_NpWebApi2, "API group is too long");
        user_ctx->RemoveUser();
        lib_ctx->RemoveUser();
        return ORBIS_NP_WEBAPI2_ERROR_PARAMETER_TOO_LONG;
    }

    Request* request{};
    s32 result =
        user_ctx->CreateRequest(api_group, path, method, content_parameter, multipart, &request);
    if (result < 0) {
        // Request creation failed
        user_ctx->RemoveUser();
        lib_ctx->RemoveUser();
        if (request) {
            delete request;
        }
        return result;
    }

    if (request_id) {
        *request_id = request->GetId();
    }
    user_ctx->RemoveUser();
    lib_ctx->RemoveUser();
    return result;
}

s32 addHttpRequestHeader(s64 request_id, const char* field_name, const char* field_value) {
    s32 lib_ctx_id = static_cast<s32>(request_id >> 0x30);
    s32 user_ctx_id = static_cast<s32>(request_id >> 0x20);
    LibraryContext* lib_ctx = getLibraryContext(lib_ctx_id);
    if (!lib_ctx) {
        LOG_ERROR(Lib_NpWebApi2, "No library context for request id {:#x}", request_id);
        return ORBIS_NP_WEBAPI2_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    UserContext* user_ctx = lib_ctx->GetUserContext(user_ctx_id);
    if (!user_ctx) {
        LOG_ERROR(Lib_NpWebApi2, "No user context for request id {:#x}", request_id);
        lib_ctx->RemoveUser();
        return ORBIS_NP_WEBAPI2_ERROR_USER_CONTEXT_NOT_FOUND;
    }

    Request* request = user_ctx->GetRequest(request_id);
    if (!request) {
        LOG_ERROR(Lib_NpWebApi2, "No request with id {:#x}", request_id);
        user_ctx->RemoveUser();
        lib_ctx->RemoveUser();
        return ORBIS_NP_WEBAPI2_ERROR_REQUEST_NOT_FOUND;
    }

    if (request->HasSent()) {
        LOG_ERROR(Lib_NpWebApi2, "Request is already sent");
        request->RemoveUser();
        user_ctx->RemoveUser();
        lib_ctx->RemoveUser();
        return ORBIS_NP_WEBAPI2_ERROR_AFTER_SEND;
    }

    s32 result = request->AddHttpRequestHeader(field_name, field_value);
    request->RemoveUser();
    user_ctx->RemoveUser();
    lib_ctx->RemoveUser();
    return result;
}

s32 setRequestTimeout(s64 request_id, u32 timeout) {
    s32 lib_ctx_id = static_cast<s32>(request_id >> 0x30);
    s32 user_ctx_id = static_cast<s32>(request_id >> 0x20);
    LibraryContext* lib_ctx = getLibraryContext(lib_ctx_id);
    if (!lib_ctx) {
        LOG_ERROR(Lib_NpWebApi2, "No library context for request id {:#x}", request_id);
        return ORBIS_NP_WEBAPI2_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    UserContext* user_ctx = lib_ctx->GetUserContext(user_ctx_id);
    if (!user_ctx) {
        LOG_ERROR(Lib_NpWebApi2, "No user context for request id {:#x}", request_id);
        lib_ctx->RemoveUser();
        return ORBIS_NP_WEBAPI2_ERROR_USER_CONTEXT_NOT_FOUND;
    }

    Request* request = user_ctx->GetRequest(request_id);
    if (!request) {
        LOG_ERROR(Lib_NpWebApi2, "No request with id {:#x}", request_id);
        user_ctx->RemoveUser();
        lib_ctx->RemoveUser();
        return ORBIS_NP_WEBAPI2_ERROR_REQUEST_NOT_FOUND;
    }

    request->SetTimeout(timeout);
    request->RemoveUser();
    user_ctx->RemoveUser();
    lib_ctx->RemoveUser();
    return ORBIS_OK;
}

static s32 checkRequestStatus(Request* request, UserContext* user_ctx, LibraryContext* lib_ctx) {
    if (request->Expired()) {
        LOG_ERROR(Lib_NpWebApi2, "Request timed out");
        request->RemoveUser();
        user_ctx->RemoveUser();
        lib_ctx->RemoveUser();
        return ORBIS_NP_WEBAPI2_ERROR_TIMEOUT;
    }

    if (request->Aborted()) {
        LOG_ERROR(Lib_NpWebApi2, "Request aborted");
        request->RemoveUser();
        user_ctx->RemoveUser();
        lib_ctx->RemoveUser();
        return ORBIS_NP_WEBAPI2_ERROR_ABORTED;
    }
    return ORBIS_OK;
}

s32 sendRequest(s64 request_id, s32 part_index, void* data, u64 data_size,
                OrbisNpWebApi2ResponseInformationOption* resp_info_option) {
    s32 lib_ctx_id = static_cast<s32>(request_id >> 0x30);
    s32 user_ctx_id = static_cast<s32>(request_id >> 0x20);
    LibraryContext* lib_ctx = getLibraryContext(lib_ctx_id);
    if (!lib_ctx) {
        LOG_ERROR(Lib_NpWebApi2, "No library context for request id {:#x}", request_id);
        return ORBIS_NP_WEBAPI2_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    UserContext* user_ctx = lib_ctx->GetUserContext(user_ctx_id);
    if (!user_ctx) {
        LOG_ERROR(Lib_NpWebApi2, "No user context for request id {:#x}", request_id);
        lib_ctx->RemoveUser();
        return ORBIS_NP_WEBAPI2_ERROR_USER_CONTEXT_NOT_FOUND;
    }

    Request* request = user_ctx->GetRequest(request_id);
    if (!request) {
        LOG_ERROR(Lib_NpWebApi2, "No request with id {:#x}", request_id);
        user_ctx->RemoveUser();
        lib_ctx->RemoveUser();
        return ORBIS_NP_WEBAPI2_ERROR_REQUEST_NOT_FOUND;
    }

    request->SetEndTime();
    if (request->IsMultipart()) {
        if (part_index == 0) {
            LOG_ERROR(Lib_NpWebApi2, "part_index 0 is prohibited for multipart requests");
            request->RemoveUser();
            user_ctx->RemoveUser();
            lib_ctx->RemoveUser();
            return ORBIS_NP_WEBAPI2_ERROR_PROHIBITED_FUNCTION_CALL;
        }
        // TODO: Multipart logic
    }

    if (!request->HasSent()) {
        request->MarkSent();
    }

    request->Lock();
    request->SetState(0);
    s32 result = checkRequestStatus(request, user_ctx, lib_ctx);
    request->Unlock();
    if (result != 0) {
        // Aborted or timed out.
        return result;
    }

    if (!EmulatorSettings.IsShadNetEnabled()) {
        LOG_INFO(Lib_NpWebApi2, "Cannot send request, you are not signed in to shadNet");
        request->RemoveUser();
        user_ctx->RemoveUser();
        lib_ctx->RemoveUser();
        return ORBIS_NP_WEBAPI2_ERROR_NOT_SIGNED_IN;
    }

    if (request->GetHttpRequestId() == 0) {
        std::string base_url = EmulatorSettings.GetShadNetWebApiServer();
        s32 template_id = user_ctx->GetHttpTemplateId();
        // Technically this should be base_url + api_group + path,
        // but shadNet doesn't seem to use the api_group for endpoints?
        std::string full_url = base_url + request->GetPath();
        result = request->CreateHttpRequest(template_id, full_url.data());
        if (result < 0) {
            // Underlying request creation failed, return.
            request->RemoveUser();
            user_ctx->RemoveUser();
            lib_ctx->RemoveUser();
            return result;
        }
    }

    request->Lock();
    request->SetState(3);
    result = checkRequestStatus(request, user_ctx, lib_ctx);
    request->Unlock();
    if (result != 0) {
        // Aborted or timed out.
        return result;
    }

    if (request->IsMultipart()) {
        // TODO: Multipart sends
        // result = request->SendMultipartHttpRequest(data, data_size, part_index);
    } else {
        result = request->SendHttpRequest(data, data_size);
    }

    if (result >= 0 && request->OutOfData()) {
        result = request->GetAllHttpResponseHeaders();
        if (result >= 0) {
            s32 http_request_id = request->GetHttpRequestId();
            s32 http_status{};
            result = Libraries::Http2::sceHttp2GetStatusCode(http_request_id, &http_status);
            if (result >= 0) {
                if (resp_info_option) {
                    resp_info_option->http_status = http_status;
                }
                if (http_status >= 400) {
                    result = http_status | 0x82f00000;
                }
            } else {
                LOG_ERROR(Lib_NpWebApi2, "Failed to get http status code, error = {:#x}", result);
            }
        }
    }

    request->Lock();
    request->SetState(0);
    s32 result2 = checkRequestStatus(request, user_ctx, lib_ctx);
    request->Unlock();
    if (result2 != 0) {
        // Aborted or timed out.
        return result2;
    }

    request->ClearEndTime();
    request->RemoveUser();
    user_ctx->RemoveUser();
    lib_ctx->RemoveUser();
    return result;
}

s32 abortRequest(s64 request_id) {
    s32 lib_ctx_id = static_cast<s32>(request_id >> 0x30);
    s32 user_ctx_id = static_cast<s32>(request_id >> 0x20);
    LibraryContext* lib_ctx = getLibraryContext(lib_ctx_id);
    if (!lib_ctx) {
        LOG_ERROR(Lib_NpWebApi2, "No library context for request id {:#x}", request_id);
        return ORBIS_NP_WEBAPI2_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    UserContext* user_ctx = lib_ctx->GetUserContext(user_ctx_id);
    if (!user_ctx) {
        LOG_ERROR(Lib_NpWebApi2, "No user context for request id {:#x}", request_id);
        lib_ctx->RemoveUser();
        return ORBIS_NP_WEBAPI2_ERROR_USER_CONTEXT_NOT_FOUND;
    }

    Request* request = user_ctx->GetRequest(request_id);
    if (!request) {
        LOG_ERROR(Lib_NpWebApi2, "No request with id {:#x}", request_id);
        user_ctx->RemoveUser();
        lib_ctx->RemoveUser();
        return ORBIS_NP_WEBAPI2_ERROR_REQUEST_NOT_FOUND;
    }

    request->RemoveUser();
    s32 result = request->Abort();
    user_ctx->RemoveUser();
    lib_ctx->RemoveUser();
    return result;
}

s32 deleteRequest(s64 request_id) {
    s32 lib_ctx_id = static_cast<s32>(request_id >> 0x30);
    s32 user_ctx_id = static_cast<s32>(request_id >> 0x20);
    LibraryContext* lib_ctx = getLibraryContext(lib_ctx_id);
    if (!lib_ctx) {
        LOG_ERROR(Lib_NpWebApi2, "No library context for request id {:#x}", request_id);
        return ORBIS_NP_WEBAPI2_ERROR_LIB_CONTEXT_NOT_FOUND;
    }

    lib_ctx->Lock();
    UserContext* user_ctx = lib_ctx->GetUserContext(user_ctx_id);
    if (!user_ctx) {
        LOG_ERROR(Lib_NpWebApi2, "No user context for request id {:#x}", request_id);
        lib_ctx->Unlock();
        lib_ctx->RemoveUser();
        return ORBIS_NP_WEBAPI2_ERROR_USER_CONTEXT_NOT_FOUND;
    }

    Request* request = user_ctx->GetRequest(request_id);
    if (!request) {
        LOG_ERROR(Lib_NpWebApi2, "No request with id {:#x}", request_id);
        user_ctx->RemoveUser();
        lib_ctx->Unlock();
        lib_ctx->RemoveUser();
        return ORBIS_NP_WEBAPI2_ERROR_REQUEST_NOT_FOUND;
    }

    if (request->IsDeleted()) {
        // Already marked for deletion
        LOG_ERROR(Lib_NpWebApi2, "Request with id {:#x} is already being deleted", request_id);
        request->RemoveUser();
        user_ctx->RemoveUser();
        lib_ctx->Unlock();
        lib_ctx->RemoveUser();
        return ORBIS_NP_WEBAPI2_ERROR_REQUEST_NOT_FOUND;
    }

    request->MarkForDeletion();
    request->Abort();
    while (request->IsBusy()) {
        lib_ctx->Unlock();
        Libraries::Kernel::sceKernelUsleep(50000);
        lib_ctx->Lock();
    }
    user_ctx->RemoveRequest(request_id);
    request->RemoveUser();

    s32 http_request_id = 0;
    request->Delete(&http_request_id);

    user_ctx->RemoveUser();
    lib_ctx->Unlock();
    lib_ctx->RemoveUser();
    Libraries::Http2::sceHttp2DeleteRequest(http_request_id);
    return ORBIS_OK;
}

}; // namespace Libraries::Np::NpWebApi2