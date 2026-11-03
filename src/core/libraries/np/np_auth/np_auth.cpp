// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include "common/logging/log.h"
#include "core/emulator_settings.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/np/np_auth/np_auth.h"
#include "core/libraries/np/np_auth/np_auth_ctx.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_handler/np_handler.h"
#include "core/libraries/system/userservice.h"
#include "core/user_settings.h"

namespace Libraries::Np::NpAuth {

static bool g_shadnet_enabled = false;
static std::mutex g_auth_request_mutex;
static constexpr s32 ORBIS_NP_AUTH_POLL_ASYNC_RET_FINISHED = 0;
static constexpr s32 ORBIS_NP_AUTH_POLL_ASYNC_RET_RUNNING = 1;
static constexpr u32 ORBIS_NP_AUTH_TIMEOUT_NO_EFFECT = 0;

struct NpAuthRequest {
    bool async = false;
    bool aborted = false;
    std::shared_ptr<AuthRequestCtx> ctx;
};

static std::map<s32, NpAuthRequest> g_auth_requests;

s32 CreateNpAuthRequest(bool async) {
    std::scoped_lock lk{g_auth_request_mutex};
    if (g_auth_requests.size() == ORBIS_NP_AUTH_REQUEST_LIMIT) {
        LOG_ERROR(Lib_NpAuth, "request limit reached, cannot create new request");
        return ORBIS_NP_AUTH_ERROR_REQUEST_MAX;
    }

    s32 req_id = ORBIS_NP_AUTH_REQUEST_ID_OFFSET + 1;
    while (g_auth_requests.contains(req_id)) {
        req_id++;
    }
    g_auth_requests[req_id].async = async;
    LOG_DEBUG(Lib_NpAuth, "called, async = {}", async);
    return req_id;
}

s32 PS4_SYSV_ABI sceNpAuthCreateRequest() {
    return CreateNpAuthRequest(false);
}

s32 PS4_SYSV_ABI sceNpAuthCreateAsyncRequest(const OrbisNpAuthCreateAsyncRequestParameter* param) {
    if (param == nullptr) {
        LOG_ERROR(Lib_NpAuth, "called with null parameter");
        return ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT;
    }
    if (param->size != sizeof(OrbisNpAuthCreateAsyncRequestParameter)) {
        LOG_ERROR(Lib_NpAuth, "called with invalid parameter size: {}", param->size);
        return ORBIS_NP_AUTH_ERROR_INVALID_SIZE;
    }

    return CreateNpAuthRequest(true);
}

static s32 FindRequest(s32 req_id, NpAuthRequest** request) {
    if (req_id <= 0) {
        LOG_ERROR(Lib_NpAuth, "called with invalid request id: {}", req_id);
        return ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT;
    }
    const auto it = g_auth_requests.find(req_id);
    if (it == g_auth_requests.end()) {
        LOG_ERROR(Lib_NpAuth, "request not found: {}", req_id);
        return ORBIS_NP_AUTH_ERROR_REQUEST_NOT_FOUND;
    }
    *request = &it->second;
    return ORBIS_OK;
}

static s32 StartRequest(s32 req_id, bool* async, std::shared_ptr<AuthRequestCtx>* ctx) {
    std::scoped_lock lk{g_auth_request_mutex};
    NpAuthRequest* request = nullptr;
    if (const s32 error = FindRequest(req_id, &request); error != ORBIS_OK) {
        return error;
    }
    if (request->ctx) {
        LOG_ERROR(Lib_NpAuth, "request already started: {}", req_id);
        return ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT;
    }
    if (request->aborted) {
        LOG_ERROR(Lib_NpAuth, "request already aborted: {}", req_id);
        return ORBIS_NP_AUTH_ERROR_ABORTED;
    }
    request->ctx = std::make_shared<AuthRequestCtx>();
    *async = request->async;
    *ctx = request->ctx;
    return ORBIS_OK;
}

// Async requests report the outcome through PollAsync/WaitAsync, sync requests block for it.
static s32 FinishStartedRequest(bool async, const std::shared_ptr<AuthRequestCtx>& ctx) {
    return async ? ORBIS_OK : ctx->Wait();
}

s32 GetAuthorizationCode(s32 req_id, const OrbisNpAuthGetAuthorizationCodeParameterA* param,
                         s32 flag, OrbisNpAuthorizationCode* auth_code, s32* issuer_id) {
    if (param == nullptr || auth_code == nullptr) {
        LOG_ERROR(Lib_NpAuth, "called with null parameter");
        return ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT;
    }
    if (param->size != sizeof(OrbisNpAuthGetAuthorizationCodeParameterA)) {
        LOG_ERROR(Lib_NpAuth, "called with invalid parameter size: {}", param->size);
        return ORBIS_NP_AUTH_ERROR_INVALID_SIZE;
    }
    if (param->client_id == nullptr || param->scope == nullptr) {
        LOG_ERROR(Lib_NpAuth, "called with null parameter");
        return ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT;
    }
    if (UserManagement.GetUserByID(param->user_id) == nullptr) {
        LOG_ERROR(Lib_NpAuth, "user not found: {}", param->user_id);
        return ORBIS_NP_ERROR_USER_NOT_FOUND;
    }

    bool async = false;
    std::shared_ptr<AuthRequestCtx> ctx;
    if (const s32 error = StartRequest(req_id, &async, &ctx); error != ORBIS_OK) {
        return error;
    }

    if (!g_shadnet_enabled) {
        ctx->SetResult(ORBIS_NP_ERROR_SIGNED_OUT);
        return FinishStartedRequest(async, ctx);
    }

    const std::string client_id(param->client_id->id,
                                strnlen(param->client_id->id, sizeof(param->client_id->id)));
    const std::string scope = param->scope;
    LOG_INFO(Lib_NpAuth, "req_id = {:#x}, async = {}, client_id = '{}', scope = '{}'", req_id,
             async, client_id, scope);

    ctx->authCode = auth_code;
    ctx->issuerId = issuer_id;
    const s32 submit =
        NpHandler::GetInstance().RequestAuthorizationCode(param->user_id, client_id, scope, ctx);
    if (submit < 0) {
        ctx->SetResult(submit);
    }
    return FinishStartedRequest(async, ctx);
}

s32 PS4_SYSV_ABI
sceNpAuthGetAuthorizationCode(s32 req_id, const OrbisNpAuthGetAuthorizationCodeParameter* param,
                              OrbisNpAuthorizationCode* auth_code, s32* issuer_id) {
    if (param == nullptr || auth_code == nullptr) {
        LOG_ERROR(Lib_NpAuth, "called with null parameter");
        return ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT;
    }
    if (param->size != sizeof(OrbisNpAuthGetAuthorizationCodeParameter)) {
        LOG_ERROR(Lib_NpAuth, "called with invalid parameter size: {}", param->size);
        return ORBIS_NP_AUTH_ERROR_INVALID_SIZE;
    }
    if (param->online_id == nullptr || param->client_id == nullptr || param->scope == nullptr) {
        LOG_ERROR(Lib_NpAuth, "called with null parameter");
        return ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT;
    }
    if (!g_shadnet_enabled) {
        LOG_ERROR(Lib_NpAuth, "shadnet is not enabled , user not found");
        return ORBIS_NP_ERROR_USER_NOT_FOUND;
    }

    const s32 user_id = NpHandler::GetInstance().GetUserIdByOnlineId(*param->online_id);
    if (user_id < 0) {
        LOG_ERROR(Lib_NpAuth, "user not found for online_id");
        return ORBIS_NP_ERROR_USER_NOT_FOUND;
    }
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

s32 PS4_SYSV_ABI sceNpAuthSetTimeout(s32 req_id, s32 resolve_retry, u32 resolve_timeout,
                                     u32 conn_timeout, u32 send_timeout, u32 recv_timeout) {
    LOG_DEBUG(Lib_NpAuth,
              "called req_id = {:#x}, resolve_retry = {}, resolve_timeout = {}, conn_timeout = {}, "
              "send_timeout = {}, recv_timeout = {}",
              req_id, resolve_retry, resolve_timeout, conn_timeout, send_timeout, recv_timeout);
    constexpr u32 MinResolveTimeoutUs = 1000000;
    constexpr u32 MinTransferTimeoutUs = 10000000;
    const auto valid = [](u32 value, u32 min) {
        return value == ORBIS_NP_AUTH_TIMEOUT_NO_EFFECT || value >= min;
    };
    if (resolve_retry < 0 || !valid(resolve_timeout, MinResolveTimeoutUs) ||
        !valid(conn_timeout, MinTransferTimeoutUs) || !valid(send_timeout, MinTransferTimeoutUs) ||
        !valid(recv_timeout, MinTransferTimeoutUs) ||
        (resolve_retry == 0 && resolve_timeout == 0 && conn_timeout == 0 && send_timeout == 0 &&
         recv_timeout == 0)) {
        LOG_ERROR(Lib_NpAuth, "invalid timeout values");
        return ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT;
    }

    std::scoped_lock lk{g_auth_request_mutex};
    NpAuthRequest* request = nullptr;
    return FindRequest(req_id, &request);
}

s32 PS4_SYSV_ABI sceNpAuthAbortRequest(s32 req_id) {
    LOG_DEBUG(Lib_NpAuth, "called req_id = {:#x}", req_id);

    std::scoped_lock lk{g_auth_request_mutex};
    NpAuthRequest* request = nullptr;
    if (const s32 error = FindRequest(req_id, &request); error != ORBIS_OK) {
        return error;
    }
    if (request->ctx) {
        request->ctx->SetResult(ORBIS_NP_AUTH_ERROR_ABORTED);
    } else {
        request->aborted = true;
    }
    return ORBIS_OK;
}

static s32 FindAsyncRequest(s32 req_id, std::shared_ptr<AuthRequestCtx>* ctx) {
    std::scoped_lock lk{g_auth_request_mutex};
    NpAuthRequest* request = nullptr;
    if (const s32 error = FindRequest(req_id, &request); error != ORBIS_OK) {
        return error;
    }
    if (!request->async || !request->ctx) {
        return ORBIS_NP_AUTH_ERROR_INVALID_ID;
    }
    *ctx = request->ctx;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpAuthWaitAsync(s32 req_id, s32* result) {
    if (result == nullptr) {
        return ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT;
    }

    std::shared_ptr<AuthRequestCtx> ctx;
    if (const s32 error = FindAsyncRequest(req_id, &ctx); error != ORBIS_OK) {
        return error;
    }
    *result = ctx->Wait();
    LOG_DEBUG(Lib_NpAuth, "called req_id = {:#x}, returning result = {:#x}", req_id,
              static_cast<u32>(*result));
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpAuthPollAsync(s32 req_id, s32* result) {
    if (result == nullptr) {
        LOG_ERROR(Lib_NpAuth, "called with null result pointer");
        return ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT;
    }

    std::shared_ptr<AuthRequestCtx> ctx;
    if (const s32 error = FindAsyncRequest(req_id, &ctx); error != ORBIS_OK) {
        return error;
    }
    std::scoped_lock lock{ctx->mutex};
    if (!ctx->result.has_value()) {
        return ORBIS_NP_AUTH_POLL_ASYNC_RET_RUNNING;
    }
    *result = *ctx->result;
    LOG_DEBUG(Lib_NpAuth, "called req_id = {:#x}, returning result = {:#x}", req_id,
              static_cast<u32>(*result));
    return ORBIS_NP_AUTH_POLL_ASYNC_RET_FINISHED;
}

s32 PS4_SYSV_ABI sceNpAuthDeleteRequest(s32 req_id) {
    LOG_DEBUG(Lib_NpAuth, "called req_id = {:#x}", req_id);

    std::scoped_lock lk{g_auth_request_mutex};
    NpAuthRequest* request = nullptr;
    if (const s32 error = FindRequest(req_id, &request); error != ORBIS_OK) {
        return error;
    }
    if (request->ctx) {
        request->ctx->SetResult(ORBIS_NP_AUTH_ERROR_ABORTED);
    }
    g_auth_requests.erase(req_id);
    return ORBIS_OK;
}

// TODO  token code taken from old np auth implementation, needs to be reworked to use shadnet and
// np handler
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

    bool async = false;
    std::shared_ptr<AuthRequestCtx> ctx;
    if (const s32 error = StartRequest(req_id, &async, &ctx); error != ORBIS_OK) {
        return error;
    }

    if (!g_shadnet_enabled) {
        ctx->SetResult(ORBIS_NP_ERROR_SIGNED_OUT);
        return FinishStartedRequest(async, ctx);
    }

    LOG_ERROR(Lib_NpAuth, "(STUBBED) called, req_id = {:#x}, async = {}", req_id, async);

    // Not sure what values are expected here, so zeroing this for now.
    std::memset(token, 0, sizeof(OrbisNpIdToken));
    std::strncpy(token->token, "TOKEN", 6);
    ctx->SetResult(ORBIS_OK);
    return FinishStartedRequest(async, ctx);
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

    // For simplicity, call sceUserServiceGetInitialUser to get the user_id.
    s32 user_id = 0;
    ASSERT(UserService::sceUserServiceGetInitialUser(&user_id) == 0);

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