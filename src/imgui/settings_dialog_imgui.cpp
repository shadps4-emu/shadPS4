//  SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#include <cmrc/cmrc.hpp>
#include <stb_image.h>

#include "core/devtools/layer.h"
#include "core/emulator_settings.h"
#include "imgui/imgui_std.h"
#include "settings_dialog_imgui.h"

#include "imgui_fonts/notosansjp_regular.ttf.g.cpp"
#include "imgui_fonts/proggyvector_regular.ttf.g.cpp"

CMRC_DECLARE(res);

namespace BigPictureMode {

const float gameImageSize = 200.f;
const float settingsIconSize = 125.f;
static std::vector<Game> settingsProfileVec = {};

static float uiScale = 1.0f;
static CurrentSettings currentSettings;
static Textures textures;
static SDL_Renderer* renderer;

static SettingsCategory currentCategory = SettingsCategory::Profiles;
static std::string currentProfile = "Global";
static bool closeOnSave = false;

void Init() {
    currentProfile = "Global";
    currentCategory = SettingsCategory::Profiles;
    LoadSettings("Global");

    SDL_Window* window = SDL_GetKeyboardFocus();
    renderer = SDL_GetRenderer(window);

    LoadEmbeddedTexture("src/images/big_picture/settings.png", textures.general);
    LoadEmbeddedTexture("src/images/big_picture/folder.png", textures.profiles);
    LoadEmbeddedTexture("src/images/big_picture/global-settings.png", textures.globalSettings);
    LoadEmbeddedTexture("src/images/big_picture/experimental.png", textures.experimental);
    LoadEmbeddedTexture("src/images/big_picture/graphics.png", textures.graphics);
    LoadEmbeddedTexture("src/images/big_picture/controller.png", textures.input);
    LoadEmbeddedTexture("src/images/big_picture/trophy.png", textures.trophy);
    LoadEmbeddedTexture("src/images/big_picture/log.png", textures.log);

    GetGameInfo(settingsProfileVec, true, textures.globalSettings);
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

    if (ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_NoDecoration)) {
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
        ImGui::BeginChild("Categories", ImVec2(0, 0), ImGuiChildFlags_AutoResizeY,
                          child_flags | ImGuiWindowFlags_HorizontalScrollbar);

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(30.0f * uiScale, 0.0f));

        AddCategory("Profiles", textures.profiles, SettingsCategory::Profiles);
        AddCategory("General", textures.general, SettingsCategory::General);
        AddCategory("Graphics", textures.graphics, SettingsCategory::Graphics);
        AddCategory("Input", textures.input, SettingsCategory::Input);
        AddCategory("Trophy", textures.trophy, SettingsCategory::Trophy);
        AddCategory("Log", textures.log, SettingsCategory::Log);

        if (currentProfile != "Global")
            AddCategory("Experimental", textures.experimental, SettingsCategory::Experimental);

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
    }

    ImGui::End();
}

void LoadCategory(SettingsCategory category) {
    ImGui::TextColored(ImVec4(0.00f, 1.00f, 1.00f, 1.00f), "%s",
                       ("Selected Profile: " + currentProfile).c_str()); // Dark Blue
    ImGui::Dummy(ImVec2(0, 20.f * uiScale));
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4.0f * uiScale, 10.0f * uiScale));

    if (category == SettingsCategory::General) {
        if (ImGui::BeginTable("SettingsTable", 2)) {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 500.0f * uiScale);
            ImGui::TableSetupColumn("Value");

            AddSettingCombo("Console Language", currentSettings.consoleLanguage);
            AddSettingSliderInt("Volume", currentSettings.volume, 0, 500);
            AddSettingBool("Show Splash Screen When Launching Game", currentSettings.showSplash);
            AddSettingCombo("Audio Backend", currentSettings.audioBackend);

            ImGui::EndTable();
        }
    } else if (category == SettingsCategory::Graphics) {
        if (ImGui::BeginTable("SettingsTable", 2)) {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 500.0f * uiScale);
            ImGui::TableSetupColumn("Value");

            AddSettingCombo("Display Mode", currentSettings.fullscreenMode);
            AddSettingCombo("Present Mode", currentSettings.presentMode);
            AddSettingSliderInt("Window Width", currentSettings.windowWidth, 0, 8000);
            AddSettingSliderInt("Window Height", currentSettings.windowHeight, 0, 7000);
            AddSettingBool("Enable HDR", currentSettings.hdrAllowed);
            AddSettingBool("Enable FSR", currentSettings.fsrEnabled);
            AddSettingBool("Enable RCAS", currentSettings.rcasEnabled);
            AddSettingSliderFloat("RCAS Attenuation", currentSettings.rcasAttenuation, 0.0f, 3.0f,
                                  3);

            ImGui::EndTable();
        }
    } else if (category == SettingsCategory::Input) {
        ImGui::Text("placeholder");
    } else if (category == SettingsCategory::Trophy) {
        ImGui::Text("placeholder");
    } else if (category == SettingsCategory::Log) {
        ImGui::Text("placeholder");
    } else if (category == SettingsCategory::Experimental) {
        ImGui::Text("placeholder");
    }

    ImGui::PopStyleVar();

    // Child Window if Needed
    if (category == SettingsCategory::Profiles) {
        ImGuiWindowFlags child_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                       ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NavFlattened;
        ImGui::BeginChild("ProfileSelect", ImVec2(0, 0), true, child_flags);
        Overlay::TextCentered("Select Global or Game-Specific Settings Profile");
        SetProfileIcons(settingsProfileVec);
        ImGui::EndChild();
    }
}

