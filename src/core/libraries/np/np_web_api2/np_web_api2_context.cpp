// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/network/http.h"
#include "core/libraries/network/http2.h"
#include "core/libraries/np/np_common.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_handler.h"
#include "core/libraries/np/np_web_api2/np_web_api2_context.h"

namespace Libraries::Np::NpWebApi2 {

u32 g_current_push_event_handle_id{};
u32 g_current_push_event_filter_id{};
u32 g_current_push_event_callback_id{};
u32 g_current_push_context_callback_id{};
u32 g_current_user_context_id{};
u64 g_current_request_id{};

void LibraryContext::CheckTimeout() {
    u64 time = Libraries::Kernel::sceKernelGetProcessTime();
    std::scoped_lock lk{this->lock};
    for (auto& [user_ctx_id, user_ctx] : this->user_contexts) {
        user_ctx->CheckTimeout();
    }
    for (auto& [handle_id, handle] : this->push_event_handles) {
        handle->CheckTimeout(time);
    }
}

s32 LibraryContext::CreateUserContext(Libraries::UserService::OrbisUserServiceUserId user_id) {
    std::scoped_lock lk{this->lock};
    if (this->user_contexts.size() >= 0x10000) {
        LOG_ERROR(Lib_NpWebApi2, "Too many user contexts");
        return ORBIS_NP_WEBAPI2_ERROR_USER_CONTEXT_MAX;
    }

    s32 actual_user_ctx_id = 0;
    do {
        g_current_user_context_id++;
        if (g_current_user_context_id >= 0x10000) {
            g_current_user_context_id = 1;
        }
        actual_user_ctx_id = (this->id << 0x10) | g_current_user_context_id;
    } while (this->user_contexts.contains(actual_user_ctx_id));

    this->user_contexts[actual_user_ctx_id] = new UserContext(this, actual_user_ctx_id, user_id);
    return actual_user_ctx_id;
}

s32 LibraryContext::CreatePushEventHandle() {
    std::scoped_lock lk{this->lock};
    if (g_current_push_event_handle_id + 1 < 0xf0000000) {
        ++g_current_push_event_handle_id;
    } else {
        g_current_push_event_handle_id = 1;
    }

    s32 new_handle_id = g_current_push_event_handle_id;
    this->push_event_handles[new_handle_id] = new PushEventHandle(new_handle_id);
    return new_handle_id;
}

PushEventHandle* LibraryContext::GetPushEventHandle(s32 handle_id) {
    std::scoped_lock lk{lock};
    if (!push_event_handles.contains(handle_id)) {
        return nullptr;
    }
    PushEventHandle* handle = push_event_handles[handle_id];
    handle->AddUser();
    return handle;
}

s32 LibraryContext::DeletePushEventHandle(PushEventHandle* handle) {
    this->Lock();
    s32 handle_id = handle->GetId();
    if (handle->IsDeleted()) {
        this->Unlock();
        LOG_ERROR(Lib_NpWebApi2, "handle with id {:#x} is already deleted", handle_id);
        return ORBIS_NP_WEBAPI2_ERROR_HANDLE_NOT_FOUND;
    }

    handle->MarkForDeletion();
    handle->Abort();

    while (handle->IsBusy()) {
        handle->RemoveUser();
        this->Unlock();
        Libraries::Kernel::sceKernelUsleep(50000);
        this->Lock();
        handle->AddUser();
    }

    if (push_event_handles.contains(handle_id)) {
        push_event_handles.erase(handle_id);
    }
    delete handle;

    return ORBIS_OK;
}

s32 LibraryContext::CreatePushEventFilter(
    s32 handle_id, const char* np_service_name, OrbisNpServiceLabel np_service_label,
    const OrbisNpWebApi2PushEventFilterParameter* filter_param, u64 filter_param_num,
    bool internal) {
    std::scoped_lock lk{this->lock};
    if (!this->push_event_handles.contains(handle_id)) {
        LOG_ERROR(Lib_NpWebApi2, "No push event handle with id {:#x}", handle_id);
        return ORBIS_NP_WEBAPI2_ERROR_HANDLE_NOT_FOUND;
    }
    PushEventHandle* handle = this->push_event_handles[handle_id];
    handle->AddUser();

    if (g_current_push_event_filter_id + 1 < 0xf0000000) {
        ++g_current_push_event_filter_id;
    } else {
        g_current_push_event_filter_id = 1;
    }

    s32 new_filter_id = g_current_push_event_filter_id;
    this->push_event_filters[new_filter_id] =
        new PushEventFilter(new_filter_id, np_service_name, np_service_label, internal);
    PushEventFilter* filter = this->push_event_filters[new_filter_id];
    s32 result = filter->Initialize(handle, filter_param, filter_param_num);
    if (result < 0) {
        // Failed to initialize filter.
        delete filter;
        this->push_event_filters.erase(new_filter_id);
        handle->RemoveUser();
        return result;
    }
    handle->RemoveUser();
    return new_filter_id;
}

UserContext* LibraryContext::GetUserContext(s32 user_ctx_id) {
    std::scoped_lock lk{this->lock};
    if (!this->user_contexts.contains(user_ctx_id)) {
        return nullptr;
    }

    UserContext* user_ctx = this->user_contexts.at(user_ctx_id);
    if (user_ctx->IsDeleted()) {
        return nullptr;
    }
    user_ctx->AddUser();
    return user_ctx;
}

UserContext* LibraryContext::GetUserContextByUserId(
    Libraries::UserService::OrbisUserServiceUserId user_id) {
    if (user_id == Libraries::UserService::ORBIS_USER_SERVICE_USER_ID_INVALID) {
        return nullptr;
    }

    std::scoped_lock lk{this->lock};
    for (auto& [user_ctx_id, user_ctx] : this->user_contexts) {
        if (user_ctx->GetUserId() == user_id) {
            user_ctx->AddUser();
            return user_ctx;
        }
    }

    return nullptr;
}

s32 PS4_SYSV_ABI internalPreSendCallback(s32 http_request_id, s32 ssl_id,
                                         Libraries::Http2::OrbisHttp2PreSendCallbackData* data,
                                         void* user_arg) {
    s64 user_ctx_id = reinterpret_cast<s64>(user_arg);
    LOG_ERROR(Lib_NpWebApi2,
              "Unimplemented, http_request_id = {:#x}, ssl_id = {:#x}, user_ctx_id = {:#x}",
              http_request_id, ssl_id, user_ctx_id);
    return ORBIS_OK;
}

s32 UserContext::Initialize() {
    const char* name = this->parent_ctx->GetName();

    char sw_version[8];
    std::memset(sw_version, 0, sizeof(sw_version));
    Libraries::Np::NpCommon::sceNpGetSdkVersion(sw_version);

    char user_agent_buf[0x40];
    std::memset(user_agent_buf, 0, sizeof(user_agent_buf));
    if (name) {
        std::snprintf(user_agent_buf, sizeof(user_agent_buf), "NpWebApi2/%s (%s)", sw_version,
                      name);
    } else {
        std::snprintf(user_agent_buf, sizeof(user_agent_buf), "NpWebApi2/%s", sw_version);
    }
    this->user_agent = std::string{user_agent_buf};

    s32 http_ctx_id = this->parent_ctx->GetHttpCtxId();
    http_template_id = Libraries::Http2::sceHttp2CreateTemplate(
        http_ctx_id, user_agent_buf,
        Libraries::Http2::OrbisHttp2HttpVersion::ORBIS_HTTP2_VERSION_2_0, 0);
    if (http_template_id < 0) {
        LOG_ERROR(Lib_NpWebApi2, "Failed to create HTTP template, error = {:#x}", http_template_id);
        return http_template_id;
    }

    s32 result = Libraries::Http2::sceHttp2SetPreSendCallback(
        http_template_id, internalPreSendCallback, reinterpret_cast<void*>(this->id));
    if (result < 0) {
        LOG_ERROR(Lib_NpWebApi2, "Failed to set pre-send callback, error = {:#x}", result);
        return result;
    }

    return result;
}

void UserContext::CheckTimeout() {
    parent_ctx->Lock();
    for (auto& [request_id, request] : requests) {
        request->CheckTimeout();
    }
    parent_ctx->Unlock();
}

s32 UserContext::CreatePushEventCallback(s32 filter_id, OrbisNpWebApi2PushEventCallback cb_func,
                                         void* user_arg) {
    this->Lock();
    if (g_current_push_event_callback_id + 1 < 0xf0000000) {
        ++g_current_push_event_callback_id;
    } else {
        g_current_push_event_callback_id = 1;
    }

    s32 new_callback_id = g_current_push_event_callback_id;
    this->push_event_callbacks[new_callback_id] =
        new PushEventCallback(new_callback_id, filter_id, cb_func, user_arg, false);
    this->Unlock();
    return new_callback_id;
}

s32 UserContext::DeletePushEventCallback(s32 callback_id) {
    this->Lock();
    if (!this->push_event_callbacks.contains(callback_id)) {
        this->Unlock();
        LOG_ERROR(Lib_NpWebApi2, "No push event callback with id {:#x}", callback_id);
        return ORBIS_NP_WEBAPI2_ERROR_PUSH_EVENT_CALLBACK_NOT_FOUND;
    }

    PushEventCallback* callback = this->push_event_callbacks[callback_id];
    while (callback->is_busy) {
        this->Unlock();
        Libraries::Kernel::sceKernelUsleep(100000);
        this->Lock();
    }

    delete callback;
    this->push_event_callbacks.erase(callback_id);
    this->Unlock();
    return ORBIS_OK;
}

void UserContext::CreatePushContext(OrbisNpWebApi2PushEventPushContextId* push_ctx_id) {
    this->Lock();
    while (this->push_contexts_busy) {
        this->Unlock();
        Libraries::Kernel::sceKernelUsleep(10000);
        this->Lock();
    }
    this->push_contexts_busy = true;
    PushEventPushContext* push_ctx = new PushEventPushContext(5000, this);
    push_ctx->Initialize();
    OrbisNpWebApi2PushEventPushContextId* new_id = push_ctx->GetId();
    std::memcpy(push_ctx_id, new_id, sizeof(*push_ctx_id));

    s64 raw_id = push_ctx->GetFakeId();
    this->push_contexts[raw_id] = push_ctx;
    this->push_contexts_busy = false;
    this->Unlock();
}

PushEventPushContext* UserContext::GetPushContext(
    const OrbisNpWebApi2PushEventPushContextId* push_ctx_id) {
    parent_ctx->Lock();
    s64 raw_id{};
    std::memcpy(&raw_id, push_ctx_id, sizeof(s64));
    if (!push_contexts.contains(raw_id)) {
        return nullptr;
    }
    PushEventPushContext* push_ctx = push_contexts[raw_id];
    parent_ctx->Unlock();
    return push_ctx;
}

void UserContext::DeletePushContext(PushEventPushContext* push_ctx) {
    this->Lock();
    while (this->push_contexts_busy) {
        this->Unlock();
        Libraries::Kernel::sceKernelUsleep(10000);
        this->Lock();
    }
    this->push_contexts_busy = true;

    while (push_ctx->CallbackRunning()) {
        this->Unlock();
        Libraries::Kernel::sceKernelUsleep(100000);
        this->Lock();
    }

    s64 raw_id = push_ctx->GetFakeId();
    this->push_contexts.erase(raw_id);
    delete push_ctx;
    this->push_contexts_busy = false;
    this->Unlock();
}

s32 UserContext::CreatePushContextCallback(s32 filter_id,
                                           OrbisNpWebApi2PushEventPushContextCallback cb_func,
                                           void* user_arg) {
    this->Lock();
    if (g_current_push_context_callback_id + 1 < 0xf0000000) {
        ++g_current_push_context_callback_id;
    } else {
        g_current_push_context_callback_id = 1;
    }

    s32 new_callback_id = g_current_push_context_callback_id;
    this->push_context_callbacks[new_callback_id] =
        new PushEventPushContextCallback(new_callback_id, filter_id, cb_func, user_arg, false);
    this->Unlock();
    return new_callback_id;
}

s32 UserContext::DeletePushContextCallback(s32 callback_id) {
    this->Lock();
    if (!this->push_context_callbacks.contains(callback_id)) {
        this->Unlock();
        LOG_ERROR(Lib_NpWebApi2, "No push event push context callback with id {:#x}", callback_id);
        return ORBIS_NP_WEBAPI2_ERROR_PUSH_EVENT_CALLBACK_NOT_FOUND;
    }

    PushEventPushContextCallback* callback = this->push_context_callbacks[callback_id];
    while (callback->is_busy) {
        this->Unlock();
        Libraries::Kernel::sceKernelUsleep(100000);
        this->Lock();
    }

    delete callback;
    this->push_context_callbacks.erase(callback_id);
    this->Unlock();
    return ORBIS_OK;
}

s32 UserContext::CreateRequest(const char* api_group, const char* path, const char* method,
                               const OrbisNpWebApi2ContentParameter* content_parameter,
                               bool multipart, Request** request) {
    this->Lock();
    s64 actual_request_id = 0;
    do {
        g_current_request_id++;
        if (g_current_request_id >> 0x20 != 0) {
            g_current_request_id = 1;
        }
        actual_request_id = (static_cast<s64>(this->id) << 0x20) | g_current_request_id;
    } while (this->requests.contains(actual_request_id));

    if (content_parameter) {
        this->requests[actual_request_id] =
            new Request(this->parent_ctx, actual_request_id, api_group, path, method, multipart,
                        content_parameter);
    } else {
        this->requests[actual_request_id] =
            new Request(this->parent_ctx, actual_request_id, api_group, path, method, multipart);
    }
    *request = this->requests[actual_request_id];
    this->Unlock();
    return ORBIS_OK;
}

Request* UserContext::GetRequest(s64 request_id) {
    this->Lock();
    if (!this->requests.contains(request_id)) {
        this->Unlock();
        return nullptr;
    }

    Request* request = this->requests[request_id];
    request->AddUser();
    this->Unlock();
    return request;
}

bool UserContext::HasBusyRequests() {
    this->Lock();
    for (auto& [request_id, request] : this->requests) {
        request->AddUser();
        bool busy = request->IsBusy();
        request->RemoveUser();
        if (busy) {
            this->Unlock();
            return true;
        }
    }
    this->Unlock();
    return false;
}

void UserContext::AbortAllRequests() {
    this->Lock();
    for (auto& [request_id, request] : this->requests) {
        request->Abort();
    }
    this->Unlock();
}

void UserContext::Delete() {
    if (this->http_template_id != 0) {
        Libraries::Http2::sceHttp2DeleteTemplate(this->http_template_id);
    }
    this->parent_ctx->RemoveUserContext(this->id);
    this->RemoveUser();

    for (auto& [request_id, request] : this->requests) {
        request->Delete(nullptr);
    }
    this->requests.clear();

    for (auto& [push_ctx_id, push_ctx] : this->push_contexts) {
        delete push_ctx;
    }
    this->push_contexts.clear();

    for (auto& [callback_id, callback] : this->push_event_callbacks) {
        delete callback;
    }
    this->push_event_callbacks.clear();

    for (auto& [callback_id, callback] : this->push_context_callbacks) {
        delete callback;
    }
    this->push_context_callbacks.clear();
    delete this;
}

static bool IsInternalHeader(const char* header_name) {
    std::string lower_name{header_name};
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(),
                   [](char c) { return std::tolower(c); });
    // These headers are all skipped if the request's library context
    // was not created through the IntInitialize functions.
    return lower_name.compare("content-type") == 0 || lower_name.compare("user-agent") == 0 ||
           lower_name.compare("authorization") == 0 ||
           lower_name.compare("x-psn-np-title-id") == 0 ||
           lower_name.compare("x-psn-np-title-token") == 0 ||
           lower_name.compare("x-psn-request-id") == 0 ||
           lower_name.compare("x-psn-sdk-ver") == 0 ||
           lower_name.compare("x-psn-debug-settings") == 0 || lower_name.compare("npdebug") == 0 ||
           lower_name.compare("x-psn-trc-check-notification-enabled") == 0 ||
           lower_name.compare("x-psn-webtrace-enabled") == 0 ||
           lower_name.compare("x-psn-npwebapi2-context") == 0 ||
           lower_name.compare("x-psn-fake-rate-limit-enabled") == 0 ||
           lower_name.compare("x-psn-fake-rate-limit-targets") == 0;
}

