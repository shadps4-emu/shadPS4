//  SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#include <SDL3/SDL.h>
#include <cmrc/cmrc.hpp>

#include "core/devtools/layer.h"
#include "core/emulator_settings.h"
#include "core/file_format/psf.h"
#include "settings_dialog_imgui.h"

#include "imgui_fonts/notosansjp_regular.ttf.g.cpp"
#include "imgui_fonts/proggyvector_regular.ttf.g.cpp"

CMRC_DECLARE(res);

namespace SettingsImGui {

const float gameImageSize = 200.f;
const float settingsIconSize = 125.f;

static std::vector<Game> gameVec = {};

static float uiScale = 1.0f;
static CurrentSettings currentSettings;
static Textures textures;
static SDL_Renderer* renderer;

static SettingsCategory currentCategory = SettingsCategory::Profiles;
static std::string currentProfile = "Global";

void Init() {
    currentProfile = "Global";
    currentCategory = SettingsCategory::Profiles;

    LoadSettings("Global");

    SDL_Window* window = SDL_GetKeyboardFocus();
    renderer = SDL_GetRenderer(window);

    LoadTextureData("src/images/big_picture/settings.png", textures.general);

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigNavCursorVisibleAlways = true;

    GetGameInfo();
    uiScale = static_cast<float>(EmulatorSettings.GetBigPictureScale() / 1000.f);
}

void DeInit() {
    EmulatorSettings.Load();
    EmulatorSettings.SetBigPictureScale(static_cast<int>(uiScale * 1000));
    EmulatorSettings.Save();
}

std::filesystem::path UpdateChecker(const std::string sceItem, std::filesystem::path game_folder) {
    std::filesystem::path outputPath;
    auto update_folder = game_folder;
    update_folder += "-UPDATE";

    auto patch_folder = game_folder;
    patch_folder += "-patch";

    if (std::filesystem::exists(update_folder / "sce_sys" / sceItem)) {
        outputPath = update_folder / "sce_sys" / sceItem;
    } else if (std::filesystem::exists(patch_folder / "sce_sys" / sceItem)) {
        outputPath = patch_folder / "sce_sys" / sceItem;
    } else {
        outputPath = game_folder / "sce_sys" / sceItem;
    }

    return outputPath;
}

void SetGameIcons() {
    ImGuiStyle& style = ImGui::GetStyle();
    const float maxAvailableWidth = ImGui::GetContentRegionAvail().x;
    const float itemSpacing = style.ItemSpacing.x; // already scaled
    const float padding = 10.0f * uiScale;
    float rowContentWidth = gameImageSize * uiScale + itemSpacing;

    for (int i = 0; i < gameVec.size(); i++) {
        ImGui::BeginGroup();
        std::string ButtonName = "Button" + std::to_string(i);
        const char* ButtonNameChar = ButtonName.c_str();

        if (ImGui::ImageButton(ButtonNameChar, (ImTextureID)gameVec[i].iconTexture,
                               ImVec2(gameImageSize * uiScale, gameImageSize * uiScale))) {
            currentProfile = i == 0 ? "Global" : gameVec[i].serial + " - " + gameVec[i].title;
            LoadSettings(gameVec[i].serial);
        }

        if (i == 0) {
            ImGui::SetItemDefaultFocus();
        }

        // Scroll to item only when newly-focused
        if (ImGui::IsItemFocused() && !gameVec[i].focusState) {
            ImGui::SetScrollHereY(0.5f);
        }

        if (ImGui::IsWindowFocused())
            gameVec[i].focusState = ImGui::IsItemFocused();

        ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + gameImageSize * uiScale);
        ImGui::TextWrapped("%s", gameVec[i].title.c_str());
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

void GetGameInfo() {
    gameVec.clear();

    Game global;
    global.title = "Global";
    global.iconTexture = textures.general;
    gameVec.push_back(global);

    for (const auto& installLoc : EmulatorSettings.GetAllGameInstallDirs()) {
        if (installLoc.enabled && std::filesystem::exists(installLoc.path)) {
            for (const auto& entry : std::filesystem::directory_iterator(installLoc.path)) {
                if (entry.path().filename().string().ends_with("-UPDATE") ||
                    entry.path().filename().string().ends_with("-patch") || !entry.is_directory()) {
                    continue;
                }

                Game game;
                game.ebootPath = entry.path() / "eboot.bin";

                const std::string iconFileName = "icon0.png";
                std::filesystem::path iconPath = UpdateChecker(iconFileName, entry.path());
                game.iconTexture = IMG_LoadTexture(renderer, iconPath.string().c_str());

                PSF psf;
                const std::string sfoFileName = "param.sfo";
                std::filesystem::path sfoPath = UpdateChecker(sfoFileName, entry.path());

                if (psf.Open(sfoPath)) {
                    if (const auto title = psf.GetString("TITLE"); title.has_value()) {
                        game.title = *title;
                    }

                    if (const auto title_id = psf.GetString("TITLE_ID"); title_id.has_value()) {
                        game.serial = *title_id;
                    }
                } else {
                    continue;
                }

                game.focusState = false;
                gameVec.push_back(game);
            }
        }
    }

    std::sort(gameVec.begin() + 1, gameVec.end(), [](const Game& a, const Game& b) {
        return a.title < b.title; // Alphabetical order
    });
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
        }

        ImGui::SetWindowFontScale(uiScale);
        ImGuiWindowFlags child_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                       ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NavFlattened;

        ImVec4 settingsColor = ImVec4(0.1f, 0.1f, 0.12f, 0.8f); // Darker gray
        ImGui::PushStyleColor(ImGuiCol_ChildBg, settingsColor);
        ImGui::BeginChild("Categories", ImVec2(0, 0), ImGuiChildFlags_AutoResizeY,
                          child_flags | ImGuiWindowFlags_HorizontalScrollbar);

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(30.0f * uiScale, 0.0f));

