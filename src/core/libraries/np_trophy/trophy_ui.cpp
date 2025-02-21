// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <chrono>
#include <mutex>
#include <imgui.h>
#include "common/assert.h"
#include "common/config.h"
#include "common/singleton.h"
#include "imgui/imgui_std.h"
#include "trophy_ui.h"

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

    std::filesystem::path trophyTypePath;
    std::filesystem::path ResourceDir;
    // covers Windows, Mac SDL, locally compiled builds
    if (std::filesystem::exists(std::filesystem::current_path() / "Resources")) {
        ResourceDir = std::filesystem::current_path() / "Resources";
    } else {
#if defined(__linux__)
        const char* AppDir = getenv("APPDIR");
        ResourceDir = std::filesystem::path(AppDir);
#elif defined(__APPLE__)
        ResourceDir = std::filesystem::current_path().parent_path() / "Resources";
#endif
    }

    if (trophy_type == "P") {
        trophyTypePath = ResourceDir / "platinum.png";
    } else if (trophy_type == "G") {
        trophyTypePath = ResourceDir / "gold.png";
    } else if (trophy_type == "S") {
        trophyTypePath = ResourceDir / "silver.png";
    } else if (trophy_type == "B") {
        trophyTypePath = ResourceDir / "bronze.png";
    }

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
        std::min(io.DisplaySize.x, (330 * AdjustWidth)),
        std::min(io.DisplaySize.y, (70 * AdjustHeight)),
    };

    SetNextWindowSize(window_size);
    SetNextWindowCollapsed(false);
    SetNextWindowPos(ImVec2(io.DisplaySize.x - (350 * AdjustWidth), (50 * AdjustHeight)));
    KeepNavHighlight();
    if (Begin("Trophy Window", nullptr,
              ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings |
                  ImGuiWindowFlags_NoInputs)) {
        if (trophy_icon) {
            SetCursorPosY((window_size.y * 0.5f) - (25 * AdjustHeight));
            Image(trophy_icon.GetTexture().im_id, ImVec2((50 * AdjustWidth), (50 * AdjustHeight)));
        } else {
            // placeholder
            const auto pos = GetCursorScreenPos();
            ImGui::GetWindowDrawList()->AddRectFilled(pos, pos + ImVec2{50.0f * AdjustHeight},
                                                      GetColorU32(ImVec4{0.7f}));
        }

        ImGui::SameLine();
        if (trophy_type_icon) {
            SetCursorPosY((window_size.y * 0.5f) - (15 * AdjustHeight));
            Image(trophy_type_icon.GetTexture().im_id,
                  ImVec2((30 * AdjustWidth), (30 * AdjustHeight)));
        } else {
            // placeholder
            const auto pos = GetCursorScreenPos();
            ImGui::GetWindowDrawList()->AddRectFilled(pos, pos + ImVec2{30.0f * AdjustHeight},
                                                      GetColorU32(ImVec4{0.7f}));
        }

        ImGui::SameLine();
        SetWindowFontScale((1.2 * AdjustHeight));
        char earned_text[] = "Trophy earned!\n%s";
        const float text_height =
            ImGui::CalcTextSize(std::strcat(earned_text, trophy_name.c_str())).y;
        SetCursorPosY((window_size.y - text_height) * 0.5f);
        TextWrapped("Trophy earned!\n%s", trophy_name.c_str());
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
