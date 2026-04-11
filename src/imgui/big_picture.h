// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <filesystem>
#include <vector>

namespace BigPictureMode {

struct Game {
    std::filesystem::path iconPath;
    std::filesystem::path ebootPath;
    std::string title;
};

void Launch();
void SetGameIcons();
void GetGameInfo();
void SceUpdateChecker(const std::string sceItem, std::filesystem::path& outputPath,
                      std::filesystem::path game_folder);

} // namespace BigPictureMode
