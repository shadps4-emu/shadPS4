// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <filesystem>
#include <map>
#include <vector>
#include <SDL3_image/SDL_image.h>
#include <imgui.h>

namespace SettingsImGui {

struct Game {
    SDL_Texture* iconTexture;
    std::filesystem::path ebootPath;
    std::string title;
    std::string serial;
    bool focusState;
};

void Init();
void DeInit();
void SetGameIcons();
void GetGameInfo();
std::filesystem::path UpdateChecker(const std::string sceItem, std::filesystem::path game_folder);

// Settings
enum class SettingsCategory {
    Profiles,
    General,
    Experimental,
};

struct Textures {
    SDL_Texture* general;
};

void LoadTextureData(std::string resourcePath, SDL_Texture*& texture);
void AddCategory(std::string name, SDL_Texture* texture, SettingsCategory category);

void DrawSettings(bool* open);
void SaveSettings(std::string profile);
void LoadSettings(std::string profile);
void LoadCategory(SettingsCategory);

void AddSettingBool(std::string name, bool& value);
void AddSettingSliderInt(std::string name, int& value, int min, int max);
void AddSettingCombo(std::string name, int& value);
int GetComboIndex(std::string selection, std::vector<std::string> options);

//////////////////// option maps for comboboxes

const std::vector<std::string> optionsLogType = {"sync", "async"};

const std::map<std::string, std::vector<std::string>> optionsMap = {
    {"Log Type", optionsLogType},
};

//////////////////// Settings struct
// Note: use int instead of std::string for all combo settings as needed by ImGui
struct CurrentSettings {
    bool logEnabled;
    int volume;
    int logType;
};

} // namespace SettingsImGui
