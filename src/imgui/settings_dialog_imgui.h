// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <map>
#include <string>
#include <vector>
#include <SDL3/SDL.h>
#include <imgui.h>

#include "big_picture.h"

namespace BigPictureMode {

enum class SettingsCategory {
    Profiles,
    General,
    Graphics,
    Input,
    Trophy,
    Log,
    Experimental,
};

struct Textures {
    SDL_Texture* profiles;
    SDL_Texture* general;
    SDL_Texture* globalSettings;
    SDL_Texture* experimental;
    SDL_Texture* graphics;
    SDL_Texture* input;
    SDL_Texture* trophy;
    SDL_Texture* log;
};

void Init();
void DeInit();

void SetProfileIcons(std::vector<Game>& games);
void LoadEmbeddedTexture(std::string resourcePath, SDL_Texture*& texture);
void AddCategory(std::string name, SDL_Texture* texture, SettingsCategory category);

void DrawSettings(bool* open);
void SaveSettings(std::string profile);
void LoadSettings(std::string profile);
void LoadCategory(SettingsCategory);

void AddSettingBool(std::string name, bool& value);
void AddSettingSliderInt(std::string name, int& value, int min, int max);
void AddSettingSliderFloat(std::string name, float& value, int min, int max, int precision);
void AddSettingCombo(std::string name, int& value);
int GetComboIndex(std::string selection, std::vector<std::string> options);

//////////////////// Settings struct
// Note: use int instead of std::string for all combo settings as needed by ImGui
// then convert to string when saving/loading

struct CurrentSettings {
    // General tab
    int consoleLanguage;
    int volume;
    bool showSplash;
    int audioBackend;

    // Graphics tab
    int fullscreenMode;
    int presentMode;
    int windowWidth;
    int windowHeight;
    bool hdrAllowed;
    bool fsrEnabled;
    bool rcasEnabled;
    float rcasAttenuation;

    // Input tab
    bool motionControls;
    bool backgroundController;
    int cursorState;
    int cursorTimeout;

    // Trophy tab
    bool trophyPopupDisabled;
    int trophySide;
    float trophyDuration;

    // Log tab
    bool logEnabled;
    bool separateLog;
    int logType;

    // Experimental tab
    int readbacksMode;
    bool readbackLinearImages;
    bool directMemoryAccess;
    bool devkitConsole;
    bool neoMode;
    bool psnSignedIn;
    bool connectedNetwork;
    bool pipelineCacheEnabled;
    bool pipelineCacheArchive;
    int extraDmem;
    int vblankFrequency;
};

//////////////////// option maps for comboboxes and other needed constants
const std::map<std::string, int> languageMap = {{"Arabic", 21},
                                                {"Czech", 23},
                                                {"Danish", 14},
                                                {"Dutch", 6},
                                                {"English (United Kingdom)", 18},
                                                {"English (United States)", 1},
                                                {"Finnish", 12},
                                                {"French (Canada)", 22},
                                                {"French (France)", 2},
                                                {"German", 4},
                                                {"Greek", 25},
                                                {"Hungarian", 24},
                                                {"Indonesian", 29},
                                                {"Italian", 5},
                                                {"Japanese", 0},
                                                {"Korean", 9},
                                                {"Norwegian (Bokmaal)", 15},
                                                {"Polish", 16},
                                                {"Portuguese (Brazil)", 17},
                                                {"Portuguese (Portugal)", 7},
                                                {"Romanian", 26},
                                                {"Russian", 8},
                                                {"Simplified Chinese", 11},
                                                {"Spanish (Latin America)", 20},
                                                {"Spanish (Spain)", 3},
                                                {"Swedish", 13},
                                                {"Thai", 27},
                                                {"Traditional Chinese", 10},
                                                {"Turkish", 19},
                                                {"Ukrainian", 30},
                                                {"Vietnamese", 28}};

const std::vector<std::string> optionsLanguage = {"Arabic",
                                                  "Czech",
                                                  "Danish",
                                                  "Dutch",
                                                  "English (United Kingdom)",
                                                  "English (United States)",
                                                  "Finnish",
                                                  "French (Canada)",
                                                  "French (France)",
                                                  "German",
                                                  "Greek",
                                                  "Hungarian",
                                                  "Indonesian",
                                                  "Italian",
                                                  "Japanese",
                                                  "Korean",
                                                  "Norwegian (Bokmaal)",
                                                  "Polish",
                                                  "Portuguese (Brazil)",
                                                  "Portuguese (Portugal)",
                                                  "Romanian",
                                                  "Russian",
                                                  "Simplified Chinese",
                                                  "Spanish (Latin America)",
                                                  "Spanish (Spain)",
                                                  "Swedish",
                                                  "Thai",
                                                  "Traditional Chinese",
                                                  "Turkish",
                                                  "Ukrainian",
                                                  "Vietnamese"};

const std::vector<std::string> optionsLogType = {"sync", "async"};
const std::vector<std::string> optionsFullscreenMode = {"Windowed", "Fullscreen",
                                                        "Fullscreen (Borderless)"};
const std::vector<std::string> optionsAudioBackend = {"SDL", "OpenAL"};
const std::vector<std::string> optionsPresentMode = {"Mailbox", "Fifo", "Immediate"};
const std::vector<std::string> optionsHideCursor = {"Never", "Idle", "Always"};
const std::vector<std::string> optionsTrophySide = {"left", "right", "top", "bottom"};
const std::vector<std::string> optionsReadbacksMode = {"Disabled", "Relaxed", "Precise"};

const std::map<std::string, std::vector<std::string>> optionsMap = {
    {"Log Type", optionsLogType},
    {"Console Language", optionsLanguage},
    {"Audio Backend", optionsAudioBackend},
    {"Display Mode", optionsFullscreenMode},
    {"Present Mode", optionsPresentMode},
    {"Hide Cursor", optionsHideCursor},
    {"Trophy Notification Position", optionsTrophySide},
    {"Readbacks Mode", optionsReadbacksMode},
};

} // namespace BigPictureMode
