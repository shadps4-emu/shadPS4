// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Common {

class NativeClock final {
public:
    explicit NativeClock();

    u64 GetTimeUS(u64 time) const;

    u64 GetUptime() const;
    u64 GetUnbiasedUptime() const;
    u64 GetTscFrequency() const;

private:
    u64 rdtsc_frequency;
    u64 us_rdtsc_factor;
};

} // namespace Common
