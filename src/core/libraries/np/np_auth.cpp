// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <chrono>
#include <cstring>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <httplib.h>
#include <nlohmann/json.hpp>
#include "common/logging/log.h"
#include "core/emulator_settings.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/np/np_auth.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_handler/np_handler.h"
#include "core/libraries/system/userservice.h"

namespace Libraries::Np::NpAuth {

static bool g_shadnet_enabled = false;
static std::mutex g_auth_request_mutex;
static constexpr s32 ORBIS_NP_AUTH_ISSUER_ID = 0x100;
static constexpr s32 ORBIS_NP_AUTH_SERVER_ERROR_BASE = static_cast<s32>(0x82E00000);
static constexpr auto ID_TOKEN_TYPE = "urn:ietf:params:oauth:token-type:id_token";

enum class NpAuthRequestState {
    Ready,
    Pending,
    Aborted,
    Complete,
};

struct IdTokenResult {
    s32 result = ORBIS_NP_AUTH_ERROR_SERVICE_DOWN;
    std::string token;
};

struct NpAuthRequest {
    NpAuthRequestState state = NpAuthRequestState::Ready;
    bool async = false;
    s32 result = ORBIS_OK;
    s32 user_id = -1;
    OrbisNpAccountId account_id = 0;
    OrbisNpIdToken* output = nullptr;
    std::shared_ptr<httplib::Client> client;
    std::shared_future<IdTokenResult> pending;
};

static std::vector<std::shared_ptr<NpAuthRequest>> g_auth_requests;

static std::shared_ptr<NpAuthRequest> FindRequest(s32 req_id) {
    const s64 index = static_cast<s64>(req_id) - ORBIS_NP_AUTH_REQUEST_ID_OFFSET - 1;
    if (index < 0 || static_cast<size_t>(index) >= g_auth_requests.size()) {
        return nullptr;
    }
    return g_auth_requests[index];
}

static void CompleteRequest(NpAuthRequest& request) {
    if (request.state != NpAuthRequestState::Pending ||
        request.pending.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
        return;
    }
    try {
        const auto& response = request.pending.get();
        request.result = response.result;
        const auto identity = NpHandler::GetInstance().GetAuthentication(request.user_id);
        if (request.result == ORBIS_OK &&
            (!identity || identity->account_id != request.account_id)) {
            request.result = ORBIS_NP_ERROR_SIGNED_OUT;
        }
        if (request.result == ORBIS_OK && request.output) {
            std::memcpy(request.output->token, response.token.data(), response.token.size());
        }
    } catch (const std::exception&) {
        request.result = ORBIS_NP_AUTH_ERROR_SERVICE_DOWN;
    }
    request.output = nullptr;
    request.state = NpAuthRequestState::Complete;
}

static IdTokenResult RequestIdToken(httplib::Client& client, const httplib::Params& params) {
    const auto response = client.Post("/2.0/oauth/token", params);
    if (!response) {
        LOG_ERROR(Lib_NpAuth, "ID token request failed: {}", httplib::to_string(response.error()));
        return {};
    }
    const auto json = nlohmann::json::parse(response->body, nullptr, false);
    if (response->status != 200) {
        LOG_ERROR(Lib_NpAuth, "ID token request returned HTTP {}", response->status);
        if (json.is_object() && json.contains("error") && json["error"].is_string()) {
            const auto error = json["error"].get<std::string>();
            if (error == "invalid_grant") {
                return {ORBIS_NP_ERROR_SIGNED_OUT, {}};
            }
            if (json.contains("error_code") && json["error_code"].is_number_unsigned()) {
                const auto code = json["error_code"].get<u64>();
                if (code > 0 && code <= 0xffff) {
                    return {ORBIS_NP_AUTH_SERVER_ERROR_BASE | static_cast<s32>(code), {}};
                }
            }
            if (error == "invalid_client" || error == "unauthorized_client") {
                return {ORBIS_NP_AUTH_ERROR_S_INVALID_CREDENTIAL, {}};
            }
            if (error == "invalid_request" || error == "invalid_scope" ||
                error == "unsupported_grant_type") {
                return {ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT, {}};
            }
        }
        return {response->status == 429 ? ORBIS_NP_AUTH_ERROR_SERVICE_BUSY
                                        : ORBIS_NP_AUTH_ERROR_SERVICE_DOWN,
                {}};
    }
    if (!json.is_object() || !json.contains("access_token") || !json["access_token"].is_string() ||
        json.value("issued_token_type", "") != ID_TOKEN_TYPE) {
        LOG_ERROR(Lib_NpAuth, "Token response has no ID token");
        return {ORBIS_NP_AUTH_ERROR_NO_TOKEN_RECEIVED, {}};
    }
    const auto token = json["access_token"].get<std::string>();
    if (token.empty() || token.size() >= sizeof(OrbisNpIdToken::token) ||
        token.find_first_not_of(
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.") !=
            std::string::npos) {
        LOG_ERROR(Lib_NpAuth, "Token response has an invalid ID token, size = {}", token.size());
        return {ORBIS_NP_AUTH_ERROR_NO_TOKEN_RECEIVED, {}};
    }
    return {ORBIS_OK, token};
}

s32 CreateNpAuthRequest(bool async) {
    std::scoped_lock lock{g_auth_request_mutex};
    size_t req_index = 0;
    while (req_index < g_auth_requests.size() && g_auth_requests[req_index]) {
        ++req_index;
    }
    if (req_index == ORBIS_NP_AUTH_REQUEST_LIMIT) {
        return ORBIS_NP_AUTH_ERROR_REQUEST_MAX;
    }
    auto request = std::make_shared<NpAuthRequest>();
    request->async = async;
    if (req_index == g_auth_requests.size()) {
        g_auth_requests.emplace_back(std::move(request));
    } else {
        g_auth_requests[req_index] = std::move(request);
    }
    LOG_DEBUG(Lib_NpAuth, "called, async = {}", async);
    return static_cast<s32>(req_index) + ORBIS_NP_AUTH_REQUEST_ID_OFFSET + 1;
}

s32 PS4_SYSV_ABI sceNpAuthCreateRequest() {
    return CreateNpAuthRequest(false);
}

s32 PS4_SYSV_ABI sceNpAuthCreateAsyncRequest(const OrbisNpAuthCreateAsyncRequestParameter* param) {
    if (param == nullptr) {
        return ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT;
    }
    if (param->size != sizeof(OrbisNpAuthCreateAsyncRequestParameter)) {
        return ORBIS_NP_AUTH_ERROR_INVALID_SIZE;
    }

    return CreateNpAuthRequest(true);
}

s32 GetAuthorizationCode(s32 req_id, const OrbisNpAuthGetAuthorizationCodeParameterA* param,
                         s32 flag, OrbisNpAuthorizationCode* auth_code, s32* issuer_id) {
    if (param == nullptr || auth_code == nullptr) {
        return ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT;
    }
    if (param->size != sizeof(OrbisNpAuthGetAuthorizationCodeParameter)) {
        return ORBIS_NP_AUTH_ERROR_INVALID_SIZE;
    }
    if (param->user_id == -1 || param->client_id == nullptr || param->scope == nullptr) {
        return ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT;
    }

    std::scoped_lock lock{g_auth_request_mutex};
    auto auth_request = FindRequest(req_id);
    if (!auth_request) {
        return ORBIS_NP_AUTH_ERROR_REQUEST_NOT_FOUND;
    }

    auto& request = *auth_request;
    if (request.state == NpAuthRequestState::Complete) {
        request.result = ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT;
        return ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT;
    } else if (request.state == NpAuthRequestState::Aborted) {
        request.result = ORBIS_NP_AUTH_ERROR_ABORTED;
        return ORBIS_NP_AUTH_ERROR_ABORTED;
    }
    if (request.state == NpAuthRequestState::Pending) {
        return ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT;
    }

    request.state = NpAuthRequestState::Complete;
    if (!g_shadnet_enabled) {
        request.result = ORBIS_NP_ERROR_SIGNED_OUT;
        // If the request is processed in some form, and it's an async request, then it returns OK.
        if (request.async) {
            return ORBIS_OK;
        }
        return ORBIS_NP_ERROR_SIGNED_OUT;
    }

    LOG_ERROR(Lib_NpAuth, "(STUBBED) called, req_id = {:#x}, async = {}", req_id, request.async);

    std::memset(auth_code, 0, sizeof(OrbisNpAuthorizationCode));
    std::strncpy(auth_code->code, "AUTHEN", 7);
    if (issuer_id != nullptr) {
        *issuer_id = ORBIS_NP_AUTH_ISSUER_ID;
    }
    request.result = ORBIS_OK;
    LOG_INFO(Lib_NpAuth, "req_id = {:#x}, async = {}, scope = '{}', code = '{}', issuer_id = {:#x}",
             req_id, request.async, param->scope ? param->scope : "(null)", auth_code->code,
             ORBIS_NP_AUTH_ISSUER_ID);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
sceNpAuthGetAuthorizationCode(s32 req_id, const OrbisNpAuthGetAuthorizationCodeParameter* param,
                              OrbisNpAuthorizationCode* auth_code, s32* issuer_id) {
    if (param == nullptr || auth_code == nullptr) {
        return ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT;
    }
    if (param->size != sizeof(OrbisNpAuthGetAuthorizationCodeParameter)) {
        return ORBIS_NP_AUTH_ERROR_INVALID_SIZE;
    }
    if (param->online_id == nullptr || param->client_id == nullptr || param->scope == nullptr) {
        return ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT;
    }
    if (!g_shadnet_enabled) {
        // Calls sceNpManagerIntGetUserIdByOnlineId to get a user id, returning any errors.
        // This call will not succeed while signed out because games cannot retrieve an online id.
        return ORBIS_NP_ERROR_USER_NOT_FOUND;
    }

    // For simplicity, call sceUserServiceGetInitialUser to get the user_id.
    s32 user_id = 0;
    ASSERT(UserService::sceUserServiceGetInitialUser(&user_id) == 0);

    // Library constructs an OrbisNpAuthGetAuthorizationCodeParameterA struct,
    // then uses that to call GetAuthorizationCode.
    OrbisNpAuthGetAuthorizationCodeParameterA internal_params;
    std::memset(&internal_params, 0, sizeof(internal_params));
    internal_params.size = sizeof(internal_params);
    internal_params.client_id = param->client_id;
    internal_params.user_id = user_id;
    internal_params.scope = param->scope;

    return GetAuthorizationCode(req_id, &internal_params, 0, auth_code, issuer_id);
}

s32 PS4_SYSV_ABI
sceNpAuthGetAuthorizationCodeA(s32 req_id, const OrbisNpAuthGetAuthorizationCodeParameterA* param,
                               OrbisNpAuthorizationCode* auth_code, s32* issuer_id) {
    return GetAuthorizationCode(req_id, param, 0, auth_code, issuer_id);
}

s32 PS4_SYSV_ABI
sceNpAuthGetAuthorizationCodeV3(s32 req_id, const OrbisNpAuthGetAuthorizationCodeParameterA* param,
                                OrbisNpAuthorizationCode* auth_code, s32* issuer_id) {
    return GetAuthorizationCode(req_id, param, 1, auth_code, issuer_id);
}

s32 GetIdToken(s32 req_id, const OrbisNpAuthGetIdTokenParameterA* param, s32 flag,
               OrbisNpIdToken* token) {
    if (param == nullptr || token == nullptr) {
        return ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT;
    }
    if (param->size != sizeof(OrbisNpAuthGetIdTokenParameterA)) {
        return ORBIS_NP_AUTH_ERROR_INVALID_SIZE;
    }
    if (param->user_id == -1 || param->client_id == nullptr || param->client_secret == nullptr ||
        param->scope == nullptr) {
        return ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT;
    }
    std::unique_lock lock{g_auth_request_mutex};
    auto request = FindRequest(req_id);
    if (!request) {
        return ORBIS_NP_AUTH_ERROR_REQUEST_NOT_FOUND;
    }
    if (request->state == NpAuthRequestState::Complete) {
        request->result = ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT;
        return request->result;
    }
    if (request->state == NpAuthRequestState::Aborted) {
        return ORBIS_NP_AUTH_ERROR_ABORTED;
    }
    if (request->state != NpAuthRequestState::Ready) {
        return ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT;
    }
    auto& handler = NpHandler::GetInstance();
    const auto identity = handler.GetAuthentication(param->user_id);
    if (!g_shadnet_enabled || !identity || identity->bearer.empty()) {
        request->state = NpAuthRequestState::Complete;
        request->result = ORBIS_NP_ERROR_SIGNED_OUT;
        return request->async ? ORBIS_OK : request->result;
    }
    if (!std::memchr(param->client_id->id, 0, sizeof(param->client_id->id)) ||
        !std::memchr(param->client_secret->secret, 0, sizeof(param->client_secret->secret)) ||
        !std::memchr(param->scope, 0, 1025) || !param->client_id->id[0] ||
        !param->client_secret->secret[0] || !param->scope[0]) {
        return ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT;
    }
    std::memset(token, 0, sizeof(*token));
    request->user_id = param->user_id;
    request->account_id = identity->account_id;
    request->output = token;
    try {
        httplib::Params params{
            {"grant_type", "urn:ietf:params:oauth:grant-type:token-exchange"},
            {"subject_token", identity->bearer},
            {"subject_token_type", "urn:ietf:params:oauth:token-type:access_token"},
            {"requested_token_type", ID_TOKEN_TYPE},
            {"client_id", param->client_id->id},
            {"client_secret", param->client_secret->secret},
            {"scope", param->scope}};
        if (flag == 1) {
            params.emplace("version", "3");
        }
        const auto endpoint = EmulatorSettings.GetShadNetWebApiServer();
        if (endpoint.empty()) {
            request->output = nullptr;
            request->state = NpAuthRequestState::Complete;
            request->result = ORBIS_NP_AUTH_ERROR_SERVICE_DOWN;
            LOG_ERROR(Lib_NpAuth, "WebAPI server address is empty");
            return request->async ? ORBIS_OK : request->result;
        }
        auto client = std::make_shared<httplib::Client>(endpoint);
        client->set_connection_timeout(5);
        client->set_read_timeout(10);
        client->set_write_timeout(5);
        client->set_payload_max_length(8192);
        request->client = client;
        request->pending = std::async(std::launch::async, [client, params = std::move(params)] {
                               return RequestIdToken(*client, params);
                           }).share();
        request->state = NpAuthRequestState::Pending;
    } catch (const std::exception&) {
        request->output = nullptr;
        request->state = NpAuthRequestState::Complete;
        request->result = ORBIS_NP_AUTH_ERROR_OUT_OF_MEMORY;
        return request->async ? ORBIS_OK : request->result;
    }
    if (request->async) {
        return ORBIS_OK;
    }
    const auto pending = request->pending;
    lock.unlock();
    pending.wait();
    lock.lock();
    CompleteRequest(*request);
    return request->result;
}

s32 PS4_SYSV_ABI sceNpAuthGetIdToken(s32 req_id, const OrbisNpAuthGetIdTokenParameter* param,
                                     OrbisNpIdToken* token) {
    if (param == nullptr || token == nullptr) {
        return ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT;
    }
    if (param->size != sizeof(OrbisNpAuthGetIdTokenParameter)) {
        return ORBIS_NP_AUTH_ERROR_INVALID_SIZE;
    }
    if (param->online_id == nullptr || param->client_id == nullptr ||
        param->client_secret == nullptr || param->scope == nullptr) {
        return ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT;
    }
    if (!g_shadnet_enabled) {
        // Calls sceNpManagerIntGetUserIdByOnlineId to get a user id, returning any errors.
        // This call will not succeed while signed out because games cannot retrieve an online id.
        return ORBIS_NP_ERROR_USER_NOT_FOUND;
    }

    const s32 user_id = NpHandler::GetInstance().GetUserIdByOnlineId(*param->online_id);
    if (user_id == -1) {
        return ORBIS_NP_ERROR_USER_NOT_FOUND;
    }

    // Library constructs an OrbisNpAuthGetIdTokenParameterA struct,
    // then uses that to call GetIdToken.
    OrbisNpAuthGetIdTokenParameterA internal_params;
    std::memset(&internal_params, 0, sizeof(internal_params));
    internal_params.size = sizeof(internal_params);
    internal_params.user_id = user_id;
    internal_params.client_id = param->client_id;
    internal_params.client_secret = param->client_secret;
    internal_params.scope = param->scope;

    return GetIdToken(req_id, &internal_params, 0, token);
}

s32 PS4_SYSV_ABI sceNpAuthGetIdTokenA(s32 req_id, const OrbisNpAuthGetIdTokenParameterA* param,
                                      OrbisNpIdToken* token) {
    return GetIdToken(req_id, param, 0, token);
}

s32 PS4_SYSV_ABI sceNpAuthGetIdTokenV3(s32 req_id, const OrbisNpAuthGetIdTokenParameterA* param,
                                       OrbisNpIdToken* token) {
    return GetIdToken(req_id, param, 1, token);
}

s32 PS4_SYSV_ABI sceNpAuthSetTimeout(s32 req_id, s32 resolve_retry, u32 resolve_timeout,
                                     u32 conn_timeout, u32 send_timeout, u32 recv_timeout) {
    LOG_ERROR(Lib_NpAuth, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpAuthAbortRequest(s32 req_id) {
    LOG_DEBUG(Lib_NpAuth, "called req_id = {:#x}", req_id);
    std::shared_ptr<httplib::Client> client;
    {
        std::scoped_lock lock{g_auth_request_mutex};
        auto request = FindRequest(req_id);
        if (!request) {
            return ORBIS_NP_AUTH_ERROR_REQUEST_NOT_FOUND;
        }
        CompleteRequest(*request);
        if (request->state == NpAuthRequestState::Complete) {
            return ORBIS_OK;
        }
        request->state = NpAuthRequestState::Aborted;
        request->result = ORBIS_NP_AUTH_ERROR_ABORTED;
        request->output = nullptr;
        client = request->client;
    }
    if (client) {
        client->stop();
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpAuthWaitAsync(s32 req_id, s32* result) {
    if (result == nullptr) {
        return ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT;
    }
    std::unique_lock lock{g_auth_request_mutex};
    auto request = FindRequest(req_id);
    if (!request) {
        return ORBIS_NP_AUTH_ERROR_REQUEST_NOT_FOUND;
    }
    if (!request->async || request->state == NpAuthRequestState::Ready) {
        return ORBIS_NP_AUTH_ERROR_INVALID_ID;
    }
    if (request->state == NpAuthRequestState::Pending) {
        const auto pending = request->pending;
        lock.unlock();
        pending.wait();
        lock.lock();
        CompleteRequest(*request);
    }
    *result = request->result;
    LOG_WARNING(Lib_NpAuth, "called req_id = {:#x}, returning result = {:#x}", req_id,
                static_cast<u32>(*result));
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpAuthPollAsync(s32 req_id, s32* result) {
    if (result == nullptr) {
        return ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT;
    }
    std::scoped_lock lock{g_auth_request_mutex};
    auto request = FindRequest(req_id);
    if (!request) {
        return ORBIS_NP_AUTH_ERROR_REQUEST_NOT_FOUND;
    }
    if (!request->async || request->state == NpAuthRequestState::Ready) {
        return ORBIS_NP_AUTH_ERROR_INVALID_ID;
    }
    CompleteRequest(*request);
    if (request->state == NpAuthRequestState::Pending) {
        return 1;
    }
    *result = request->result;
    LOG_WARNING(Lib_NpAuth, "called req_id = {:#x}, returning result = {:#x}", req_id,
                static_cast<u32>(*result));
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpAuthDeleteRequest(s32 req_id) {
    LOG_DEBUG(Lib_NpAuth, "called req_id = {:#x}", req_id);
    std::shared_ptr<NpAuthRequest> request;
    {
        std::scoped_lock lock{g_auth_request_mutex};
        request = FindRequest(req_id);
        if (!request) {
            return ORBIS_NP_AUTH_ERROR_REQUEST_NOT_FOUND;
        }
        request->state = NpAuthRequestState::Aborted;
        request->result = ORBIS_NP_AUTH_ERROR_ABORTED;
        request->output = nullptr;
        g_auth_requests[req_id - ORBIS_NP_AUTH_REQUEST_ID_OFFSET - 1].reset();
    }
    if (request->client) {
        request->client->stop();
    }
    if (request->pending.valid()) {
        request->pending.wait();
    }
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    g_shadnet_enabled = EmulatorSettings.IsShadNetEnabled();

    LIB_FUNCTION("6bwFkosYRQg", "libSceNpAuth", 1, "libSceNpAuth", sceNpAuthCreateRequest);
    LIB_FUNCTION("N+mr7GjTvr8", "libSceNpAuth", 1, "libSceNpAuth", sceNpAuthCreateAsyncRequest);
    LIB_FUNCTION("KxGkOrQJTqY", "libSceNpAuth", 1, "libSceNpAuth", sceNpAuthGetAuthorizationCode);
    LIB_FUNCTION("qAUXQ9GdWp8", "libSceNpAuth", 1, "libSceNpAuth", sceNpAuthGetAuthorizationCodeA);
    LIB_FUNCTION("KI4dHLlTNl0", "libSceNpAuth", 1, "libSceNpAuth", sceNpAuthGetAuthorizationCodeV3);
    LIB_FUNCTION("uaB-LoJqHis", "libSceNpAuth", 1, "libSceNpAuth", sceNpAuthGetIdToken);
    LIB_FUNCTION("CocbHVIKPE8", "libSceNpAuth", 1, "libSceNpAuth", sceNpAuthGetIdTokenA);
    LIB_FUNCTION("RdsFVsgSpZY", "libSceNpAuth", 1, "libSceNpAuth", sceNpAuthGetIdTokenV3);
    LIB_FUNCTION("PM3IZCw-7m0", "libSceNpAuth", 1, "libSceNpAuth", sceNpAuthSetTimeout);
    LIB_FUNCTION("cE7wIsqXdZ8", "libSceNpAuth", 1, "libSceNpAuth", sceNpAuthAbortRequest);
    LIB_FUNCTION("SK-S7daqJSE", "libSceNpAuth", 1, "libSceNpAuth", sceNpAuthWaitAsync);
    LIB_FUNCTION("gjSyfzSsDcE", "libSceNpAuth", 1, "libSceNpAuth", sceNpAuthPollAsync);
    LIB_FUNCTION("H8wG9Bk-nPc", "libSceNpAuth", 1, "libSceNpAuth", sceNpAuthDeleteRequest);

    LIB_FUNCTION("KxGkOrQJTqY", "libSceNpAuthCompat", 1, "libSceNpAuth",
                 sceNpAuthGetAuthorizationCode);
    LIB_FUNCTION("uaB-LoJqHis", "libSceNpAuthCompat", 1, "libSceNpAuth", sceNpAuthGetIdToken);
};

} // namespace Libraries::Np::NpAuth
