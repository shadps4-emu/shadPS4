// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

namespace Libraries::VideoOut {
struct SceVideoOutVblankStatus {
    u64 count = 0;
    u64 process_time = 0;
    u64 tsc = 0;
    u64 reserved[1] = {0};
    u8 flags = 0;
    u8 pad1[7] = {};
};
} // namespace Libraries::VideoOut
