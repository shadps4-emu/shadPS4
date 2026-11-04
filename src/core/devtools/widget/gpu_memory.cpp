//  SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#include "gpu_memory.h"

#include <cstdio>
#include <imgui.h>

using namespace ImGui;

namespace Core::Devtools::Widget {

namespace {

float Mib(u64 bytes) {
    return static_cast<float>(bytes) / (1024.0f * 1024.0f);
}

u64 TotalBytes(const GpuMemoryGroup& group) {
    u64 total = 0;
    for (const auto& row : group.rows) {
        total += row.bytes;
    }
    return total;
}

} // Anonymous namespace

void GpuMemoryViewer::Draw() {
    SetNextWindowSize({360.0f, 300.0f}, ImGuiCond_FirstUseEver);
    if (!Begin("GPU Memory Usage", &open)) {
        End();
        return;
    }
    last_drawn_ms.store(NowMs(), std::memory_order_relaxed);

    std::map<std::string, GpuMemoryGroup> snapshot;
    {
        std::scoped_lock lock{mutex};
        snapshot = groups;
    }

    if (snapshot.empty()) {
        TextDisabled("Nothing published yet");
        End();
        return;
    }

    u64 total = 0;
    for (const auto& [name, group] : snapshot) {
        total += TotalBytes(group);
    }
    Text("Total: %.1f MiB", Mib(total));

    for (const auto& [name, group] : snapshot) {
        char header[160];
        std::snprintf(header, sizeof(header), "%s  %.1f MiB", name.c_str(), Mib(TotalBytes(group)));
        SeparatorText(header);
        PushID(name.c_str());
        if (BeginTable("rows", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            TableSetupColumn("Memory type", ImGuiTableColumnFlags_WidthStretch);
            TableSetupColumn("MiB", ImGuiTableColumnFlags_WidthFixed);
            TableHeadersRow();
            for (const auto& row : group.rows) {
                TableNextRow();
                TableNextColumn();
                TextUnformatted(row.name.c_str());
                TableNextColumn();
                Text("%.1f", Mib(row.bytes));
            }
            EndTable();
        }
        PopID();
    }

    End();
}

} // namespace Core::Devtools::Widget
