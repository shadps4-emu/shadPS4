// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

template <typename T, typename U>
T HashCombine(const T& seed, const U& value) {
    return seed ^ (static_cast<T>(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2));
}