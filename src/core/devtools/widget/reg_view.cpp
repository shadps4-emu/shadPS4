//  SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#include <imgui.h>

#include "reg_view.h"

using namespace ImGui;

namespace Core::Devtools::Widget {

RegView::RegView() {
    static int unique_id = 0;
    id = unique_id++;
}

void RegView::Draw() {
    char name[32];
    snprintf(name, sizeof(name), "Reg view###reg_dump_%d", id);
    if (Begin(name, &open, ImGuiWindowFlags_NoSavedSettings)) {
        if (BeginTable("Enable shaders", 2)) {
            for (int i = 0; i < DebugStateType::RegDump::MaxShaderStages; i++) {
                TableNextRow();
                TableSetColumnIndex(0);
                const char* names[] = {"vs", "ps", "gs", "es", "hs", "ls"};
                Text("%s", names[i]);
                TableSetColumnIndex(1);
                Text("%X", data.regs.stage_enable.IsStageEnabled(i));
                TableSetColumnIndex(0);
            }
            EndTable();
        }
    }
    End();
}

} // namespace Core::Devtools::Widget