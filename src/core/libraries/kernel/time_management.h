// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

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
int PS4_SYSV_ABI sceKernelClockGettime(s32 clock_id, OrbisKernelTimespec* tp);
s32 PS4_SYSV_ABI sceKernelGettimezone(OrbisKernelTimezone* tz);
int PS4_SYSV_ABI sceKernelConvertLocaltimeToUtc(time_t param_1, int64_t param_2, time_t* seconds,
                                                OrbisKernelTimezone* timezone, int* dst_seconds);
void timeSymbolsRegister(Core::Loader::SymbolsResolver* sym);

} // namespace Libraries::Kernel
