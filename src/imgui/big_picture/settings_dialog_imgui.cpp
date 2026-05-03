//  SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#include <map>
#include <ranges>
#include <ImGuiFileDialog.h>
#include <cmrc/cmrc.hpp>
#include <stb_image.h>

#include "common/elf_info.h"
#include "common/logging/log.h"
#include "common/path_util.h"
#include "core/devtools/layer.h"
#include "imgui/imgui_std.h"
#include "settings_dialog_imgui.h"

CMRC_DECLARE(res);

constexpr float gameImageSize = 200.f;
constexpr float settingsIconSize = 125.f;

namespace ImGuiEmuSettings {

int SettingsWindow::GetComboIndex(std::string selection, std::vector<std::string> options) {
    for (int i = 0; i < options.size(); i++) {
        if (selection == options[i])
            return i;
    }

    return 0;
}

void SettingsWindow::LoadSettings(std::string profile) {
    const bool isSpecific = currentProfile != "Global";
    isSpecific ? EmulatorSettings.Load(profile) : EmulatorSettings.Load();

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

void SettingsWindow::SaveSettings(std::string profile) {
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

    isSpecific ? EmulatorSettings.Save(profile) : EmulatorSettings.Save();
}

void SettingsWindow::SaveInstallDirs() {
    std::string profile;
    const bool isGlobal = currentProfile == "Global";
    if (!isGlobal) {
        profile = currentProfile.substr(0, 9);
        EmulatorSettings.Load();
    }

    EmulatorSettings.SetAllGameInstallDirs(m_GameInstallDirs);
    EmulatorSettings.Save();

    if (!isGlobal) {
        EmulatorSettings.Load(profile);
    }

    if (!isGameRunning) {
        GetProfileInfo();
    }
}

void SettingsWindow::GetProfileInfo() {
    GetGameIconInfo(profileIcons);

    BigPictureMode::IconInfo global;
    global.title = "Global";
    profileIcons.emplace(profileIcons.begin(), global);
}

SettingsWindow::SettingsWindow(bool gameRunning) : isGameRunning(gameRunning) {
    auto resource = cmrc::res::get_filesystem();
    auto loadTexture = [&](const std::string& resourcePath,
                           std::variant<SDL_Texture*, ImGui::RefCountedTexture>& texture) {
        auto file = resource.open(resourcePath);
        std::vector<u8> texData = std::vector<u8>(file.begin(), file.end());
        gameRunning ? texture = ImGui::RefCountedTexture::DecodePngTexture(texData)
                    : texture = BigPictureMode::LoadSdlTextureData(texData);
    };

    loadTexture("src/images/big_picture/settings.png", generalTexture);
    loadTexture("src/images/big_picture/experimental.png", experimentalTexture);
    loadTexture("src/images/big_picture/graphics.png", graphicsTexture);
    loadTexture("src/images/big_picture/controller.png", inputTexture);
    loadTexture("src/images/big_picture/trophy.png", trophyTexture);
    loadTexture("src/images/big_picture/log.png", logTexture);
    loadTexture("src/images/big_picture/folder.png", foldersTexture);
    loadTexture("src/images/big_picture/profiles.png", profilesTexture);

    auto languageKeys = std::views::keys(languageMap);
    languageOptions.assign(languageKeys.begin(), languageKeys.end());

    currentProfile = "Global";
    m_GameInstallDirs = EmulatorSettings.GetAllGameInstallDirs();
    currentCategory = isGameRunning ? SettingsCategory::General : SettingsCategory::Profiles;
    uiScale = static_cast<float>(EmulatorSettings.GetBigPictureScale() / 1000.f);

    bool customConfigFound = false;
    if (isGameRunning) {
        runningGameSerial = std::string(Common::ElfInfo::Instance().GameSerial());
        std::filesystem::path customConfigFile =
            Common::FS::GetUserPath(Common::FS::PathType::CustomConfigs) /
            (runningGameSerial + ".json");

        if (std::filesystem::exists(customConfigFile)) {
            customConfigFound = true;
            currentProfile =
                runningGameSerial + " - " + std::string(Common::ElfInfo::Instance().Title());
        }
    } else {
        GetProfileInfo();
    }

    customConfigFound ? LoadSettings(runningGameSerial) : LoadSettings("Global");
}

void SettingsWindow::DeInit() {
    EmulatorSettings.Load();
    EmulatorSettings.SetBigPictureScale(static_cast<int>(uiScale * 1000));
    EmulatorSettings.Save();

    if (isGameRunning && !runningGameSerial.empty()) {
        EmulatorSettings.Load(runningGameSerial);
    }
}
void SettingsWindow::DrawSettings(bool* open) {
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.06f, 0.06f, 0.06f, 1.00f)); // black
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.20f, 0.40f, 0.70f, 1.00f));   // blue
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered,
                          ImVec4(0.25f, 0.50f, 0.85f, 1.00f)); // lighter blue
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive,
                          ImVec4(0.26f, 0.59f, 0.98f, 0.80f)); // another light blue
    ImGui::PushStyleColor(ImGuiCol_SliderGrab,
                          ImVec4(0.26f, 0.59f, 0.98f, 0.80f)); // another light blue

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f * uiScale);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10.0f * uiScale, 10.0f * uiScale));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f * uiScale, 10.0f * uiScale));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.5f * uiScale);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20.0f * uiScale, 20.0f * uiScale));
    ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, 20.0f * uiScale);

    SetupWindow();
    DrawCategoryTabs();
    DrawMainContent(open);

    ImGui::PopStyleVar(8);
    ImGui::PopStyleColor(5);

    ImGui::End();
}

