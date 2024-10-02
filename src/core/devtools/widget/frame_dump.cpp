//  SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#include <cstdio>
#include <fmt/chrono.h>
#include <imgui.h>
#include <magic_enum.hpp>

#include "common/io_file.h"
#include "frame_dump.h"
#include "imgui_internal.h"
#include "imgui_memory_editor.h"

using namespace ImGui;
using namespace DebugStateType;

#define C_V(label, value, var, out)                                                                \
    if (Selectable(label, var == value)) {                                                         \
        var = value;                                                                               \
        selected_cmd = -1;                                                                         \
        out = true;                                                                                \
    }

// 00 to 99
static std::array<char, 3> small_int_to_str(const s32 i) {
    std::array<char, 3> label{};
    if (i == -1) {
        label[0] = 'N';
        label[1] = 'A';
    } else {
        label[0] = i / 10 + '0';
        label[1] = i % 10 + '0';
    }
    return label;
}

namespace Core::Devtools::Widget {

FrameDumpViewer::FrameDumpViewer(FrameDump _frame_dump) : frame_dump(std::move(_frame_dump)) {
    static int unique_id = 0;
    id = unique_id++;

    selected_queue_type = QueueType::dcb;
    selected_submit_num = 0;
    selected_queue_num2 = 0;

    cmd_list_viewer.reserve(frame_dump.queues.size());
    for (const auto& cmd : frame_dump.queues) {
        cmd_list_viewer.emplace_back(this, cmd.data);
        if (cmd.type == QueueType::dcb && cmd.submit_num == selected_submit_num &&
            cmd.num2 == selected_queue_num2) {
            selected_cmd = cmd_list_viewer.size() - 1;
        }
    }

    cmdb_view.Open = false;
    cmdb_view.ReadOnly = true;
}

FrameDumpViewer::~FrameDumpViewer() {}

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
            SetWindowSize(window, ImVec2{470.0f, 600.0f});
        }
        BeginGroup();
        TextEx("Queue type");
        SameLine();
        if (BeginCombo("##select_queue_type", magic_enum::enum_name(selected_queue_type).data(),
                       ImGuiComboFlags_WidthFitPreview)) {
            bool selected = false;
#define COMBO(x) C_V(magic_enum::enum_name(x).data(), x, selected_queue_type, selected)
            COMBO(QueueType::acb)
            COMBO(QueueType::dcb);
            COMBO(QueueType::ccb);
            if (selected) {
                selected_submit_num = selected_queue_num2 = -1;
            }
            EndCombo();
        }
        SameLine();
        TextEx("Submit num");
        SameLine();
        if (BeginCombo("##select_submit_num", small_int_to_str(selected_submit_num).data(),
                       ImGuiComboFlags_WidthFitPreview)) {
            std::array<bool, 32> available_submits{};
            for (const auto& cmd : frame_dump.queues) {
                if (cmd.type == selected_queue_type) {
                    available_submits[cmd.submit_num] = true;
                }
            }
            bool selected = false;
            for (int i = 0; i < available_submits.size(); ++i) {
                if (available_submits[i]) {
                    char label[3]{};
                    label[0] = i / 10 + '0';
                    label[1] = i % 10 + '0';
                    C_V(label, i, selected_submit_num, selected);
                }
            }
            if (selected) {
                selected_queue_num2 = -1;
            }
            EndCombo();
        }
        SameLine();
        TextEx(selected_queue_type == QueueType::acb ? "Queue num" : "Buffer num");
        SameLine();
        if (BeginCombo("##select_queue_num2", small_int_to_str(selected_queue_num2).data(),
                       ImGuiComboFlags_WidthFitPreview)) {
            std::array<bool, 32> available_queues{};
            for (const auto& cmd : frame_dump.queues) {
                if (cmd.type == selected_queue_type && cmd.submit_num == selected_submit_num) {
                    available_queues[cmd.num2] = true;
                }
            }
            bool selected = false;
            for (int i = 0; i < available_queues.size(); ++i) {
                if (available_queues[i]) {
                    char label[3]{};
                    label[0] = i / 10 + '0';
                    label[1] = i % 10 + '0';
                    C_V(label, i, selected_queue_num2, selected);
                }
            }
            if (selected) {
                const auto it = std::ranges::find_if(frame_dump.queues, [&](const auto& cmd) {
                    return cmd.type == selected_queue_type &&
                           cmd.submit_num == selected_submit_num && cmd.num2 == selected_queue_num2;
                });
                if (it != frame_dump.queues.end()) {
                    selected_cmd = std::distance(frame_dump.queues.begin(), it);
                }
            }
            EndCombo();
        }
        SameLine();
        BeginDisabled(selected_cmd == -1);
        if (SmallButton("Dump cmd")) {
            auto now_time = fmt::localtime(std::time(nullptr));
            const auto fname = fmt::format("{:%F %H-%M-%S} {}_{}_{}.bin", now_time,
                                           magic_enum::enum_name(selected_queue_type),
                                           selected_submit_num, selected_queue_num2);
            Common::FS::IOFile file(fname, Common::FS::FileAccessMode::Write);
            auto& data = frame_dump.queues[selected_cmd].data;
            if (file.IsOpen()) {
                DebugState.ShowDebugMessage(fmt::format("Dumping cmd as {}", fname));
                file.Write(data);
            } else {
                DebugState.ShowDebugMessage(fmt::format("Failed to save {}", fname));
                LOG_ERROR(Core, "Failed to open file {}", fname);
            }
        }
        EndDisabled();
        EndGroup();

        if (selected_cmd != -1) {
            cmd_list_viewer[selected_cmd].Draw();
        }
    }
    End();

    if (cmdb_view.Open && selected_cmd != -1) {
        auto& cmd = frame_dump.queues[selected_cmd].data;
        auto cmd_size = cmd.size() * sizeof(u32);
        MemoryEditor::Sizes s;
        cmdb_view.CalcSizes(s, cmd_size, (size_t)cmd.data());
        SetNextWindowSizeConstraints(ImVec2(0.0f, 0.0f), ImVec2(s.WindowWidth, FLT_MAX));

        char name[64];
        snprintf(name, sizeof(name), "[GFX] Command buffer %d###cmdbuf_hex_%d", id, id);
        if (Begin(name, &cmdb_view.Open, ImGuiWindowFlags_NoScrollbar)) {
            cmdb_view.DrawContents(cmd.data(), cmd_size, (size_t)cmd.data());
        }
        End();
    }
}

} // namespace Core::Devtools::Widget

#undef C_V