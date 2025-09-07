//  SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#include "module_list.h"

#include <imgui.h>

#include "common.h"
#include "core/debug_state.h"
#include "imgui/imgui_std.h"

using namespace ImGui;

namespace Core::Devtools::Widget {
void ModuleList::Draw() {
    SetNextWindowSize({550.0f, 600.0f}, ImGuiCond_FirstUseEver);
    if (!Begin("Module List", &open)) {
        End();
        return;
    }

    if (BeginTable("ModuleTable", 3,
                   ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_Sortable |
                       ImGuiTableFlags_RowBg)) {
        TableSetupColumn("Modulname", ImGuiTableColumnFlags_WidthStretch);
        TableHeadersRow();

        std::scoped_lock lock(modules_mutex);
        for (const auto& module : modules) {
            TableNextRow();

            TableSetColumnIndex(0);
            TextUnformatted(module.name.c_str());

            TableSetColumnIndex(1);
            if (module.is_sys_module) {
                TextColored({0.2f, 0.6f, 0.8f, 1.0f}, "System Module");
            } else {
                TextColored({0.8f, 0.4f, 0.2f, 1.0f}, "Game Module");
            }

            TableSetColumnIndex(2);
            if (module.is_lle) {
                TextColored({0.4f, 0.7f, 0.4f, 1.0f}, "LLE");
            } else {
                TextColored({0.7f, 0.4f, 0.5f, 1.0f}, "HLE");
            }
        }
        EndTable();
    }

    End();
}

} // namespace Core::Devtools::Widget