s32 Request::AddHttpRequestHeader(const char* field_name, const char* field_value) {
    if (!this->parent_ctx->IsInternal() && IsInternalHeader(field_name)) {
        LOG_WARNING(Lib_NpWebApi2, "This is not an internal context, skipping field name {}",
                    field_name);
        return ORBIS_OK;
    }

    this->Lock();
    this->http_headers.emplace_back(new HttpRequestHeader(field_name, field_value));
    this->Unlock();
    return ORBIS_OK;
}

s32 Request::CreateHttpRequest(s32 http_template_id, const char* url) {
    const char* method = this->method.empty() ? "UNKNOWN" : this->method.data();
    s32 http_request_id = Libraries::Http2::sceHttp2CreateRequestWithURL(http_template_id, method,
                                                                         url, this->content_length);
    if (http_request_id < 0) {
        LOG_ERROR(Lib_NpWebApi2, "Failed to create Http2 request, error = {:#x}", http_request_id);
        return http_request_id;
    }
    this->http_request_id = http_request_id;
    s32 result = Libraries::Http2::sceHttp2AddRequestHeader(http_request_id, "Content-Type",
                                                            this->content_type.data(), 0);
    if (result < 0) {
        LOG_ERROR(Lib_NpWebApi2, "Failed to add Content-Type request header, error = {:#x}",
                  result);
        return result;
    }

    UserContext* user_ctx = this->parent_ctx->GetUserContext(this->id >> 0x20);
    const std::string bearer = NpHandler::GetInstance().GetBearerToken(user_ctx->GetUserId());
    if (!bearer.empty()) {
        const std::string auth_value = "Bearer " + bearer;
        result = Libraries::Http2::sceHttp2AddRequestHeader(http_request_id, "Authorization",
                                                            auth_value.data(), 0);
    }
    if (bearer.empty() || result < 0) {
        LOG_WARNING(Lib_NpWebApi2, "Failed to add Authorization request header");
    }

    // This would go on to add several other PSN-specific headers.
    // As shadNet does not appear to use them, the logic is skipped for now.

    for (HttpRequestHeader* header : this->http_headers) {
        s32 result = Libraries::Http2::sceHttp2AddRequestHeader(
            http_request_id, header->field_name.data(), header->field_value.data(), 0);
        if (result < 0) {
            LOG_ERROR(Lib_NpWebApi2,
                      "Failed to add request header, name = {}, value = {}, error = {:#x}",
                      header->field_name.data(), header->field_value.data(), result);
            user_ctx->RemoveUser();
            return result;
        }
    }
    user_ctx->RemoveUser();
    return result;
}

