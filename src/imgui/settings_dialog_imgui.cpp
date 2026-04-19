//  SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#include <map>
#include <ranges>
#include <ImGuiFileDialog.h>
#include <cmrc/cmrc.hpp>
#include <stb_image.h>

#include "core/devtools/layer.h"
#include "core/emulator_settings.h"
#include "imgui/imgui_std.h"
#include "settings_dialog_imgui.h"

CMRC_DECLARE(res);
namespace BigPictureMode {

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

//////////////// Texture data
SDL_Texture* profilesTexture;
SDL_Texture* generalTexture;
SDL_Texture* globalSettingsTexture;
SDL_Texture* experimentalTexture;
SDL_Texture* graphicsTexture;
SDL_Texture* inputTexture;
SDL_Texture* trophyTexture;
SDL_Texture* logTexture;
SDL_Texture* foldersTexture;

//////////////// Gui variable
const float gameImageSize = 200.f;
const float settingsIconSize = 125.f;
std::vector<Game> settingsProfileVec = {};
std::vector<GameInstallDir> m_GameInstallDirs = {};

float uiScale = 1.0f;
SDL_Renderer* renderer;

SettingsCategory currentCategory = SettingsCategory::Profiles;
std::string currentProfile = "Global";
bool closeOnSave = false;
bool showFileDialog = false;

void Init() {
    auto languageKeys = std::views::keys(languageMap);
    languageOptions.assign(languageKeys.begin(), languageKeys.end());

    currentProfile = "Global";
    currentCategory = SettingsCategory::Profiles;
    LoadSettings("Global");
    m_GameInstallDirs = EmulatorSettings.GetAllGameInstallDirs();

    SDL_Window* window = SDL_GetKeyboardFocus();
    renderer = SDL_GetRenderer(window);

    LoadEmbeddedTexture("src/images/big_picture/settings.png", generalTexture);
    LoadEmbeddedTexture("src/images/big_picture/profiles.png", profilesTexture);
    LoadEmbeddedTexture("src/images/big_picture/global-settings.png", globalSettingsTexture);
    LoadEmbeddedTexture("src/images/big_picture/experimental.png", experimentalTexture);
    LoadEmbeddedTexture("src/images/big_picture/graphics.png", graphicsTexture);
    LoadEmbeddedTexture("src/images/big_picture/controller.png", inputTexture);
    LoadEmbeddedTexture("src/images/big_picture/trophy.png", trophyTexture);
    LoadEmbeddedTexture("src/images/big_picture/log.png", logTexture);
    LoadEmbeddedTexture("src/images/big_picture/folder.png", foldersTexture);

    GetGameInfo(settingsProfileVec, true, globalSettingsTexture);
    uiScale = static_cast<float>(EmulatorSettings.GetBigPictureScale() / 1000.f);
}

void DeInit() {
    EmulatorSettings.Load();
    EmulatorSettings.SetBigPictureScale(static_cast<int>(uiScale * 1000));
    EmulatorSettings.Save();
}

void DrawSettings(bool* open) {
    if (!*open)
        return;

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);

    if (ImGui::Begin("Settings", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollWithMouse)) {
        if (ImGui::IsWindowAppearing()) {
            Init();
            closeOnSave = false;
        }

        ImGui::DrawPrettyBackground();
        ImGui::SetWindowFontScale(uiScale);
        ImGuiWindowFlags child_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                       ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NavFlattened;

        ImVec4 settingsColor = ImVec4(0.1f, 0.1f, 0.12f, 0.8f); // Darker gray
        ImGui::PushStyleColor(ImGuiCol_ChildBg, settingsColor);
        float vertSize = (settingsIconSize * uiScale + ImGui::CalcTextSize("Profiles").y) +
                         ImGui::GetStyle().FramePadding.x * 2.f * uiScale + 20.0 * uiScale;

        ImGui::BeginChild("Categories", ImVec2(0, vertSize), true,
                          child_flags | ImGuiWindowFlags_HorizontalScrollbar);

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(30.0f * uiScale, 0.0f));

