//  SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#include "module_list.h"

#include <imgui.h>

#include "common.h"
#include "core/debug_state.h"
#include "imgui/imgui_std.h"

using namespace ImGui;

namespace Core::Devtools::Widget {

void ModuleList::Draw() {
    {
        std::scoped_lock lock(s_modules_mutex);
        modules.clear();
        for (const auto& entry : s_modules) {
            ModuleInfo info;
            info.name = entry.name;
            modules.push_back(info);
        }
    }

    SetNextWindowSize({500.0f, 600.0f}, ImGuiCond_FirstUseEver);
    if (!Begin("LLE Module List", &open)) {
        End();
        return;
    }

    if (BeginTable("ModuleTable", 1,
                   ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_Sortable |
                       ImGuiTableFlags_RowBg)) {
        TableSetupColumn("Modulname", ImGuiTableColumnFlags_WidthStretch);
        TableHeadersRow();

        for (const auto& module : modules) {
            TableNextRow();

            TableSetColumnIndex(0);
            TextUnformatted(module.name.c_str());
        }
        EndTable();
    }

    End();
}

} // namespace Core::Devtools::Widget