// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

struct AjmSidebandStream {
    s32 input_consumed;
    s32 output_written;
    u64 total_decoded_samples;
};