        // Must add categories in enum order for L1/R1 to work correctly, with experimental last
        AddCategory("Profiles", profilesTexture, SettingsCategory::Profiles);
        AddCategory("General", generalTexture, SettingsCategory::General);
        AddCategory("Graphics", graphicsTexture, SettingsCategory::Graphics);
        AddCategory("Input", inputTexture, SettingsCategory::Input);
        AddCategory("Trophy", trophyTexture, SettingsCategory::Trophy);
        AddCategory("Game Folders", foldersTexture, SettingsCategory::Folders);
        AddCategory("Log", logTexture, SettingsCategory::Log);

        if (currentProfile != "Global")
            AddCategory("Experimental", experimentalTexture, SettingsCategory::Experimental);

        ImGui::PopStyleVar();
        ImGui::EndChild(); // Categories

        if (ImGui::IsWindowAppearing()) {
            ImGui::SetKeyboardFocusHere();
        }

        ImGui::BeginChild("ContentRegion", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), true,
                          child_flags);
        ImGui::PopStyleColor();

        LoadCategory(currentCategory);

        ImGui::EndChild();
        ImGui::Separator();

        ImGui::SetNextItemWidth(300.0f * uiScale);
        static float sliderScale2 = 1.0f;
        if (ImGui::IsWindowAppearing()) {
            sliderScale2 = uiScale;
        }

        ImGui::SliderFloat("UI Scale", &sliderScale2, 0.25f, 3.0f);
        // Only update when user is not interacting with slider
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            uiScale = sliderScale2;
        }
        ImGui::SameLine();

        // Align buttons right
        float buttonsWidth = ImGui::CalcTextSize("Save").x + ImGui::CalcTextSize("Cancel").x +
                             ImGui::CalcTextSize("Apply").x +
                             ImGui::GetStyle().FramePadding.x * 6.0f +
                             ImGui::GetStyle().ItemSpacing.x * 2;
        ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - buttonsWidth);

        if (ImGui::Button("Save")) {
            closeOnSave = true;
            ImGui::OpenPopup("Save Confirmation");
        }

        ImGui::SameLine();
        if (ImGui::Button("Apply")) {
            ImGui::OpenPopup("Save Confirmation");
        }

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal("Save Confirmation", nullptr,
                                   ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("%s", ("Profile Saved:\n" + currentProfile).c_str());
            ImGui::Separator();

            if (ImGui::Button("OK", ImVec2(250 * uiScale, 0))) {
                std::string profile = currentProfile;
                if (currentProfile != "Global") {
                    profile = currentProfile.substr(0, 9);
                }

                SaveSettings(profile);
                if (closeOnSave) {
                    DeInit();
                    *open = false;
                    ImGui::CloseCurrentPopup();
                } else {
                    ImGui::CloseCurrentPopup();
                }
            }

            ImGui::EndPopup();
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            DeInit();
            *open = false;
        }

        SettingsCategory lastCategory =
            currentProfile != "Global" ? SettingsCategory::Experimental : SettingsCategory::Log;
        // Navigate categories with Tab / R1 / L1
        if (ImGui::IsKeyPressed(ImGuiKey_GamepadR1) || ImGui::IsKeyPressed(ImGuiKey_Tab)) {
            int currentIndex = static_cast<int>(currentCategory);
            currentCategory == lastCategory
                ? currentCategory = static_cast<SettingsCategory>(0)
                : currentCategory = static_cast<SettingsCategory>(currentIndex + 1);
        }

        if (ImGui::IsKeyPressed(ImGuiKey_GamepadL1)) {
            int currentIndex = static_cast<int>(currentCategory);
            currentIndex == 0 ? currentCategory = lastCategory
                              : currentCategory = static_cast<SettingsCategory>(currentIndex - 1);
        }
    }

    ImGui::End();
}

