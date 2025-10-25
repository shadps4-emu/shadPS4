//  SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#include <cstdio>
#include <ctime>
#include <fmt/chrono.h>
#include <imgui.h>
#include <magic_enum/magic_enum.hpp>

#include "common/io_file.h"
#include "core/devtools/options.h"
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

FrameDumpViewer::FrameDumpViewer(const FrameDump& _frame_dump)
    : frame_dump(std::make_shared<FrameDump>(_frame_dump)) {
    static int unique_id = 0;
    id = unique_id++;

    selected_queue_type = QueueType::dcb;
    selected_submit_num = 0;
    selected_queue_num2 = 0;

    has_queue_type.fill(false);
    cmd_list_viewer.reserve(frame_dump->queues.size());
    for (const auto& cmd : frame_dump->queues) {
        if (!cmd.data.empty()) {
            has_queue_type[static_cast<s32>(cmd.type)] = true;
        }
        const auto fname = fmt::format("F{} {}_{:02}_{:02}", frame_dump->frame_id,
                                       magic_enum::enum_name(cmd.type), cmd.submit_num, cmd.num2);
        cmd_list_viewer.emplace_back(frame_dump.get(), cmd.data, cmd.base_addr, fname);
        if (cmd.type == QueueType::dcb && cmd.submit_num == 0 && cmd.num2 == 0) {
            selected_cmd = static_cast<s32>(cmd_list_viewer.size() - 1);
        }
    }
}

FrameDumpViewer::~FrameDumpViewer() = default;

void FrameDumpViewer::Draw() {
    if (!is_open) {
        return;
    }

    const auto try_select = [&, this] {
        const auto it = std::ranges::find_if(frame_dump->queues, [&](const auto& cmd) {
            return cmd.type == selected_queue_type &&
                   (selected_submit_num == -1 || cmd.submit_num == selected_submit_num) &&
                   (selected_queue_num2 == -1 || cmd.num2 == selected_queue_num2);
        });
        if (it != frame_dump->queues.end()) {
            selected_cmd = static_cast<s32>(std::distance(frame_dump->queues.begin(), it));
            selected_submit_num = static_cast<s32>(frame_dump->queues[selected_cmd].submit_num);
            selected_queue_num2 = static_cast<s32>(frame_dump->queues[selected_cmd].num2);
        }
    };

    bool is_showing = Options.frame_dump_render_on_collapse;
    bool is_collapsed = true;

    char name[32];
    snprintf(name, sizeof(name), "Frame #%d dump", frame_dump->frame_id);
    if (Begin(name, &is_open, ImGuiWindowFlags_NoSavedSettings)) {
        is_showing = true;
        is_collapsed = false;

        if (IsWindowAppearing()) {
            auto window = GetCurrentWindow();
            static ImGuiID dock_id = ImHashStr("FrameDumpDock");
            SetWindowDock(window, dock_id, ImGuiCond_Once | ImGuiCond_FirstUseEver);
            SetWindowSize(window, ImVec2{470.0f, 600.0f});
        }
        BeginGroup();
        TextEx("Queue type");
        SameLine();
        if (BeginCombo("##select_queue_type", magic_enum::enum_name(selected_queue_type).data(),
                       ImGuiComboFlags_WidthFitPreview)) {
            bool selected = false;
#define COMBO(x)                                                                                   \
    if (has_queue_type[static_cast<s32>(x)])                                                       \
    C_V(magic_enum::enum_name(x).data(), x, selected_queue_type, selected)
            COMBO(QueueType::dcb);
            COMBO(QueueType::ccb);
            COMBO(QueueType::acb);
            if (selected) {
                selected_submit_num = selected_queue_num2 = -1;
                try_select();
            }
            EndCombo();
        }
        SameLine();
        BeginDisabled(selected_cmd == -1);
        if (SmallButton("Dump cmd")) {
            auto time = std::time(nullptr);
            auto now_time = *std::localtime(&time);
            const auto fname = fmt::format("{:%F %H-%M-%S} {}_{}_{}.bin", now_time,
                                           magic_enum::enum_name(selected_queue_type),
                                           selected_submit_num, selected_queue_num2);
            Common::FS::IOFile file(fname, Common::FS::FileAccessMode::Create);
            const auto& data = frame_dump->queues[selected_cmd].data;
            if (file.IsOpen()) {
                DebugState.ShowDebugMessage(fmt::format("Dumping cmd as {}", fname));
                file.Write(data);
            } else {
                DebugState.ShowDebugMessage(fmt::format("Failed to save {}", fname));
                LOG_ERROR(Core, "Failed to open file {}", fname);
            }
        }
        EndDisabled();
        SameLine();
        if (BeginMenu("Filter")) {

            TextUnformatted("Shader name");
            SameLine();
            InputText("##filter_shader", filter.shader_name, sizeof(filter.shader_name));

            ImGui::EndMenu();
        }

        TextEx("Submit num");
        SameLine();
        if (BeginCombo("##select_submit_num", small_int_to_str(selected_submit_num).data(),
                       ImGuiComboFlags_WidthFitPreview)) {
            std::array<bool, 32> available_submits{false};
            for (const auto& cmd : frame_dump->queues) {
                if (cmd.type == selected_queue_type && !cmd.data.empty()) {
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
                try_select();
            }
            EndCombo();
        }
        SameLine();
        TextEx(selected_queue_type == QueueType::acb ? "Queue num" : "Buffer num");
        SameLine();
        if (BeginCombo("##select_queue_num2", small_int_to_str(selected_queue_num2).data(),
                       ImGuiComboFlags_WidthFitPreview)) {
            std::array<bool, 32> available_queues{false};
            for (const auto& cmd : frame_dump->queues) {
                if (cmd.type == selected_queue_type && cmd.submit_num == selected_submit_num &&
                    !cmd.data.empty()) {
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
                try_select();
            }
            EndCombo();
        }
        EndGroup();
    }
    if (is_showing && selected_cmd != -1) {
        cmd_list_viewer[selected_cmd].Draw(is_collapsed, filter);
    }
    End();
}

} // namespace Core::Devtools::Widget

#undef C_V