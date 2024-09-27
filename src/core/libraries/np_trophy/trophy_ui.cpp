// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <chrono>
#include <imgui.h>
#include "common/assert.h"
#include "imgui/imgui_std.h"
#include "imgui/imgui_texture.h"
#include "trophy_ui.h"

using namespace ImGui;
using namespace Libraries::NpTrophy;

TrophyUI::TrophyUI() {
    AddLayer(this);
}

TrophyUI::~TrophyUI() {
    Finish();
}

void Libraries::NpTrophy::TrophyUI::AddTrophyToQueue(std::string trophyIconPath,
                                                     std::string trophyName) {
    TrophyInfo newInfo;
    newInfo.trophyIconPath = trophyIconPath;
    newInfo.trophyName = trophyName;
    trophyQueue.push_back(newInfo);
}

void TrophyUI::Finish() {
    RemoveLayer(this);
}

bool displayingTrophy;
std::chrono::steady_clock::time_point trophyStartedTime;
bool iconLoaded = false;
RefCountedTexture trophyIcon;

void TrophyUI::Draw() {
    const auto& io = GetIO();

    const ImVec2 window_size{
        std::min(io.DisplaySize.x, 250.f),
        std::min(io.DisplaySize.y, 70.f),
    };

    if (trophyQueue.size() != 0) {
        if (!displayingTrophy) {
            displayingTrophy = true;
            trophyStartedTime = std::chrono::steady_clock::now();
        }

        std::chrono::steady_clock::time_point timeNow = std::chrono::steady_clock::now();
        std::chrono::seconds duration =
            std::chrono::duration_cast<std::chrono::seconds>(timeNow - trophyStartedTime);

        if (duration.count() >= 5) {
            trophyQueue.erase(trophyQueue.begin());
            displayingTrophy = false;
            iconLoaded = false;
        }

        if (trophyQueue.size() != 0) {
            SetNextWindowSize(window_size);
            SetNextWindowCollapsed(false);
            SetNextWindowPos(ImVec2(io.DisplaySize.x - 250, 50));
            KeepNavHighlight();

            TrophyInfo currentTrophyInfo = trophyQueue[0];

            if (!iconLoaded) {
                if (std::filesystem::exists(currentTrophyInfo.trophyIconPath)) {
                    trophyIcon = RefCountedTexture::DecodePngFile(currentTrophyInfo.trophyIconPath);
                    iconLoaded = true;
                } else {
                    LOG_ERROR(Lib_NpTrophy, "Couldnt load trophy icon at {}",
                              currentTrophyInfo.trophyIconPath);
                }
            }

            if (Begin("Trophy Window", nullptr,
                      ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings |
                          ImGuiWindowFlags_NoInputs)) {
                if (iconLoaded) {
                    Image(trophyIcon.GetTexture().im_id, ImVec2(50, 50));
                    ImGui::SameLine();
                }
                TextWrapped("Trophy earned!\n%s", currentTrophyInfo.trophyName.c_str());
            }
            End();
        }
    }
}