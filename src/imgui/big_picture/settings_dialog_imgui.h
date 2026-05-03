// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <map>
#include <variant>
#include <SDL3/SDL.h>
#include <imgui.h>

#include "big_picture.h"
#include "core/emulator_settings.h"
#include "imgui/imgui_texture.h"

namespace ImGuiEmuSettings {

class SettingsWindow {

public:
    SettingsWindow(bool gameRunning);
    void DrawSettings(bool* open);

private:
    enum class SettingsCategory {
        Profiles,
        General,
        Graphics,
        Input,
        Trophy,
        Folders,
        Log,
        Experimental,
    };

    void SaveSettings(std::string profile);
    void LoadSettings(std::string profile);
    void SaveInstallDirs();

    void SetupWindow();
    void DeInit();
    void GetProfileInfo();

    void DrawMainContent(bool* open);
    void DrawSettingsTable(SettingsCategory);
    void DrawProfileSelector();
    void DrawGameFolderManager();
    void DrawCategoryTabs();
    void AddCategory(std::string name, std::variant<SDL_Texture*, ImGui::RefCountedTexture> texture,
                     SettingsCategory category);

    void AddSettingCheckbox(std::string name, bool& value);
    void AddSettingSliderInt(std::string name, int& value, int min, int max);
    void AddSettingSliderFloat(std::string name, float& value, int min, int max, int precision);
    void AddSettingCombo(std::string name, int& value, std::vector<std::string> options);
    int GetComboIndex(std::string selection, std::vector<std::string> options);

    std::vector<BigPictureMode::IconInfo> profileIcons = {};
    std::vector<GameInstallDir> m_GameInstallDirs = {};

    float uiScale = 1.0f;
    SettingsCategory currentCategory = SettingsCategory::Profiles;
    std::string currentProfile = "Global";

    std::string runningGameSerial = "";
    bool isGameRunning = false;
    bool closeOnSave = false;
    int deleteProfileIndex = -1;

    std::variant<SDL_Texture*, ImGui::RefCountedTexture> profilesTexture;
    std::variant<SDL_Texture*, ImGui::RefCountedTexture> generalTexture;
    std::variant<SDL_Texture*, ImGui::RefCountedTexture> experimentalTexture;
    std::variant<SDL_Texture*, ImGui::RefCountedTexture> graphicsTexture;
    std::variant<SDL_Texture*, ImGui::RefCountedTexture> inputTexture;
    std::variant<SDL_Texture*, ImGui::RefCountedTexture> trophyTexture;
    std::variant<SDL_Texture*, ImGui::RefCountedTexture> logTexture;
    std::variant<SDL_Texture*, ImGui::RefCountedTexture> foldersTexture;

    //////////////////// options for comboboxes
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
    std::vector<std::string> languageOptions; // assigned from keys above
    const std::vector<std::string> fullscreenModeOptions = {"Windowed", "Fullscreen",
                                                            "Fullscreen (Borderless)"};
    const std::vector<std::string> audioBackendOptions = {"SDL", "OpenAL"};
    const std::vector<std::string> presentModeOptions = {"Mailbox", "Fifo", "Immediate"};
    const std::vector<std::string> hideCursorOptions = {"Never", "Idle", "Always"};
    const std::vector<std::string> trophySideOptions = {"left", "right", "top", "bottom"};
    const std::vector<std::string> readbacksModeOptions = {"Disabled", "Relaxed", "Precise"};

    //////////////// Setting Variables
    //////////////// Note:: Use int for all comboboxes as needed by ImGui

    // General tab
    int consoleLanguageSetting;
    int volumeSetting;
    bool showSplashSetting;
    int audioBackendSetting;

    // Graphics tab
    int fullscreenModeSetting;
    int presentModeSetting;
    int windowWidthSetting;
    int windowHeightSetting;
    bool hdrAllowedSetting;
    bool fsrEnabledSetting;
    bool rcasEnabledSetting;
    float rcasAttenuationSetting;

    // Input tab
    bool motionControlsSetting;
    bool backgroundControllerSetting;
    int cursorStateSetting;
    int cursorTimeoutSetting;

    // Trophy tab
    bool trophyPopupDisabledSetting;
    int trophySideSetting;
    float trophyDurationSetting;

    // Log tab
    bool logEnableSetting;
    bool logSeparateSetting;
    bool logSyncSetting;

    // Experimental tab
    int readbacksModeSetting;
    bool readbackLinearImagesSetting;
    bool directMemoryAccessSetting;
    bool devkitConsoleSetting;
    bool neoModeSetting;
    bool shadnetEnabledSetting;
    bool connectedNetworkSetting;
    bool pipelineCacheEnabledSetting;
    bool pipelineCacheArchiveSetting;
    int extraDmemSetting;
    int vblankFrequencySetting;
};

} // namespace ImGuiEmuSettings
