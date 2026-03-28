// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "sce_video_out_refresh_rate.h"

namespace Libraries::VideoOut {
struct SceVideoOutResolutionStatus {
    s32 full_width = 1280;
    s32 full_height = 720;
    s32 pane_width = 1280;
    s32 pane_height = 720;
    u64 refresh_rate = SCE_VIDEO_OUT_REFRESH_RATE_59_94HZ;
    float screen_size_in_inch = 50;
    u16 flags = 0;
    u16 reserved0 = 0;
    u32 reserved1[3] = {0};
};
} // namespace Libraries::VideoOut
