// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

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

s32 PS4_SYSV_ABI sceNpAuthCreateRequest() {
    LOG_WARNING(Lib_NpAuth, "(DUMMY) called");
    return 1;
}

s32 PS4_SYSV_ABI sceNpAuthCreateAsyncRequest(const OrbisNpAuthCreateAsyncRequestParameter* param) {
    if (param == nullptr) {
        return ORBIS_NP_AUTH_ERROR_INVALID_ARGUMENT;
    }
    if (param->size != sizeof(OrbisNpAuthCreateAsyncRequestParameter)) {
        return ORBIS_NP_AUTH_ERROR_INVALID_SIZE;
    }

    LOG_ERROR(Lib_NpAuth, "(STUBBED) called");
    return ORBIS_OK;
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

    // From here the actual authorization code request is performed.
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
        // returns the result of sceNpManagerIntGetUserIdByOnlineId,
        // which will never succeed since games cannot retrieve a valid online id while signed out.
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

    LOG_ERROR(Lib_NpAuth, "(STUBBED) called, req_id = {:#x}", req_id);
    return GetAuthorizationCode(req_id, &internal_params, 0, auth_code, issuer_id);
}

s32 PS4_SYSV_ABI
sceNpAuthGetAuthorizationCodeA(s32 req_id, const OrbisNpAuthGetAuthorizationCodeParameterA* param,
                               OrbisNpAuthorizationCode* auth_code, s32* issuer_id) {
    LOG_ERROR(Lib_NpAuth, "(STUBBED) called, req_id = {:#x}", req_id);
    return GetAuthorizationCode(req_id, param, 0, auth_code, issuer_id);
}

s32 PS4_SYSV_ABI
sceNpAuthGetAuthorizationCodeV3(s32 req_id, const OrbisNpAuthGetAuthorizationCodeParameterA* param,
                                OrbisNpAuthorizationCode* auth_code, s32* issuer_id) {
    LOG_ERROR(Lib_NpAuth, "(STUBBED) called, req_id = {:#x}", req_id);
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

    // From here the actual id token request is performed.
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

    LOG_ERROR(Lib_NpAuth, "(STUBBED) called, req_id = {:#x}", req_id);
    return GetIdToken(req_id, &internal_params, 0, token);
}

s32 PS4_SYSV_ABI sceNpAuthGetIdTokenA(s32 req_id, const OrbisNpAuthGetIdTokenParameterA* param,
                                      OrbisNpIdToken* token) {
    LOG_ERROR(Lib_NpAuth, "(STUBBED) called, req_id = {:#x}", req_id);
    return GetIdToken(req_id, param, 0, token);
}

s32 PS4_SYSV_ABI sceNpAuthGetIdTokenV3(s32 req_id, const OrbisNpAuthGetIdTokenParameterA* param,
                                       OrbisNpIdToken* token) {
    LOG_ERROR(Lib_NpAuth, "(STUBBED) called, req_id = {:#x}", req_id);
    return GetIdToken(req_id, param, 1, token);
}

s32 PS4_SYSV_ABI sceNpAuthSetTimeout(s32 req_id, s32 resolve_retry, u32 resolve_timeout,
                                     u32 conn_timeout, u32 send_timeout, u32 recv_timeout) {
    LOG_ERROR(Lib_NpAuth, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpAuthWaitAsync(s32 req_id, s32* result) {
    LOG_ERROR(Lib_NpAuth, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpAuthPollAsync(s32 req_id, s32* result) {
    LOG_ERROR(Lib_NpAuth, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpAuthAbortRequest(s32 req_id) {
    LOG_ERROR(Lib_NpAuth, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpAuthDeleteRequest(s32 req_id) {
    LOG_WARNING(Lib_NpAuth, "(DUMMY) called");
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
    LIB_FUNCTION("SK-S7daqJSE", "libSceNpAuth", 1, "libSceNpAuth", sceNpAuthWaitAsync);
    LIB_FUNCTION("gjSyfzSsDcE", "libSceNpAuth", 1, "libSceNpAuth", sceNpAuthPollAsync);
    LIB_FUNCTION("cE7wIsqXdZ8", "libSceNpAuth", 1, "libSceNpAuth", sceNpAuthAbortRequest);
    LIB_FUNCTION("H8wG9Bk-nPc", "libSceNpAuth", 1, "libSceNpAuth", sceNpAuthDeleteRequest);

    LIB_FUNCTION("KxGkOrQJTqY", "libSceNpAuthCompat", 1, "libSceNpAuth",
                 sceNpAuthGetAuthorizationCode);
    LIB_FUNCTION("uaB-LoJqHis", "libSceNpAuthCompat", 1, "libSceNpAuth", sceNpAuthGetIdToken);
};

} // namespace Libraries::Np::NpAuth