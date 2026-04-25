// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

struct AjmSidebandStatisticsMemory {
    u32 instance_free;
    u32 buffer_free;
    u32 batch_size;
    u32 input_size;
    u32 output_size;
    u32 small_size;
};
