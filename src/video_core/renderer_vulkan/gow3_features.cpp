// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/elf_info.h"
#include "common/logging/log.h"
#include "video_core/renderer_vulkan/gow3_features.h"

bool Gow3Features::storage_image_sync = false;
bool Gow3Features::batch_storage_download = false;
bool Gow3Features::rt_alias_copy = false;
bool Gow3Features::_1x1_readback = false;
bool Gow3Features::depth_clear_skip = false;

void Gow3Features::Init() {
    const auto& serial = Common::ElfInfo::Instance().GameSerial();
    const bool is_gow3 = serial == "CUSA01623" || serial == "CUSA01715" || serial == "CUSA01740";
    if (!is_gow3)
        return;

    storage_image_sync = true;
    LOG_INFO(Render, "[GOW3] storage_image_sync enabled");

    // batch_storage_download = true;  // GoW3v0: sync mode only
    // LOG_INFO(Render, "[GOW3] batch_storage_download enabled");

    rt_alias_copy = true;
    LOG_INFO(Render, "[GOW3] rt_alias_copy enabled");

    _1x1_readback = true;
    LOG_INFO(Render, "[GOW3] 1x1_readback enabled");

    depth_clear_skip = true;
    LOG_INFO(Render, "[GOW3] depth_clear_skip enabled");
}