void SettingsWindow::SetupWindow() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);

    ImGui::Begin("Settings", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::DrawPrettyBackground();
    ImGui::SetWindowFontScale(uiScale);

    if (ImGui::IsWindowAppearing()) {
        ImGui::SetKeyboardFocusHere();
    }

    SettingsCategory firstCategory =
        isGameRunning ? SettingsCategory::General : SettingsCategory::Profiles;
    SettingsCategory lastCategory =
        currentProfile != "Global" ? SettingsCategory::Experimental : SettingsCategory::Log;

    // Navigate categories with Tab / R1 / L1
    if (ImGui::IsKeyPressed(ImGuiKey_GamepadR1) || ImGui::IsKeyPressed(ImGuiKey_Tab)) {
        int currentIndex = static_cast<int>(currentCategory);
        currentCategory == lastCategory
            ? currentCategory = static_cast<SettingsCategory>(firstCategory)
            : currentCategory = static_cast<SettingsCategory>(currentIndex + 1);
    }

    if (ImGui::IsKeyPressed(ImGuiKey_GamepadL1)) {
        int currentIndex = static_cast<int>(currentCategory);
        currentIndex == static_cast<int>(firstCategory)
            ? currentCategory = lastCategory
            : currentCategory = static_cast<SettingsCategory>(currentIndex - 1);
    }
}

void SettingsWindow::DrawCategoryTabs() {
    ImVec4 settingsColor = ImVec4(0.1f, 0.1f, 0.12f, 0.8f); // Darker gray
    ImGui::PushStyleColor(ImGuiCol_ChildBg, settingsColor);

    float vertSize = (settingsIconSize * uiScale + ImGui::CalcTextSize("Profiles").y) +
                     ImGui::GetStyle().FramePadding.y * 4.f + 20.0 * uiScale;
    ImGuiWindowFlags child_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                   ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NavFlattened;
    ImGui::BeginChild("Categories", ImVec2(0, vertSize), true,
                      child_flags | ImGuiWindowFlags_HorizontalScrollbar |
                          ImGuiWindowFlags_NoScrollWithMouse);

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(30.0f * uiScale, 0.0f));

    // Must add categories in enum order for L1/R1 to work correctly
    if (!isGameRunning) {
        AddCategory("Profiles", profilesTexture, SettingsCategory::Profiles);
    }

    AddCategory("General", generalTexture, SettingsCategory::General);
    AddCategory("Graphics", graphicsTexture, SettingsCategory::Graphics);
    AddCategory("Input", inputTexture, SettingsCategory::Input);
    AddCategory("Trophy", trophyTexture, SettingsCategory::Trophy);
    AddCategory("Game Folders", foldersTexture, SettingsCategory::Folders);
    AddCategory("Log", logTexture, SettingsCategory::Log);

    if (currentProfile != "Global") {
        AddCategory("Experimental", experimentalTexture, SettingsCategory::Experimental);
    }

    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    ImGui::EndChild();
}

