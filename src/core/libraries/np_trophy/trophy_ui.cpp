// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <chrono>
#include <imgui.h>
#include "common/assert.h"
#include "common/singleton.h"
#include "imgui/imgui_std.h"
#include "trophy_ui.h"

using namespace ImGui;
namespace Libraries::NpTrophy {

std::optional<TrophyUI> current_trophy_ui;
std::queue<TrophyInfo> trophy_queue;

TrophyUI::TrophyUI(std::filesystem::path trophyIconPath, std::string trophyName)
    : trophy_icon_path(trophyIconPath), trophy_name(trophyName) {
    trophy_start_time = std::chrono::system_clock::now().time_since_epoch().count();
    trophy_time_now = trophy_start_time;
    if (std::filesystem::exists(trophy_icon_path)) {
        trophy_icon = RefCountedTexture::DecodePngFile(trophy_icon_path);
    } else {
        LOG_ERROR(Lib_NpTrophy, "Couldnt load trophy icon at {}", trophy_icon_path.string());
    }
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

    const ImVec2 window_size{
        std::min(io.DisplaySize.x, 250.f),
        std::min(io.DisplaySize.y, 70.f),
    };

    SetNextWindowSize(window_size);
    SetNextWindowCollapsed(false);
    SetNextWindowPos(ImVec2(io.DisplaySize.x - 250, 50));
    KeepNavHighlight();

    if (Begin("Trophy Window", nullptr,
              ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings |
                  ImGuiWindowFlags_NoInputs)) {
        if (trophy_icon) {
            Image(trophy_icon.GetTexture().im_id, ImVec2(50, 50));
            ImGui::SameLine();
        } else {
            // placeholder
            ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(1040, 60), ImVec2(1090, 110),
                                                      GetColorU32(ImVec4{0.7f}));
            ImGui::Indent(60);
        }
        TextWrapped("Trophy earned!\n%s", trophy_name.c_str());
    }
    End();

    trophy_time_now += io.DeltaTime * 1000;
    if (trophy_time_now >= trophy_start_time + 5000) {
        if (!trophy_queue.empty()) {
            TrophyInfo next_trophy = trophy_queue.front();
            trophy_queue.pop();
            current_trophy_ui.emplace(next_trophy.trophy_icon_path, next_trophy.trophy_name);
        } else {
            current_trophy_ui.reset();
        }
    }
}

void AddTrophyToQueue(std::filesystem::path trophyIconPath, std::string trophyName) {
    if (current_trophy_ui.has_value()) {
        TrophyInfo new_trophy;
        new_trophy.trophy_icon_path = trophyIconPath;
        new_trophy.trophy_name = trophyName;
        trophy_queue.push(new_trophy);
    } else {
        current_trophy_ui.emplace(trophyIconPath, trophyName);
    }
}

} // namespace Libraries::NpTrophy