s32 Request::SendHttpRequest(void* data, u64 data_size) {
    s32 result = 0;
    if (this->content_length != 0 && this->sent_data == 0) {
        // Real library seems to do some calculations, some parts including IPC calls.
        // Not entirely sure how accurate this is in practice.
        result = Libraries::Http2::sceHttp2SetRequestContentLength(this->http_request_id,
                                                                   this->content_length);
        if (result < 0) {
            LOG_ERROR(Lib_NpWebApi2, "Failed to set content length, error = {:#x}", result);
            return result;
        }
    }
    if (!data || data_size == 0 || this->content_length == 0) {
        result = Libraries::Http2::sceHttp2SendRequest(this->http_request_id, nullptr, 0);
        if (result < 0) {
            LOG_ERROR(Lib_NpWebApi2, "Failed to send request, error = {:#x}", result);
            return result;
        }
    }

    u64 actual_size = std::min<u64>(data_size, this->content_length - this->sent_data);
    if (this->content_length <= this->sent_data) {
        // Nothing left to send
        return result;
    }
    result = Libraries::Http2::sceHttp2SendRequest(this->http_request_id, data, actual_size);
    if (result < 0) {
        LOG_ERROR(Lib_NpWebApi2, "Failed to send request, error = {:#x}", result);
        return result;
    }
    this->sent_data += actual_size;
    return result;
}

