// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <chrono>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <cmrc/cmrc.hpp>
#include <common/path_util.h>
#include <imgui.h>

#ifdef ENABLE_QT_GUI
#include <qt_gui/background_music_player.h>
#endif

#include "common/assert.h"
#include "common/config.h"
#include "common/singleton.h"
#include "imgui/imgui_std.h"
#include "trophy_ui.h"

CMRC_DECLARE(res);
namespace fs = std::filesystem;
using namespace ImGui;
namespace Libraries::NpTrophy {

std::optional<TrophyUI> current_trophy_ui;
std::queue<TrophyInfo> trophy_queue;
std::mutex queueMtx;
bool isLeftSide;
double trophy_timer;

TrophyUI::TrophyUI(const std::filesystem::path& trophyIconPath, const std::string& trophyName,
                   const std::string_view& rarity)
    : trophy_name(trophyName), trophy_type(rarity) {

    isLeftSide = Config::leftSideTrophy();
    trophy_timer = Config::getTrophyNotificationDuration();

    if (std::filesystem::exists(trophyIconPath)) {
        trophy_icon = RefCountedTexture::DecodePngFile(trophyIconPath);
    } else {
        LOG_ERROR(Lib_NpTrophy, "Couldnt load trophy icon at {}",
                  fmt::UTF(trophyIconPath.u8string()));
    }

    std::string pathString = "src/images/";

    if (trophy_type == "P") {
        pathString += "platinum.png";
    } else if (trophy_type == "G") {
        pathString += "gold.png";
    } else if (trophy_type == "S") {
        pathString += "silver.png";
    } else if (trophy_type == "B") {
        pathString += "bronze.png";
    }

    const auto CustomTrophy_Dir = Common::FS::GetUserPath(Common::FS::PathType::CustomTrophy);
    std::string customPath;

    if (trophy_type == "P" && fs::exists(CustomTrophy_Dir / "platinum.png")) {
        customPath = (CustomTrophy_Dir / "platinum.png").string();
    } else if (trophy_type == "G" && fs::exists(CustomTrophy_Dir / "gold.png")) {
        customPath = (CustomTrophy_Dir / "gold.png").string();
    } else if (trophy_type == "S" && fs::exists(CustomTrophy_Dir / "silver.png")) {
        customPath = (CustomTrophy_Dir / "silver.png").string();
    } else if (trophy_type == "B" && fs::exists(CustomTrophy_Dir / "bronze.png")) {
        customPath = (CustomTrophy_Dir / "bronze.png").string();
    }

    std::vector<u8> imgdata;
    if (!customPath.empty()) {
        std::ifstream file(customPath, std::ios::binary);
        if (file) {
            imgdata = std::vector<u8>(std::istreambuf_iterator<char>(file),
                                      std::istreambuf_iterator<char>());
        } else {
            LOG_ERROR(Lib_NpTrophy, "Could not open custom file for trophy in {}", customPath);
        }
    } else {
        auto resource = cmrc::res::get_filesystem();
        auto file = resource.open(pathString);
        imgdata = std::vector<u8>(file.begin(), file.end());
    }

    trophy_type_icon = RefCountedTexture::DecodePngTexture(imgdata);

    AddLayer(this);

#ifdef ENABLE_QT_GUI
    QString musicPath = QString::fromStdString(CustomTrophy_Dir.string() + "/trophy.mp3");
    if (fs::exists(musicPath.toStdString())) {
        BackgroundMusicPlayer::getInstance().setVolume(100);
        BackgroundMusicPlayer::getInstance().playMusic(musicPath, false);
    }
#endif
}

TrophyUI::~TrophyUI() {
    Finish();
}

void TrophyUI::Finish() {
    RemoveLayer(this);
}

float fade_opacity = 0.0f;                 // Initial opacity (invisible)
ImVec2 start_pos = ImVec2(1280.0f, 50.0f); // Starts off screen, right
ImVec2 target_pos = ImVec2(0.0f, 50.0f);   // Final position
float animation_duration = 0.5f;           // Animation duration
float elapsed_time = 0.0f;                 // Animation time
float fade_out_duration = 0.5f;            // Final fade duration

void TrophyUI::Draw() {
    const auto& io = GetIO();

    float AdjustWidth = io.DisplaySize.x / 1280;
    float AdjustHeight = io.DisplaySize.y / 720;
    const ImVec2 window_size{
        std::min(io.DisplaySize.x, (350 * AdjustWidth)),
        std::min(io.DisplaySize.y, (70 * AdjustHeight)),
    };

    elapsed_time += io.DeltaTime;
    float progress = std::min(elapsed_time / animation_duration, 1.0f);

    // left or right position
    float final_pos_x;
    if (isLeftSide) {
        start_pos.x = -window_size.x;
        final_pos_x = 20 * AdjustWidth;
    } else {
        start_pos.x = io.DisplaySize.x;
        final_pos_x = io.DisplaySize.x - window_size.x - 20 * AdjustWidth;
    }

    ImVec2 current_pos = ImVec2(start_pos.x + (final_pos_x - start_pos.x) * progress,
                                start_pos.y + (target_pos.y - start_pos.y) * progress);

    trophy_timer -= io.DeltaTime;

    // If the remaining time of the trophy is less than or equal to 1 second, the fade-out begins.
    if (trophy_timer <= 1.0f) {
        float fade_out_time = 1.0f - (trophy_timer / 1.0f);
        fade_opacity = 1.0f - fade_out_time;
    } else {
        // Fade in , 0 to 1
        fade_opacity = progress;
    }

    fade_opacity = std::max(0.0f, std::min(fade_opacity, 1.0f));

    SetNextWindowSize(window_size);
    SetNextWindowPos(current_pos);
    SetNextWindowCollapsed(false);
    KeepNavHighlight();
    PushStyleVar(ImGuiStyleVar_Alpha, fade_opacity);

    if (Begin("Trophy Window", nullptr,
              ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings |
                  ImGuiWindowFlags_NoInputs)) {

        // Displays the trophy icon
        if (trophy_type_icon) {
            SetCursorPosY((window_size.y * 0.5f) - (25 * AdjustHeight));
            Image(trophy_type_icon.GetTexture().im_id,
                  ImVec2((50 * AdjustWidth), (50 * AdjustHeight)));
            ImGui::SameLine();
        } else {
            // Placeholder
            const auto pos = GetCursorScreenPos();
            ImGui::GetWindowDrawList()->AddRectFilled(pos, pos + ImVec2{50.0f * AdjustHeight},
                                                      GetColorU32(ImVec4{0.7f}));
            ImGui::Indent(60);
        }

        // Displays the name of the trophy
        const std::string combinedString = "Trophy earned!\n%s" + trophy_name;
        const float wrap_width =
            CalcWrapWidthForPos(GetCursorScreenPos(), (window_size.x - (60 * AdjustWidth)));
        SetWindowFontScale(1.2 * AdjustHeight);
        // If trophy name exceeds 1 line
        if (CalcTextSize(trophy_name.c_str()).x > wrap_width) {
            SetCursorPosY(5 * AdjustHeight);
            if (CalcTextSize(trophy_name.c_str()).x > (wrap_width * 2)) {
                SetWindowFontScale(0.95 * AdjustHeight);
            } else {
                SetWindowFontScale(1.1 * AdjustHeight);
            }
        } else {
            const float text_height = ImGui::CalcTextSize(combinedString.c_str()).y;
            SetCursorPosY((window_size.y - text_height) * 0.5);
        }
        ImGui::PushTextWrapPos(window_size.x - (60 * AdjustWidth));
        TextWrapped("Trophy earned!\n%s", trophy_name.c_str());
        ImGui::SameLine(window_size.x - (60 * AdjustWidth));

        // Displays the trophy icon
        if (trophy_icon) {
            SetCursorPosY((window_size.y * 0.5f) - (25 * AdjustHeight));
            Image(trophy_icon.GetTexture().im_id, ImVec2((50 * AdjustWidth), (50 * AdjustHeight)));
        } else {
            // Placeholder
            const auto pos = GetCursorScreenPos();
            ImGui::GetWindowDrawList()->AddRectFilled(pos, pos + ImVec2{50.0f * AdjustHeight},
                                                      GetColorU32(ImVec4{0.7f}));
        }
    }
    End();

    PopStyleVar();

    if (trophy_timer <= 0) {
        std::lock_guard<std::mutex> lock(queueMtx);
        if (!trophy_queue.empty()) {
            TrophyInfo next_trophy = trophy_queue.front();
            trophy_queue.pop();
            current_trophy_ui.emplace(next_trophy.trophy_icon_path, next_trophy.trophy_name,
                                      next_trophy.trophy_type);
        } else {
            current_trophy_ui.reset();
        }
    }
}

void AddTrophyToQueue(const std::filesystem::path& trophyIconPath, const std::string& trophyName,
                      const std::string_view& rarity) {
    std::lock_guard<std::mutex> lock(queueMtx);

    if (Config::getisTrophyPopupDisabled()) {
        return;
    } else if (current_trophy_ui.has_value()) {
        current_trophy_ui.reset();
    }

    TrophyInfo new_trophy;
    new_trophy.trophy_icon_path = trophyIconPath;
    new_trophy.trophy_name = trophyName;
    new_trophy.trophy_type = rarity;
    trophy_queue.push(new_trophy);

    if (!current_trophy_ui.has_value()) {
#ifdef ENABLE_QT_GUI
        BackgroundMusicPlayer::getInstance().stopMusic();
#endif
        // Resetting the animation for the next trophy
        elapsed_time = 0.0f;                // Resetting animation time
        fade_opacity = 0.0f;                // Starts invisible
        start_pos = ImVec2(1280.0f, 50.0f); // Starts off screen, right
        TrophyInfo next_trophy = trophy_queue.front();
        trophy_queue.pop();
        current_trophy_ui.emplace(next_trophy.trophy_icon_path, next_trophy.trophy_name,
                                  next_trophy.trophy_type);
    }
}

} // namespace Libraries::NpTrophy
