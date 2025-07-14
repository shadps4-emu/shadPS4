// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

template <typename T1, typename T2>
[[nodiscard]] constexpr u64 HashCombine(T1 seed, T2 hash) noexcept {
    u64 s = static_cast<u64>(seed);
    u64 h = static_cast<u64>(hash);

    return s ^ (h + 0x9e3779b9 + (s << 12) + (s >> 4));
}
