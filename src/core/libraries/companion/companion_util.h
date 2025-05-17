// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::CompanionUtil {

s32 PS4_SYSV_ABI sceCompanionUtilGetEvent();
s32 PS4_SYSV_ABI sceCompanionUtilGetRemoteOskEvent();
s32 PS4_SYSV_ABI sceCompanionUtilInitialize();
s32 PS4_SYSV_ABI sceCompanionUtilOptParamInitialize();
s32 PS4_SYSV_ABI sceCompanionUtilTerminate();

void RegisterlibSceCompanionUtil(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::CompanionUtil