void SettingsWindow::AddCategory(std::string name,
                                 std::variant<SDL_Texture*, ImGui::RefCountedTexture> texture,
                                 SettingsCategory category) {
    ImGui::SameLine();
    ImGui::BeginGroup();

    // make button appear hovered as long as category is selected, otherwise dull its hovered color
    currentCategory == category
        ? ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered])
        : ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.235f, 0.392f, 0.624f, 1.00f));

    ImTextureID id = nullptr;
    if (std::holds_alternative<SDL_Texture*>(texture)) {
        id = ImTextureID(std::get<SDL_Texture*>(texture));
    } else if (std::holds_alternative<ImGui::RefCountedTexture>(texture)) {
        id = std::get<ImGui::RefCountedTexture>(texture).GetTexture().im_id;
    }

    if (id != nullptr) {
        if (ImGui::ImageButton(name.c_str(), id,
                               ImVec2(settingsIconSize * uiScale, settingsIconSize * uiScale))) {
            currentCategory = category;
        }
    }

    ImGui::PopStyleColor();

    ImGui::SetCursorPosX(
        (ImGui::GetCursorPosX() +
         (settingsIconSize * uiScale - ImGui::CalcTextSize(name.c_str()).x) * 0.5f) +
        ImGui::GetStyle().FramePadding.x);
    ImGui::Text("%s", name.c_str());
    ImGui::EndGroup();
}