void LoadCategory(SettingsCategory category) {
    if (category != SettingsCategory::Folders) {
        ImGui::TextColored(ImVec4(0.00f, 1.00f, 1.00f, 1.00f), "%s",
                           ("Selected Profile: " + currentProfile).c_str()); // Dark Blue
    }
    ImGui::Dummy(ImVec2(0, 20.f * uiScale));
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4.0f * uiScale, 10.0f * uiScale));

    ImGuiWindowFlags child_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                   ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NavFlattened;

    if (category == SettingsCategory::General) {
        if (ImGui::BeginTable("SettingsTable", 2)) {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 500.0f * uiScale);
            ImGui::TableSetupColumn("Value");

            AddSettingCombo("Console Language", consoleLanguageSetting, languageOptions);
            AddSettingSliderInt("Volume", volumeSetting, 0, 500);
            AddSettingBool("Show Splash Screen When Launching Game", showSplashSetting);
            AddSettingCombo("Audio Backend", audioBackendSetting, audioBackendOptions);

            ImGui::EndTable();
        }
    } else if (category == SettingsCategory::Graphics) {
        if (ImGui::BeginTable("SettingsTable", 2)) {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 500.0f * uiScale);
            ImGui::TableSetupColumn("Value");

            AddSettingCombo("Display Mode", fullscreenModeSetting, fullscreenModeOptions);
            AddSettingCombo("Present Mode", presentModeSetting, presentModeOptions);
            AddSettingSliderInt("Window Width", windowWidthSetting, 0, 8000);
            AddSettingSliderInt("Window Height", windowHeightSetting, 0, 7000);
            AddSettingBool("Enable HDR", hdrAllowedSetting);
            AddSettingBool("Enable FSR", fsrEnabledSetting);

            if (fsrEnabledSetting) {
                AddSettingBool("Enable RCAS", rcasEnabledSetting);
            }

            if (rcasEnabledSetting && fsrEnabledSetting) {
                AddSettingSliderFloat("RCAS Attenuation", rcasAttenuationSetting, 0.0f, 3.0f, 3);
            }

            ImGui::EndTable();
        }
    } else if (category == SettingsCategory::Input) {
        if (ImGui::BeginTable("SettingsTable", 2)) {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 500.0f * uiScale);
            ImGui::TableSetupColumn("Value");

            AddSettingBool("Enable Motion Controls", motionControlsSetting);
            AddSettingBool("Enable Background Controller Input", backgroundControllerSetting);
            AddSettingCombo("Hide Cursor", cursorStateSetting, hideCursorOptions);

            if (cursorStateSetting == 1) {
                AddSettingSliderInt("Hide Cursor Idle Timeout", cursorTimeoutSetting, 1, 10);
            }

            ImGui::EndTable();
        }
    } else if (category == SettingsCategory::Trophy) {
        if (ImGui::BeginTable("SettingsTable", 2)) {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 500.0f * uiScale);
            ImGui::TableSetupColumn("Value");

            AddSettingBool("Disable Trophy Notification", trophyPopupDisabledSetting);
            if (!trophyPopupDisabledSetting) {
                AddSettingCombo("Trophy Notification Position", trophySideSetting,
                                trophySideOptions);
                AddSettingSliderFloat("Trophy Notification Duration", trophyDurationSetting, 0.f,
                                      10.f, 1);
            }

            ImGui::EndTable();
        }
    } else if (category == SettingsCategory::Folders) {
        if (ImGui::Button("Add Folder", ImVec2(400.f * uiScale, 0))) {
            ImGuiFileDialog::Instance()->OpenDialog(
                "OpenFolder", "Add shadPS4 game folder", nullptr, ".", 1, nullptr,
                ImGuiFileDialogFlags_DisableCreateDirectoryButton |
                    ImGuiFileDialogFlags_DontShowHiddenFiles);

            ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->Pos);
            ImGui::SetNextWindowSize(viewport->Size);
        }

        if (ImGuiFileDialog::Instance()->Display("OpenFolder",
                                                 child_flags | ImGuiWindowFlags_NoMove)) {
            if (ImGuiFileDialog::Instance()->IsOk()) {
                GameInstallDir dir;
                dir.path = ImGuiFileDialog::Instance()->GetCurrentPath();
                dir.enabled = true;

#ifdef WIN32
                // replace \ with / on windows
                std::string pathString = dir.path.string();
                size_t pos = 0;
                while ((pos = pathString.find("\\", pos)) != std::string::npos) {
                    pathString.replace(pos, 1, "/");
                    pos += 2;
                }

                dir.path = pathString;
#endif

                m_GameInstallDirs.push_back(dir);
                SaveInstallDirs();
            }

            ImGuiFileDialog::Instance()->Close();
        }

    } else if (category == SettingsCategory::Log) {
        if (ImGui::BeginTable("SettingsTable", 2)) {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 500.0f * uiScale);
            ImGui::TableSetupColumn("Value");

            AddSettingBool("Enable Logging", logEnableSetting);
            if (logEnableSetting) {
                AddSettingBool("Separate Log Files", logSeparateSetting);
                AddSettingBool("Log Sync", logSyncSetting);
            }

            ImGui::EndTable();
        }
    } else if (category == SettingsCategory::Experimental) {
        if (ImGui::BeginTable("SettingsTable", 2)) {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 500.0f * uiScale);
            ImGui::TableSetupColumn("Value");

            AddSettingSliderInt("Additional DMem Allocation", extraDmemSetting, 0, 20000);
            AddSettingSliderInt("Vblank Frequency", vblankFrequencySetting, 30, 360);
            AddSettingCombo("Readbacks Mode", readbacksModeSetting, readbacksModeOptions);
            AddSettingBool("Enable Readback Linear Images", readbackLinearImagesSetting);
            AddSettingBool("Enable Direct Memory Access", directMemoryAccessSetting);
            AddSettingBool("Enable Devkit Console Mode", devkitConsoleSetting);
            AddSettingBool("Enable PS4 Neo Mode", neoModeSetting);
            AddSettingBool("Enable ShadNet", shadnetEnabledSetting);
            AddSettingBool("Set Network Connected to True", connectedNetworkSetting);
            AddSettingBool("Enable Shader Cache", pipelineCacheEnabledSetting);

            if (pipelineCacheEnabledSetting) {
                AddSettingBool("Compress Shader Cache to Zip File", pipelineCacheArchiveSetting);
            }

            ImGui::EndTable();
        }
    }

    ImGui::PopStyleVar();

    // Child Window if Needed
    if (category == SettingsCategory::Profiles) {

        ImGui::BeginChild("ProfileSelect", ImVec2(0, 0), true, child_flags);
        Overlay::TextCentered("Select Global or Game-Specific Settings Profile");
        SetProfileIcons(settingsProfileVec);
        ImGui::EndChild();
    } else if (category == SettingsCategory::Folders) {
        ImGui::BeginChild("Game Folder List", ImVec2(0, 0), true, child_flags);

        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0, 5.0f * uiScale));
        if (ImGui::BeginTable("FoldersTable", 3)) {
            ImGui::TableSetupColumn("FolderButton", ImGuiTableColumnFlags_WidthFixed,
                                    300.0f * uiScale);
            ImGui::TableSetupColumn("FolderEnabled", ImGuiTableColumnFlags_WidthFixed, 0.0f);
            ImGui::TableSetupColumn("FolderPath");

            for (int i = 0; i < m_GameInstallDirs.size(); i++) {
                std::string buttonLabel = "Remove Folder##" + std::to_string(i);
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                if (ImGui::Button(buttonLabel.c_str(), ImVec2(280 * uiScale, 0))) {
                    m_GameInstallDirs.erase(m_GameInstallDirs.begin() + i);
                    SaveInstallDirs();
                }

                ImGui::TableNextColumn();
                std::string checkboxLabel = "##EnableFolder" + std::to_string(i);
                bool previousState = m_GameInstallDirs[i].enabled;
                ImGui::Checkbox(checkboxLabel.c_str(), &m_GameInstallDirs[i].enabled);
                ImGui::SameLine();
                ImGui::Dummy(ImVec2(5.0 * uiScale, 0));

                if (m_GameInstallDirs[i].enabled != previousState) {
                    SaveInstallDirs();
                }

                ImGui::TableNextColumn();
                ImGui::TextWrapped("%s", m_GameInstallDirs[i].path.string().c_str());
            }

            ImGui::EndTable();
        }
        ImGui::PopStyleVar();

        ImGui::EndChild();
    }
}