void SaveSettings(std::string profile) {
    bool isSpecific = currentProfile != "Global";

    /////////// General Tab
    EmulatorSettings.SetConsoleLanguage(
        languageMap.at(optionsLanguage.at(currentSettings.consoleLanguage)), isSpecific);
    EmulatorSettings.SetVolumeSlider(currentSettings.volume, isSpecific);
    EmulatorSettings.SetShowSplash(currentSettings.showSplash, isSpecific);
    EmulatorSettings.SetAudioBackend(currentSettings.audioBackend, isSpecific);

    /////////// Graphics Tab
    bool isFullscreen = currentSettings.fullscreenMode != 0;
    EmulatorSettings.SetFullScreen(isFullscreen);
    EmulatorSettings.SetFullScreenMode(optionsFullscreenMode.at(currentSettings.fullscreenMode),
                                       isSpecific);
    EmulatorSettings.SetPresentMode(optionsPresentMode.at(currentSettings.presentMode), isSpecific);
    EmulatorSettings.SetWindowHeight(currentSettings.windowHeight, isSpecific);
    EmulatorSettings.SetWindowWidth(currentSettings.windowWidth, isSpecific);
    EmulatorSettings.SetHdrAllowed(currentSettings.hdrAllowed, isSpecific);
    EmulatorSettings.SetFsrEnabled(currentSettings.fsrEnabled, isSpecific);
    EmulatorSettings.SetRcasEnabled(currentSettings.rcasEnabled, isSpecific);
    EmulatorSettings.SetRcasAttenuation(static_cast<int>(currentSettings.rcasAttenuation * 1000),
                                        isSpecific);

    /////////// Log Tab
    EmulatorSettings.SetLogEnabled(currentSettings.logEnabled, isSpecific);
    EmulatorSettings.SetLogType(optionsLogType.at(currentSettings.logType), isSpecific);

    if (currentProfile == "Global") {
        EmulatorSettings.Save();
    } else {
        EmulatorSettings.Save(profile);
    }
}

void LoadSettings(std::string profile) {
    if (currentProfile == "Global") {
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
    currentSettings.consoleLanguage = GetComboIndex(language, optionsLanguage);
    currentSettings.volume = EmulatorSettings.GetVolumeSlider();
    currentSettings.showSplash = EmulatorSettings.IsShowSplash();
    currentSettings.audioBackend = EmulatorSettings.GetAudioBackend();

    /////////// Graphics Tab
    currentSettings.fullscreenMode =
        GetComboIndex(EmulatorSettings.GetFullScreenMode(), optionsFullscreenMode);
    currentSettings.presentMode =
        GetComboIndex(EmulatorSettings.GetPresentMode(), optionsPresentMode);
    currentSettings.windowHeight = EmulatorSettings.GetWindowHeight();
    currentSettings.windowWidth = EmulatorSettings.GetWindowWidth();
    currentSettings.hdrAllowed = EmulatorSettings.IsHdrAllowed();
    currentSettings.fsrEnabled = EmulatorSettings.IsFsrEnabled();
    currentSettings.rcasEnabled = EmulatorSettings.IsRcasEnabled();
    currentSettings.rcasAttenuation =
        static_cast<float>(EmulatorSettings.GetRcasAttenuation() * 0.001f);

    /////////// Log Tab
    currentSettings.logEnabled = EmulatorSettings.IsLogEnabled();
    currentSettings.logType = GetComboIndex(EmulatorSettings.GetLogType(), optionsLogType);
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

void AddSettingCombo(std::string name, int& value) {
    std::string label = "##" + name;
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TextWrapped("%s", name.c_str());

    std::vector<std::string> options = optionsMap.at(name);
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

} // namespace BigPictureMode
