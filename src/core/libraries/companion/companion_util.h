// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::CompanionUtil {

constexpr u32 ORBIS_COMPANION_UTIL_OK = 0;

struct sceCompanionUtilEvent {
    std::uint8_t blob[0x104]{}; /// 0x104 bytes of data, dont know what it is exactly
};

struct sceCompanionUtilContext {
    std::uint8_t blob[0x27B]{}; /// 0x27B bytes of data, dont know what it is exactly
};

u32 PS4_SYSV_ABI getEvent(sceCompanionUtilContext* ctx, sceCompanionUtilEvent* outEvent,
                          s32 param_3);
s32 PS4_SYSV_ABI sceCompanionUtilGetEvent(sceCompanionUtilEvent* outEvent);
s32 PS4_SYSV_ABI sceCompanionUtilGetRemoteOskEvent();
s32 PS4_SYSV_ABI sceCompanionUtilInitialize();
s32 PS4_SYSV_ABI sceCompanionUtilOptParamInitialize();
s32 PS4_SYSV_ABI sceCompanionUtilTerminate();

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::CompanionUtil