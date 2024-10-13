//  SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#include <filesystem>
#include <optional>
#include <string>
#include <imgui.h>
#include <magic_enum.hpp>
#include <stdio.h>

#include "common.h"
#include "common/io_file.h"
#include "core/devtools/options.h"
#include "imgui_internal.h"
#include "reg_view.h"

#if defined(_WIN32)
#define popen _popen
#define pclose _pclose
#endif

using namespace ImGui;
using magic_enum::enum_name;

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
void RegView::SelectShader(int id) {
    selected_shader = id;
    if (!shader_decomp.contains(id)) {
        ProcessShader(id);
    }
}

void RegView::DrawRegs() {
    const auto& regs = data.regs;

    if (BeginTable("REGS", 2, ImGuiTableFlags_Borders)) {

        auto& s = regs.screen_scissor;
        DrawRow("Scissor", "(%d, %d, %d, %d)", s.top_left_x, s.top_left_y, s.bottom_right_x,
                s.bottom_right_y);

        auto cc_mode = regs.color_control.mode.Value();
        DrawRow("Color control", "%X (%s)", cc_mode, enum_name(cc_mode).data());

        const auto open_new_popup = [&](int cb, auto... args) {
            if (GetIO().KeyShift) {
                auto& pop = extra_reg_popup.emplace_back();
                pop.SetData(args...);
                pop.open = true;
            } else if (last_selected_cb == cb && default_reg_popup.open) {
                default_reg_popup.open = false;
            } else {
                last_selected_cb = cb;
                default_reg_popup.SetData(args...);
                if (!default_reg_popup.open) {
                    default_reg_popup.open = true;
                    auto popup_pos =
                        GetCurrentContext()->LastItemData.Rect.Max + ImVec2(5.0f, 0.0f);
                    SetNextWindowPos(popup_pos, ImGuiCond_Always);
                    default_reg_popup.Draw();
                }
            }
        };

        for (int cb = 0; cb < AmdGpu::Liverpool::NumColorBuffers; ++cb) {
            PushID(cb);

            TableNextRow();
            TableNextColumn();

            const auto& buffer = regs.color_buffers[cb];

            Text("Color buffer %d", cb);
            TableNextColumn();
            if (!buffer || !regs.color_target_mask.GetMask(cb)) {
                TextUnformatted("N/A");
            } else {
                const char* text = last_selected_cb == cb && default_reg_popup.open ? "x" : "->";
                if (SmallButton(text)) {
                    open_new_popup(cb, buffer, batch_id, cb);
                }
            }

            PopID();
        }

        TableNextRow();
        TableNextColumn();
        TextUnformatted("Depth buffer");
        TableNextColumn();
        if (regs.depth_buffer.Address() == 0 || !regs.depth_control.depth_enable) {
            TextUnformatted("N/A");
        } else {
            constexpr auto depth_id = 0xF3;
            const char* text = last_selected_cb == depth_id && default_reg_popup.open ? "x" : "->";
            if (SmallButton(text)) {
                open_new_popup(depth_id, regs.depth_buffer, regs.depth_control, batch_id);
            }
        }

        EndTable();
    }
}

RegView::RegView() {
    static int unique_id = 0;
    id = unique_id++;

    char name[128];
    snprintf(name, sizeof(name), "BatchView###reg_dump_%d", id);
    SetNextWindowPos({400.0f, 200.0f});
    SetNextWindowSize({450.0f, 500.0f});
    ImGuiID root_dock_id;
    Begin(name);
    {
        char dock_name[64];
        snprintf(dock_name, sizeof(dock_name), "BatchView###reg_dump_%d/dock_space", id);
        root_dock_id = ImHashStr(dock_name);
        DockSpace(root_dock_id);
    }
    End();

    ImGuiID up1, down1;

    DockBuilderRemoveNodeChildNodes(root_dock_id);
    DockBuilderSplitNode(root_dock_id, ImGuiDir_Up, 0.2f, &up1, &down1);

    snprintf(name, sizeof(name), "User data###reg_dump_%d/user_data", id);
    DockBuilderDockWindow(name, up1);

    snprintf(name, sizeof(name), "Regs###reg_dump_%d/regs", id);
    DockBuilderDockWindow(name, down1);

    snprintf(name, sizeof(name), "Disassembly###reg_dump_%d/disassembly", id);
    DockBuilderDockWindow(name, down1);

    DockBuilderFinish(root_dock_id);
}

void RegView::SetData(DebugStateType::RegDump data, u32 batch_id) {
    this->data = std::move(data);
    this->batch_id = batch_id;
    // clear cache
    selected_shader = -1;
    shader_decomp.clear();
    default_reg_popup.open = false;
    extra_reg_popup.clear();
}

void RegView::Draw() {

    char name[128];
    snprintf(name, sizeof(name), "BatchView %u###reg_dump_%d", batch_id, id);
    if (Begin(name, &open, ImGuiWindowFlags_MenuBar)) {
        const char* names[] = {"vs", "ps", "gs", "es", "hs", "ls"};

        if (BeginMenuBar()) {
            if (BeginMenu("Stage")) {
                for (int i = 0; i < DebugStateType::RegDump::MaxShaderStages; i++) {
                    if (data.regs.stage_enable.IsStageEnabled(i)) {
                        bool selected = selected_shader == i;
                        if (Selectable(names[i], &selected)) {
                            SelectShader(i);
                        }
                    }
                }
                ImGui::EndMenu();
            }
            if (BeginMenu("Windows")) {
                Checkbox("Registers", &show_registers);
                Checkbox("User data", &show_user_data);
                Checkbox("Disassembly", &show_disassembly);
                ImGui::EndMenu();
            }
            EndMenuBar();
        }

        char dock_name[64];
        snprintf(dock_name, sizeof(dock_name), "BatchView###reg_dump_%d/dock_space", id);
        auto root_dock_id = ImHashStr(dock_name);
        DockSpace(root_dock_id);
    }
    End();

    auto get_shader = [&]() -> ShaderCache* {
        auto shader_cache = shader_decomp.find(selected_shader);
        if (shader_cache == shader_decomp.end()) {
            return nullptr;
        }
        return &shader_cache->second;
    };

    if (show_user_data) {
        snprintf(name, sizeof(name), "User data###reg_dump_%d/user_data", id);
        if (Begin(name, &show_user_data)) {
            auto shader = get_shader();
            if (!shader) {
                Text("Select a stage");
            } else {
                shader->hex_view.DrawContents(shader->user_data.data(), shader->user_data.size());
            }
        }
        End();
    }

    if (show_disassembly) {
        snprintf(name, sizeof(name), "Disassembly###reg_dump_%d/disassembly", id);
        if (Begin(name, &show_disassembly)) {
            auto shader = get_shader();
            if (!shader) {
                Text("Select a stage");
            } else {
                shader->dis_view.Render("Disassembly", GetContentRegionAvail());
            }
        }
        End();
    }

    if (show_registers) {
        snprintf(name, sizeof(name), "Regs###reg_dump_%d/regs", id);
        if (Begin(name, &show_registers)) {
            DrawRegs();
        }
        End();
    }

    if (default_reg_popup.open) {
        default_reg_popup.Draw();
    }
    for (auto it = extra_reg_popup.begin(); it != extra_reg_popup.end();) {
        if (!it->open) {
            it = extra_reg_popup.erase(it);
            continue;
        }
        it->Draw();
        ++it;
    }
}

} // namespace Core::Devtools::Widget