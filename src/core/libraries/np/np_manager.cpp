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

static bool g_signed_in = false;
static s32 g_active_requests = 0;
static std::vector<OrbisNpRequestState> g_requests;

s32 PS4_SYSV_ABI sceNpCreateRequest() {
    LOG_DEBUG(Lib_NpManager, "called");
    if (g_active_requests == ORBIS_NP_MANAGER_REQUEST_LIMIT) {
        return ORBIS_NP_ERROR_REQUEST_MAX;
    }

    s32 req_index = 0;
    while (req_index < g_requests.size()) {
        // Find first nonexistant request
        if (g_requests[req_index] == OrbisNpRequestState::None) {
            // There is no request at this index, set the index to ready then break.
            g_requests[req_index] = OrbisNpRequestState::Ready;
            break;
        }
        req_index++;
    }

    if (req_index == g_requests.size()) {
        // There are no requests to replace.
        g_requests.emplace_back(OrbisNpRequestState::Ready);
    }

    // Offset by one, first returned ID is 0x20000001
    g_active_requests++;
    return req_index + ORBIS_NP_MANAGER_REQUEST_ID_OFFSET + 1;
}

s32 PS4_SYSV_ABI sceNpCheckNpAvailability(s32 req_id, OrbisNpOnlineId* online_id) {
    if (online_id == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    s32 req_index = req_id - ORBIS_NP_MANAGER_REQUEST_ID_OFFSET - 1;
    if (g_active_requests == 0 || g_requests.size() <= req_index ||
        g_requests[req_index] == OrbisNpRequestState::None) {
        return ORBIS_NP_ERROR_REQUEST_NOT_FOUND;
    }

    if (g_requests[req_index] == OrbisNpRequestState::Complete) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    g_requests[req_index] = OrbisNpRequestState::Complete;
    if (!g_signed_in) {
        return ORBIS_NP_ERROR_SIGNED_OUT;
    }

    LOG_ERROR(Lib_NpManager, "(STUBBED) called, req_id = {:#x}", req_id);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpCheckNpAvailabilityA(s32 req_id,
                                           Libraries::UserService::OrbisUserServiceUserId user_id) {
    s32 req_index = req_id - ORBIS_NP_MANAGER_REQUEST_ID_OFFSET - 1;
    if (g_active_requests == 0 || g_requests.size() <= req_index ||
        g_requests[req_index] == OrbisNpRequestState::None) {
        return ORBIS_NP_ERROR_REQUEST_NOT_FOUND;
    }

    if (g_requests[req_index] == OrbisNpRequestState::Complete) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    g_requests[req_index] = OrbisNpRequestState::Complete;
    if (!g_signed_in) {
        return ORBIS_NP_ERROR_SIGNED_OUT;
    }

    LOG_ERROR(Lib_NpManager, "(STUBBED) called, req_id = {:#x}", req_id);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpCheckNpReachability(s32 req_id,
                                          Libraries::UserService::OrbisUserServiceUserId user_id) {
    s32 req_index = req_id - ORBIS_NP_MANAGER_REQUEST_ID_OFFSET - 1;
    if (g_active_requests == 0 || g_requests.size() <= req_index ||
        g_requests[req_index] == OrbisNpRequestState::None) {
        return ORBIS_NP_ERROR_REQUEST_NOT_FOUND;
    }

    if (g_requests[req_index] == OrbisNpRequestState::Complete) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    g_requests[req_index] = OrbisNpRequestState::Complete;
    if (!g_signed_in) {
        return ORBIS_NP_ERROR_SIGNED_OUT;
    }

    LOG_ERROR(Lib_NpManager, "(STUBBED) called, req_id = {:#x}", req_id);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpGetAccountLanguage(s32 req_id, OrbisNpOnlineId* online_id,
                                         OrbisNpLanguageCode* language) {
    if (online_id == nullptr || language == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    s32 req_index = req_id - ORBIS_NP_MANAGER_REQUEST_ID_OFFSET - 1;
    if (g_active_requests == 0 || g_requests.size() <= req_index ||
        g_requests[req_index] == OrbisNpRequestState::None) {
        return ORBIS_NP_ERROR_REQUEST_NOT_FOUND;
    }

    if (g_requests[req_index] == OrbisNpRequestState::Complete) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    g_requests[req_index] = OrbisNpRequestState::Complete;
    if (!g_signed_in) {
        return ORBIS_NP_ERROR_SIGNED_OUT;
    }

    std::memset(language, 0, sizeof(OrbisNpLanguageCode));
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpGetAccountLanguageA(s32 req_id,
                                          Libraries::UserService::OrbisUserServiceUserId user_id,
                                          OrbisNpLanguageCode* language) {
    if (language == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    s32 req_index = req_id - ORBIS_NP_MANAGER_REQUEST_ID_OFFSET - 1;
    if (g_active_requests == 0 || g_requests.size() <= req_index ||
        g_requests[req_index] == OrbisNpRequestState::None) {
        return ORBIS_NP_ERROR_REQUEST_NOT_FOUND;
    }

    if (g_requests[req_index] == OrbisNpRequestState::Complete) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    g_requests[req_index] = OrbisNpRequestState::Complete;
    if (!g_signed_in) {
        return ORBIS_NP_ERROR_SIGNED_OUT;
    }

    std::memset(language, 0, sizeof(OrbisNpLanguageCode));
    LOG_ERROR(Lib_NpManager, "(STUBBED) called, user_id = {}", user_id);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpGetParentalControlInfo(s32 req_id, OrbisNpOnlineId* online_id, s8* age,
                                             OrbisNpParentalControlInfo* info) {
    if (online_id == nullptr || age == nullptr || info == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    s32 req_index = req_id - ORBIS_NP_MANAGER_REQUEST_ID_OFFSET - 1;
    if (g_active_requests == 0 || g_requests.size() <= req_index ||
        g_requests[req_index] == OrbisNpRequestState::None) {
        return ORBIS_NP_ERROR_REQUEST_NOT_FOUND;
    }

    if (g_requests[req_index] == OrbisNpRequestState::Complete) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    g_requests[req_index] = OrbisNpRequestState::Complete;
    if (!g_signed_in) {
        return ORBIS_NP_ERROR_SIGNED_OUT;
    }

    // TODO: Add to config?
    *age = 13;
    std::memset(info, 0, sizeof(OrbisNpParentalControlInfo));
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
sceNpGetParentalControlInfoA(s32 req_id, Libraries::UserService::OrbisUserServiceUserId user_id,
                             s8* age, OrbisNpParentalControlInfo* info) {
    if (age == nullptr || info == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    s32 req_index = req_id - ORBIS_NP_MANAGER_REQUEST_ID_OFFSET - 1;
    if (g_active_requests == 0 || g_requests.size() <= req_index ||
        g_requests[req_index] == OrbisNpRequestState::None) {
        return ORBIS_NP_ERROR_REQUEST_NOT_FOUND;
    }

    if (g_requests[req_index] == OrbisNpRequestState::Complete) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    g_requests[req_index] = OrbisNpRequestState::Complete;
    if (!g_signed_in) {
        return ORBIS_NP_ERROR_SIGNED_OUT;
    }

    // TODO: Add to config?
    *age = 13;
    std::memset(info, 0, sizeof(OrbisNpParentalControlInfo));
    LOG_ERROR(Lib_NpManager, "(STUBBED) called, user_id = {}", user_id);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpDeleteRequest(s32 req_id) {
    LOG_DEBUG(Lib_NpManager, "called req_id = {:#x}", req_id);
    s32 req_index = req_id - ORBIS_NP_MANAGER_REQUEST_ID_OFFSET - 1;
    if (g_active_requests == 0 || g_requests.size() <= req_index ||
        g_requests[req_index] == OrbisNpRequestState::None) {
        return ORBIS_NP_ERROR_REQUEST_NOT_FOUND;
    }

    g_active_requests--;
    g_requests[req_index] = OrbisNpRequestState::None;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpGetAccountCountry(OrbisNpOnlineId* online_id,
                                        OrbisNpCountryCode* country_code) {
    if (online_id == nullptr || country_code == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    if (!g_signed_in) {
        return ORBIS_NP_ERROR_SIGNED_OUT;
    }
    std::memset(country_code, 0, sizeof(OrbisNpCountryCode));
    // TODO: get NP country code from config
    std::memcpy(country_code->country_code, "us", 2);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpGetAccountCountryA(Libraries::UserService::OrbisUserServiceUserId user_id,
                                         OrbisNpCountryCode* country_code) {
    if (country_code == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    if (!g_signed_in) {
        return ORBIS_NP_ERROR_SIGNED_OUT;
    }
    std::memset(country_code, 0, sizeof(OrbisNpCountryCode));
    // TODO: get NP country code from config
    std::memcpy(country_code->country_code, "us", 2);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpGetAccountDateOfBirth(OrbisNpOnlineId* online_id,
                                            OrbisNpDate* date_of_birth) {
    if (online_id == nullptr || date_of_birth == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    if (!g_signed_in) {
        return ORBIS_NP_ERROR_SIGNED_OUT;
    }

    // TODO: maybe add to config?
    date_of_birth->day = 1;
    date_of_birth->month = 1;
    date_of_birth->year = 2000;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpGetAccountDateOfBirthA(Libraries::UserService::OrbisUserServiceUserId user_id,
                                             OrbisNpDate* date_of_birth) {
    if (date_of_birth == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    if (!g_signed_in) {
        return ORBIS_NP_ERROR_SIGNED_OUT;
    }

    // TODO: maybe add to config?
    date_of_birth->day = 1;
    date_of_birth->month = 1;
    date_of_birth->year = 2000;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpGetGamePresenceStatus(OrbisNpOnlineId* online_id,
                                            OrbisNpGamePresenseStatus* game_status) {
    if (online_id == nullptr || game_status == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    *game_status =
        g_signed_in ? OrbisNpGamePresenseStatus::Online : OrbisNpGamePresenseStatus::Offline;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpGetGamePresenceStatusA(Libraries::UserService::OrbisUserServiceUserId user_id,
                                             OrbisNpGamePresenseStatus* game_status) {
    if (game_status == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    *game_status =
        g_signed_in ? OrbisNpGamePresenseStatus::Online : OrbisNpGamePresenseStatus::Offline;
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

s32 PS4_SYSV_ABI sceNpGetNpReachabilityState(Libraries::UserService::OrbisUserServiceUserId user_id,
                                             OrbisNpReachabilityState* state) {
    if (state == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    *state =
        g_signed_in ? OrbisNpReachabilityState::Reachable : OrbisNpReachabilityState::Unavailable;
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
    g_signed_in = Config::getPSNSignedIn();

    LIB_FUNCTION("GpLQDNKICac", "libSceNpManager", 1, "libSceNpManager", sceNpCreateRequest);
    LIB_FUNCTION("2rsFmlGWleQ", "libSceNpManager", 1, "libSceNpManager", sceNpCheckNpAvailability);
    LIB_FUNCTION("8Z2Jc5GvGDI", "libSceNpManager", 1, "libSceNpManager", sceNpCheckNpAvailabilityA);
    LIB_FUNCTION("KfGZg2y73oM", "libSceNpManager", 1, "libSceNpManager", sceNpCheckNpReachability);
    LIB_FUNCTION("KZ1Mj9yEGYc", "libSceNpManager", 1, "libSceNpManager", sceNpGetAccountLanguage);
    LIB_FUNCTION("TPMbgIxvog0", "libSceNpManager", 1, "libSceNpManager", sceNpGetAccountLanguageA);
    LIB_FUNCTION("ilwLM4zOmu4", "libSceNpManager", 1, "libSceNpManager",
                 sceNpGetParentalControlInfo);
    LIB_FUNCTION("m9L3O6yst-U", "libSceNpManager", 1, "libSceNpManager",
                 sceNpGetParentalControlInfoA);
    LIB_FUNCTION("S7QTn72PrDw", "libSceNpManager", 1, "libSceNpManager", sceNpDeleteRequest);
    LIB_FUNCTION("Ghz9iWDUtC4", "libSceNpManager", 1, "libSceNpManager", sceNpGetAccountCountry);
    LIB_FUNCTION("JT+t00a3TxA", "libSceNpManager", 1, "libSceNpManager", sceNpGetAccountCountryA);
    LIB_FUNCTION("8VBTeRf1ZwI", "libSceNpManager", 1, "libSceNpManager",
                 sceNpGetAccountDateOfBirth);
    LIB_FUNCTION("q3M7XzBKC3s", "libSceNpManager", 1, "libSceNpManager",
                 sceNpGetAccountDateOfBirthA);
    LIB_FUNCTION("IPb1hd1wAGc", "libSceNpManager", 1, "libSceNpManager",
                 sceNpGetGamePresenceStatus);
    LIB_FUNCTION("oPO9U42YpgI", "libSceNpManager", 1, "libSceNpManager",
                 sceNpGetGamePresenceStatusA);
    LIB_FUNCTION("e-ZuhGEoeC4", "libSceNpManager", 1, "libSceNpManager",
                 sceNpGetNpReachabilityState);

    LIB_FUNCTION("a8R9-75u4iM", "libSceNpManager", 1, "libSceNpManager", sceNpGetAccountId);
    LIB_FUNCTION("rbknaUjpqWo", "libSceNpManager", 1, "libSceNpManager", sceNpGetAccountIdA);
    LIB_FUNCTION("p-o74CnoNzY", "libSceNpManager", 1, "libSceNpManager", sceNpGetNpId);
    LIB_FUNCTION("XDncXQIJUSk", "libSceNpManager", 1, "libSceNpManager", sceNpGetOnlineId);
    LIB_FUNCTION("eQH7nWPcAgc", "libSceNpManager", 1, "libSceNpManager", sceNpGetState);
    LIB_FUNCTION("Oad3rvY-NJQ", "libSceNpManager", 1, "libSceNpManager", sceNpHasSignedUp);
    LIB_FUNCTION("3Zl8BePTh9Y", "libSceNpManager", 1, "libSceNpManager", sceNpCheckCallback);
    LIB_FUNCTION("JELHf4xPufo", "libSceNpManager", 1, "libSceNpManager", sceNpCheckCallbackForLib);
    LIB_FUNCTION("JELHf4xPufo", "libSceNpManagerForToolkit", 1, "libSceNpManager",
                 sceNpCheckCallbackForLib);
    LIB_FUNCTION("0c7HbXRKUt4", "libSceNpManagerForToolkit", 1, "libSceNpManager",
                 sceNpRegisterStateCallbackForToolkit);

    LIB_FUNCTION("2rsFmlGWleQ", "libSceNpManagerCompat", 1, "libSceNpManager",
                 sceNpCheckNpAvailability);
    LIB_FUNCTION("a8R9-75u4iM", "libSceNpManagerCompat", 1, "libSceNpManager", sceNpGetAccountId);
    LIB_FUNCTION("KZ1Mj9yEGYc", "libSceNpManagerCompat", 1, "libSceNpManager",
                 sceNpGetAccountLanguage);
    LIB_FUNCTION("Ghz9iWDUtC4", "libSceNpManagerCompat", 1, "libSceNpManager",
                 sceNpGetAccountCountry);
    LIB_FUNCTION("8VBTeRf1ZwI", "libSceNpManagerCompat", 1, "libSceNpManager",
                 sceNpGetAccountDateOfBirth);
    LIB_FUNCTION("IPb1hd1wAGc", "libSceNpManagerCompat", 1, "libSceNpManager",
                 sceNpGetGamePresenceStatus);
    LIB_FUNCTION("ilwLM4zOmu4", "libSceNpManagerCompat", 1, "libSceNpManager",
                 sceNpGetParentalControlInfo);
};

} // namespace Libraries::Np::NpManager
