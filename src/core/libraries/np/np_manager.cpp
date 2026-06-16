// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>
#include <cctype>
#include <deque>
#include <map>
#include <mutex>
#include <variant>

#include <core/user_settings.h>
#include "common/elf_info.h"
#include "common/logging/log.h"
#include "core/emulator_settings.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/kernel/process.h"
#include "core/libraries/libs.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_manager.h"
#include "core/tls.h"
#include "core/user_manager.h"
#include "np_handler.h"

namespace Libraries::Np::NpManager {

static bool g_shadnet_enabled = false;
static s32 g_firmware_version = -1;
static s32 g_active_requests = 0;
static std::mutex g_request_mutex;

static std::map<std::string, std::function<void()>> g_np_callbacks;
static std::mutex g_np_callbacks_mutex;
static std::mutex g_np_state_events_mutex;
static std::mutex g_np_state_callbacks_mutex;

constexpr s32 ORBIS_NP_STATE_CALLBACK_MAX = 8;

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

static void FillCountryCodeFromProfile(Libraries::UserService::OrbisUserServiceUserId user_id,
                                       OrbisNpCountryCode* out) {
    std::memset(out, 0, sizeof(OrbisNpCountryCode));
    const User* u = UserManagement.GetUserByID(user_id);
    const std::string cfg = u ? u->np_country : std::string();
    const bool ok = cfg.size() == 2 && std::isalpha(static_cast<unsigned char>(cfg[0])) &&
                    std::isalpha(static_cast<unsigned char>(cfg[1]));
    const char* src = ok ? cfg.c_str() : "us";
    std::memcpy(out->country_code, src, 2);
}

static void FillLanguageCodeFromProfile(Libraries::UserService::OrbisUserServiceUserId user_id,
                                        OrbisNpLanguageCode* out) {
    std::memset(out, 0, sizeof(OrbisNpLanguageCode));
    const User* u = UserManagement.GetUserByID(user_id);
    const std::string cfg = u ? u->np_language : std::string();
    const bool ok = cfg.size() == 2 && std::isalpha(static_cast<unsigned char>(cfg[0])) &&
                    std::isalpha(static_cast<unsigned char>(cfg[1]));
    const char* src = ok ? cfg.c_str() : "en";
    std::memcpy(out->code, src, 2);
}

static s8 GetAgeFromProfile(Libraries::UserService::OrbisUserServiceUserId user_id) {
    const User* u = UserManagement.GetUserByID(user_id);
    const int v = u ? static_cast<int>(u->np_age) : 0;
    if (v <= 0 || v > 127) {
        return 13;
    }
    return static_cast<s8>(v);
}

static void FillDateOfBirthFromProfile(Libraries::UserService::OrbisUserServiceUserId user_id,
                                       OrbisNpDate* out) {
    const User* u = UserManagement.GetUserByID(user_id);
    const std::string s = u ? u->np_date_of_birth : std::string();

    int y = 0, m = 0, d = 0;
    bool ok = s.size() == 10 && s[4] == '-' && s[7] == '-';
    if (ok) {
        auto is_digit_at = [&](size_t i) { return std::isdigit(static_cast<unsigned char>(s[i])); };
        for (size_t i : {0u, 1u, 2u, 3u, 5u, 6u, 8u, 9u}) {
            if (!is_digit_at(i)) {
                ok = false;
                break;
            }
        }
    }
    if (ok) {
        y = (s[0] - '0') * 1000 + (s[1] - '0') * 100 + (s[2] - '0') * 10 + (s[3] - '0');
        m = (s[5] - '0') * 10 + (s[6] - '0');
        d = (s[8] - '0') * 10 + (s[9] - '0');
        ok = y >= 1900 && y <= 2100 && m >= 1 && m <= 12 && d >= 1 && d <= 31;
    }

    out->year = ok ? static_cast<u16>(y) : 2000;
    out->month = ok ? static_cast<u16>(m) : 1;
    out->day = ok ? static_cast<u16>(d) : 1;
}

static std::vector<NpRequest> g_requests;

static s32 CreateNpRequest(bool async) {
    std::scoped_lock lk{g_request_mutex};

    if (g_active_requests == ORBIS_NP_MANAGER_REQUEST_LIMIT) {
        return ORBIS_NP_ERROR_REQUEST_MAX;
    }

    s32 req_index = 0;
    while (req_index < static_cast<s32>(g_requests.size())) {
        // Find first nonexistant request
        if (g_requests[req_index].state == NpRequestState::None) {
            // There is no request at this index, set the index to ready then break.
            g_requests[req_index].state = NpRequestState::Ready;
            g_requests[req_index].async = async;
            break;
        }
        req_index++;
    }

    if (req_index == static_cast<s32>(g_requests.size())) {
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
    if (req_id == 0) {
        *out_err = ORBIS_NP_ERROR_INVALID_ARGUMENT;
        return nullptr;
    }
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

s32 PS4_SYSV_ABI sceNpCheckNpAvailability(s32 req_id, OrbisNpOnlineId* online_id) {
    if (online_id == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    const s32 user_id = Libraries::Np::NpHandler::GetInstance().GetUserIdByOnlineId(*online_id);
    if (user_id == -1) {
        return ORBIS_NP_ERROR_USER_NOT_FOUND;
    }
    return sceNpCheckNpAvailabilityA(req_id, user_id);
}

s32 PS4_SYSV_ABI sceNpCheckNpAvailabilityA(s32 req_id,
                                           Libraries::UserService::OrbisUserServiceUserId user_id) {
    if (user_id == Libraries::UserService::ORBIS_USER_SERVICE_USER_ID_INVALID) {
        LOG_ERROR(Lib_NpManager, "invalid user_id {}", user_id);
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
    return CompleteRequest(*req, ORBIS_OK);
}

s32 PS4_SYSV_ABI sceNpCheckNpReachability(s32 req_id,
                                          Libraries::UserService::OrbisUserServiceUserId user_id) {
    if (user_id == Libraries::UserService::ORBIS_USER_SERVICE_USER_ID_INVALID) {
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
    if (param->user_id == Libraries::UserService::ORBIS_USER_SERVICE_USER_ID_INVALID) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    if (param->features < 1 || param->features > 3) {
        // TODO: If compiled SDK version is greater or equal to fw 3.50,
        // // error if param->features != 1 instead.
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    // The reserved field must be zero-initialized by the caller.
    for (u8 b : param->reserved) {
        if (b != 0) {
            return ORBIS_NP_ERROR_INVALID_ARGUMENT;
        }
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

s32 PS4_SYSV_ABI sceNpGetAccountLanguage(s32 req_id, OrbisNpOnlineId* online_id,
                                         OrbisNpLanguageCode* language) {
    if (online_id == nullptr || language == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    const s32 user_id = Libraries::Np::NpHandler::GetInstance().GetUserIdByOnlineId(*online_id);
    if (user_id == -1) {
        return ORBIS_NP_ERROR_USER_NOT_FOUND;
    }
    return sceNpGetAccountLanguageA(req_id, user_id, language);
}

s32 PS4_SYSV_ABI sceNpGetAccountLanguageA(s32 req_id,
                                          Libraries::UserService::OrbisUserServiceUserId user_id,
                                          OrbisNpLanguageCode* language) {
    if (language == nullptr ||
        user_id == Libraries::UserService::ORBIS_USER_SERVICE_USER_ID_INVALID) {
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
    FillLanguageCodeFromProfile(user_id, language);
    return CompleteRequest(*req, ORBIS_OK);
}

s32 PS4_SYSV_ABI sceNpGetParentalControlInfo(s32 req_id, OrbisNpOnlineId* online_id, s8* age,
                                             OrbisNpParentalControlInfo* info) {
    if (online_id == nullptr || age == nullptr || info == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    const s32 user_id = Libraries::Np::NpHandler::GetInstance().GetUserIdByOnlineId(*online_id);
    if (user_id == -1) {
        return ORBIS_NP_ERROR_USER_NOT_FOUND;
    }
    return sceNpGetParentalControlInfoA(req_id, user_id, age, info);
}

s32 PS4_SYSV_ABI
sceNpGetParentalControlInfoA(s32 req_id, Libraries::UserService::OrbisUserServiceUserId user_id,
                             s8* age, OrbisNpParentalControlInfo* info) {
    if (age == nullptr || info == nullptr ||
        user_id == Libraries::UserService::ORBIS_USER_SERVICE_USER_ID_INVALID) {
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
    *age = GetAgeFromProfile(user_id);
    std::memset(info, 0, sizeof(OrbisNpParentalControlInfo));
    return CompleteRequest(*req, ORBIS_OK);
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

s32 PS4_SYSV_ABI sceNpGetAccountCountry(OrbisNpOnlineId* online_id,
                                        OrbisNpCountryCode* country_code) {
    if (online_id == nullptr || country_code == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    const s32 user_id = Libraries::Np::NpHandler::GetInstance().GetUserIdByOnlineId(*online_id);
    if (user_id == -1) {
        return ORBIS_NP_ERROR_USER_NOT_FOUND;
    }
    if (!g_shadnet_enabled || !Libraries::Np::NpHandler::GetInstance().IsPsnSignedIn(user_id)) {
        return ORBIS_NP_ERROR_SIGNED_OUT;
    }
    FillCountryCodeFromProfile(user_id, country_code);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpGetAccountCountryA(Libraries::UserService::OrbisUserServiceUserId user_id,
                                         OrbisNpCountryCode* country_code) {
    if (country_code == nullptr ||
        user_id == Libraries::UserService::ORBIS_USER_SERVICE_USER_ID_INVALID) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    if (UserManagement.GetUserByID(user_id) == nullptr) {
        return ORBIS_NP_ERROR_USER_NOT_FOUND;
    }
    if (!g_shadnet_enabled || !Libraries::Np::NpHandler::GetInstance().IsPsnSignedIn(user_id)) {
        return ORBIS_NP_ERROR_SIGNED_OUT;
    }
    FillCountryCodeFromProfile(user_id, country_code);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpGetAccountDateOfBirth(OrbisNpOnlineId* online_id,
                                            OrbisNpDate* date_of_birth) {
    if (online_id == nullptr || date_of_birth == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    const s32 user_id = Libraries::Np::NpHandler::GetInstance().GetUserIdByOnlineId(*online_id);
    if (user_id == -1) {
        return ORBIS_NP_ERROR_USER_NOT_FOUND;
    }
    if (!g_shadnet_enabled || !Libraries::Np::NpHandler::GetInstance().IsPsnSignedIn(user_id)) {
        return ORBIS_NP_ERROR_SIGNED_OUT;
    }
    FillDateOfBirthFromProfile(user_id, date_of_birth);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpGetAccountDateOfBirthA(Libraries::UserService::OrbisUserServiceUserId user_id,
                                             OrbisNpDate* date_of_birth) {
    if (date_of_birth == nullptr ||
        user_id == Libraries::UserService::ORBIS_USER_SERVICE_USER_ID_INVALID) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    if (UserManagement.GetUserByID(user_id) == nullptr) {
        return ORBIS_NP_ERROR_USER_NOT_FOUND;
    }
    if (!g_shadnet_enabled || !Libraries::Np::NpHandler::GetInstance().IsPsnSignedIn(user_id)) {
        return ORBIS_NP_ERROR_SIGNED_OUT;
    }
    FillDateOfBirthFromProfile(user_id, date_of_birth);
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
    if (!g_shadnet_enabled) {
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
    if (UserManagement.GetUserByID(user_id) == nullptr) {
        *account_id = 0;
        return ORBIS_NP_ERROR_USER_NOT_FOUND;
    }
    if (!g_shadnet_enabled) {
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
    const auto* user = UserManagement.GetUserByID(user_id);
    if (user == nullptr) {
        return ORBIS_NP_ERROR_USER_NOT_FOUND;
    }
    if (!g_shadnet_enabled) {
        return ORBIS_NP_ERROR_SIGNED_OUT;
    }
    memset(np_id, 0, sizeof(OrbisNpId));
    strncpy(np_id->handle.data, user->user_name.c_str(), sizeof(np_id->handle.data) - 1);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpGetOnlineId(Libraries::UserService::OrbisUserServiceUserId user_id,
                                  OrbisNpOnlineId* online_id) {
    LOG_DEBUG(Lib_NpManager, "user_id {}", user_id);
    if (online_id == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    const auto* user = UserManagement.GetUserByID(user_id);
    if (user == nullptr) {
        return ORBIS_NP_ERROR_USER_NOT_FOUND;
    }
    if (!g_shadnet_enabled) {
        return ORBIS_NP_ERROR_SIGNED_OUT;
    }
    memset(online_id, 0, sizeof(OrbisNpOnlineId));
    strncpy(online_id->data, user->user_name.c_str(), sizeof(online_id->data) - 1);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpGetNpReachabilityState(Libraries::UserService::OrbisUserServiceUserId user_id,
                                             OrbisNpReachabilityState* state) {
    if (state == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    if (UserManagement.GetUserByID(user_id) == nullptr) {
        return ORBIS_NP_ERROR_USER_NOT_FOUND;
    }

    if (user_id == Libraries::UserService::ORBIS_USER_SERVICE_USER_ID_INVALID) {
        if (g_firmware_version < 0 || g_firmware_version >= Common::ElfInfo::FW_400) {
            return ORBIS_NP_ERROR_INVALID_ARGUMENT;
        }
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
    if (UserManagement.GetUserByID(user_id) == nullptr) {
        return ORBIS_NP_ERROR_USER_NOT_FOUND;
    }
    if (user_id == Libraries::UserService::ORBIS_USER_SERVICE_USER_ID_INVALID) {
        if (g_firmware_version < 0 || g_firmware_version >= Common::ElfInfo::FW_900) {
            return ORBIS_NP_ERROR_INVALID_ARGUMENT;
        }
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
    *user_id = 1000;
    LOG_DEBUG(Lib_NpManager, "userid({}) = {}", account_id, *user_id);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpHasSignedUp(Libraries::UserService::OrbisUserServiceUserId user_id,
                                  bool* has_signed_up) {
    LOG_DEBUG(Lib_NpManager, "called");
    if (has_signed_up == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    *has_signed_up = false;
    const User* u = UserManagement.GetUserByID(user_id);
    if (u == nullptr) {
        return ORBIS_NP_ERROR_USER_NOT_FOUND;
    }
    if (user_id == Libraries::UserService::ORBIS_USER_SERVICE_USER_ID_INVALID) {
        if (g_firmware_version < 0 || g_firmware_version >= Common::ElfInfo::FW_900) {
            return ORBIS_NP_ERROR_INVALID_ARGUMENT;
        }
    }
    // A user has signed up if they have a shadNet npid configured.
    // This is independent of shadnet_enabled and current connection state.
    *has_signed_up = !u->shadnet_npid.empty();
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpSetContentRestriction(const OrbisNpContentRestriction* restriction) {
    LOG_ERROR(Lib_NpManager, "(STUBBED) called");
    if (restriction == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    if (restriction->size != sizeof(OrbisNpContentRestriction)) {
        return ORBIS_NP_ERROR_INVALID_SIZE;
    }
    if (restriction->default_age_restriction < 0 || restriction->age_restriction_count > 0x100) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    if (restriction->age_restriction_count > 0 && restriction->age_restriction == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpSetNpTitleId(const OrbisNpTitleId* title_id,
                                   const OrbisNpTitleSecret* title_secret) {
    if (title_id == nullptr || title_secret == nullptr) {
        LOG_ERROR(Lib_NpManager, "called with invalid arguments");
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    LOG_ERROR(Lib_NpManager, "(STUBBED) called, title_id = {}", title_id->id);
    return ORBIS_OK;
}

struct NpStateCallbackForNpToolkit {
    OrbisNpStateCallbackForNpToolkit func;
    void* userdata;
};

NpStateCallbackForNpToolkit NpStateCbForNp;

struct LegacyNpStateCallback {
    OrbisNpStateCallback func;
    void* userdata;
};

LegacyNpStateCallback LegacyNpStateCb;

struct NpStateCallbackAEntry {
    OrbisNpStateCallbackA func;
    void* userdata;
    bool in_use;
};

static std::array<NpStateCallbackAEntry, ORBIS_NP_STATE_CALLBACK_MAX> g_np_state_callbacks{};

struct PendingNpStateEvent {
    Libraries::UserService::OrbisUserServiceUserId user_id;
    OrbisNpState state;
    OrbisNpId np_id;
    bool has_np_id;
};

static std::deque<PendingNpStateEvent> g_np_state_events;

static void QueueNpStateEvent(Libraries::UserService::OrbisUserServiceUserId user_id,
                              OrbisNpState state) {
    const auto* user = UserManagement.GetUserByID(user_id);
    if (user == nullptr) {
        return;
    }

    PendingNpStateEvent event{};
    event.user_id = user_id;
    event.state = state;
    event.has_np_id = state == OrbisNpState::SignedIn;
    if (event.has_np_id) {
        std::strncpy(event.np_id.handle.data, user->user_name.c_str(),
                     sizeof(event.np_id.handle.data) - 1);
    }

    std::scoped_lock lk{g_np_state_events_mutex};
    g_np_state_events.emplace_back(event);
}

void NotifyNpStateFromUserServiceEvent(Libraries::UserService::OrbisUserServiceEventType event_type,
                                       Libraries::UserService::OrbisUserServiceUserId user_id) {
    switch (event_type) {
    case Libraries::UserService::OrbisUserServiceEventType::Login:
        QueueNpStateEvent(user_id,
                          g_shadnet_enabled ? OrbisNpState::SignedIn : OrbisNpState::SignedOut);
        break;
    case Libraries::UserService::OrbisUserServiceEventType::Logout:
        QueueNpStateEvent(user_id, OrbisNpState::SignedOut);
        break;
    default:
        break;
    }
}

static s32 RegisterStateCallbackA(OrbisNpStateCallbackA callback, void* userdata) {
    if (callback == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    std::scoped_lock lk{g_np_state_callbacks_mutex};

    for (const auto& entry : g_np_state_callbacks) {
        if (!entry.in_use) {
            continue;
        }
        if (entry.func == callback) {
            return ORBIS_NP_ERROR_CALLBACK_ALREADY_REGISTERED;
        }
    }

    for (size_t i = 0; i < g_np_state_callbacks.size(); ++i) {
        auto& entry = g_np_state_callbacks[i];
        if (entry.in_use) {
            continue;
        }
        entry.func = callback;
        entry.userdata = userdata;
        entry.in_use = true;
        return static_cast<s32>(i + 1);
    }

    return ORBIS_NP_ERROR_CALLBACK_MAX;
}

static s32 UnregisterStateCallbackAById(s32 callback_id) {
    if (callback_id <= 0 || callback_id > static_cast<s32>(g_np_state_callbacks.size())) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    std::scoped_lock lk{g_np_state_callbacks_mutex};

    auto& entry = g_np_state_callbacks[callback_id - 1];
    if (!entry.in_use) {
        return ORBIS_NP_ERROR_CALLBACK_NOT_REGISTERED;
    }

    entry = {};
    return ORBIS_OK;
}

static void DispatchPendingNpStateCallbacks() {
    std::deque<PendingNpStateEvent> pending_events;
    LegacyNpStateCallback legacy_callback{};
    std::array<NpStateCallbackAEntry, ORBIS_NP_STATE_CALLBACK_MAX> callbacks;
    {
        std::scoped_lock lk{g_np_state_events_mutex, g_np_state_callbacks_mutex};
        if (g_np_state_events.empty()) {
            return;
        }
        pending_events.swap(g_np_state_events);
        legacy_callback = LegacyNpStateCb;
        callbacks = g_np_state_callbacks;
    }

    for (auto& event : pending_events) {
        if (legacy_callback.func != nullptr) {
            legacy_callback.func(event.user_id, event.state,
                                 event.has_np_id ? &event.np_id : nullptr,
                                 legacy_callback.userdata);
        }

        for (const auto& entry : callbacks) {
            if (!entry.in_use) {
                continue;
            }

            if (entry.func != nullptr) {
                entry.func(event.user_id, event.state, entry.userdata);
            }
        }

        if (NpStateCbForNp.func != nullptr) {
            NpStateCbForNp.func(event.user_id, event.state, NpStateCbForNp.userdata);
        }
    }
}

s32 PS4_SYSV_ABI sceNpCheckCallback() {
    LOG_DEBUG(Lib_NpManager, "(STUBBED) called");
    DispatchPendingNpStateCallbacks();

    std::scoped_lock lk{g_np_callbacks_mutex};

    for (auto i : g_np_callbacks) {
        (i.second)();
    }

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpCheckCallbackForLib() {
    LOG_DEBUG(Lib_NpManager, "(STUBBED) called");
    DispatchPendingNpStateCallbacks();
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpRegisterStateCallback(OrbisNpStateCallback callback, void* userdata) {
    if (callback == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    std::scoped_lock lk{g_np_state_callbacks_mutex};
    if (LegacyNpStateCb.func != nullptr) {
        return ORBIS_NP_ERROR_CALLBACK_ALREADY_REGISTERED;
    }

    LOG_INFO(Lib_NpManager, "called, userdata = {}", userdata);
    LegacyNpStateCb.func = callback;
    LegacyNpStateCb.userdata = userdata;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpUnregisterStateCallback(s32 callback_id) {
    LOG_INFO(Lib_NpManager, "called, callback_id = {}", callback_id);
    if (callback_id != 0) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }

    std::scoped_lock lk{g_np_state_callbacks_mutex};
    if (LegacyNpStateCb.func == nullptr) {
        return ORBIS_NP_ERROR_CALLBACK_NOT_REGISTERED;
    }

    LegacyNpStateCb = {};
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpRegisterStateCallbackA(OrbisNpStateCallbackA callback, void* userdata) {
    LOG_INFO(Lib_NpManager, "called, userdata = {}", userdata);
    return RegisterStateCallbackA(callback, userdata);
}

s32 PS4_SYSV_ABI sceNpUnregisterStateCallbackA(s32 callback_id) {
    LOG_INFO(Lib_NpManager, "called, callback_id = {}", callback_id);
    return UnregisterStateCallbackAById(callback_id);
}

struct NpReachabilityStateCallback {
    OrbisNpReachabilityStateCallback func;
    void* userdata;
};

NpReachabilityStateCallback NpReachabilityCb;

s32 PS4_SYSV_ABI sceNpRegisterNpReachabilityStateCallback(OrbisNpReachabilityStateCallback callback,
                                                          void* userdata) {
    if (callback == nullptr) {
        return ORBIS_NP_ERROR_INVALID_ARGUMENT;
    }
    if (NpReachabilityCb.func != nullptr) {
        return ORBIS_NP_ERROR_CALLBACK_ALREADY_REGISTERED;
    }

    LOG_INFO(Lib_NpManager, "called");
    NpReachabilityCb.func = callback;
    NpReachabilityCb.userdata = userdata;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpUnregisterNpReachabilityStateCallback() {
    if (NpReachabilityCb.func == nullptr) {
        return ORBIS_NP_ERROR_CALLBACK_NOT_REGISTERED;
    }

    NpReachabilityCb = {};
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
    ASSERT_MSG(Libraries::Kernel::sceKernelGetCompiledSdkVersion(&g_firmware_version) == ORBIS_OK,
               "Failed to get compiled SDK version.");
    g_shadnet_enabled = EmulatorSettings.IsShadNetEnabled();

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
    LIB_FUNCTION("A2CQ3kgSopQ", "libSceNpManager", 1, "libSceNpManager",
                 sceNpSetContentRestriction);
    LIB_FUNCTION("Ec63y59l9tw", "libSceNpManager", 1, "libSceNpManager", sceNpSetNpTitleId);

    LIB_FUNCTION("3Zl8BePTh9Y", "libSceNpManager", 1, "libSceNpManager", sceNpCheckCallback);
    LIB_FUNCTION("JELHf4xPufo", "libSceNpManager", 1, "libSceNpManager", sceNpCheckCallbackForLib);
    LIB_FUNCTION("VfRSmPmj8Q8", "libSceNpManager", 1, "libSceNpManager",
                 sceNpRegisterStateCallback);
    LIB_FUNCTION("mjjTXh+NHWY", "libSceNpManager", 1, "libSceNpManager",
                 sceNpUnregisterStateCallback);
    LIB_FUNCTION("qQJfO8HAiaY", "libSceNpManager", 1, "libSceNpManager",
                 sceNpRegisterStateCallbackA);
    LIB_FUNCTION("M3wFXbYQtAA", "libSceNpManager", 1, "libSceNpManager",
                 sceNpUnregisterStateCallbackA);
    LIB_FUNCTION("hw5KNqAAels", "libSceNpManager", 1, "libSceNpManager",
                 sceNpRegisterNpReachabilityStateCallback);
    LIB_FUNCTION("cRILAEvn+9M", "libSceNpManager", 1, "libSceNpManager",
                 sceNpUnregisterNpReachabilityStateCallback);
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
