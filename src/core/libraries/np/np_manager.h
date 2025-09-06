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

using OrbisNpStateCallbackForNpToolkit = PS4_SYSV_ABI void (*)(s32 userId, OrbisNpState state,
                                                               void* userdata);

enum class OrbisNpRequestState {
    None = 0,
    Ready = 1,
    Complete = 2,
};

struct OrbisNpCountryCode {
    char country_code[2];
    char end;
    char pad;
};

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Np::NpManager
