// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

namespace Libraries::VideoOut {
struct FlipStatus {
    u64 count = 0;
    u64 process_time = 0;
    u64 tsc = 0;
    s64 flip_arg = -1;
    u64 submit_tsc = 0;
    u64 reserved0 = 0;
    s32 gc_queue_num = 0;
    s32 flip_pending_num = 0;
    s32 current_buffer = -1;
    u32 reserved1 = 0;
};
} // namespace Libraries::VideoOut