        AddCategory("Profiles", textures.general, SettingsCategory::Profiles);
        AddCategory("General", textures.general, SettingsCategory::General);

        if (currentProfile != "Global")
            AddCategory("Experimental", textures.general, SettingsCategory::Experimental);

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
                             ImGui::GetStyle().FramePadding.x * 4.0f +
                             ImGui::GetStyle().ItemSpacing.x;
        ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - buttonsWidth);

        if (ImGui::Button("Save")) {
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
                DeInit();
                *open = false;
                ImGui::CloseCurrentPopup();
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
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4.0f * uiScale, 10.0f * uiScale));

    if (category == SettingsCategory::Profiles) {
        ImGui::Text("%s", ("Selected Profile: " + currentProfile).c_str());
        ImGui::Dummy(ImVec2(0, 20.f * uiScale));
    } else if (category == SettingsCategory::General) {
        if (ImGui::BeginTable("SettingsTable", 2)) {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 300.0f * uiScale);
            ImGui::TableSetupColumn("Value");

            AddSettingBool("Log Enabled", currentSettings.logEnabled);
            AddSettingCombo("Log Type", currentSettings.logType);
            AddSettingSliderInt("Volume", currentSettings.volume, 0, 500);

            ImGui::EndTable();
        }
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
        SetGameIcons();
        ImGui::EndChild();
    }
}

void SaveSettings(std::string profile) {
    bool isSpecific = currentProfile != "Global";

    EmulatorSettings.SetLogEnabled(currentSettings.logEnabled, isSpecific);
    EmulatorSettings.SetVolumeSlider(currentSettings.volume, isSpecific);
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

    currentSettings.logEnabled = EmulatorSettings.IsLogEnabled();
    currentSettings.volume = EmulatorSettings.GetVolumeSlider();
    currentSettings.logType = GetComboIndex(EmulatorSettings.GetLogType(), optionsLogType);
}

void AddCategory(std::string name, SDL_Texture* texture, SettingsCategory category) {
    ImGui::SameLine();
    ImGui::BeginGroup();
    if (ImGui::ImageButton(name.c_str(), ImTextureID(texture),
                           ImVec2(settingsIconSize * uiScale, settingsIconSize * uiScale))) {
        currentCategory = category;
    }

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

    ImGui::Text("%s", name.c_str());
    ImGui::TableNextColumn();
    ImGui::Checkbox(label.c_str(), &value);
}

void AddSettingSliderInt(std::string name, int& value, int min, int max) {
    std::string label = "##" + name;
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::Text("%s", name.c_str());

    ImGui::TableNextColumn();
    ImGui::SliderInt(label.c_str(), &value, min, max);
}

void AddSettingCombo(std::string name, int& value) {
    std::string label = "##" + name;
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::Text("%s", name.c_str());

    std::vector<std::string> options = optionsMap.at(name);
    ImGui::TableNextColumn();

    if (ImGui::BeginCombo(label.c_str(), options[value].c_str())) {
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

void LoadTextureData(std::string resourcePath, SDL_Texture*& texture) {
    auto resource = cmrc::res::get_filesystem();
    auto file = resource.open(resourcePath);
    std::vector<u8> texData = std::vector<u8>(file.begin(), file.end());
    SDL_IOStream* ioMem = SDL_IOFromMem(texData.data(), texData.size());
    texture = IMG_LoadTexture_IO(renderer, ioMem, true);
}

} // namespace SettingsImGui
