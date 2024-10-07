//  SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#include <filesystem>
#include <optional>
#include <string>
#include <imgui.h>
#include <stdio.h>

#include "common/io_file.h"
#include "core/devtools/options.h"
#include "imgui_internal.h"
#include "reg_view.h"

#if defined(_WIN32)
#define popen _popen
#define pclose _pclose
#endif

using namespace ImGui;

static std::optional<std::string> exec_cli(const char* cli) {
    std::array<char, 64> buffer{};
    std::string output;
    const auto f = popen(cli, "r");
    if (!f) {
        pclose(f);
        return {};
    }
    while (fgets(buffer.data(), buffer.size(), f)) {
        output += buffer.data();
    }
    pclose(f);
    return output;
}

namespace Core::Devtools::Widget {

void RegView::ProcessShader(int shader_id) {
    auto shader = data.stages[shader_id];

    std::string shader_dis;

    if (Options.disassembly_cli.empty()) {
        shader_dis = "No disassembler set";
    } else {
        auto bin_path = std::filesystem::temp_directory_path() / "shadps4_tmp_shader.bin";

        constexpr std::string_view src_arg = "{src}";
        std::string cli = Options.disassembly_cli;
        const auto pos = cli.find(src_arg);
        if (pos == std::string::npos) {
            DebugState.ShowDebugMessage("Disassembler CLI does not contain {src} argument");
        } else {
            cli.replace(pos, src_arg.size(), "\"" + bin_path.string() + "\"");
            Common::FS::IOFile file(bin_path, Common::FS::FileAccessMode::Write);
            file.Write(shader.code);
            file.Close();

            auto result = exec_cli(cli.c_str());
            shader_dis = result.value_or("Could not disassemble shader");
            if (shader_dis.empty()) {
                shader_dis = "Disassembly empty or failed";
            }

            std::filesystem::remove(bin_path);
        }
    }

    MemoryEditor hex_view;
    hex_view.Open = true;
    hex_view.ReadOnly = true;
    hex_view.Cols = 16;
    hex_view.OptShowAscii = false;

    TextEditor dis_view;
    dis_view.SetPalette(TextEditor::GetDarkPalette());
    dis_view.SetReadOnly(true);
    dis_view.SetText(shader_dis);

    ShaderCache cache{
        .hex_view = hex_view,
        .dis_view = dis_view,
        .user_data = shader.user_data.user_data,
    };
    shader_decomp.emplace(shader_id, std::move(cache));
}

RegView::RegView() {
    static int unique_id = 0;
    id = unique_id++;

    char name[64];
    snprintf(name, sizeof(name), "BatchView###reg_dump_%d", id);
    Begin(name);
    SetWindowPos({400.0f, 200.0f});

    char dock_name[64];
    snprintf(dock_name, sizeof(dock_name), "BatchView###reg_dump_%d/dock_space", id);
    auto root_dock_id = ImHashStr(dock_name);
    DockSpace(root_dock_id, {}, ImGuiDockNodeFlags_AutoHideTabBar);

    End();

    ImGuiID up1, down1;

    auto center = DockBuilderAddNode(root_dock_id);
    DockBuilderSplitNode(center, ImGuiDir_Up, 0.2f, &up1, &down1);

    snprintf(name, sizeof(name), "User data###reg_dump_%d/user_data", id);
    DockBuilderDockWindow(name, up1);

    snprintf(name, sizeof(name), "Disassembly###reg_dump_%d/disassembly", id);
    DockBuilderDockWindow(name, down1);

    DockBuilderFinish(root_dock_id);
}

void RegView::SetData(DebugStateType::RegDump data) {
    this->data = std::move(data);
    // clear cache
    selected_shader = -1;
    shader_decomp.clear();
}

void RegView::Draw() {

    char name[64];
    snprintf(name, sizeof(name), "BatchView###reg_dump_%d", id);
    if (Begin(name, &open, ImGuiWindowFlags_MenuBar)) {
        const char* names[] = {"vs", "ps", "gs", "es", "hs", "ls"};

        if (BeginMenuBar()) {
            if (BeginMenu("Stage")) {
                for (int i = 0; i < DebugStateType::RegDump::MaxShaderStages; i++) {
                    if (data.regs.stage_enable.IsStageEnabled(i)) {
                        bool selected = selected_shader == i;
                        if (Selectable(names[i], &selected)) {
                            selected_shader = i;
                        }
                    }
                }
                ImGui::EndMenu();
            }
            if (BeginMenu("Windows")) {
                Checkbox("User data", &show_user_data);
                Checkbox("Disassembly", &show_disassembly);
                ImGui::EndMenu();
            }
            EndMenuBar();
        }
        if (selected_shader == -1) {
            Text("Select a stage");
            End();
            return;
        }

        char dock_name[64];
        snprintf(dock_name, sizeof(dock_name), "BatchView###reg_dump_%d/dock_space", id);
        auto root_dock_id = ImHashStr(dock_name);
        DockSpace(root_dock_id, {}, ImGuiDockNodeFlags_AutoHideTabBar);
    }
    End();

    auto shader_cache = shader_decomp.find(selected_shader);
    if (shader_cache == shader_decomp.end()) {
        ProcessShader(selected_shader);
        return;
    }
    auto& shader = shader_cache->second;

    snprintf(name, sizeof(name), "User data###reg_dump_%d/user_data", id);
    if (Begin(name, &show_user_data)) {
        shader.hex_view.DrawContents(shader.user_data.data(), shader.user_data.size());
    }
    End();

    snprintf(name, sizeof(name), "Disassembly###reg_dump_%d/disassembly", id);
    if (Begin(name, &show_disassembly)) {
        shader.dis_view.Render("Disassembly", GetContentRegionAvail());
    }
    End();
}

} // namespace Core::Devtools::Widget