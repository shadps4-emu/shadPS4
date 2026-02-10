// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <functional>

#include "common/types.h"
#include "core/libraries/np/np_error.h"
#include "core/libraries/np/np_types.h"
#include "core/libraries/system/userservice.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Np::NpManager {

constexpr s32 ORBIS_NP_MANAGER_REQUEST_LIMIT = 0x20;
constexpr s32 ORBIS_NP_MANAGER_REQUEST_ID_OFFSET = 0x20000000;

enum class OrbisNpState : u32 {
    Unknown = 0,
    SignedOut = 1,
    SignedIn = 2,
};

enum class OrbisNpReachabilityState {
    Unavailable = 0,
    Available = 1,
    Reachable = 2,
};

using OrbisNpStateCallback =
    PS4_SYSV_ABI void (*)(Libraries::UserService::OrbisUserServiceUserId userId, OrbisNpState state,
                          OrbisNpId* npId, void* userdata);
using OrbisNpStateCallbackA = PS4_SYSV_ABI void (*)(
    Libraries::UserService::OrbisUserServiceUserId userId, OrbisNpState state, void* userdata);
using OrbisNpStateCallbackForNpToolkit = PS4_SYSV_ABI void (*)(
    Libraries::UserService::OrbisUserServiceUserId userId, OrbisNpState state, void* userdata);
using OrbisNpReachabilityStateCallback =
    PS4_SYSV_ABI void (*)(Libraries::UserService::OrbisUserServiceUserId userId,
                          OrbisNpReachabilityState state, void* userdata);

enum class OrbisNpGamePresenseStatus {
    Offline = 0,
    Online = 1,
};

struct OrbisNpCountryCode {
    char country_code[2];
    char end;
    char pad;
};

struct OrbisNpDate {
    u16 year;
    u8 month;
    u8 day;
};

struct OrbisNpLanguageCode {
    char code[6];
    char padding[10];
};

struct OrbisNpParentalControlInfo {
    bool content_restriction;
    bool chat_restriction;
    bool user_generated_content_restriction;
};

struct OrbisNpCheckPlusParameter {
    u64 size;
    Libraries::UserService::OrbisUserServiceUserId user_id;
    u8 padding[4];
    u64 features;
    u8 reserved[32];
};

struct OrbisNpCheckPlusResult {
    bool authorized;
    u8 reserved[32];
};

struct OrbisNpCreateAsyncRequestParameter {
    u64 size;
    u64 cpu_affinity_mask;
    s32 thread_priority;
    u8 padding[4];
};

void RegisterNpCallback(std::string key, std::function<void()> cb);
void DeregisterNpCallback(std::string key);

s32 PS4_SYSV_ABI sceNpGetNpId(Libraries::UserService::OrbisUserServiceUserId user_id,
                              OrbisNpId* np_id);
s32 PS4_SYSV_ABI sceNpGetOnlineId(Libraries::UserService::OrbisUserServiceUserId user_id,
                                  OrbisNpOnlineId* online_id);

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Np::NpManager
