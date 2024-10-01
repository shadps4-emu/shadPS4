//  SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#include <cstdio>
#include <imgui.h>
#include <magic_enum.hpp>

#include "cmd_list.h"
#include "frame_dump.h"
#include "imgui_internal.h"

using namespace ImGui;

namespace Core::Devtools::Widget {

FrameDumpViewer::FrameDumpViewer(DebugStateType::FrameDump frame_dump)
    : frame_dump(std::move(frame_dump)) {
    static int unique_id = 0;
    id = unique_id++;
}

void FrameDumpViewer::Draw() {
    if (!is_open) {
        return;
    }

    char name[32];
    snprintf(name, sizeof(name), "Frame #%d dump", id);
    static ImGuiID dock_id = ImHashStr("FrameDumpDock");
    SetNextWindowDockID(dock_id, ImGuiCond_Appearing);
    if (Begin(name, &is_open, ImGuiWindowFlags_NoSavedSettings)) {
        if (IsWindowAppearing()) {
            auto window = GetCurrentWindow();
            SetWindowSize(window, ImVec2{450.0f, 500.0f});
        }
        if (BeginTabBar("Queues")) {
            for (auto& cmd : frame_dump.queues) {
                char tab_name[64];
                snprintf(tab_name, sizeof(tab_name), "%s - %d %d",
                         magic_enum::enum_name(cmd.type).data(), cmd.submit_num, cmd.num2);
                if (BeginTabItem(tab_name)) {
                    CmdListViewer(cmd.data).Draw();
                    EndTabItem();
                }
            }
            EndTabBar();
        }
    }
    End();
}

} // namespace Core::Devtools::Widget