s32 Request::GetAllHttpResponseHeaders() {
    char* header = nullptr;
    u64 size = 0;
    s32 result =
        Libraries::Http2::sceHttp2GetAllResponseHeaders(this->http_request_id, &header, &size);
    if (result < 0) {
        LOG_ERROR(Lib_NpWebApi2, "Failed to get response headers, error = {:#x}", result);
        return result;
    }
    this->http_response_headers = header;
    this->http_response_header_size = size;
    return result;
}

s32 Request::ParseHttpResponseHeaders(const char* field_name, char* value, u64 value_size,
                                      u64* value_size_out) {
    const char* temp_val{};
    u64 temp_size{};
    s32 result = Libraries::Http::sceHttpParseResponseHeader(this->http_response_headers,
                                                             this->http_response_header_size,
                                                             field_name, &temp_val, &temp_size);
    if (result >= 0) {
        if (value) {
            std::memset(value, 0, value_size);
            u64 size = std::min<u64>(value_size, temp_size);
            std::memcpy(value, temp_val, size);
        }
        if (value_size_out) {
            *value_size_out = temp_size;
        }
        result = ORBIS_OK;
    }
    return result;
}

s32 Request::Abort() {
    this->Lock();
    s32 result = 0;
    if (this->Aborted()) {
        // Nothing to do.
        this->Unlock();
        return result;
    }
    this->aborted = true;

    // Real library has multiple states, we don't since our logic is simpler.
    if (this->send_state == 3) {
        // In the middle of an Http request, that needs aborting.
        result = Libraries::Http2::sceHttp2AbortRequest(this->http_request_id);
        if (result < 0) {
            LOG_ERROR(Lib_NpWebApi2, "Failed to abort http request, error = {:#x}", result);
        }
    }
    this->Unlock();
    return result;
}

s32 Request::Delete(s32* http_request) {
    for (HttpRequestHeader* header : this->http_headers) {
        delete header;
    }
    this->http_headers.clear();

    if (!http_request && this->http_request_id) {
        Libraries::Http2::sceHttp2DeleteRequest(this->http_request_id);
    } else {
        *http_request = this->http_request_id;
    }
    while (this->user_count > 0) {
        Libraries::Kernel::sceKernelUsleep(50000);
    }
    delete this;
    return ORBIS_OK;
}

}; // namespace Libraries::Np::NpWebApi2