// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/network/http2.h"
#include "core/libraries/np/np_common.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_handler.h"
#include "core/libraries/np/np_web_api2/np_web_api2_context.h"

namespace Libraries::Np::NpWebApi2 {

s32 g_current_user_context_id{};
s64 g_current_request_id{};

s32 LibraryContext::CreateUserContext(Libraries::UserService::OrbisUserServiceUserId user_id) {
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

UserContext* LibraryContext::GetUserContext(s32 user_ctx_id) {
    std::scoped_lock lk{this->lock};
    if (!this->user_contexts.contains(user_ctx_id)) {
        return nullptr;
    }

    UserContext* user_ctx = this->user_contexts.at(user_ctx_id);
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
    s32 http_template_id = Libraries::Http2::sceHttp2CreateTemplate(
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

    // TODO: sceNpPush2 interactions
    return result;
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
        actual_request_id = (static_cast<s64>(this->id) << 0x20) | g_current_user_context_id;
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
        return nullptr;
    }

    Request* request = this->requests[request_id];
    request->AddUser();
    this->Unlock();
    return request;
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

}; // namespace Libraries::Np::NpWebApi2