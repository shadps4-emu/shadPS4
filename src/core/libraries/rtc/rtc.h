// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <sys/types.h>
#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Rtc {

constexpr int ORBIS_RTC_DAYOFWEEK_SUNDAY = 0;
constexpr int ORBIS_RTC_DAYOFWEEK_MONDAY = 1;
constexpr int ORBIS_RTC_DAYOFWEEK_TUESDAY = 2;
constexpr int ORBIS_RTC_DAYOFWEEK_WEDNESDAY = 3;
constexpr int ORBIS_RTC_DAYOFWEEK_THURSDAY = 4;
constexpr int ORBIS_RTC_DAYOFWEEK_FRIDAY = 5;
constexpr int ORBIS_RTC_DAYOFWEEK_SATURDAY = 6;

constexpr int ORBIS_RTC_CLOCK_ID_NETWORK = 0x10;
constexpr int ORBIS_RTC_CLOCK_ID_DEBUG_NETWORK = 0x11;
constexpr int ORBIS_RTC_CLOCK_ID_AD_NETWORK = 0x12;

constexpr s64 UNIX_EPOCH_TICKS = 0xdcbffeff2bc000;
constexpr s64 WIN32_FILETIME_EPOCH_TICKS = 0xb36168b6a58000;

struct OrbisRtcTick {
    u64 tick;
};

struct OrbisRtcDateTime {
    u16 year;
    u16 month;
    u16 day;
    u16 hour;
    u16 minute;
    u16 second;
    u32 microsecond;
};

int PS4_SYSV_ABI sceRtcCheckValid(OrbisRtcDateTime* pTime);
int PS4_SYSV_ABI sceRtcCompareTick(OrbisRtcTick* pTick1, OrbisRtcTick* pTick2);
int PS4_SYSV_ABI sceRtcConvertLocalTimeToUtc(OrbisRtcTick* pTickLocal, OrbisRtcTick* pTickUtc);
int PS4_SYSV_ABI sceRtcConvertUtcToLocalTime(OrbisRtcTick* pTickUtc, OrbisRtcTick* pTickLocal);
int PS4_SYSV_ABI sceRtcEnd();
int PS4_SYSV_ABI sceRtcFormatRFC2822(char* pszDateTime, const OrbisRtcTick* pTickUtc, int minutes);
int PS4_SYSV_ABI sceRtcFormatRFC2822LocalTime(char* pszDateTime, const OrbisRtcTick* pTickUtc);
int PS4_SYSV_ABI sceRtcFormatRFC3339(char* pszDateTime, const OrbisRtcTick* pTickUtc, int minutes);
int PS4_SYSV_ABI sceRtcFormatRFC3339LocalTime(char* pszDateTime, const OrbisRtcTick* pTickUtc);
int PS4_SYSV_ABI sceRtcFormatRFC3339Precise(char* pszDateTime, const OrbisRtcTick* pTickUtc,
                                            int minutes);
int PS4_SYSV_ABI sceRtcFormatRFC3339PreciseLocalTime(char* pszDateTime,
                                                     const OrbisRtcTick* pTickUtc);
