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

struct OrbisRtcDateTime {
    int16_t year;
    int16_t month;
    int16_t day;
    int16_t hour;
    int16_t minute;
    int16_t second;
    int32_t microsecond;
};

int PS4_SYSV_ABI sceRtcCheckValid(OrbisRtcDateTime* pTime);
int PS4_SYSV_ABI sceRtcCompareTick(OrbisRtcTick* pTick1, OrbisRtcTick* pTick2);
int PS4_SYSV_ABI sceRtcConvertLocalTimeToUtc(OrbisRtcTick* pTick1, OrbisRtcTick* pTick2);
int PS4_SYSV_ABI sceRtcConvertUtcToLocalTime(OrbisRtcTick* pTick1, OrbisRtcTick* pTick2);
int PS4_SYSV_ABI sceRtcEnd();
int PS4_SYSV_ABI sceRtcFormatRFC2822();
int PS4_SYSV_ABI sceRtcFormatRFC2822LocalTime();
int PS4_SYSV_ABI sceRtcFormatRFC3339();
int PS4_SYSV_ABI sceRtcFormatRFC3339LocalTime();
int PS4_SYSV_ABI sceRtcFormatRFC3339Precise();
int PS4_SYSV_ABI sceRtcFormatRFC3339PreciseLocalTime();
int PS4_SYSV_ABI sceRtcGetCurrentAdNetworkTick(OrbisRtcTick* pTick);
int PS4_SYSV_ABI sceRtcGetCurrentClock(OrbisRtcDateTime* pTime, int minutesToAdd);
int PS4_SYSV_ABI sceRtcGetCurrentClockLocalTime(OrbisRtcDateTime* pTick);
int PS4_SYSV_ABI sceRtcGetCurrentDebugNetworkTick(OrbisRtcTick* pTick);
int PS4_SYSV_ABI sceRtcGetCurrentNetworkTick(OrbisRtcTick* pTick);
int PS4_SYSV_ABI sceRtcGetCurrentRawNetworkTick(OrbisRtcTick* pTick);
int PS4_SYSV_ABI sceRtcGetCurrentTick(OrbisRtcTick* pTick);
int PS4_SYSV_ABI sceRtcGetDayOfWeek();
int PS4_SYSV_ABI sceRtcGetDaysInMonth(int year, int month);
int PS4_SYSV_ABI sceRtcGetDosTime();
int PS4_SYSV_ABI sceRtcGetTick(OrbisRtcDateTime* pTime, OrbisRtcTick* pTick);
int PS4_SYSV_ABI sceRtcGetTickResolution();
int PS4_SYSV_ABI sceRtcGetTime_t();
int PS4_SYSV_ABI sceRtcGetWin32FileTime();
int PS4_SYSV_ABI sceRtcInit();
int PS4_SYSV_ABI sceRtcIsLeapYear(int yearInt);
int PS4_SYSV_ABI sceRtcParseDateTime();
int PS4_SYSV_ABI sceRtcParseRFC3339();
int PS4_SYSV_ABI sceRtcSetConf();
int PS4_SYSV_ABI sceRtcSetCurrentAdNetworkTick();
int PS4_SYSV_ABI sceRtcSetCurrentDebugNetworkTick();
int PS4_SYSV_ABI sceRtcSetCurrentNetworkTick();
int PS4_SYSV_ABI sceRtcSetCurrentTick();
int PS4_SYSV_ABI sceRtcSetDosTime(OrbisRtcDateTime* pTime, u32 dosTime);
int PS4_SYSV_ABI sceRtcSetTick(OrbisRtcDateTime* pTime, OrbisRtcTick* pTick);
int PS4_SYSV_ABI sceRtcSetTime_t(OrbisRtcDateTime* pTime, u64 iTime);
int PS4_SYSV_ABI sceRtcSetWin32FileTime(OrbisRtcDateTime* pTime, u64 win32Time);
int PS4_SYSV_ABI sceRtcTickAddDays(OrbisRtcTick* pTick1, OrbisRtcTick* pTick2, s32 addingDays);
int PS4_SYSV_ABI sceRtcTickAddHours(OrbisRtcTick* pTick1, OrbisRtcTick* pTick2, s32 addingHours);
int PS4_SYSV_ABI sceRtcTickAddMicroseconds(OrbisRtcTick* pTick1, OrbisRtcTick* pTick2,
                                           s64 addingMicroseconds);
int PS4_SYSV_ABI sceRtcTickAddMinutes(OrbisRtcTick* pTick1, OrbisRtcTick* pTick2,
                                      s64 addingMinutes);
int PS4_SYSV_ABI sceRtcTickAddMonths(OrbisRtcTick* pTick1, OrbisRtcTick* pTick2, s32 addingMonths);
int PS4_SYSV_ABI sceRtcTickAddSeconds(OrbisRtcTick* pTick1, OrbisRtcTick* pTick2,
                                      s64 addingSeconds);
int PS4_SYSV_ABI sceRtcTickAddTicks(OrbisRtcTick* pTick1, OrbisRtcTick* pTick2, s64 addingTicks);
int PS4_SYSV_ABI sceRtcTickAddWeeks(OrbisRtcTick* pTick1, OrbisRtcTick* pTick2, s32 addingWeeks);
int PS4_SYSV_ABI sceRtcTickAddYears(OrbisRtcTick* pTick1, OrbisRtcTick* pTick2, s16 addingYears);

void RegisterlibSceRtc(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Rtc