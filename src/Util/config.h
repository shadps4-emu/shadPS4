// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <filesystem>
#include "common/types.h"

namespace Config {
void load(const std::filesystem::path& path);
void save(const std::filesystem::path& path);

bool isNeoMode();
u32 getLogLevel();

u32 getScreenWidth();
u32 getScreenHeight();

}; // namespace Config
