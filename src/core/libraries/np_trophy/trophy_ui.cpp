// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <chrono>
#include <mutex>
#include <cmrc/cmrc.hpp>
#include <imgui.h>
#include "common/assert.h"
#include "common/config.h"
#include "common/singleton.h"
#include "imgui/imgui_std.h"
#include "trophy_ui.h"

CMRC_DECLARE(res);

using namespace ImGui;
namespace Libraries::NpTrophy {

std::optional<TrophyUI> current_trophy_ui;
std::queue<TrophyInfo> trophy_queue;
std::mutex queueMtx;

TrophyUI::TrophyUI(const std::filesystem::path& trophyIconPath, const std::string& trophyName,
                   const std::string_view& rarity)
    : trophy_name(trophyName), trophy_type(rarity) {

    if (std::filesystem::exists(trophyIconPath)) {
        trophy_icon = RefCountedTexture::DecodePngFile(trophyIconPath);
    } else {
        LOG_ERROR(Lib_NpTrophy, "Couldnt load trophy icon at {}",
                  fmt::UTF(trophyIconPath.u8string()));
    }

    std::string pathString;
    if (trophy_type == "P") {
        pathString = "Resources/platinum.png";
    } else if (trophy_type == "G") {
        pathString = "Resources/gold.png";
    } else if (trophy_type == "S") {
        pathString = "Resources/silver.png";
    } else if (trophy_type == "B") {
        pathString = "Resources/bronze.png";
    }

    auto resource = cmrc::res::get_filesystem();
    auto trophytypefile = resource.open(pathString);
    std::filesystem::path trophyTypePath = pathString;
    if (std::filesystem::exists(trophyTypePath))
        trophy_type_icon = RefCountedTexture::DecodePngFile(trophyTypePath);

    AddLayer(this);
}

TrophyUI::~TrophyUI() {
    Finish();
}

void TrophyUI::Finish() {
    RemoveLayer(this);
}

void TrophyUI::Draw() {
    const auto& io = GetIO();

    float AdjustWidth = io.DisplaySize.x / 1280;
    float AdjustHeight = io.DisplaySize.y / 720;
    const ImVec2 window_size{
        std::min(io.DisplaySize.x, (350 * AdjustWidth)),
        std::min(io.DisplaySize.y, (70 * AdjustHeight)),
    };

    SetNextWindowSize(window_size);
    SetNextWindowCollapsed(false);
    SetNextWindowPos(ImVec2(io.DisplaySize.x - (370 * AdjustWidth), (50 * AdjustHeight)));
    KeepNavHighlight();
    if (Begin("Trophy Window", nullptr,
              ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings |
                  ImGuiWindowFlags_NoInputs)) {
        if (trophy_type_icon) {
            SetCursorPosY((window_size.y * 0.5f) - (25 * AdjustHeight));
            Image(trophy_type_icon.GetTexture().im_id,
                  ImVec2((50 * AdjustWidth), (50 * AdjustHeight)));
        } else {
            // placeholder
            const auto pos = GetCursorScreenPos();
            ImGui::GetWindowDrawList()->AddRectFilled(pos, pos + ImVec2{50.0f * AdjustHeight},
                                                      GetColorU32(ImVec4{0.7f}));
        }

        ImGui::SameLine();

        SetWindowFontScale((1.2 * AdjustHeight));
        char earned_text[] = "Trophy earned!\n%s";
        const float text_height =
            ImGui::CalcTextSize(std::strcat(earned_text, trophy_name.c_str())).y;
        SetCursorPosY((window_size.y - text_height) * 0.5f);

        ImGui::PushTextWrapPos(window_size.x - (60 * AdjustWidth));
        TextWrapped("Trophy earned!\n%s", trophy_name.c_str());
        ImGui::SameLine(window_size.x - (60 * AdjustWidth));

        if (trophy_icon) {
            SetCursorPosY((window_size.y * 0.5f) - (25 * AdjustHeight));
            Image(trophy_icon.GetTexture().im_id, ImVec2((50 * AdjustWidth), (50 * AdjustHeight)));
        } else {
            // placeholder
            const auto pos = GetCursorScreenPos();
            ImGui::GetWindowDrawList()->AddRectFilled(pos, pos + ImVec2{30.0f * AdjustHeight},
                                                      GetColorU32(ImVec4{0.7f}));
        }
    }
    End();

    trophy_timer -= io.DeltaTime;
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
        TrophyInfo new_trophy;
        new_trophy.trophy_icon_path = trophyIconPath;
        new_trophy.trophy_name = trophyName;
        new_trophy.trophy_type = rarity;
        trophy_queue.push(new_trophy);
    } else {
        current_trophy_ui.emplace(trophyIconPath, trophyName, rarity);
    }
}

} // namespace Libraries::NpTrophy
