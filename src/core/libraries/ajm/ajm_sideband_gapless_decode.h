// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

struct AjmSidebandGaplessDecode {
    u32 total_samples;
    u16 skip_samples;
    u16 skipped_samples;
};