void SettingsWindow::DrawMainContent(bool* open) {
    ImVec4 settingsColor = ImVec4(0.1f, 0.1f, 0.12f, 0.8f); // Darker gray
    ImGui::PushStyleColor(ImGuiCol_ChildBg, settingsColor);
    ImGuiWindowFlags child_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                   ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NavFlattened;

    std::string centeredText;
    currentCategory == SettingsCategory::Folders
        ? centeredText = "Manage shadPS4 Game Folders"
        : centeredText = "Selected Profile: " + currentProfile;

    ImGui::Separator();
    Overlay::TextCentered(centeredText.c_str());
    ImGui::Separator();

    if (currentCategory == SettingsCategory::Profiles) {
        DrawProfileSelector();
    } else if (currentCategory == SettingsCategory::Folders) {
        DrawGameFolderManager();
    } else {
        DrawSettingsTable(currentCategory);
    }

    ImGui::PopStyleColor();

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
                         ImGui::CalcTextSize("Apply").x + ImGui::GetStyle().FramePadding.x * 6.0f +
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
    if (ImGui::BeginPopupModal("Save Confirmation", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
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
                closeOnSave = false;
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
}

void SettingsWindow::DrawProfileSelector() {
    ImGuiWindowFlags child_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                   ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NavFlattened;
    ImVec4 settingsColor = ImVec4(0.1f, 0.1f, 0.12f, 0.8f); // Darker gray
    ImGui::PushStyleColor(ImGuiCol_ChildBg, settingsColor);
    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);

    ImGui::BeginChild("Profile Selection", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), true,
                      child_flags);
    ImGui::PopStyleColor();

    if (ImGui::BeginTable("ProfilesTable", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders)) {
        ImGui::TableSetupColumn("CreateDeleteButton", ImGuiTableColumnFlags_WidthFixed,
                                400.0f * uiScale);
        ImGui::TableSetupColumn("Profile");

        for (int i = 0; i < profileIcons.size(); i++) {
            const std::filesystem::path customConfigFile =
                Common::FS::GetUserPath(Common::FS::PathType::CustomConfigs) /
                (profileIcons[i].serial + ".json");
            const bool gameConfigExists = std::filesystem::exists(customConfigFile);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            std::string deleteButtonLabel = "Delete game-specific config##" + std::to_string(i);

            if (gameConfigExists) {
                // Different shades of red depending on button state
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.0f, 0.0f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.0f, 0.0f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.0f, 0.0f, 1.0f));

                if (ImGui::Button(deleteButtonLabel.c_str(),
                                  ImVec2(ImGui::GetContentRegionAvail().x, 0.0f))) {
                    deleteProfileIndex = i;
                }

                ImGui::PopStyleColor(3);
            }

            ImGui::TableNextColumn();
            std::string profileLabel =
                i == 0 ? "Global" : profileIcons[i].serial + " - " + profileIcons[i].title;
            if (ImGui::Button(profileLabel.c_str(),
                              ImVec2(ImGui::GetContentRegionAvail().x, 0.0f))) {

                currentProfile = profileLabel;
                if (currentProfile == "Global") {
                    LoadSettings("Global");
                } else {
                    LoadSettings(profileIcons[i].serial);
                    if (!gameConfigExists) {
                        SaveSettings(profileIcons[i].serial);
                    }
                }
            }
        }

        ImGui::EndTable();
    }

    if (deleteProfileIndex != -1) {
        ImGui::OpenPopup("Confirm Delete");
    }

    ImGui::PopStyleVar(3);
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Confirm Delete", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        const std::string title = profileIcons[deleteProfileIndex].title;
        const std::string message = "Delete game-specific config file for " + title + "?";
        const std::filesystem::path path =
            Common::FS::GetUserPath(Common::FS::PathType::CustomConfigs) /
            (profileIcons[deleteProfileIndex].serial + ".json");

        ImGui::Text("%s", message.c_str());
        ImGui::Separator();

        if (ImGui::Button("OK", ImVec2(120 * uiScale, 0))) {
            try {
                std::filesystem::remove(path);
            } catch (const std::exception& e) {
                LOG_ERROR(ImGui, "Could not delete config file {}: {}", path.string(), e.what());
            }

            deleteProfileIndex = -1;
            currentProfile = "Global";
            LoadSettings("Global");
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(120 * uiScale, 0))) {
            deleteProfileIndex = -1;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    ImGui::EndChild();
}

void SettingsWindow::DrawGameFolderManager() {
    ImGuiWindowFlags child_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                   ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NavFlattened;
    ImGui::BeginChild("ContentRegion", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), true,
                      child_flags);

    if (ImGui::Button("Add Folder", ImVec2(400.f * uiScale, 0))) {
        ImGuiFileDialog::Instance()->OpenDialog("OpenFolder", "Add shadPS4 game folder", nullptr,
                                                ".", 1, nullptr,
                                                ImGuiFileDialogFlags_DisableCreateDirectoryButton |
                                                    ImGuiFileDialogFlags_DontShowHiddenFiles);

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
    }

    if (ImGuiFileDialog::Instance()->Display("OpenFolder", child_flags | ImGuiWindowFlags_NoMove)) {
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

    ImGui::BeginChild("Game Folder List", ImVec2(0, 0), true, child_flags);

    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0, 5.0f * uiScale));
    if (ImGui::BeginTable("FoldersTable", 3)) {
        ImGui::TableSetupColumn("FolderButton", ImGuiTableColumnFlags_WidthFixed, 300.0f * uiScale);
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
    ImGui::EndChild();
}

