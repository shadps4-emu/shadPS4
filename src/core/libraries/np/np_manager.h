// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

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

using OrbisNpStateCallbackForNpToolkit = PS4_SYSV_ABI void (*)(s32 userId, OrbisNpState state,
                                                               void* userdata);

enum class OrbisNpGamePresenseStatus {
    Offline = 0,
    Online = 1,
};

enum class OrbisNpReachabilityState {
    Unavailable = 0,
    Available = 1,
    Reachable = 2,
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

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Np::NpManager
