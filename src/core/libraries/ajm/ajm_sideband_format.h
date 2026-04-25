// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/ajm/ajm_format_encoding.h"

struct AjmSidebandFormat {
    u32 num_channels;
    u32 channel_mask;
    u32 sampl_freq;
    AjmFormatEncoding sample_encoding;
    u32 bitrate;
    u32 reserved;
};
