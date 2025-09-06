// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Np::NpAuth {

s32 PS4_SYSV_ABI sceNpAuthGetAuthorizationCode();
s32 PS4_SYSV_ABI sceNpAuthGetIdToken();
s32 PS4_SYSV_ABI sceNpAuthAbortRequest();
s32 PS4_SYSV_ABI sceNpAuthCreateAsyncRequest();
s32 PS4_SYSV_ABI sceNpAuthCreateRequest();
s32 PS4_SYSV_ABI sceNpAuthDeleteRequest(s32 id);
s32 PS4_SYSV_ABI sceNpAuthGetAuthorizationCodeA();
s32 PS4_SYSV_ABI sceNpAuthGetAuthorizationCodeV3();
s32 PS4_SYSV_ABI sceNpAuthGetIdTokenA();
s32 PS4_SYSV_ABI sceNpAuthGetIdTokenV3();
s32 PS4_SYSV_ABI sceNpAuthPollAsync();
s32 PS4_SYSV_ABI sceNpAuthSetTimeout();
s32 PS4_SYSV_ABI sceNpAuthWaitAsync();

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Np::NpAuth