// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

struct Gow3Features {
    static bool storage_image_sync;
    static bool batch_storage_download;
    static bool rt_alias_copy;
    static bool _1x1_readback;
    static bool depth_clear_skip;

    /// Initialize based on game serial. Call once after the game has been loaded.
    static void Init();
};
