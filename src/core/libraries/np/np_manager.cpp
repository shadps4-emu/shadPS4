// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <map>
#include <mutex>
#include <variant>

#include <core/user_settings.h>
#include "common/logging/log.h"
#include "core/emulator_settings.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_manager.h"
#include "core/tls.h"
#include "np_handler.h"

namespace Libraries::Np::NpManager {

static bool g_shadnet_enabled = false;
static s32 g_active_requests = 0;
static std::mutex g_request_mutex;

static std::map<std::string, std::function<void()>> g_np_callbacks;
static std::mutex g_np_callbacks_mutex;

// Callbacks

struct NpStateCallbackForNpToolkit {
    OrbisNpStateCallbackForNpToolkit func;
    void* userdata;
};
static NpStateCallbackForNpToolkit NpStateCbForNp;

struct NpStateCallback {
    std::variant<OrbisNpStateCallback, OrbisNpStateCallbackA> func;
    void* userdata;
};
static NpStateCallback NpStateCb;

struct NpReachabilityStateCallback {
    OrbisNpReachabilityStateCallback func;
    void* userdata;
};
static NpReachabilityStateCallback NpReachabilityCb;

// Internal types for storing request-related information
enum class NpRequestState {
    None = 0,
    Ready = 1,
    Aborted = 2,
    Complete = 3,
};

struct NpRequest {
    NpRequestState state;
    bool async;
    s32 result;
};

static std::vector<NpRequest> g_requests;

static s32 CreateNpRequest(bool async) {
    if (g_active_requests == ORBIS_NP_MANAGER_REQUEST_LIMIT) {
        return ORBIS_NP_ERROR_REQUEST_MAX;
    }

    std::scoped_lock lk{g_request_mutex};

    s32 req_index = 0;
    while (req_index < g_requests.size()) {
        // Find first nonexistant request
        if (g_requests[req_index].state == NpRequestState::None) {
            // There is no request at this index, set the index to ready then break.
            g_requests[req_index].state = NpRequestState::Ready;
            g_requests[req_index].async = async;
            break;
        }
        req_index++;
    }

    if (req_index == g_requests.size()) {
        // There are no requests to replace.
        NpRequest new_request{NpRequestState::Ready, async, 0};
        g_requests.emplace_back(new_request);
    }

    // Offset by one, first returned ID is 0x20000001
    g_active_requests++;
    return req_index + ORBIS_NP_MANAGER_REQUEST_ID_OFFSET + 1;
}

// Validate a request ID and return the NpRequest*, or nullptr + error code.
// Writes the error code to *out_err when returning nullptr.
static NpRequest* GetRequest(s32 req_id, s32* out_err) {
    s32 req_index = req_id - ORBIS_NP_MANAGER_REQUEST_ID_OFFSET - 1;
    if (g_active_requests == 0 || req_index < 0 ||
        req_index >= static_cast<s32>(g_requests.size()) ||
        g_requests[req_index].state == NpRequestState::None) {
        *out_err = ORBIS_NP_ERROR_REQUEST_NOT_FOUND;
        return nullptr;
    }
    auto& req = g_requests[req_index];
    if (req.state == NpRequestState::Complete) {
        req.result = ORBIS_NP_ERROR_INVALID_ARGUMENT;
        *out_err = ORBIS_NP_ERROR_INVALID_ARGUMENT;
        return nullptr;
    }
    if (req.state == NpRequestState::Aborted) {
        req.result = ORBIS_NP_ERROR_ABORTED;
        *out_err = ORBIS_NP_ERROR_ABORTED;
        return nullptr;
    }
    return &req;
}

// Complete a request and return OK (or SIGNED_OUT for async when disabled).
static s32 CompleteRequest(NpRequest& req, s32 result) {
    req.state = NpRequestState::Complete;
    req.result = result;
    if (result != ORBIS_OK && req.async) {
        // Async requests always return OK immediately; result is read via PollAsync/WaitAsync.
        return ORBIS_OK;
    }
    return result;
}

s32 PS4_SYSV_ABI sceNpCreateRequest() {
    LOG_DEBUG(Lib_NpManager, "called");
    return CreateNpRequest(false);
}

s32 PS4_SYSV_ABI sceNpCreateAsyncRequest(const OrbisNpCreateAsyncRequestParameter* param) {
    LOG_DEBUG(Lib_NpManager, "called");
    if (param == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    if (param->size != sizeof(OrbisNpCreateAsyncRequestParameter)) {
        return ORBIS_NP_ERROR_INVALID_SIZE;
    }

    return CreateNpRequest(true);
}

s32 PS4_SYSV_ABI sceNpAbortRequest(s32 req_id) {
    LOG_DEBUG(Lib_NpManager, "called req_id = {:#x}", req_id);

    std::scoped_lock lk{g_request_mutex};

    s32 req_index = req_id - ORBIS_NP_MANAGER_REQUEST_ID_OFFSET - 1;
    if (g_active_requests == 0 || req_index < 0 ||
        req_index >= static_cast<s32>(g_requests.size()) ||
        g_requests[req_index].state == NpRequestState::None) {
        return ORBIS_NP_ERROR_REQUEST_NOT_FOUND;
    }

    if (g_requests[req_index].state == NpRequestState::Complete) {
        // If the request is already complete, abort is ignored.
        return ORBIS_OK;
    }

    g_requests[req_index].state = NpRequestState::Aborted;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpPollAsync(s32 req_id, s32* result) {
    if (result == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    std::scoped_lock lk{g_request_mutex};

    s32 req_index = req_id - ORBIS_NP_MANAGER_REQUEST_ID_OFFSET - 1;
    if (g_active_requests == 0 || req_index < 0 ||
        req_index >= static_cast<s32>(g_requests.size()) ||
        g_requests[req_index].state == NpRequestState::None) {
        return ORBIS_NP_ERROR_REQUEST_NOT_FOUND;
    }

    if (!g_requests[req_index].async || g_requests[req_index].state == NpRequestState::Ready) {
        return ORBIS_NP_ERROR_INVALID_ID;
    }

    // Since we're not actually performing any sort of network request here,
    // we can just set result based on the request and return.
    *result = g_requests[req_index].result;
    LOG_WARNING(Lib_NpManager, "called req_id = {:#x}, returning result = {:#x}", req_id,
                static_cast<u32>(*result));
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpWaitAsync(s32 req_id, s32* result) {
    if (result == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    std::scoped_lock lk{g_request_mutex};

    s32 req_index = req_id - ORBIS_NP_MANAGER_REQUEST_ID_OFFSET - 1;
    if (g_active_requests == 0 || req_index < 0 ||
        req_index >= static_cast<s32>(g_requests.size()) ||
        g_requests[req_index].state == NpRequestState::None) {
        return ORBIS_NP_ERROR_REQUEST_NOT_FOUND;
    }

    if (!g_requests[req_index].async || g_requests[req_index].state == NpRequestState::Ready) {
        return ORBIS_NP_ERROR_INVALID_ID;
    }

    // Since we're not actually performing any sort of network request here,
    // we can just set result based on the request and return.
    *result = g_requests[req_index].result;
    LOG_WARNING(Lib_NpManager, "called req_id = {:#x}, returning result = {:#x}", req_id,
                static_cast<u32>(*result));
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpDeleteRequest(s32 req_id) {
    LOG_DEBUG(Lib_NpManager, "called req_id = {:#x}", req_id);

    std::scoped_lock lk{g_request_mutex};

    s32 req_index = req_id - ORBIS_NP_MANAGER_REQUEST_ID_OFFSET - 1;
    if (g_active_requests == 0 || req_index < 0 ||
        req_index >= static_cast<s32>(g_requests.size()) ||
        g_requests[req_index].state == NpRequestState::None) {
        return ORBIS_NP_ERROR_REQUEST_NOT_FOUND;
    }

    g_active_requests--;
    g_requests[req_index].state = NpRequestState::None;
    return ORBIS_OK;
}

// Availabity and reachability checks.
s32 PS4_SYSV_ABI sceNpCheckNpAvailability(s32 req_id, OrbisNpOnlineId* online_id) {
    if (online_id == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    std::scoped_lock lk{g_request_mutex};
    s32 err;
    NpRequest* req = GetRequest(req_id, &err);
    if (!req)
        return err;
    const s32 user_id = Libraries::Np::NpHandler::GetInstance().GetUserIdByOnlineId(*online_id);
    if (!g_shadnet_enabled || user_id == -1 ||
        !Libraries::Np::NpHandler::GetInstance().IsPsnSignedIn(user_id)) {
        return CompleteRequest(*req, ORBIS_NP_ERROR_SIGNED_OUT);
    }
    LOG_DEBUG(Lib_NpManager, "req_id = {:#x}", req_id);
    return CompleteRequest(*req, ORBIS_OK);
}

s32 PS4_SYSV_ABI sceNpCheckNpAvailabilityA(s32 req_id,
                                           Libraries::UserService::OrbisUserServiceUserId user_id) {
    std::scoped_lock lk{g_request_mutex};
    s32 err;
    NpRequest* req = GetRequest(req_id, &err);
    if (!req)
        return err;
    if (!g_shadnet_enabled || !Libraries::Np::NpHandler::GetInstance().IsPsnSignedIn(user_id)) {
        return CompleteRequest(*req, ORBIS_NP_ERROR_SIGNED_OUT);
    }
    LOG_DEBUG(Lib_NpManager, "req_id = {:#x}, user_id = {}", req_id, user_id);
    return CompleteRequest(*req, ORBIS_OK);
}

s32 PS4_SYSV_ABI sceNpCheckNpReachability(s32 req_id,
                                          Libraries::UserService::OrbisUserServiceUserId user_id) {
    std::scoped_lock lk{g_request_mutex};
    s32 err;
    NpRequest* req = GetRequest(req_id, &err);
    if (!req)
        return err;
    if (!g_shadnet_enabled || !Libraries::Np::NpHandler::GetInstance().IsPsnSignedIn(user_id)) {
        return CompleteRequest(*req, ORBIS_NP_ERROR_SIGNED_OUT);
    }
    LOG_DEBUG(Lib_NpManager, "req_id = {:#x}, user_id = {}", req_id, user_id);
    return CompleteRequest(*req, ORBIS_OK);
}

s32 PS4_SYSV_ABI sceNpCheckPlus(s32 req_id, const OrbisNpCheckPlusParameter* param,
                                OrbisNpCheckPlusResult* result) {
    if (req_id == 0 || param == nullptr || result == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    if (param->size != sizeof(OrbisNpCheckPlusParameter)) {
        return ORBIS_NP_ERROR_INVALID_SIZE;
    }
    if (param->features < 1 || param->features > 3) {
        // TODO: If compiled SDK version is greater or equal to fw 3.50,
        // // error if param->features != 1 instead.
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    std::scoped_lock lk{g_request_mutex};
    s32 err;
    NpRequest* req = GetRequest(req_id, &err);
    if (!req)
        return err;
    if (!g_shadnet_enabled ||
        !Libraries::Np::NpHandler::GetInstance().IsPsnSignedIn(param->user_id)) {
        return CompleteRequest(*req, ORBIS_NP_ERROR_SIGNED_OUT);
    }
    LOG_DEBUG(Lib_NpManager, "req_id = {:#x}, features = {:#x}", req_id, param->features);
    // Grant PS+ — shadNet has no subscription gating.
    result->authorized = true;
    return CompleteRequest(*req, ORBIS_OK);
}

// Account info
s32 PS4_SYSV_ABI sceNpGetAccountLanguage(s32 req_id, OrbisNpOnlineId* online_id,
                                         OrbisNpLanguageCode* language) {
    if (online_id == nullptr || language == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    std::scoped_lock lk{g_request_mutex};
    s32 err;
    NpRequest* req = GetRequest(req_id, &err);
    if (!req)
        return err;
    const s32 user_id = Libraries::Np::NpHandler::GetInstance().GetUserIdByOnlineId(*online_id);
    if (!g_shadnet_enabled || user_id == -1 ||
        !Libraries::Np::NpHandler::GetInstance().IsPsnSignedIn(user_id)) {
        return CompleteRequest(*req, ORBIS_NP_ERROR_SIGNED_OUT);
    }
    LOG_DEBUG(Lib_NpManager, "req_id = {:#x}", req_id);
    std::memset(language, 0, sizeof(OrbisNpLanguageCode));
    std::memcpy(language->code, "en", 2); // return "en" for now, TODO make it configurable
    return CompleteRequest(*req, ORBIS_OK);
}

s32 PS4_SYSV_ABI sceNpGetAccountLanguageA(s32 req_id,
                                          Libraries::UserService::OrbisUserServiceUserId user_id,
                                          OrbisNpLanguageCode* language) {
    if (language == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    std::scoped_lock lk{g_request_mutex};
    s32 err;
    NpRequest* req = GetRequest(req_id, &err);
    if (!req)
        return err;
    if (!g_shadnet_enabled || !Libraries::Np::NpHandler::GetInstance().IsPsnSignedIn(user_id)) {
        return CompleteRequest(*req, ORBIS_NP_ERROR_SIGNED_OUT);
    }
    LOG_DEBUG(Lib_NpManager, "req_id = {:#x}, user_id = {}", req_id, user_id);
    std::memset(language, 0, sizeof(OrbisNpLanguageCode));
    std::memcpy(language->code, "en", 2); // return "en" for now, TODO make it configurable
    return CompleteRequest(*req, ORBIS_OK);
}

s32 PS4_SYSV_ABI sceNpGetParentalControlInfo(s32 req_id, OrbisNpOnlineId* online_id, s8* age,
                                             OrbisNpParentalControlInfo* info) {
    if (online_id == nullptr || age == nullptr || info == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    std::scoped_lock lk{g_request_mutex};
    s32 err;
    NpRequest* req = GetRequest(req_id, &err);
    if (!req)
        return err;
    const s32 user_id = Libraries::Np::NpHandler::GetInstance().GetUserIdByOnlineId(*online_id);
    if (!g_shadnet_enabled || user_id == -1 ||
        !Libraries::Np::NpHandler::GetInstance().IsPsnSignedIn(user_id)) {
        return CompleteRequest(*req, ORBIS_NP_ERROR_SIGNED_OUT);
    }
    LOG_DEBUG(Lib_NpManager, "req_id = {:#x}", req_id);
    *age = 13; // TODO make it configurable?
    std::memset(info, 0, sizeof(OrbisNpParentalControlInfo));
    return CompleteRequest(*req, ORBIS_OK);
}

s32 PS4_SYSV_ABI
sceNpGetParentalControlInfoA(s32 req_id, Libraries::UserService::OrbisUserServiceUserId user_id,
                             s8* age, OrbisNpParentalControlInfo* info) {
    if (age == nullptr || info == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    std::scoped_lock lk{g_request_mutex};
    s32 err;
    NpRequest* req = GetRequest(req_id, &err);
    if (!req)
        return err;
    if (!g_shadnet_enabled || !Libraries::Np::NpHandler::GetInstance().IsPsnSignedIn(user_id)) {
        return CompleteRequest(*req, ORBIS_NP_ERROR_SIGNED_OUT);
    }
    LOG_DEBUG(Lib_NpManager, "req_id = {:#x}, user_id = {}", req_id, user_id);
    *age = 13; // TODO make it configurable?
    std::memset(info, 0, sizeof(OrbisNpParentalControlInfo));
    return CompleteRequest(*req, ORBIS_OK);
}

s32 PS4_SYSV_ABI sceNpGetAccountCountry(OrbisNpOnlineId* online_id,
                                        OrbisNpCountryCode* country_code) {
    if (online_id == nullptr || country_code == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    const s32 user_id = Libraries::Np::NpHandler::GetInstance().GetUserIdByOnlineId(*online_id);
    if (!g_shadnet_enabled || user_id == -1 ||
        !Libraries::Np::NpHandler::GetInstance().IsPsnSignedIn(user_id)) {
        return ORBIS_NP_ERROR_SIGNED_OUT;
    }
    std::memset(country_code, 0, sizeof(OrbisNpCountryCode));
    std::memcpy(country_code->country_code, "us", 2); // TODO: get NP country code from config
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpGetAccountCountryA(Libraries::UserService::OrbisUserServiceUserId user_id,
                                         OrbisNpCountryCode* country_code) {
    if (country_code == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    if (!g_shadnet_enabled || !Libraries::Np::NpHandler::GetInstance().IsPsnSignedIn(user_id)) {
        return ORBIS_NP_ERROR_SIGNED_OUT;
    }
    std::memset(country_code, 0, sizeof(OrbisNpCountryCode));
    std::memcpy(country_code->country_code, "us", 2); // TODO: get NP country code from config
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpGetAccountDateOfBirth(OrbisNpOnlineId* online_id,
                                            OrbisNpDate* date_of_birth) {
    if (online_id == nullptr || date_of_birth == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    const s32 user_id = Libraries::Np::NpHandler::GetInstance().GetUserIdByOnlineId(*online_id);
    if (!g_shadnet_enabled || user_id == -1 ||
        !Libraries::Np::NpHandler::GetInstance().IsPsnSignedIn(user_id)) {
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
    if (!g_shadnet_enabled || !Libraries::Np::NpHandler::GetInstance().IsPsnSignedIn(user_id)) {
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
    const s32 user_id = Libraries::Np::NpHandler::GetInstance().GetUserIdByOnlineId(*online_id);
    *game_status = (g_shadnet_enabled && user_id != -1 &&
                    Libraries::Np::NpHandler::GetInstance().IsPsnSignedIn(user_id))
                       ? OrbisNpGamePresenseStatus::Online
                       : OrbisNpGamePresenseStatus::Offline;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpGetGamePresenceStatusA(Libraries::UserService::OrbisUserServiceUserId user_id,
                                             OrbisNpGamePresenseStatus* game_status) {
    if (game_status == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    *game_status =
        (g_shadnet_enabled && Libraries::Np::NpHandler::GetInstance().IsPsnSignedIn(user_id))
            ? OrbisNpGamePresenseStatus::Online
            : OrbisNpGamePresenseStatus::Offline;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpGetAccountId(OrbisNpOnlineId* online_id, u64* account_id) {
    LOG_DEBUG(Lib_NpManager, "called");
    if (online_id == nullptr || account_id == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    const s32 user_id = Libraries::Np::NpHandler::GetInstance().GetUserIdByOnlineId(*online_id);
    if (!g_shadnet_enabled || user_id == -1 ||
        !Libraries::Np::NpHandler::GetInstance().IsPsnSignedIn(user_id)) {
        *account_id = 0;
        return ORBIS_NP_ERROR_SIGNED_OUT;
    }
    *account_id = Libraries::Np::NpHandler::GetInstance().GetAccountId(user_id);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpGetAccountIdA(Libraries::UserService::OrbisUserServiceUserId user_id,
                                    u64* account_id) {
    LOG_DEBUG(Lib_NpManager, "user_id {}", user_id);
    if (account_id == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    if (!g_shadnet_enabled || !Libraries::Np::NpHandler::GetInstance().IsPsnSignedIn(user_id)) {
        *account_id = 0;
        return ORBIS_NP_ERROR_SIGNED_OUT;
    }
    *account_id = Libraries::Np::NpHandler::GetInstance().GetAccountId(user_id);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpGetNpId(Libraries::UserService::OrbisUserServiceUserId user_id,
                              OrbisNpId* np_id) {
    LOG_DEBUG(Lib_NpManager, "user_id {}", user_id);
    if (np_id == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    if (!g_shadnet_enabled || !Libraries::Np::NpHandler::GetInstance().IsPsnSignedIn(user_id)) {
        return ORBIS_NP_ERROR_SIGNED_OUT;
    }
    *np_id = Libraries::Np::NpHandler::GetInstance().GetNpId(user_id);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpGetOnlineId(Libraries::UserService::OrbisUserServiceUserId user_id,
                                  OrbisNpOnlineId* online_id) {
    LOG_DEBUG(Lib_NpManager, "user_id {}", user_id);
    if (online_id == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    if (!g_shadnet_enabled || !Libraries::Np::NpHandler::GetInstance().IsPsnSignedIn(user_id)) {
        return ORBIS_NP_ERROR_SIGNED_OUT;
    }
    *online_id = Libraries::Np::NpHandler::GetInstance().GetOnlineId(user_id);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpGetNpReachabilityState(Libraries::UserService::OrbisUserServiceUserId user_id,
                                             OrbisNpReachabilityState* state) {
    if (state == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    *state = (g_shadnet_enabled && Libraries::Np::NpHandler::GetInstance().IsPsnSignedIn(user_id))
                 ? OrbisNpReachabilityState::Reachable
                 : OrbisNpReachabilityState::Unavailable;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpGetState(Libraries::UserService::OrbisUserServiceUserId user_id,
                               OrbisNpState* state) {
    if (state == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    if (!g_shadnet_enabled) {
        *state = OrbisNpState::SignedOut;
        LOG_DEBUG(Lib_NpManager, "shadNet disabled,SignedOut");
        return ORBIS_OK;
    }
    *state = Libraries::Np::NpHandler::GetInstance().IsPsnSignedIn(user_id)
                 ? OrbisNpState::SignedIn
                 : OrbisNpState::SignedOut;
    LOG_DEBUG(Lib_NpManager, "user_id={} state={}", user_id,
              *state == OrbisNpState::SignedIn ? "SignedIn" : "SignedOut");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
sceNpGetUserIdByAccountId(u64 account_id, Libraries::UserService::OrbisUserServiceUserId* user_id) {
    if (user_id == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    if (!g_shadnet_enabled) {
        return ORBIS_NP_ERROR_SIGNED_OUT;
    }
    const s32 found = Libraries::Np::NpHandler::GetInstance().GetUserIdByAccountId(account_id);
    if (found == -1) {
        return ORBIS_NP_ERROR_USER_NOT_FOUND;
    }
    *user_id = found;
    LOG_DEBUG(Lib_NpManager, "account_id={} → user_id={}", account_id, *user_id);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpHasSignedUp(Libraries::UserService::OrbisUserServiceUserId user_id,
                                  bool* has_signed_up) {
    if (has_signed_up == nullptr ||
        user_id == Libraries::UserService::ORBIS_USER_SERVICE_USER_ID_INVALID) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    const User* u = UserManagement.GetUserByID(user_id);
    if (!u) {
        return ORBIS_NP_ERROR_USER_NOT_FOUND;
    }
    // A user has signed up if they have a shadNet npid configured.
    // This is independent of shadnet_enabled and current connection state.
    *has_signed_up = !u->shadnet_npid.empty();
    return ORBIS_OK;
}

// Callbacks

s32 PS4_SYSV_ABI sceNpCheckCallback() {
    LOG_DEBUG(Lib_NpManager, "called");
    std::scoped_lock lk{g_np_callbacks_mutex};
    for (auto& [key, cb] : g_np_callbacks) {
        cb();
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpCheckCallbackForLib() {
    LOG_DEBUG(Lib_NpManager, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpRegisterStateCallback(OrbisNpStateCallback callback, void* userdata) {
    LOG_DEBUG(Lib_NpManager, "called");
    NpStateCb.func = callback;
    NpStateCb.userdata = userdata;
    // Register with NpHandler so it fires on live state changes.
    // The non-A variant additionally passes an OrbisNpId* built from the user's
    // shadnet_npid or nullptr when signing out.
    return Libraries::Np::NpHandler::GetInstance().RegisterStateCallback(
        [callback, userdata](Libraries::UserService::OrbisUserServiceUserId uid,
                             Libraries::Np::NpManager::OrbisNpState state) {
            // Use NpHandler's cached NpId — built from shadnet_npid at login.
            const OrbisNpId& cached = Libraries::Np::NpHandler::GetInstance().GetNpId(uid);
            OrbisNpId* np_id_ptr =
                (state == OrbisNpState::SignedIn && cached.handle.data[0] != '\0')
                    ? const_cast<OrbisNpId*>(&cached)
                    : nullptr;
            callback(uid, state, np_id_ptr, userdata);
        },
        userdata);
}

s32 PS4_SYSV_ABI sceNpRegisterStateCallbackA(OrbisNpStateCallbackA callback, void* userdata) {
    LOG_DEBUG(Lib_NpManager, "called");
    NpStateCb.func = callback;
    NpStateCb.userdata = userdata;
    // Register with NpHandler so the callback fires on live connection state changes.
    return Libraries::Np::NpHandler::GetInstance().RegisterStateCallback(
        [callback, userdata](Libraries::UserService::OrbisUserServiceUserId uid,
                             Libraries::Np::NpManager::OrbisNpState state) {
            callback(uid, state, userdata);
        },
        userdata);
}

s32 PS4_SYSV_ABI sceNpRegisterNpReachabilityStateCallback(OrbisNpReachabilityStateCallback callback,
                                                          void* userdata) {
    static s32 id = 0;
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    NpReachabilityCb.func = callback;
    NpReachabilityCb.userdata = userdata;
    return ++id; // TODO recheck?
}

s32 PS4_SYSV_ABI sceNpRegisterStateCallbackForToolkit(OrbisNpStateCallbackForNpToolkit callback,
                                                      void* userdata) {
    static s32 id = 0;
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    NpStateCbForNp.func = callback;
    NpStateCbForNp.userdata = userdata;
    return ++id; // TODO recheck?
}

void RegisterNpCallback(std::string key, std::function<void()> cb) {
    std::scoped_lock lk{g_np_callbacks_mutex};
    LOG_DEBUG(Lib_NpManager, "registering callback processing for {}", key);
    g_np_callbacks.emplace(key, cb);
}

void DeregisterNpCallback(std::string key) {
    std::scoped_lock lk{g_np_callbacks_mutex};
    LOG_DEBUG(Lib_NpManager, "deregistering callback processing for {}", key);
    g_np_callbacks.erase(key);
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    g_shadnet_enabled = EmulatorSettings.IsShadNetEnabled();
    Libraries::Np::NpHandler::GetInstance().Initialize();

    LIB_FUNCTION("GpLQDNKICac", "libSceNpManager", 1, "libSceNpManager", sceNpCreateRequest);
    LIB_FUNCTION("eiqMCt9UshI", "libSceNpManager", 1, "libSceNpManager", sceNpCreateAsyncRequest);
    LIB_FUNCTION("2rsFmlGWleQ", "libSceNpManager", 1, "libSceNpManager", sceNpCheckNpAvailability);
    LIB_FUNCTION("8Z2Jc5GvGDI", "libSceNpManager", 1, "libSceNpManager", sceNpCheckNpAvailabilityA);
    LIB_FUNCTION("KfGZg2y73oM", "libSceNpManager", 1, "libSceNpManager", sceNpCheckNpReachability);
    LIB_FUNCTION("r6MyYJkryz8", "libSceNpManager", 1, "libSceNpManager", sceNpCheckPlus);
    LIB_FUNCTION("KZ1Mj9yEGYc", "libSceNpManager", 1, "libSceNpManager", sceNpGetAccountLanguage);
    LIB_FUNCTION("TPMbgIxvog0", "libSceNpManager", 1, "libSceNpManager", sceNpGetAccountLanguageA);
    LIB_FUNCTION("ilwLM4zOmu4", "libSceNpManager", 1, "libSceNpManager",
                 sceNpGetParentalControlInfo);
    LIB_FUNCTION("m9L3O6yst-U", "libSceNpManager", 1, "libSceNpManager",
                 sceNpGetParentalControlInfoA);
    LIB_FUNCTION("OzKvTvg3ZYU", "libSceNpManager", 1, "libSceNpManager", sceNpAbortRequest);
    LIB_FUNCTION("jyi5p9XWUSs", "libSceNpManager", 1, "libSceNpManager", sceNpWaitAsync);
    LIB_FUNCTION("uqcPJLWL08M", "libSceNpManager", 1, "libSceNpManager", sceNpPollAsync);
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
    LIB_FUNCTION("VgYczPGB5ss", "libSceNpManager", 1, "libSceNpManager", sceNpGetUserIdByAccountId);
    LIB_FUNCTION("Oad3rvY-NJQ", "libSceNpManager", 1, "libSceNpManager", sceNpHasSignedUp);
    LIB_FUNCTION("3Zl8BePTh9Y", "libSceNpManager", 1, "libSceNpManager", sceNpCheckCallback);
    LIB_FUNCTION("JELHf4xPufo", "libSceNpManager", 1, "libSceNpManager", sceNpCheckCallbackForLib);
    LIB_FUNCTION("VfRSmPmj8Q8", "libSceNpManager", 1, "libSceNpManager",
                 sceNpRegisterStateCallback);
    LIB_FUNCTION("qQJfO8HAiaY", "libSceNpManager", 1, "libSceNpManager",
                 sceNpRegisterStateCallbackA);
    LIB_FUNCTION("hw5KNqAAels", "libSceNpManager", 1, "libSceNpManager",
                 sceNpRegisterNpReachabilityStateCallback);
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
