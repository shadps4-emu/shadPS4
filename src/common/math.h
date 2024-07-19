// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

static inline u32 IntLog2(u32 i) {
    return 31u - __builtin_clz(i | 1u);
}