void SaveSettings(std::string profile) {
    const bool isSpecific = currentProfile != "Global";

    /////////// General Tab
    EmulatorSettings.SetConsoleLanguage(languageMap.at(languageOptions.at(consoleLanguageSetting)),
                                        isSpecific);
    EmulatorSettings.SetVolumeSlider(volumeSetting, isSpecific);
    EmulatorSettings.SetShowSplash(showSplashSetting, isSpecific);
    EmulatorSettings.SetAudioBackend(audioBackendSetting, isSpecific);

    /////////// Graphics Tab
    bool isFullscreen = fullscreenModeSetting != 0;
    EmulatorSettings.SetFullScreen(isFullscreen);
    EmulatorSettings.SetFullScreenMode(fullscreenModeOptions.at(fullscreenModeSetting), isSpecific);
    EmulatorSettings.SetPresentMode(presentModeOptions.at(presentModeSetting), isSpecific);
    EmulatorSettings.SetWindowHeight(windowHeightSetting, isSpecific);
    EmulatorSettings.SetWindowWidth(windowWidthSetting, isSpecific);
    EmulatorSettings.SetHdrAllowed(hdrAllowedSetting, isSpecific);
    EmulatorSettings.SetFsrEnabled(fsrEnabledSetting, isSpecific);
    EmulatorSettings.SetRcasEnabled(rcasEnabledSetting, isSpecific);
    EmulatorSettings.SetRcasAttenuation(static_cast<int>(rcasAttenuationSetting * 1000),
                                        isSpecific);

    /////////// Input Tab
    EmulatorSettings.SetMotionControlsEnabled(motionControlsSetting, isSpecific);
    EmulatorSettings.SetBackgroundControllerInput(backgroundControllerSetting, isSpecific);
    EmulatorSettings.SetCursorState(cursorStateSetting, isSpecific);
    EmulatorSettings.SetCursorHideTimeout(cursorTimeoutSetting, isSpecific);

    /////////// Trophy Tab
    EmulatorSettings.SetTrophyPopupDisabled(trophyPopupDisabledSetting, isSpecific);
    EmulatorSettings.SetTrophyNotificationSide(trophySideOptions.at(trophySideSetting), isSpecific);
    EmulatorSettings.SetTrophyNotificationDuration(static_cast<double>(trophyDurationSetting));

    /////////// Log Tab
    EmulatorSettings.SetLogEnable(logEnableSetting, isSpecific);
    EmulatorSettings.SetLogSeparate(logSeparateSetting, isSpecific);
    EmulatorSettings.SetLogSync(logSyncSetting, isSpecific);

    /////////// Experimental Tab
    if (isSpecific) {
        EmulatorSettings.SetReadbacksMode(readbacksModeSetting, true);
        EmulatorSettings.SetReadbackLinearImagesEnabled(readbackLinearImagesSetting, true);
        EmulatorSettings.SetDirectMemoryAccessEnabled(directMemoryAccessSetting, true);
        EmulatorSettings.SetDevKit(devkitConsoleSetting, true);
        EmulatorSettings.SetNeo(neoModeSetting, true);
        EmulatorSettings.SetShadNetEnabled(shadnetEnabledSetting, true);
        EmulatorSettings.SetConnectedToNetwork(connectedNetworkSetting, true);
        EmulatorSettings.SetPipelineCacheEnabled(pipelineCacheEnabledSetting, true);
        EmulatorSettings.SetPipelineCacheArchived(pipelineCacheArchiveSetting, true);
        EmulatorSettings.SetExtraDmemInMBytes(extraDmemSetting, true);
        EmulatorSettings.SetVblankFrequency(vblankFrequencySetting, true);
    }

    if (!isSpecific) {
        EmulatorSettings.Save();
    } else {
        EmulatorSettings.Save(profile);
    }
}

