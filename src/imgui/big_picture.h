// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <filesystem>
#include <SDL3_image/SDL_image.h>

namespace BigPictureMode {

struct Game {
    SDL_Texture* iconTexture;
    std::filesystem::path ebootPath;
    std::string title;
    std::string serial;
};

void Launch();
void SetGameIcons(bool ForSettings = false);
void GetGameInfo();
std::filesystem::path UpdateChecker(const std::string sceItem, std::filesystem::path game_folder);

// Settings

enum class SettingsType {
    Profiles,
    General,
};

struct CurrentSettings {
    bool logSetting;
};

void DrawSettings();
void SaveSettings(std::string profile);
void LoadSettings(std::string profile);
void LoadCategory(SettingsType);

} // namespace BigPictureMode
