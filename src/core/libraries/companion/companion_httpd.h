// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::CompanionHttpd {

s32 PS4_SYSV_ABI sceCompanionHttpdAddHeader();
s32 PS4_SYSV_ABI sceCompanionHttpdGet2ndScreenStatus();
s32 PS4_SYSV_ABI sceCompanionHttpdGetEvent();
s32 PS4_SYSV_ABI sceCompanionHttpdGetUserId();
s32 PS4_SYSV_ABI sceCompanionHttpdInitialize();
s32 PS4_SYSV_ABI sceCompanionHttpdInitialize2();
s32 PS4_SYSV_ABI sceCompanionHttpdOptParamInitialize();
s32 PS4_SYSV_ABI sceCompanionHttpdRegisterRequestBodyReceptionCallback();
s32 PS4_SYSV_ABI sceCompanionHttpdRegisterRequestCallback();
s32 PS4_SYSV_ABI sceCompanionHttpdRegisterRequestCallback2();
s32 PS4_SYSV_ABI sceCompanionHttpdSetBody();
s32 PS4_SYSV_ABI sceCompanionHttpdSetStatus();
s32 PS4_SYSV_ABI sceCompanionHttpdStart();
s32 PS4_SYSV_ABI sceCompanionHttpdStop();
s32 PS4_SYSV_ABI sceCompanionHttpdTerminate();
s32 PS4_SYSV_ABI sceCompanionHttpdUnregisterRequestBodyReceptionCallback();
s32 PS4_SYSV_ABI sceCompanionHttpdUnregisterRequestCallback();

void RegisterlibSceCompanionHttpd(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::CompanionHttpd