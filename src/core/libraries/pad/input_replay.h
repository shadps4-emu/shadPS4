// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <filesystem>
#include <functional>
#include "common/types.h"
#include "core/libraries/pad/pad.h"

namespace Libraries::Pad::InputReplay {

enum class ApiKind : u32 {
    Read = 1,
    ReadState = 2,
};

using LiveRead = std::function<int(OrbisPadData*, s32)>;

bool ConfigureRecord(const std::filesystem::path& path, bool exit_after_replay);
bool ConfigureReplay(const std::filesystem::path& path, bool exit_after_replay,
                     bool ordered_match = false);
void RequestToggle();
bool IsEnabled();
int RejectUnsupported(const char* api_name, u32 progression);

int Dispatch(ApiKind api, s32 handle, OrbisPadData* data, s32 capacity, u32 progression,
             const LiveRead& live_read);

int RunSelfTest(const std::filesystem::path& directory);

} // namespace Libraries::Pad::InputReplay
