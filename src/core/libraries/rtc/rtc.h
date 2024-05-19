// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Rtc {

struct OrbisRtcTick {
    u64 tick;
};

int PS4_SYSV_ABI sceRtcCheckValid();
int PS4_SYSV_ABI sceRtcCompareTick();
int PS4_SYSV_ABI sceRtcConvertLocalTimeToUtc();
int PS4_SYSV_ABI sceRtcConvertUtcToLocalTime();
int PS4_SYSV_ABI sceRtcEnd();
int PS4_SYSV_ABI sceRtcFormatRFC2822();
int PS4_SYSV_ABI sceRtcFormatRFC2822LocalTime();
int PS4_SYSV_ABI sceRtcFormatRFC3339();
int PS4_SYSV_ABI sceRtcFormatRFC3339LocalTime();
int PS4_SYSV_ABI sceRtcFormatRFC3339Precise();
int PS4_SYSV_ABI sceRtcFormatRFC3339PreciseLocalTime();
int PS4_SYSV_ABI sceRtcGetCurrentAdNetworkTick();
int PS4_SYSV_ABI sceRtcGetCurrentClock();
int PS4_SYSV_ABI sceRtcGetCurrentClockLocalTime();
int PS4_SYSV_ABI sceRtcGetCurrentDebugNetworkTick();
int PS4_SYSV_ABI sceRtcGetCurrentNetworkTick();
int PS4_SYSV_ABI sceRtcGetCurrentRawNetworkTick();
int PS4_SYSV_ABI sceRtcGetCurrentTick(OrbisRtcTick* pTick);
int PS4_SYSV_ABI sceRtcGetDayOfWeek();
int PS4_SYSV_ABI sceRtcGetDaysInMonth();
int PS4_SYSV_ABI sceRtcGetDosTime();
int PS4_SYSV_ABI sceRtcGetTick();
int PS4_SYSV_ABI sceRtcGetTickResolution();
int PS4_SYSV_ABI sceRtcGetTime_t();
int PS4_SYSV_ABI sceRtcGetWin32FileTime();
int PS4_SYSV_ABI sceRtcInit();
int PS4_SYSV_ABI sceRtcIsLeapYear();
int PS4_SYSV_ABI sceRtcParseDateTime();
int PS4_SYSV_ABI sceRtcParseRFC3339();
int PS4_SYSV_ABI sceRtcSetConf();
int PS4_SYSV_ABI sceRtcSetCurrentAdNetworkTick();
int PS4_SYSV_ABI sceRtcSetCurrentDebugNetworkTick();
int PS4_SYSV_ABI sceRtcSetCurrentNetworkTick();
int PS4_SYSV_ABI sceRtcSetCurrentTick();
int PS4_SYSV_ABI sceRtcSetDosTime();
int PS4_SYSV_ABI sceRtcSetTick();
int PS4_SYSV_ABI sceRtcSetTime_t();
int PS4_SYSV_ABI sceRtcSetWin32FileTime();
int PS4_SYSV_ABI sceRtcTickAddDays();
int PS4_SYSV_ABI sceRtcTickAddHours();
int PS4_SYSV_ABI sceRtcTickAddMicroseconds();
int PS4_SYSV_ABI sceRtcTickAddMinutes();
int PS4_SYSV_ABI sceRtcTickAddMonths();
int PS4_SYSV_ABI sceRtcTickAddSeconds();
int PS4_SYSV_ABI sceRtcTickAddTicks();
int PS4_SYSV_ABI sceRtcTickAddWeeks();
int PS4_SYSV_ABI sceRtcTickAddYears();

void RegisterlibSceRtc(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Rtc