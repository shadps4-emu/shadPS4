// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

[[nodiscard]] inline u64 HashCombine(const u64 seed, const u64 hash) {
    return seed ^ (hash + 0x9e3779b9 + (seed << 12) + (seed >> 4));
}

[[nodiscard]] inline u32 HashCombine(const u32 seed, const u32 hash) {
    return seed ^ (hash + 0x9e3779b9 + (seed << 6) + (seed >> 2));
}