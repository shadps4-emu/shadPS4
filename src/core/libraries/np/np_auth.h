// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/np/np_types.h"
#include "core/libraries/system/userservice.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Np::NpAuth {

constexpr s32 ORBIS_NP_AUTH_REQUEST_LIMIT = 0x10;
constexpr s32 ORBIS_NP_AUTH_REQUEST_ID_OFFSET = 0x10000000;

struct OrbisNpAuthCreateAsyncRequestParameter {
    u64 size;
    u64 cpu_affinity_mask;
    s32 thread_priority;
    u8 padding[4];
};

struct OrbisNpAuthGetAuthorizationCodeParameter {
    u64 size;
    const OrbisNpOnlineId* online_id;
    const OrbisNpClientId* client_id;
    const char* scope;
};

struct OrbisNpAuthGetAuthorizationCodeParameterA {
    u64 size;
    Libraries::UserService::OrbisUserServiceUserId user_id;
    u8 padding[4];
    const OrbisNpClientId* client_id;
    const char* scope;
};

struct OrbisNpAuthGetIdTokenParameter {
    u64 size;
    const OrbisNpOnlineId* online_id;
    const OrbisNpClientId* client_id;
    const OrbisNpClientSecret* client_secret;
    const char* scope;
};

struct OrbisNpAuthGetIdTokenParameterA {
    u64 size;
    Libraries::UserService::OrbisUserServiceUserId user_id;
    u8 padding[4];
    const OrbisNpClientId* client_id;
    const OrbisNpClientSecret* client_secret;
    const char* scope;
};

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Np::NpAuth