// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <chrono>

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/kernel/libkernel.h"
#include "core/libraries/kernel/time_management.h"
#include "core/libraries/libs.h"
#include "rtc.h"
#include "rtc_error.h"

namespace Libraries::Rtc {

int PS4_SYSV_ABI sceRtcCheckValid(OrbisRtcDateTime* pTime) {
    LOG_TRACE(Lib_Rtc, "called");

    if (pTime == nullptr)
        return ORBIS_RTC_ERROR_INVALID_POINTER;

    if (pTime->year == 0 || pTime->year > 9999)
        return ORBIS_RTC_ERROR_INVALID_YEAR;

    if (pTime->month == 0 || pTime->month > 12)
        return ORBIS_RTC_ERROR_INVALID_MONTH;

    if (pTime->day == 0)
        return ORBIS_RTC_ERROR_INVALID_DAY;

    using namespace std::chrono;
    year chronoYear = year(pTime->year);
    month chronoMonth = month(pTime->month);
    int lastDay =
        static_cast<int>(unsigned(year_month_day_last{chronoYear / chronoMonth / last}.day()));

    if (pTime->day > lastDay)
        return ORBIS_RTC_ERROR_INVALID_DAY;

    if (pTime->hour >= 24)
        return ORBIS_RTC_ERROR_INVALID_HOUR;

    if (pTime->minute >= 60)
        return ORBIS_RTC_ERROR_INVALID_MINUTE;

    if (pTime->second >= 60)
        return ORBIS_RTC_ERROR_INVALID_SECOND;

    if (pTime->microsecond >= 1000000)
        return ORBIS_RTC_ERROR_INVALID_MICROSECOND;

    return SCE_OK;
}

int PS4_SYSV_ABI sceRtcCompareTick(OrbisRtcTick* pTick1, OrbisRtcTick* pTick2) {
    LOG_TRACE(Lib_Rtc, "called");

    if (pTick1 == nullptr || pTick2 == nullptr)
        return ORBIS_RTC_ERROR_INVALID_POINTER;

    if (pTick1->tick <= pTick2->tick)
        return 1;
    else
        return 0;

    return ORBIS_FAIL;
}

int PS4_SYSV_ABI sceRtcConvertLocalTimeToUtc(OrbisRtcTick* pTickLocal, OrbisRtcTick* pTickUtc) {
    LOG_TRACE(Lib_Rtc, "called");

    if (pTickLocal == nullptr)
        return ORBIS_RTC_ERROR_INVALID_POINTER;

    time_t seconds;
    Kernel::OrbisKernelTimezone timezone;

    int convertValue = Kernel::sceKernelConvertLocaltimeToUtc(
        (pTickLocal->tick + 0xff23400100d44000U) / 1000000, 0xffffffff, &seconds, &timezone, 0);

    if (convertValue >= 0) {
        convertValue = sceRtcTickAddMinutes(
            pTickUtc, pTickLocal, -((timezone.tz_dsttime + timezone.tz_minuteswest) / 60));
    }

    return convertValue;
}

int PS4_SYSV_ABI sceRtcConvertUtcToLocalTime(OrbisRtcTick* pTickUtc, OrbisRtcTick* pTickLocal) {
    LOG_TRACE(Lib_Rtc, "called");

    if (pTickUtc == nullptr)
        return ORBIS_RTC_ERROR_INVALID_POINTER;

    time_t seconds;
    Kernel::OrbisTimesec timezone;

    int convertValue = Kernel::sceKernelConvertUtcToLocaltime(
        (pTickUtc->tick + 0xff23400100d44000U) / 1000000, &seconds, &timezone, 0);

    if (convertValue >= 0) {
        auto newTime = ((timezone.dst_sec + timezone.west_sec) / 60) * 60000000;

        if (pTickLocal == nullptr)
            return ORBIS_RTC_ERROR_INVALID_POINTER;

        if (newTime < 0) {
            if (pTickUtc->tick < -newTime) {
                return ORBIS_RTC_ERROR_INVALID_VALUE;
            }
        } else {
            if ((pTickUtc->tick + newTime) < pTickUtc->tick) {
                return ORBIS_RTC_ERROR_INVALID_VALUE;
            }
        }
        pTickLocal->tick = pTickUtc->tick + newTime;
        convertValue = 0;
    }

    return convertValue;
}

int PS4_SYSV_ABI sceRtcEnd() {
    return SCE_OK;
}

int PS4_SYSV_ABI sceRtcFormatRFC2822(char* pszDateTime, const OrbisRtcTick* pTickUtc,
                                     int iTimeZoneMinutes) {
    LOG_ERROR(Lib_Rtc, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRtcFormatRFC2822LocalTime(char* pszDateTime, const OrbisRtcTick* pTickUtc) {
    LOG_ERROR(Lib_Rtc, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRtcFormatRFC3339(char* pszDateTime, const OrbisRtcTick* pTickUtc,
                                     int iTimeZoneMinutes) {
    LOG_ERROR(Lib_Rtc, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRtcFormatRFC3339LocalTime(char* pszDateTime, const OrbisRtcTick* pTickUtc) {
    LOG_ERROR(Lib_Rtc, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRtcFormatRFC3339Precise(char* pszDateTime, const OrbisRtcTick* pTickUtc,
                                            int iTimeZoneMinutes) {
    LOG_ERROR(Lib_Rtc, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRtcFormatRFC3339PreciseLocalTime(char* pszDateTime,
                                                     const OrbisRtcTick* pTickUtc) {
    LOG_ERROR(Lib_Rtc, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRtcGetCurrentAdNetworkTick(OrbisRtcTick* pTick) {
    LOG_TRACE(Lib_Rtc, "called");

    if (pTick == nullptr)
        return ORBIS_RTC_ERROR_INVALID_POINTER;

    Kernel::OrbisKernelTimespec clocktime;
    int returnValue = Kernel::sceKernelClockGettime(Kernel::ORBIS_CLOCK_REALTIME, &clocktime);

    if (returnValue == 0) {
        pTick->tick = clocktime.tv_nsec / 1000 + clocktime.tv_sec * 1000000 + UNIX_EPOCH_TICKS;
    } else {
        return ORBIS_RTC_ERROR_NOT_INITIALIZED;
    }

    return SCE_OK;
}

int PS4_SYSV_ABI sceRtcGetCurrentClock(OrbisRtcDateTime* pTime, int timeZone) {
    LOG_TRACE(Lib_Rtc, "called");

    if (pTime == nullptr)
        return ORBIS_RTC_ERROR_DATETIME_UNINITIALIZED;

    Kernel::OrbisKernelTimespec clocktime;
    int returnValue = Kernel::sceKernelClockGettime(Kernel::ORBIS_CLOCK_REALTIME, &clocktime);

    if (returnValue == 0) {
        OrbisRtcTick clockTick;
        clockTick.tick = clocktime.tv_nsec / 1000 + clocktime.tv_sec * 1000000 + UNIX_EPOCH_TICKS;

        sceRtcTickAddMinutes(&clockTick, &clockTick, timeZone);
        sceRtcSetTick(pTime, &clockTick);
    }

    LOG_INFO(Lib_Rtc, "Got Current Clock {}/{}/{} @ {}:{}", pTime->year, pTime->month, pTime->day,
             pTime->hour, pTime->minute);

    return returnValue;
}

int PS4_SYSV_ABI sceRtcGetCurrentClockLocalTime(OrbisRtcDateTime* pTime) {
    LOG_TRACE(Lib_Rtc, "called");

    if (pTime == nullptr)
        return ORBIS_RTC_ERROR_DATETIME_UNINITIALIZED;

    Kernel::OrbisKernelTimezone timeZone;
    int returnValue = Kernel::sceKernelGettimezone(&timeZone);

    if (returnValue >= 0) {
        Kernel::OrbisKernelTimespec clocktime;

        // calculate total timezone offset for converting UTC to local time
        uint64_t tzOffset = -(timeZone.tz_minuteswest - (timeZone.tz_dsttime * 60));

        if (returnValue >= 0) {
            OrbisRtcTick newTick;
            sceRtcGetCurrentTick(&newTick);
            sceRtcTickAddMinutes(&newTick, &newTick, tzOffset);
            sceRtcSetTick(pTime, &newTick);
        }
    }

    LOG_INFO(Lib_Rtc, "Got Local Time {}/{}/{} @ {}:{}", pTime->year, pTime->month, pTime->day,
             pTime->hour, pTime->minute);
    return returnValue;
}

int PS4_SYSV_ABI sceRtcGetCurrentDebugNetworkTick(OrbisRtcTick* pTick) {
    LOG_TRACE(Lib_Rtc, "called");

    if (pTick == nullptr)
        return ORBIS_RTC_ERROR_INVALID_POINTER;

    Kernel::OrbisKernelTimespec clocktime;
    int returnValue = Kernel::sceKernelClockGettime(Kernel::ORBIS_CLOCK_REALTIME, &clocktime);

    if (returnValue == 0) {
        pTick->tick = clocktime.tv_nsec / 1000 + clocktime.tv_sec * 1000000 + UNIX_EPOCH_TICKS;
    } else {
        return ORBIS_RTC_ERROR_NOT_INITIALIZED;
    }

    return SCE_OK;
}

int PS4_SYSV_ABI sceRtcGetCurrentNetworkTick(OrbisRtcTick* pTick) {
    LOG_TRACE(Lib_Rtc, "called");

    if (pTick == nullptr)
        return ORBIS_RTC_ERROR_INVALID_POINTER;

    Kernel::OrbisKernelTimespec clocktime;
    int returnValue = Kernel::sceKernelClockGettime(Kernel::ORBIS_CLOCK_REALTIME, &clocktime);

    if (returnValue == 0) {
        pTick->tick = clocktime.tv_nsec / 1000 + clocktime.tv_sec * 1000000 + UNIX_EPOCH_TICKS;
    } else {
        return ORBIS_RTC_ERROR_NOT_INITIALIZED;
    }

    return SCE_OK;
}

int PS4_SYSV_ABI sceRtcGetCurrentRawNetworkTick(OrbisRtcTick* pTick) {
    LOG_TRACE(Lib_Rtc, "called");

    if (pTick == nullptr)
        return ORBIS_RTC_ERROR_INVALID_POINTER;

    Kernel::OrbisKernelTimespec clocktime;
    int returnValue = Kernel::sceKernelClockGettime(Kernel::ORBIS_CLOCK_REALTIME, &clocktime);

    if (returnValue == 0) {
        pTick->tick = clocktime.tv_nsec / 1000 + clocktime.tv_sec * 1000000 + UNIX_EPOCH_TICKS;
    } else {
        return ORBIS_RTC_ERROR_NOT_INITIALIZED;
    }

    return SCE_OK;
}

int PS4_SYSV_ABI sceRtcGetCurrentTick(OrbisRtcTick* pTick) {
    LOG_TRACE(Lib_Rtc, "called");

    if (pTick == nullptr)
        return ORBIS_RTC_ERROR_DATETIME_UNINITIALIZED;

    Kernel::OrbisKernelTimespec clocktime;
    int returnValue = Kernel::sceKernelClockGettime(Kernel::ORBIS_CLOCK_REALTIME, &clocktime);

    if (returnValue >= 0) {
        pTick->tick = clocktime.tv_nsec / 1000 + clocktime.tv_sec * 1000000 + UNIX_EPOCH_TICKS;
    }

    return SCE_OK;
}

int PS4_SYSV_ABI sceRtcGetDayOfWeek(int year, int month, int day) {
    LOG_TRACE(Lib_Rtc, "called");

    int sdk_version = 0;
    int sdkResult = Kernel::sceKernelGetCompiledSdkVersion(&sdk_version);
    if (sdkResult != ORBIS_OK) {
        sdk_version = 0;
    }

    if (sdk_version < 0x3000000) {
        if (year < 1) {
            return ORBIS_RTC_ERROR_INVALID_YEAR;
        }
        if (month > 12 || month <= 0) {
            return ORBIS_RTC_ERROR_INVALID_MONTH;
        }
    } else {
        if (year > 9999 || year < 1) {
            return ORBIS_RTC_ERROR_INVALID_YEAR;
        }
        if (month > 12 || month <= 0) {
            return ORBIS_RTC_ERROR_INVALID_MONTH;
        }
    }

    int daysInMonth = sceRtcGetDaysInMonth(year, month);

    if (day <= 0 || day > daysInMonth)
        return ORBIS_RTC_ERROR_INVALID_DAY;

    std::chrono::sys_days chrono_time{std::chrono::year(year) / std::chrono::month(month) /
                                      std::chrono::day(day)};
    std::chrono::weekday chrono_weekday{chrono_time};

    LOG_INFO(Lib_Rtc, "Got Day Of Week : {}", chrono_weekday.c_encoding());

    return chrono_weekday.c_encoding();
}

int PS4_SYSV_ABI sceRtcGetDaysInMonth(int year, int month) {
    LOG_TRACE(Lib_Rtc, "called");

    if (year <= 0)
        return ORBIS_RTC_ERROR_INVALID_YEAR;

    if (month <= 0 || month > 12)
        return ORBIS_RTC_ERROR_INVALID_MONTH;

    std::chrono::year chronoYear = std::chrono::year(year);
    std::chrono::month chronoMonth = std::chrono::month(month);
    int lastDay = static_cast<int>(unsigned(
        std::chrono::year_month_day_last{chronoYear / chronoMonth / std::chrono::last}.day()));

    LOG_INFO(Lib_Rtc, "Got Days In Month : {}", lastDay);

    return lastDay;
}

int PS4_SYSV_ABI sceRtcGetDosTime(OrbisRtcDateTime* pTime, unsigned int* dosTime) {
    LOG_ERROR(Lib_Rtc, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRtcGetTick(OrbisRtcDateTime* pTime, OrbisRtcTick* pTick) {
    LOG_TRACE(Lib_Rtc, "called");

    if (pTime == nullptr || pTick == nullptr)
        return ORBIS_RTC_ERROR_INVALID_POINTER;

    int isTimeValid = sceRtcCheckValid(pTime);
    if (isTimeValid != 0)
        return isTimeValid;

    if (pTime->month > 2) {
        pTime->month -= 3;
    } else {
        pTime->month += 9;
        pTime->year -= 1;
    }

    int c = pTime->year / 100;
    int ya = pTime->year - 100 * c;

    u64 days;
    u64 msec;

    days = ((146097 * c) >> 2) + ((1461 * ya) >> 2) + (153 * pTime->month + 2) / 5 + pTime->day;
    days -= 307;
    days *= 86400000000;

    msec = pTime->hour * 3600000000 + pTime->minute * 60000000 + pTime->second * 1000000 +
           pTime->microsecond;

    pTick->tick = days + msec;

    return SCE_OK;
}

unsigned int PS4_SYSV_ABI sceRtcGetTickResolution() {
    LOG_TRACE(Lib_Rtc, "called");

    return 1000000;
}

int PS4_SYSV_ABI sceRtcGetTime_t(OrbisRtcDateTime* pTime, time_t* llTime) {
    LOG_ERROR(Lib_Rtc, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRtcGetWin32FileTime(OrbisRtcDateTime* pTime, uint64_t* ulWin32Time) {
    LOG_ERROR(Lib_Rtc, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRtcInit() {
    return SCE_OK;
}

int PS4_SYSV_ABI sceRtcIsLeapYear(int yearInt) {
    LOG_TRACE(Lib_Rtc, "called");

    if (yearInt < 1)
        return ORBIS_RTC_ERROR_INVALID_YEAR;

    using namespace std::chrono;

    year_month_day_last ymdl{year(yearInt) / February / last};
    return (ymdl.day() == 29d);
}

int PS4_SYSV_ABI sceRtcParseDateTime(OrbisRtcTick* pTickUtc, const char* pszDateTime) {
    LOG_ERROR(Lib_Rtc, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRtcParseRFC3339(OrbisRtcTick* pTickUtc, const char* pszDateTime) {
    LOG_ERROR(Lib_Rtc, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRtcSetConf() {
    LOG_ERROR(Lib_Rtc, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRtcSetCurrentAdNetworkTick() {
    LOG_ERROR(Lib_Rtc, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRtcSetCurrentDebugNetworkTick() {
    LOG_ERROR(Lib_Rtc, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRtcSetCurrentNetworkTick() {
    LOG_ERROR(Lib_Rtc, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRtcSetCurrentTick() {
    LOG_ERROR(Lib_Rtc, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRtcSetDosTime(OrbisRtcDateTime* pTime, u32 dosTime) {
    LOG_TRACE(Lib_Rtc, "called");

    if (pTime == nullptr)
        return ORBIS_RTC_ERROR_INVALID_POINTER;

    pTime->microsecond = 0;
    pTime->second = (dosTime << 1) & 0x3e;
    pTime->minute = (dosTime >> 5) & 0x3f;
    pTime->hour = ((dosTime & 0xf800) >> 0xb);

    int days = (dosTime >> 0x10);

    pTime->day = days & 0x1f;
    pTime->month = (days >> 5) & 0xf;
    pTime->year = (days >> 9) + 0x7bc;

    return SCE_OK;
}

int PS4_SYSV_ABI sceRtcSetTick(OrbisRtcDateTime* pTime, OrbisRtcTick* pTick) {
    LOG_TRACE(Lib_Rtc, "called");

    if (pTime == nullptr || pTick == nullptr)
        return ORBIS_RTC_ERROR_INVALID_POINTER;

    u32 ly, ld, lm, j;
    u64 days, msec;

    days = pTick->tick / 86400000000;
    msec = pTick->tick % 86400000000;

    days += 307;

    j = (days << 2) - 1;
    ly = j / 146097;

    j -= (146097 * ly);
    ld = j >> 2;

    j = ((ld << 2) + 3) / 1461;
    ld = (((ld << 2) + 7) - 1461 * j) >> 2;

    lm = (5 * ld - 3) / 153;
    ld = (5 * ld + 2 - 153 * lm) / 5;
    ly = 100 * ly + j;

    if (lm < 10) {
        lm += 3;
    } else {
        lm -= 9;
        ly++;
    }

    pTime->year = ly;
    pTime->month = lm;
    pTime->day = ld;

    pTime->hour = msec / 3600000000;
    msec %= 3600000000;
    pTime->minute = msec / 60000000;
    msec %= 60000000;
    pTime->second = msec / 1000000;
    msec %= 1000000;
    pTime->microsecond = msec;

    return SCE_OK;
}

int PS4_SYSV_ABI sceRtcSetTime_t(OrbisRtcDateTime* pTime, time_t llTime) {
    LOG_TRACE(Lib_Rtc, "called");

    if (pTime == nullptr)
        return ORBIS_RTC_ERROR_INVALID_POINTER;

    int sdk_version;
    int sdkResult = Kernel::sceKernelGetCompiledSdkVersion(&sdk_version);
    if (sdkResult != ORBIS_OK) {
        sdk_version = 0;
    }

    OrbisRtcTick newTick;
    if (sdk_version < 0x3000000) {
        newTick.tick = (llTime & 0xffffffff) * 1000000;
    } else {
        if (llTime < 0) {
            return ORBIS_RTC_ERROR_INVALID_VALUE;
        }
        newTick.tick = llTime * 1000000;
    }

    newTick.tick += UNIX_EPOCH_TICKS;
    sceRtcSetTick(pTime, &newTick);

    return SCE_OK;
}

int PS4_SYSV_ABI sceRtcSetWin32FileTime(OrbisRtcDateTime* pTime, uint64_t ulWin32Time) {
    LOG_TRACE(Lib_Rtc, "called");

    if (pTime == nullptr)
        return ORBIS_RTC_ERROR_INVALID_POINTER;

    u64 convertedTime = (ulWin32Time / 10) + 0xb36168b6a58000;

    OrbisRtcTick convertedTick;
    convertedTick.tick = convertedTime;

    sceRtcSetTick(pTime, &convertedTick);

    return SCE_OK;
}

int PS4_SYSV_ABI sceRtcTickAddDays(OrbisRtcTick* pTick1, OrbisRtcTick* pTick2, int32_t lAdd) {
    LOG_TRACE(Lib_Rtc, "called");

    if (pTick1 == nullptr || pTick2 == nullptr)
        return ORBIS_RTC_ERROR_INVALID_POINTER;

    pTick1->tick = (lAdd * 86400000000) + pTick2->tick;

    return SCE_OK;
}

int PS4_SYSV_ABI sceRtcTickAddHours(OrbisRtcTick* pTick1, OrbisRtcTick* pTick2, int32_t lAdd) {
    LOG_TRACE(Lib_Rtc, "called");

    if (pTick1 == nullptr || pTick2 == nullptr)
        return ORBIS_RTC_ERROR_INVALID_POINTER;

    pTick1->tick = (lAdd * 3600000000) + pTick2->tick;

    return SCE_OK;
}

int PS4_SYSV_ABI sceRtcTickAddMicroseconds(OrbisRtcTick* pTick1, OrbisRtcTick* pTick2,
                                           int64_t lAdd) {
    LOG_TRACE(Lib_Rtc, "called");

    if (pTick1 == nullptr || pTick2 == nullptr)
        return ORBIS_RTC_ERROR_INVALID_POINTER;

    pTick1->tick = lAdd + pTick2->tick;

    return SCE_OK;
}

int PS4_SYSV_ABI sceRtcTickAddMinutes(OrbisRtcTick* pTick1, OrbisRtcTick* pTick2, int64_t lAdd) {
    LOG_TRACE(Lib_Rtc, "called");

    if (pTick1 == nullptr || pTick2 == nullptr)
        return ORBIS_RTC_ERROR_INVALID_POINTER;

    pTick1->tick = (lAdd * 60000000) + pTick2->tick;

    return SCE_OK;
}

int PS4_SYSV_ABI sceRtcTickAddMonths(OrbisRtcTick* pTick1, OrbisRtcTick* pTick2, int32_t lAdd) {
    LOG_TRACE(Lib_Rtc, "called");

    if (pTick1 == nullptr || pTick2 == nullptr)
        return ORBIS_RTC_ERROR_INVALID_POINTER;

    if (lAdd == 0) {
        pTick1->tick = pTick2->tick;
        return SCE_OK;
    }

    OrbisRtcDateTime time;
    s64 s;
    s64 tempMonth;

    sceRtcSetTick(&time, pTick1);

    if (lAdd >= 0) {
        s = 1;
    } else {
        s = -1;
    }

    time.year += (lAdd / 12);
    tempMonth = time.month + (lAdd % 12) - 1;

    if (tempMonth > 11 || tempMonth < 0) {
        tempMonth -= s * 12;
        time.year += s;
    }

    time.month = tempMonth + 1;

    using namespace std::chrono;
    year chronoYear = year(time.year);
    month chronoMonth = month(time.month);
    int lastDay =
        static_cast<int>(unsigned(year_month_day_last{chronoYear / chronoMonth / last}.day()));

    if (time.day > lastDay) {
        time.day = lastDay;
    }

    u64 timeIsValid = sceRtcCheckValid(&time);
    if (timeIsValid == 0) {
        sceRtcGetTick(&time, pTick1);
    } else {
        return timeIsValid;
    }

    return SCE_OK;
}

int PS4_SYSV_ABI sceRtcTickAddSeconds(OrbisRtcTick* pTick1, OrbisRtcTick* pTick2, int64_t lAdd) {
    LOG_TRACE(Lib_Rtc, "called");

    if (pTick1 == nullptr || pTick2 == nullptr)
        return ORBIS_RTC_ERROR_INVALID_POINTER;

    pTick1->tick = (lAdd * 1000000) + pTick2->tick;

    return SCE_OK;
}

int PS4_SYSV_ABI sceRtcTickAddTicks(OrbisRtcTick* pTick1, OrbisRtcTick* pTick2, int64_t lAdd) {
    LOG_TRACE(Lib_Rtc, "called");

    if (pTick1 == nullptr || pTick2 == nullptr)
        return ORBIS_RTC_ERROR_INVALID_POINTER;

    pTick1->tick = lAdd + pTick2->tick;

    return SCE_OK;
}

int PS4_SYSV_ABI sceRtcTickAddWeeks(OrbisRtcTick* pTick1, OrbisRtcTick* pTick2, int32_t lAdd) {
    LOG_TRACE(Lib_Rtc, "called");

    if (pTick1 == nullptr || pTick2 == nullptr)
        return ORBIS_RTC_ERROR_INVALID_POINTER;

    pTick1->tick = (lAdd * 0x8cd0e3a000) + pTick2->tick;

    return SCE_OK;
}

int PS4_SYSV_ABI sceRtcTickAddYears(OrbisRtcTick* pTick1, OrbisRtcTick* pTick2, int32_t lAdd) {
    LOG_TRACE(Lib_Rtc, "called");

    if (pTick1 == nullptr || pTick2 == nullptr)
        return ORBIS_RTC_ERROR_INVALID_POINTER;

    OrbisRtcDateTime time;

    if (lAdd == 0) {
        pTick1->tick = pTick2->tick;
        return SCE_OK;
    }

    sceRtcSetTick(&time, pTick1);

    time.year += lAdd;

    u64 timeIsValid = sceRtcCheckValid(&time);
    if (timeIsValid == 0) {
        sceRtcGetTick(&time, pTick1);
    } else {
        return timeIsValid;
    }

    return SCE_OK;
}

void RegisterlibSceRtc(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("lPEBYdVX0XQ", "libSceRtc", 1, "libSceRtc", 1, 1, sceRtcCheckValid);
    LIB_FUNCTION("fNaZ4DbzHAE", "libSceRtc", 1, "libSceRtc", 1, 1, sceRtcCompareTick);
    LIB_FUNCTION("8Yr143yEnRo", "libSceRtc", 1, "libSceRtc", 1, 1, sceRtcConvertLocalTimeToUtc);
    LIB_FUNCTION("M1TvFst-jrM", "libSceRtc", 1, "libSceRtc", 1, 1, sceRtcConvertUtcToLocalTime);
    LIB_FUNCTION("8SljQx6pDP8", "libSceRtc", 1, "libSceRtc", 1, 1, sceRtcEnd);
    LIB_FUNCTION("eiuobaF-hK4", "libSceRtc", 1, "libSceRtc", 1, 1, sceRtcFormatRFC2822);
    LIB_FUNCTION("AxHBk3eat04", "libSceRtc", 1, "libSceRtc", 1, 1, sceRtcFormatRFC2822LocalTime);
    LIB_FUNCTION("WJ3rqFwymew", "libSceRtc", 1, "libSceRtc", 1, 1, sceRtcFormatRFC3339);
    LIB_FUNCTION("DwuHIlLGW8I", "libSceRtc", 1, "libSceRtc", 1, 1, sceRtcFormatRFC3339LocalTime);
    LIB_FUNCTION("lja0nNPWojg", "libSceRtc", 1, "libSceRtc", 1, 1, sceRtcFormatRFC3339Precise);
    LIB_FUNCTION("tOZ6fwwHZOA", "libSceRtc", 1, "libSceRtc", 1, 1,
                 sceRtcFormatRFC3339PreciseLocalTime);
    LIB_FUNCTION("LN3Zcb72Q0c", "libSceRtc", 1, "libSceRtc", 1, 1, sceRtcGetCurrentAdNetworkTick);
    LIB_FUNCTION("8lfvnRMqwEM", "libSceRtc", 1, "libSceRtc", 1, 1, sceRtcGetCurrentClock);
    LIB_FUNCTION("ZPD1YOKI+Kw", "libSceRtc", 1, "libSceRtc", 1, 1, sceRtcGetCurrentClockLocalTime);
    LIB_FUNCTION("Ot1DE3gif84", "libSceRtc", 1, "libSceRtc", 1, 1,
                 sceRtcGetCurrentDebugNetworkTick);
    LIB_FUNCTION("zO9UL3qIINQ", "libSceRtc", 1, "libSceRtc", 1, 1, sceRtcGetCurrentNetworkTick);
    LIB_FUNCTION("HWxHOdbM-Pg", "libSceRtc", 1, "libSceRtc", 1, 1, sceRtcGetCurrentRawNetworkTick);
    LIB_FUNCTION("18B2NS1y9UU", "libSceRtc", 1, "libSceRtc", 1, 1, sceRtcGetCurrentTick);
    LIB_FUNCTION("CyIK-i4XdgQ", "libSceRtc", 1, "libSceRtc", 1, 1, sceRtcGetDayOfWeek);
    LIB_FUNCTION("3O7Ln8AqJ1o", "libSceRtc", 1, "libSceRtc", 1, 1, sceRtcGetDaysInMonth);
    LIB_FUNCTION("E7AR4o7Ny7E", "libSceRtc", 1, "libSceRtc", 1, 1, sceRtcGetDosTime);
    LIB_FUNCTION("8w-H19ip48I", "libSceRtc", 1, "libSceRtc", 1, 1, sceRtcGetTick);
    LIB_FUNCTION("jMNwqYr4R-k", "libSceRtc", 1, "libSceRtc", 1, 1, sceRtcGetTickResolution);
    LIB_FUNCTION("BtqmpTRXHgk", "libSceRtc", 1, "libSceRtc", 1, 1, sceRtcGetTime_t);
    LIB_FUNCTION("jfRO0uTjtzA", "libSceRtc", 1, "libSceRtc", 1, 1, sceRtcGetWin32FileTime);
    LIB_FUNCTION("LlodCMDbk3o", "libSceRtc", 1, "libSceRtc", 1, 1, sceRtcInit);
    LIB_FUNCTION("Ug8pCwQvh0c", "libSceRtc", 1, "libSceRtc", 1, 1, sceRtcIsLeapYear);
    LIB_FUNCTION("NxEI1KByvCI", "libSceRtc", 1, "libSceRtc", 1, 1, sceRtcParseDateTime);
    LIB_FUNCTION("99bMGglFW3I", "libSceRtc", 1, "libSceRtc", 1, 1, sceRtcParseRFC3339);
    LIB_FUNCTION("fFLgmNUpChg", "libSceRtc", 1, "libSceRtc", 1, 1, sceRtcSetConf);
    LIB_FUNCTION("sV2tK+yOhBU", "libSceRtc", 1, "libSceRtc", 1, 1, sceRtcSetCurrentAdNetworkTick);
    LIB_FUNCTION("VLDUPKmw5L8", "libSceRtc", 1, "libSceRtc", 1, 1,
                 sceRtcSetCurrentDebugNetworkTick);
    LIB_FUNCTION("qhDBtIo+auw", "libSceRtc", 1, "libSceRtc", 1, 1, sceRtcSetCurrentNetworkTick);
    LIB_FUNCTION("d4fHLCGmY80", "libSceRtc", 1, "libSceRtc", 1, 1, sceRtcSetCurrentTick);
    LIB_FUNCTION("aYPCd1cChyg", "libSceRtc", 1, "libSceRtc", 1, 1, sceRtcSetDosTime);
    LIB_FUNCTION("ueega6v3GUw", "libSceRtc", 1, "libSceRtc", 1, 1, sceRtcSetTick);
    LIB_FUNCTION("bDEVVP4bTjQ", "libSceRtc", 1, "libSceRtc", 1, 1, sceRtcSetTime_t);
    LIB_FUNCTION("n5JiAJXsbcs", "libSceRtc", 1, "libSceRtc", 1, 1, sceRtcSetWin32FileTime);
    LIB_FUNCTION("NR1J0N7L2xY", "libSceRtc", 1, "libSceRtc", 1, 1, sceRtcTickAddDays);
    LIB_FUNCTION("MDc5cd8HfCA", "libSceRtc", 1, "libSceRtc", 1, 1, sceRtcTickAddHours);
    LIB_FUNCTION("XPIiw58C+GM", "libSceRtc", 1, "libSceRtc", 1, 1, sceRtcTickAddMicroseconds);
    LIB_FUNCTION("mn-tf4QiFzk", "libSceRtc", 1, "libSceRtc", 1, 1, sceRtcTickAddMinutes);
    LIB_FUNCTION("CL6y9q-XbuQ", "libSceRtc", 1, "libSceRtc", 1, 1, sceRtcTickAddMonths);
    LIB_FUNCTION("07O525HgICs", "libSceRtc", 1, "libSceRtc", 1, 1, sceRtcTickAddSeconds);
    LIB_FUNCTION("AqVMssr52Rc", "libSceRtc", 1, "libSceRtc", 1, 1, sceRtcTickAddTicks);
    LIB_FUNCTION("gI4t194c2W8", "libSceRtc", 1, "libSceRtc", 1, 1, sceRtcTickAddWeeks);
    LIB_FUNCTION("-5y2uJ62qS8", "libSceRtc", 1, "libSceRtc", 1, 1, sceRtcTickAddYears);
};

} // namespace Libraries::Rtc
