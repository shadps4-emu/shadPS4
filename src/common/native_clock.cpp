// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/native_clock.h"
#include "common/rdtsc.h"
#include "common/uint128.h"

#include <chrono>

#ifdef _WIN64
#include <Windows.h>

#define MM_SHARED_USER_DATA_VA 0x7ffe0000
#define QpcBias ((ULONGLONG volatile*)(MM_SHARED_USER_DATA_VA + 0x3b0))
#endif

namespace Common {

NativeClock::NativeClock()
    : rdtsc_frequency{EstimateRDTSCFrequency()},
      us_rdtsc_factor{GetFixedPoint64Factor(std::micro::den, rdtsc_frequency)} {}

u64 NativeClock::GetTimeUS(u64 time) const {
    return MultiplyHigh(time, us_rdtsc_factor);
}

u64 NativeClock::GetUptime() const {
#ifdef _WIN64
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return counter.QuadPart;
#else
    return FencedRDTSC();
#endif
}

u64 NativeClock::GetUnbiasedUptime() const {
#ifdef _WIN64
    ULONGLONG bias = 0;
    u64 qpc = 0;
    do {
        bias = *QpcBias;
        qpc = GetUptime();
    } while (bias != *QpcBias);
    return qpc - bias;
#else
    return GetUptime();
#endif
}

u64 NativeClock::GetTscFrequency() const {
    return rdtsc_frequency;
}

} // namespace Common
