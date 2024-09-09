// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <imgui.h>
#include "common/assert.h"
#include "imgui/imgui_std.h"
#include "trophy_ui.h"

using namespace ImGui;
using namespace Libraries::NpTrophy;

TrophyUI::TrophyUI(int trophyId, std::string trophyName, TrophyType trophyType)
                   : trophyId(trophyId), trophyName(trophyName), trophyType(trophyType) {
    first_render = true;
    AddLayer(this);
}

TrophyUI::TrophyUI() {
    first_render = true;
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
        std::min(io.DisplaySize.x, 200.f),
        std::min(io.DisplaySize.y, 125.f),
    };

    CentralizeWindow();
    SetNextWindowSize(window_size);
    SetNextWindowFocus();
    SetNextWindowCollapsed(false);
    SetNextWindowPos(ImVec2(io.DisplaySize.x - 200, 50));
    PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
    KeepNavHighlight();

    if (trophyId != -1) {
        if (Begin("Trophy Window", nullptr,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings)) {
            Text("Trophy earned!");
            Text(trophyName.c_str());

            End();
        }
    }

    PopStyleColor();
    first_render = false;
}