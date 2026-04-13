// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <filesystem>
#include <map>
#include <vector>
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
    Experimental,
};

// Note: use int instead of std::string for all combo settings as needed by ImGui
struct CurrentSettings {
    bool logEnabled;
    int volume;
    int logType;
};

void DrawSettings();
void SaveSettings(std::string profile);
void LoadSettings(std::string profile);
void LoadCategory(SettingsType);

void AddCategory(std::string name, SDL_Texture* texture, SettingsType type);
void AddSettingBool(std::string name, bool& value);
void AddSettingSliderInt(std::string name, int& value, int min, int max);
void AddSettingCombo(std::string name, int& value);
int GetComboIndex(std::string selection, std::vector<std::string> options);

//////////////////// option maps for comboboxes

const std::vector<std::string> optionsLogType = {"sync", "async"};

const std::map<std::string, std::vector<std::string>> optionsMap = {
    {"Log Type", optionsLogType},
};

} // namespace BigPictureMode
