// SPDX-FileCopyrightText: Copyright 2023 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <thread>
#include "common/rdtsc.h"
#include "common/uint128.h"

#ifdef _WIN64
#include <windows.h>
#endif

namespace Common {

static constexpr size_t SecondToNanoseconds = 1000000000ULL;

template <u64 Nearest>
static u64 RoundToNearest(u64 value) {
    const auto mod = value % Nearest;
    return mod >= (Nearest / 2) ? (value - mod + Nearest) : (value - mod);
}

static u64 GetTimeNs() {
#ifdef _WIN64
    // GetSystemTimePreciseAsFileTime returns the file time in 100ns units.
    static constexpr u64 Multiplier = 100;
    // Convert Windows epoch to Unix epoch.
    static constexpr u64 WindowsEpochToUnixEpoch = 0x19DB1DED53E8000LL;
    FILETIME filetime;
    GetSystemTimePreciseAsFileTime(&filetime);
    return Multiplier * ((static_cast<u64>(filetime.dwHighDateTime) << 32) +
                         static_cast<u64>(filetime.dwLowDateTime) - WindowsEpochToUnixEpoch);
#elif defined(__APPLE__)
    return clock_gettime_nsec_np(CLOCK_REALTIME);
#else
    timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec * SecondToNanoseconds + ts.tv_nsec;
#endif
}

u64 EstimateRDTSCFrequency() {
    // Discard the first result measuring the rdtsc.
    FencedRDTSC();
    std::this_thread::sleep_for(std::chrono::milliseconds{1});
    FencedRDTSC();

    // Get the current time.
    const auto start_time = GetTimeNs();
    const u64 tsc_start = FencedRDTSC();
    // Wait for 100 milliseconds.
    std::this_thread::sleep_for(std::chrono::milliseconds{100});
    const auto end_time = GetTimeNs();
    const u64 tsc_end = FencedRDTSC();
    // Calculate differences.
    const u64 tsc_diff = tsc_end - tsc_start;
    const u64 tsc_freq = MultiplyAndDivide64(tsc_diff, 1000000000ULL, end_time - start_time);
    return RoundToNearest<100'000>(tsc_freq);
}

} // namespace Common
