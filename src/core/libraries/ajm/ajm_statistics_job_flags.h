// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/ajm/ajm_statistics_flags.h"

union AjmStatisticsJobFlags {
    u64 raw;
    struct {
        u64 version : 3;
        u64 : 12;
        AjmStatisticsFlags statistics_flags : 17;
        u64 : 32;
    };
};
static_assert(sizeof(AjmStatisticsJobFlags) == 8);
