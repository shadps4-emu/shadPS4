// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <filesystem>
#include "types.h"

namespace Config {
void load(const std::filesystem::path& path);
void save(const std::filesystem::path& path);

bool isNeoMode();
std::string getLogFilter();
std::string getLogType();

u32 getScreenWidth();
u32 getScreenHeight();
s32 getGpuId();

bool debugDump();
bool isLleLibc();
bool showSplash();
bool nullGpu();
bool dumpShaders();
bool dumpPM4();

}; // namespace Config
