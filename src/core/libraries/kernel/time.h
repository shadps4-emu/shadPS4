// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <chrono>
#include <sys/types.h>
#include "common/types.h"

namespace Common {
class NativeClock;
}

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Kernel {

struct OrbisKernelTimeval {
    s64 tv_sec;
    s64 tv_usec;
};

struct OrbisKernelTimezone {
    s32 tz_minuteswest;
    s32 tz_dsttime;
};

struct OrbisKernelTimespec {
    s64 tv_sec;
    s64 tv_nsec;

    std::chrono::system_clock::time_point TimePoint() const noexcept {
        using namespace std::chrono;
        const auto duration =
            duration_cast<system_clock::duration>(seconds{tv_sec} + nanoseconds{tv_nsec});
        return system_clock::time_point{duration};
    }
};

struct OrbisTimesec {
    time_t t;
    u32 west_sec;
    u32 dst_sec;
};

constexpr int ORBIS_CLOCK_REALTIME = 0;
constexpr int ORBIS_CLOCK_VIRTUAL = 1;
constexpr int ORBIS_CLOCK_PROF = 2;
constexpr int ORBIS_CLOCK_MONOTONIC = 4;
constexpr int ORBIS_CLOCK_UPTIME = 5;
constexpr int ORBIS_CLOCK_UPTIME_PRECISE = 7;
constexpr int ORBIS_CLOCK_UPTIME_FAST = 8;
constexpr int ORBIS_CLOCK_REALTIME_PRECISE = 9;
constexpr int ORBIS_CLOCK_REALTIME_FAST = 10;
constexpr int ORBIS_CLOCK_MONOTONIC_PRECISE = 11;
constexpr int ORBIS_CLOCK_MONOTONIC_FAST = 12;
constexpr int ORBIS_CLOCK_SECOND = 13;
constexpr int ORBIS_CLOCK_THREAD_CPUTIME_ID = 14;
constexpr int ORBIS_CLOCK_PROCTIME = 15;
constexpr int ORBIS_CLOCK_EXT_NETWORK = 16;
constexpr int ORBIS_CLOCK_EXT_DEBUG_NETWORK = 17;
constexpr int ORBIS_CLOCK_EXT_AD_NETWORK = 18;
constexpr int ORBIS_CLOCK_EXT_RAW_NETWORK = 19;

namespace Dev {
u64& GetInitialPtc();

Common::NativeClock* GetClock();
} // namespace Dev

u64 PS4_SYSV_ABI sceKernelGetTscFrequency();
u64 PS4_SYSV_ABI sceKernelGetProcessTime();
u64 PS4_SYSV_ABI sceKernelGetProcessTimeCounter();
u64 PS4_SYSV_ABI sceKernelGetProcessTimeCounterFrequency();
u64 PS4_SYSV_ABI sceKernelReadTsc();
s32 PS4_SYSV_ABI sceKernelClockGettime(u32 clock_id, OrbisKernelTimespec* tp);
s32 PS4_SYSV_ABI sceKernelGettimezone(OrbisKernelTimezone* tz);
s32 PS4_SYSV_ABI sceKernelConvertLocaltimeToUtc(time_t param_1, int64_t param_2, time_t* seconds,
                                                OrbisKernelTimezone* timezone, s32* dst_seconds);

s32 PS4_SYSV_ABI sceKernelConvertUtcToLocaltime(time_t time, time_t* local_time, OrbisTimesec* st,
                                                u64* dst_sec);
s32 PS4_SYSV_ABI sceKernelUsleep(u32 microseconds);
s32 PS4_SYSV_ABI posix_clock_settime(s32 clock_id, OrbisKernelTimespec* tp);
s32 PS4_SYSV_ABI posix_settimeofday(OrbisKernelTimeval* _tv, OrbisKernelTimezone* _tz);
s32 PS4_SYSV_ABI sceKernelSettimeofday(OrbisKernelTimeval* _tv, OrbisKernelTimezone* _tz);

void RegisterTime(Core::Loader::SymbolsResolver* sym);

} // namespace Libraries::Kernel