void SettingsWindow::DrawSettingsTable(SettingsCategory category) {
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4.0f * uiScale, 10.0f * uiScale));
    ImGuiWindowFlags child_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                   ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NavFlattened;

    ImGui::BeginChild("ContentRegion", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), true,
                      child_flags);

    if (category == SettingsCategory::General) {
        if (ImGui::BeginTable("SettingsTable", 2)) {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 500.0f * uiScale);
            ImGui::TableSetupColumn("Value");

            AddSettingCombo("Console Language", consoleLanguageSetting, languageOptions);
            AddSettingSliderInt("Volume", volumeSetting, 0, 500);
            AddSettingCheckbox("Show Splash Screen When Launching Game", showSplashSetting);
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
            AddSettingCheckbox("Enable HDR", hdrAllowedSetting);
            AddSettingCheckbox("Enable FSR", fsrEnabledSetting);

            if (fsrEnabledSetting) {
                AddSettingCheckbox("Enable RCAS", rcasEnabledSetting);
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

            AddSettingCheckbox("Enable Motion Controls", motionControlsSetting);
            AddSettingCheckbox("Enable Background Controller Input", backgroundControllerSetting);
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

            AddSettingCheckbox("Disable Trophy Notification", trophyPopupDisabledSetting);
            if (!trophyPopupDisabledSetting) {
                AddSettingCombo("Trophy Notification Position", trophySideSetting,
                                trophySideOptions);
                AddSettingSliderFloat("Trophy Notification Duration", trophyDurationSetting, 0.f,
                                      10.f, 1);
            }

            ImGui::EndTable();
        }
    } else if (category == SettingsCategory::Log) {
        if (ImGui::BeginTable("SettingsTable", 2)) {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 500.0f * uiScale);
            ImGui::TableSetupColumn("Value");

            AddSettingCheckbox("Enable Logging", logEnableSetting);
            if (logEnableSetting) {
                AddSettingCheckbox("Separate Log Files", logSeparateSetting);
                AddSettingCheckbox("Log Sync", logSyncSetting);
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
            AddSettingCheckbox("Enable Readback Linear Images", readbackLinearImagesSetting);
            AddSettingCheckbox("Enable Direct Memory Access", directMemoryAccessSetting);
            AddSettingCheckbox("Enable Devkit Console Mode", devkitConsoleSetting);
            AddSettingCheckbox("Enable PS4 Neo Mode", neoModeSetting);
            AddSettingCheckbox("Enable ShadNet", shadnetEnabledSetting);
            AddSettingCheckbox("Set Network Connected to True", connectedNetworkSetting);
            AddSettingCheckbox("Enable Shader Cache", pipelineCacheEnabledSetting);

            if (pipelineCacheEnabledSetting) {
                AddSettingCheckbox("Compress Shader Cache to Zip File",
                                   pipelineCacheArchiveSetting);
            }

            ImGui::EndTable();
        }
    }

    ImGui::PopStyleVar();
    ImGui::EndChild();
}

void SettingsWindow::AddSettingCheckbox(std::string name, bool& value) {
    std::string label = "##" + name;
    ImGui::TableNextRow();
    ImGui::TableNextColumn();

    ImGui::TextWrapped("%s", name.c_str());
    ImGui::TableNextColumn();
    ImGui::Checkbox(label.c_str(), &value);
}

void SettingsWindow::AddSettingSliderInt(std::string name, int& value, int min, int max) {
    std::string label = "##" + name;
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TextWrapped("%s", name.c_str());

    ImGui::TableNextColumn();
    ImGui::SliderInt(label.c_str(), &value, min, max);
}

void SettingsWindow::AddSettingSliderFloat(std::string name, float& value, int min, int max,
                                           int precision) {
    std::string label = "##" + name;
    std::string precisionString = "%." + std::to_string(precision) + "f";

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TextWrapped("%s", name.c_str());

    ImGui::TableNextColumn();
    ImGui::SliderFloat(label.c_str(), &value, min, max, precisionString.c_str());
}

void SettingsWindow::AddSettingCombo(std::string name, int& value,
                                     std::vector<std::string> options) {
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

} // namespace ImGuiEmuSettings