void LoadSettings(std::string profile) {
    const bool isSpecific = currentProfile != "Global";
    if (!isSpecific) {
        EmulatorSettings.Load();
    } else {
        EmulatorSettings.Load(profile);
    }

    /////////// General Tab
    int languageIndex = EmulatorSettings.GetConsoleLanguage();
    std::string language;
    for (const auto& [key, value] : languageMap) {
        if (value == languageIndex) {
            language = key;
        }
    }
    consoleLanguageSetting = GetComboIndex(language, languageOptions);
    volumeSetting = EmulatorSettings.GetVolumeSlider();
    showSplashSetting = EmulatorSettings.IsShowSplash();
    audioBackendSetting = EmulatorSettings.GetAudioBackend();

    /////////// Graphics Tab
    fullscreenModeSetting =
        GetComboIndex(EmulatorSettings.GetFullScreenMode(), fullscreenModeOptions);
    presentModeSetting = GetComboIndex(EmulatorSettings.GetPresentMode(), presentModeOptions);
    windowHeightSetting = EmulatorSettings.GetWindowHeight();
    windowWidthSetting = EmulatorSettings.GetWindowWidth();
    hdrAllowedSetting = EmulatorSettings.IsHdrAllowed();
    fsrEnabledSetting = EmulatorSettings.IsFsrEnabled();
    rcasEnabledSetting = EmulatorSettings.IsRcasEnabled();
    rcasAttenuationSetting = static_cast<float>(EmulatorSettings.GetRcasAttenuation() * 0.001f);

    /////////// Input Tab
    motionControlsSetting = EmulatorSettings.IsMotionControlsEnabled();
    backgroundControllerSetting = EmulatorSettings.IsBackgroundControllerInput();
    cursorStateSetting = EmulatorSettings.GetCursorState();
    cursorTimeoutSetting = EmulatorSettings.GetCursorHideTimeout();

    /////////// Trophy Tab
    trophyPopupDisabledSetting = EmulatorSettings.IsTrophyPopupDisabled();
    trophySideSetting =
        GetComboIndex(EmulatorSettings.GetTrophyNotificationSide(), trophySideOptions);
    trophyDurationSetting = static_cast<float>(EmulatorSettings.GetTrophyNotificationDuration());

    /////////// Log Tab
    logEnableSetting = EmulatorSettings.IsLogEnable();
    logSeparateSetting = EmulatorSettings.IsLogSeparate();
    logSyncSetting = EmulatorSettings.IsLogSync();

    /////////// Experimental Tab
    if (isSpecific) {
        readbacksModeSetting = EmulatorSettings.GetReadbacksMode();
        readbackLinearImagesSetting = EmulatorSettings.IsReadbackLinearImagesEnabled();
        directMemoryAccessSetting = EmulatorSettings.IsDirectMemoryAccessEnabled();
        devkitConsoleSetting = EmulatorSettings.IsDevKit();
        neoModeSetting = EmulatorSettings.IsNeo();
        shadnetEnabledSetting = EmulatorSettings.IsShadNetEnabled();
        connectedNetworkSetting = EmulatorSettings.IsConnectedToNetwork();
        pipelineCacheEnabledSetting = EmulatorSettings.IsPipelineCacheEnabled();
        pipelineCacheArchiveSetting = EmulatorSettings.IsPipelineCacheArchived();
        extraDmemSetting = EmulatorSettings.GetExtraDmemInMBytes();
        vblankFrequencySetting = EmulatorSettings.GetVblankFrequency();
    }
}