int PS4_SYSV_ABI sceRtcGetCurrentAdNetworkTick(OrbisRtcTick* pTick);
int PS4_SYSV_ABI sceRtcGetCurrentClock(OrbisRtcDateTime* pTime, int timeZone);
int PS4_SYSV_ABI sceRtcGetCurrentClockLocalTime(OrbisRtcDateTime* pTime);
int PS4_SYSV_ABI sceRtcGetCurrentDebugNetworkTick(OrbisRtcTick* pTick);
int PS4_SYSV_ABI sceRtcGetCurrentNetworkTick(OrbisRtcTick* pTick);
int PS4_SYSV_ABI sceRtcGetCurrentRawNetworkTick(OrbisRtcTick* pTick);
int PS4_SYSV_ABI sceRtcGetCurrentTick(OrbisRtcTick* pTick);
int PS4_SYSV_ABI sceRtcGetDayOfWeek(int year, int month, int day);
int PS4_SYSV_ABI sceRtcGetDaysInMonth(int year, int month);
int PS4_SYSV_ABI sceRtcGetDosTime(OrbisRtcDateTime* pTime, u32* dosTime);
int PS4_SYSV_ABI sceRtcGetTick(OrbisRtcDateTime* pTime, OrbisRtcTick* pTick);
u32 PS4_SYSV_ABI sceRtcGetTickResolution();
int PS4_SYSV_ABI sceRtcGetTime_t(OrbisRtcDateTime* pTime, time_t* llTime);
int PS4_SYSV_ABI sceRtcGetWin32FileTime(OrbisRtcDateTime* pTime, uint64_t* ulWin32Time);
int PS4_SYSV_ABI sceRtcInit();
int PS4_SYSV_ABI sceRtcIsLeapYear(int yearInt);
int PS4_SYSV_ABI sceRtcParseDateTime(OrbisRtcTick* pTickUtc, const char* pszDateTime);
int PS4_SYSV_ABI sceRtcParseRFC3339(OrbisRtcTick* pTickUtc, const char* pszDateTime);
void PS4_SYSV_ABI sceRtcSetConf(void* p1, void* p2, s32 minuteswest, s32 dsttime);
int PS4_SYSV_ABI sceRtcSetCurrentAdNetworkTick(OrbisRtcTick* pTick);
int PS4_SYSV_ABI sceRtcSetCurrentDebugNetworkTick(OrbisRtcTick* pTick);
int PS4_SYSV_ABI sceRtcSetCurrentNetworkTick(OrbisRtcTick* pTick);
int PS4_SYSV_ABI sceRtcSetCurrentTick(OrbisRtcTick* pTick);
int PS4_SYSV_ABI sceRtcSetDosTime(OrbisRtcDateTime* pTime, u32 dosTime);
int PS4_SYSV_ABI sceRtcSetTick(OrbisRtcDateTime* pTime, OrbisRtcTick* pTick);
int PS4_SYSV_ABI sceRtcSetTime_t(OrbisRtcDateTime* pTime, time_t llTime);
int PS4_SYSV_ABI sceRtcSetWin32FileTime(OrbisRtcDateTime* pTime, int64_t ulWin32Time);
int PS4_SYSV_ABI sceRtcTickAddDays(OrbisRtcTick* pTick1, OrbisRtcTick* pTick2, int32_t lAdd);
int PS4_SYSV_ABI sceRtcTickAddHours(OrbisRtcTick* pTick1, OrbisRtcTick* pTick2, int32_t lAdd);
int PS4_SYSV_ABI sceRtcTickAddMicroseconds(OrbisRtcTick* pTick1, OrbisRtcTick* pTick2,
                                           int64_t lAdd);
int PS4_SYSV_ABI sceRtcTickAddMinutes(OrbisRtcTick* pTick1, OrbisRtcTick* pTick2, int64_t lAdd);
int PS4_SYSV_ABI sceRtcTickAddMonths(OrbisRtcTick* pTick1, OrbisRtcTick* pTick2, int32_t lAdd);
int PS4_SYSV_ABI sceRtcTickAddSeconds(OrbisRtcTick* pTick1, OrbisRtcTick* pTick2, int64_t lAdd);
int PS4_SYSV_ABI sceRtcTickAddTicks(OrbisRtcTick* pTick1, OrbisRtcTick* pTick2, int64_t lAdd);
int PS4_SYSV_ABI sceRtcTickAddWeeks(OrbisRtcTick* pTick1, OrbisRtcTick* pTick2, int32_t lAdd);
int PS4_SYSV_ABI sceRtcTickAddYears(OrbisRtcTick* pTick1, OrbisRtcTick* pTick2, int32_t lAdd);

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Rtc
