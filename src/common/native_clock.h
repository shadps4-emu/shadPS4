// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <chrono>
#include "common/types.h"

namespace Common {

class NativeClock final {
public:
    explicit NativeClock();

    u64 GetTscFrequency() const {
        return rdtsc_frequency;
    }

    u64 GetTimeNS(u64 base_ptc = 0) const;
    u64 GetTimeUS(u64 base_ptc = 0) const;
    u64 GetTimeMS(u64 base_ptc = 0) const;
    u64 GetUptime() const;
    u64 GetProcessTimeUS() const;

private:
    u64 rdtsc_frequency;
    u64 ns_rdtsc_factor;
    u64 us_rdtsc_factor;
    u64 ms_rdtsc_factor;
};

} // namespace Common