void AddCategory(std::string name, SDL_Texture* texture, SettingsCategory category) {
    ImGui::SameLine();
    ImGui::BeginGroup();

    // make button appear hovered as long as category is selected, otherwise dull it's hovered color
    currentCategory == category
        ? ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered])
        : ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.235f, 0.392f, 0.624f, 1.00f));

    if (ImGui::ImageButton(name.c_str(), ImTextureID(texture),
                           ImVec2(settingsIconSize * uiScale, settingsIconSize * uiScale))) {
        currentCategory = category;
    }

    ImGui::PopStyleColor();

    ImGui::SetCursorPosX(
        (ImGui::GetCursorPosX() +
         (settingsIconSize * uiScale - ImGui::CalcTextSize(name.c_str()).x) * 0.5f) +
        ImGui::GetStyle().FramePadding.x);
    ImGui::Text("%s", name.c_str());
    ImGui::EndGroup();
}

void AddSettingBool(std::string name, bool& value) {
    std::string label = "##" + name;
    ImGui::TableNextRow();
    ImGui::TableNextColumn();

    ImGui::TextWrapped("%s", name.c_str());
    ImGui::TableNextColumn();
    ImGui::Checkbox(label.c_str(), &value);
}

void AddSettingSliderInt(std::string name, int& value, int min, int max) {
    std::string label = "##" + name;
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TextWrapped("%s", name.c_str());

    ImGui::TableNextColumn();
    ImGui::SliderInt(label.c_str(), &value, min, max);
}

