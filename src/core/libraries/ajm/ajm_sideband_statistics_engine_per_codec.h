// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

struct AjmSidebandStatisticsEnginePerCodec {
    u8 codec_count;
    u8 codec_id[3];
    float codec_percentage[3];
};
