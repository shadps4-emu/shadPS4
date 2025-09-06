// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/config.h"
#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_manager.h"
#include "core/tls.h"

namespace Libraries::Np::NpManager {

static bool g_signed_in = Config::getPSNSignedIn();

s32 PS4_SYSV_ABI sceNpCreateRequest() {
    LOG_ERROR(Lib_NpManager, "(DUMMY) called");
    static s32 id = 0;
    return ++id;
}

s32 PS4_SYSV_ABI sceNpDeleteRequest(s32 req_id) {
    LOG_ERROR(Lib_NpManager, "(DUMMY) called req_id = {}", req_id);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpGetAccountCountry(OrbisNpOnlineId* online_id,
                                        OrbisNpCountryCode* country_code) {
    LOG_DEBUG(Lib_NpManager, "called");
    if (country_code == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    if (!g_signed_in) {
        return ORBIS_NP_ERROR_SIGNED_OUT;
    }
    ::memset(country_code, 0, sizeof(OrbisNpCountryCode));
    // TODO: get NP country code from config
    ::memcpy(country_code->country_code, "us", 2);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpGetAccountCountryA(Libraries::UserService::OrbisUserServiceUserId user_id,
                                         OrbisNpCountryCode* country_code) {
    LOG_DEBUG(Lib_NpManager, "called, user_id = {}", user_id);
    if (country_code == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    if (!g_signed_in) {
        return ORBIS_NP_ERROR_SIGNED_OUT;
    }
    ::memset(country_code, 0, sizeof(OrbisNpCountryCode));
    // TODO: get NP country code from config
    ::memcpy(country_code->country_code, "us", 2);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpGetAccountId(OrbisNpOnlineId* online_id, u64* account_id) {
    LOG_DEBUG(Lib_NpManager, "called");
    if (online_id == nullptr || account_id == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    if (!g_signed_in) {
        *account_id = 0;
        return ORBIS_NP_ERROR_SIGNED_OUT;
    }
    *account_id = 0xFEEDFACE;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpGetAccountIdA(Libraries::UserService::OrbisUserServiceUserId user_id,
                                    u64* account_id) {
    LOG_DEBUG(Lib_NpManager, "user_id {}", user_id);
    if (account_id == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    if (!g_signed_in) {
        *account_id = 0;
        return ORBIS_NP_ERROR_SIGNED_OUT;
    }
    *account_id = 0xFEEDFACE;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpGetNpId(Libraries::UserService::OrbisUserServiceUserId user_id,
                              OrbisNpId* np_id) {
    LOG_DEBUG(Lib_NpManager, "user_id {}", user_id);
    if (np_id == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    if (!g_signed_in) {
        return ORBIS_NP_ERROR_SIGNED_OUT;
    }
    memset(np_id, 0, sizeof(OrbisNpId));
    strncpy(np_id->handle.data, Config::getUserName().c_str(), sizeof(np_id->handle.data));
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpGetOnlineId(Libraries::UserService::OrbisUserServiceUserId user_id,
                                  OrbisNpOnlineId* online_id) {
    LOG_DEBUG(Lib_NpManager, "user_id {}", user_id);
    if (online_id == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    if (!g_signed_in) {
        return ORBIS_NP_ERROR_SIGNED_OUT;
    }
    memset(online_id, 0, sizeof(OrbisNpOnlineId));
    strncpy(online_id->data, Config::getUserName().c_str(), sizeof(online_id->data));
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpGetState(Libraries::UserService::OrbisUserServiceUserId user_id,
                               OrbisNpState* state) {
    if (state == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    *state = g_signed_in ? OrbisNpState::SignedIn : OrbisNpState::SignedOut;
    LOG_DEBUG(Lib_NpManager, "Signed {}", g_signed_in ? "in" : "out");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpHasSignedUp(Libraries::UserService::OrbisUserServiceUserId user_id,
                                  bool* has_signed_up) {
    LOG_DEBUG(Lib_NpManager, "called");
    if (has_signed_up == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    *has_signed_up = g_signed_in ? true : false;
    return ORBIS_OK;
}

struct NpStateCallbackForNpToolkit {
    OrbisNpStateCallbackForNpToolkit func;
    void* userdata;
};

NpStateCallbackForNpToolkit NpStateCbForNp;

s32 PS4_SYSV_ABI sceNpCheckCallback() {
    LOG_DEBUG(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpCheckCallbackForLib() {
    LOG_DEBUG(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpRegisterStateCallbackForToolkit(OrbisNpStateCallbackForNpToolkit callback,
                                                      void* userdata) {
    static s32 id = 0;
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    NpStateCbForNp.func = callback;
    NpStateCbForNp.userdata = userdata;
    return id;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("GpLQDNKICac", "libSceNpManager", 1, "libSceNpManager", 1, 1, sceNpCreateRequest);
    LIB_FUNCTION("S7QTn72PrDw", "libSceNpManager", 1, "libSceNpManager", 1, 1, sceNpDeleteRequest);
    LIB_FUNCTION("Ghz9iWDUtC4", "libSceNpManagerCompat", 1, "libSceNpManager", 1, 1,
                 sceNpGetAccountCountry);
    LIB_FUNCTION("Ghz9iWDUtC4", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpGetAccountCountry);
    LIB_FUNCTION("JT+t00a3TxA", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpGetAccountCountryA);
    LIB_FUNCTION("a8R9-75u4iM", "libSceNpManagerCompat", 1, "libSceNpManager", 1, 1,
                 sceNpGetAccountId);
    LIB_FUNCTION("a8R9-75u4iM", "libSceNpManager", 1, "libSceNpManager", 1, 1, sceNpGetAccountId);
    LIB_FUNCTION("rbknaUjpqWo", "libSceNpManager", 1, "libSceNpManager", 1, 1, sceNpGetAccountIdA);
    LIB_FUNCTION("p-o74CnoNzY", "libSceNpManager", 1, "libSceNpManager", 1, 1, sceNpGetNpId);
    LIB_FUNCTION("XDncXQIJUSk", "libSceNpManager", 1, "libSceNpManager", 1, 1, sceNpGetOnlineId);
    LIB_FUNCTION("eQH7nWPcAgc", "libSceNpManager", 1, "libSceNpManager", 1, 1, sceNpGetState);
    LIB_FUNCTION("Oad3rvY-NJQ", "libSceNpManager", 1, "libSceNpManager", 1, 1, sceNpHasSignedUp);
    LIB_FUNCTION("3Zl8BePTh9Y", "libSceNpManager", 1, "libSceNpManager", 1, 1, sceNpCheckCallback);
    LIB_FUNCTION("JELHf4xPufo", "libSceNpManager", 1, "libSceNpManager", 1, 1,
                 sceNpCheckCallbackForLib);
    LIB_FUNCTION("JELHf4xPufo", "libSceNpManagerForToolkit", 1, "libSceNpManager", 1, 1,
                 sceNpCheckCallbackForLib);
    LIB_FUNCTION("0c7HbXRKUt4", "libSceNpManagerForToolkit", 1, "libSceNpManager", 1, 1,
                 sceNpRegisterStateCallbackForToolkit);
};

} // namespace Libraries::Np::NpManager