void AddSettingSliderFloat(std::string name, float& value, int min, int max, int precision) {
    std::string label = "##" + name;
    std::string precisionString = "%." + std::to_string(precision) + "f";

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TextWrapped("%s", name.c_str());

    ImGui::TableNextColumn();
    ImGui::SliderFloat(label.c_str(), &value, min, max, precisionString.c_str());
}

void AddSettingCombo(std::string name, int& value, std::vector<std::string> options) {
    std::string label = "##" + name;
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TextWrapped("%s", name.c_str());

    ImGui::TableNextColumn();
    const char* combo_value = options[value].c_str();
    if (ImGui::BeginCombo(label.c_str(), combo_value)) {
        for (int i = 0; i < options.size(); i++) {
            const bool selected = (i == value);
            if (ImGui::Selectable(options[i].c_str(), selected))
                value = i;

            // Set the initial focus when opening the combo
            if (selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
}

int GetComboIndex(std::string selection, std::vector<std::string> options) {
    for (int i = 0; i < options.size(); i++) {
        if (selection == options[i])
            return i;
    }

    return 0;
}

void LoadEmbeddedTexture(std::string resourcePath, SDL_Texture*& texture) {
    auto resource = cmrc::res::get_filesystem();
    auto file = resource.open(resourcePath);
    std::vector<char> texData = std::vector<char>(file.begin(), file.end());

    BigPictureMode::LoadTextureData(texData, texture, renderer);
}

void SetProfileIcons(std::vector<Game>& games) {
    ImGuiStyle& style = ImGui::GetStyle();
    const float maxAvailableWidth = ImGui::GetContentRegionAvail().x;
    const float itemSpacing = style.ItemSpacing.x; // already scaled
    const float padding = 10.0f * uiScale;
    float rowContentWidth = gameImageSize * uiScale + itemSpacing;

    for (int i = 0; i < games.size(); i++) {
        ImGui::BeginGroup();
        std::string ButtonName = "Button" + std::to_string(i);
        const char* ButtonNameChar = ButtonName.c_str();

        bool isNextItemFocused = (ImGui::GetID(ButtonNameChar) == ImGui::GetFocusID());
        bool popColor = false;
        if (isNextItemFocused) {
            ImGui::PushStyleColor(ImGuiCol_Button,
                                  ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered]);
            popColor = true;
        }

        if (ImGui::ImageButton(ButtonNameChar, (ImTextureID)games[i].iconTexture,
                               ImVec2(gameImageSize * uiScale, gameImageSize * uiScale))) {
            currentProfile = i == 0 ? "Global" : games[i].serial + " - " + games[i].title;
            LoadSettings(games[i].serial);
        }

        if (popColor) {
            ImGui::PopStyleColor();
        }

        // Scroll to item only when newly-focused
        if (ImGui::IsItemFocused() && !games[i].focusState) {
            ImGui::SetScrollHereY(0.5f);
        }

        if (ImGui::IsWindowFocused()) {
            games[i].focusState = ImGui::IsItemFocused();
        }

        ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + gameImageSize * uiScale);
        ImGui::TextWrapped("%s", games[i].title.c_str());
        ImGui::PopTextWrapPos();
        ImGui::EndGroup();

        // Use same line if content fits horizontally, move to next line if not
        rowContentWidth += (gameImageSize * uiScale + itemSpacing * 2 + padding);
        if (rowContentWidth < maxAvailableWidth) {
            ImGui::SameLine(0.0f, padding);
        } else {
            ImGui::Dummy(ImVec2(0.0f, padding));
            rowContentWidth = gameImageSize * uiScale + itemSpacing;
        }
    }
}

void SaveInstallDirs() {
    std::string profile;
    const bool isGlobal = currentProfile == "Global";
    if (!isGlobal) {
        profile = currentProfile.substr(0, 9);
    }

    if (!isGlobal) {
        EmulatorSettings.Load();
    }

    EmulatorSettings.SetAllGameInstallDirs(m_GameInstallDirs);
    EmulatorSettings.Save();

    if (!isGlobal) {
        EmulatorSettings.Load(profile);
    }

    GetGameInfo(settingsProfileVec, true, globalSettingsTexture);
}

} // namespace BigPictureMode
