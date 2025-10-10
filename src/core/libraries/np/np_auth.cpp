// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <mutex>
#include "common/config.h"
#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/np/np_auth.h"
#include "core/libraries/np/np_auth_error.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/system/userservice.h"

namespace Libraries::Np::NpAuth {

static bool g_signed_in = false;
static s32 g_active_auth_requests = 0;
static std::mutex g_auth_request_mutex;

// Internal types for storing request-related information
enum class NpAuthRequestState {
    None = 0,
    Ready = 1,
    Aborted = 2,
    Complete = 3,
};

struct NpAuthRequest {
    NpAuthRequestState state;
    bool async;
    s32 result;
};

static std::vector<NpAuthRequest> g_auth_requests;

s32 CreateNpAuthRequest(bool async) {
    if (g_active_auth_requests == ORBIS_NP_AUTH_REQUEST_LIMIT) {
        return ORBIS_NP_AUTH_ERROR_REQUEST_MAX;
    }

    std::scoped_lock lk{g_auth_request_mutex};

    s32 req_index = 0;
    while (req_index < g_auth_requests.size()) {
        // Find first nonexistant request
        if (g_auth_requests[req_index].state == NpAuthRequestState::None) {
            // There is no request at this index, set the index to ready then break.
            g_auth_requests[req_index].state = NpAuthRequestState::Ready;
            g_auth_requests[req_index].async = async;
            break;
        }
        req_index++;
    }

    if (req_index == g_auth_requests.size()) {
        // There are no requests to replace.
        NpAuthRequest new_request{NpAuthRequestState::Ready, async, 0};
        g_auth_requests.emplace_back(new_request);
    }

    // Offset by one, first returned ID is 0x10000001
    g_active_auth_requests++;
    LOG_DEBUG(Lib_NpAuth, "called, async = {}", async);
    return req_index + ORBIS_NP_AUTH_REQUEST_ID_OFFSET + 1;
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

    std::scoped_lock lk{g_auth_request_mutex};

    // From here the actual authorization code request is performed.
    s32 req_index = req_id - ORBIS_NP_AUTH_REQUEST_ID_OFFSET - 1;
    if (g_active_auth_requests == 0 || g_auth_requests.size() <= req_index ||
        g_auth_requests[req_index].state == NpAuthRequestState::None) {
        return ORBIS_NP_AUTH_ERROR_REQUEST_NOT_FOUND;
    }

    auto& request = g_auth_requests[req_index];
    if (request.state == NpAuthRequestState::Complete) {
        request.result = ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT;
        return ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT;
    } else if (request.state == NpAuthRequestState::Aborted) {
        request.result = ORBIS_NP_AUTH_ERROR_ABORTED;
        return ORBIS_NP_AUTH_ERROR_ABORTED;
    }

    request.state = NpAuthRequestState::Complete;
    if (!g_signed_in) {
        request.result = ORBIS_NP_ERROR_SIGNED_OUT;
        // If the request is processed in some form, and it's an async request, then it returns OK.
        if (request.async) {
            return ORBIS_OK;
        }
        return ORBIS_NP_ERROR_SIGNED_OUT;
    }

    LOG_ERROR(Lib_NpAuth, "(STUBBED) called, req_id = {:#x}, async = {}", req_id, request.async);

    // Not sure what values are expected here, so zeroing these for now.
    std::memset(auth_code, 0, sizeof(OrbisNpAuthorizationCode));
    *issuer_id = 0;
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
    if (!g_signed_in) {
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

    std::scoped_lock lk{g_auth_request_mutex};

    // From here the actual authorization code request is performed.
    s32 req_index = req_id - ORBIS_NP_AUTH_REQUEST_ID_OFFSET - 1;
    if (g_active_auth_requests == 0 || g_auth_requests.size() <= req_index ||
        g_auth_requests[req_index].state == NpAuthRequestState::None) {
        return ORBIS_NP_AUTH_ERROR_REQUEST_NOT_FOUND;
    }

    auto& request = g_auth_requests[req_index];
    if (request.state == NpAuthRequestState::Complete) {
        request.result = ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT;
        return ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT;
    } else if (request.state == NpAuthRequestState::Aborted) {
        request.result = ORBIS_NP_AUTH_ERROR_ABORTED;
        return ORBIS_NP_AUTH_ERROR_ABORTED;
    }

    request.state = NpAuthRequestState::Complete;
    if (!g_signed_in) {
        request.result = ORBIS_NP_ERROR_SIGNED_OUT;
        // If the request is processed in some form, and it's an async request, then it returns OK.
        if (request.async) {
            return ORBIS_OK;
        }
        return ORBIS_NP_ERROR_SIGNED_OUT;
    }

    LOG_ERROR(Lib_NpAuth, "(STUBBED) called, req_id = {:#x}, async = {}", req_id, request.async);

    // Not sure what values are expected here, so zeroing this for now.
    std::memset(token, 0, sizeof(OrbisNpIdToken));
    return ORBIS_OK;
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
    if (!g_signed_in) {
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

s32 PS4_SYSV_ABI sceNpAuthSetTimeout(s32 req_id, s32 resolve_retry, u32 resolve_timeout,
                                     u32 conn_timeout, u32 send_timeout, u32 recv_timeout) {
    LOG_ERROR(Lib_NpAuth, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpAuthAbortRequest(s32 req_id) {
    LOG_DEBUG(Lib_NpAuth, "called req_id = {:#x}", req_id);

    std::scoped_lock lk{g_auth_request_mutex};

    s32 req_index = req_id - ORBIS_NP_AUTH_REQUEST_ID_OFFSET - 1;
    if (g_active_auth_requests == 0 || g_auth_requests.size() <= req_index ||
        g_auth_requests[req_index].state == NpAuthRequestState::None) {
        return ORBIS_NP_AUTH_ERROR_REQUEST_NOT_FOUND;
    }

    if (g_auth_requests[req_index].state == NpAuthRequestState::Complete) {
        // If the request is already complete, abort is ignored.
        return ORBIS_OK;
    }

    g_auth_requests[req_index].state = NpAuthRequestState::Aborted;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpAuthWaitAsync(s32 req_id, s32* result) {
    if (result == nullptr) {
        return ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT;
    }

    std::scoped_lock lk{g_auth_request_mutex};

    s32 req_index = req_id - ORBIS_NP_AUTH_REQUEST_ID_OFFSET - 1;
    if (g_active_auth_requests == 0 || g_auth_requests.size() <= req_index ||
        g_auth_requests[req_index].state == NpAuthRequestState::None) {
        return ORBIS_NP_AUTH_ERROR_REQUEST_NOT_FOUND;
    }

    if (!g_auth_requests[req_index].async ||
        g_auth_requests[req_index].state == NpAuthRequestState::Ready) {
        return ORBIS_NP_AUTH_ERROR_INVALID_ID;
    }

    // Since we're not actually performing any sort of network request here,
    // we can just set result based on the request and return.
    *result = g_auth_requests[req_index].result;
    LOG_WARNING(Lib_NpAuth, "called req_id = {:#x}, returning result = {:#x}", req_id,
                static_cast<u32>(*result));
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpAuthPollAsync(s32 req_id, s32* result) {
    if (result == nullptr) {
        return ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT;
    }

    std::scoped_lock lk{g_auth_request_mutex};

    s32 req_index = req_id - ORBIS_NP_AUTH_REQUEST_ID_OFFSET - 1;
    if (g_active_auth_requests == 0 || g_auth_requests.size() <= req_index ||
        g_auth_requests[req_index].state == NpAuthRequestState::None) {
        return ORBIS_NP_AUTH_ERROR_REQUEST_NOT_FOUND;
    }

    if (!g_auth_requests[req_index].async ||
        g_auth_requests[req_index].state == NpAuthRequestState::Ready) {
        return ORBIS_NP_AUTH_ERROR_INVALID_ID;
    }

    // Since we're not actually performing any sort of network request here,
    // we can just set result based on the request and return.
    *result = g_auth_requests[req_index].result;
    LOG_WARNING(Lib_NpAuth, "called req_id = {:#x}, returning result = {:#x}", req_id,
                static_cast<u32>(*result));
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpAuthDeleteRequest(s32 req_id) {
    LOG_DEBUG(Lib_NpAuth, "called req_id = {:#x}", req_id);

    std::scoped_lock lk{g_auth_request_mutex};

    s32 req_index = req_id - ORBIS_NP_AUTH_REQUEST_ID_OFFSET - 1;
    if (g_active_auth_requests == 0 || g_auth_requests.size() <= req_index ||
        g_auth_requests[req_index].state == NpAuthRequestState::None) {
        return ORBIS_NP_AUTH_ERROR_REQUEST_NOT_FOUND;
    }

    g_active_auth_requests--;
    g_auth_requests[req_index].state = NpAuthRequestState::None;
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    g_signed_in = Config::getPSNSignedIn();

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