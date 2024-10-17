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
#include "imgui/imgui_std.h"
#include "imgui_internal.h"
#include "reg_view.h"

#if defined(_WIN32)
#define popen _popen
#define pclose _pclose
#endif

using namespace ImGui;
using magic_enum::enum_name;

constexpr auto depth_id = 0xF3;

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
    std::vector<u32> shader_code;
    Vulkan::Liverpool::UserData user_data;
    if (data.is_compute) {
        shader_code = data.cs_data.code;
        user_data = data.cs_data.cs_program.user_data;
    } else {
        const auto& s = data.stages[shader_id];
        shader_code = s.code;
        user_data = s.user_data.user_data;
    }

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
            file.Write(shader_code);
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
    hex_view.Cols = 8;
    hex_view.OptShowAscii = false;
    hex_view.OptShowOptions = false;

    TextEditor dis_view;
    dis_view.SetPalette(TextEditor::GetDarkPalette());
    dis_view.SetReadOnly(true);
    dis_view.SetText(shader_dis);

    ShaderCache cache{
        .hex_view = hex_view,
        .dis_view = dis_view,
        .user_data = user_data,
    };
    shader_decomp.emplace(shader_id, std::move(cache));
}
void RegView::SelectShader(int id) {
    selected_shader = id;
    if (!shader_decomp.contains(id)) {
        ProcessShader(id);
    }
}

void RegView::DrawComputeRegs() {
    const auto& cs = data.cs_data.cs_program;

    if (BeginTable("CREGS", 2, ImGuiTableFlags_Borders)) {
        TableNextRow();

        // clang-format off
        DrawValueRowList(
            "DISPATCH_INITIATOR",     cs.dispatch_initiator,
            "DIM_X",                  cs.dim_x,
            "DIM_Y",                  cs.dim_y,
            "DIM_Z",                  cs.dim_z,
            "START_X",                cs.start_x,
            "START_Y",                cs.start_y,
            "START_Z",                cs.start_z,
            "NUM_THREAD_X.FULL",      cs.num_thread_x.full,
            "NUM_THREAD_X.PARTIAL",   cs.num_thread_x.partial,
            "NUM_THREAD_Y.FULL",      cs.num_thread_y.full,
            "NUM_THREAD_Y.PARTIAL",   cs.num_thread_y.partial,
            "NUM_THREAD_Z.FULL",      cs.num_thread_z.full,
            "NUM_THREAD_Z.PARTIAL",   cs.num_thread_z.partial,
            "MAX_WAVE_ID",            cs.max_wave_id,
            "SETTINGS.NUM_VGPRS",     cs.settings.num_vgprs,
            "SETTINGS.NUM_SGPRS",     cs.settings.num_sgprs,
            "SETTINGS.NUM_USER_REGS", cs.settings.num_user_regs,
            "SETTINGS.TGID_ENABLE",   cs.settings.tgid_enable,
            "SETTINGS.LDS_DWORDS",    cs.settings.lds_dwords,
            "RESOURCE_LIMITS",        cs.resource_limits
        );
        // clang-format on

        EndTable();
    }
}

void RegView::DrawGraphicsRegs() {
    const auto& regs = data.regs;

    if (BeginTable("REGS", 2, ImGuiTableFlags_Borders)) {
        TableNextRow();

        DrawValueRow("Primitive type", regs.primitive_type);

        const auto open_new_popup = [&](int cb, auto... args) {
            const auto pos = GetItemRectMax() + ImVec2(5.0f, 0.0f);
            if (GetIO().KeyShift) {
                auto& pop = extra_reg_popup.emplace_back();
                pop.SetData(title, args...);
                pop.open = true;
                pop.SetPos(pos, true);
            } else if (last_selected_cb == cb && default_reg_popup.open) {
                default_reg_popup.open = false;
            } else {
                last_selected_cb = cb;
                default_reg_popup.SetData(title, args...);
                if (!default_reg_popup.open || !default_reg_popup.moved) {
                    default_reg_popup.open = true;
                    default_reg_popup.SetPos(pos, true);
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
                    open_new_popup(cb, buffer, cb);
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
            const char* text = last_selected_cb == depth_id && default_reg_popup.open ? "x" : "->";
            if (SmallButton(text)) {
                open_new_popup(depth_id, regs.depth_buffer, regs.depth_control);
            }
        }

        auto& s = regs.screen_scissor;
        DrawRow("Scissor", "(%d, %d, %d, %d)", s.top_left_x, s.top_left_y, s.bottom_right_x,
                s.bottom_right_y);

        DrawValueRow("Color control", regs.color_control.mode);

        DrawRow("Primitive restart", "%X (IDX: %X)", regs.enable_primitive_restart & 1,
                regs.primitive_restart_index);
        // clang-format off
        DrawValueRowList(
            "Polygon mode", regs.polygon_control.PolyMode(),
            "Cull mode",    regs.polygon_control.CullingMode(),
            "Clip Space",   regs.clipper_control.clip_space,
            "Front face",   regs.polygon_control.front_face,
            "Num Samples",  regs.aa_config.NumSamples()
        );
        // clang-format on

        EndTable();
    }
}

RegView::RegView() {
    static int unique_id = 0;
    id = unique_id++;

    char name[128];
    snprintf(name, sizeof(name), "###reg_dump_%d", id);
    SetNextWindowPos({400.0f, 200.0f});
    SetNextWindowSize({290.0f, 435.0f});
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
    DockBuilderSplitNode(root_dock_id, ImGuiDir_Up, 0.19f, &up1, &down1);

    snprintf(name, sizeof(name), "User data###reg_dump_%d/user_data", id);
    DockBuilderDockWindow(name, up1);

    snprintf(name, sizeof(name), "Regs###reg_dump_%d/regs", id);
    DockBuilderDockWindow(name, down1);

    snprintf(name, sizeof(name), "Disassembly###reg_dump_%d/disassembly", id);
    DockBuilderDockWindow(name, down1);

    DockBuilderFinish(root_dock_id);
}

void RegView::SetData(DebugStateType::RegDump _data, const std::string& base_title, u32 batch_id) {
    this->data = std::move(_data);
    this->batch_id = batch_id;
    this->title = fmt::format("{}/Batch {}", base_title, batch_id);
    // clear cache
    shader_decomp.clear();
    if (data.is_compute) {
        selected_shader = -2;
        last_selected_cb = -1;
        default_reg_popup.open = false;
        ProcessShader(-2);
    } else {
        const auto& regs = data.regs;
        if (selected_shader >= 0 && !regs.stage_enable.IsStageEnabled(selected_shader)) {
            selected_shader = -1;
        }
        if (default_reg_popup.open) {
            default_reg_popup.open = false;
            if (last_selected_cb == depth_id) {
                const auto& has_depth =
                    regs.depth_buffer.Address() != 0 && regs.depth_control.depth_enable;
                if (has_depth) {
                    default_reg_popup.SetData(title, regs.depth_buffer, regs.depth_control);
                    default_reg_popup.open = true;
                }
            } else if (last_selected_cb >= 0 &&
                       last_selected_cb < AmdGpu::Liverpool::NumColorBuffers) {
                const auto& buffer = regs.color_buffers[last_selected_cb];
                const bool has_cb = buffer && regs.color_target_mask.GetMask(last_selected_cb);
                if (has_cb) {
                    default_reg_popup.SetData(title, buffer, last_selected_cb);
                    default_reg_popup.open = true;
                }
            }
        }
    }
    extra_reg_popup.clear();
}

void RegView::SetPos(ImVec2 pos) {
    char name[128];
    snprintf(name, sizeof(name), "%s###reg_dump_%d", title.c_str(), id);
    Begin(name, &open, ImGuiWindowFlags_MenuBar);
    SetWindowPos(pos);
    KeepWindowInside();
    last_pos = GetWindowPos();
    moved = false;
    End();
}

void RegView::Draw() {
    char name[128];
    snprintf(name, sizeof(name), "%s###reg_dump_%d", title.c_str(), id);

    if (Begin(name, &open, ImGuiWindowFlags_MenuBar)) {
        if (GetWindowPos() != last_pos) {
            moved = true;
        }

        const char* names[] = {"vs", "ps", "gs", "es", "hs", "ls"};

        if (BeginMenuBar()) {
            if (BeginMenu("Windows")) {
                Checkbox("Registers", &show_registers);
                Checkbox("User data", &show_user_data);
                Checkbox("Disassembly", &show_disassembly);
                ImGui::EndMenu();
            }
            EndMenuBar();
        }

        if (!data.is_compute &&
            BeginChild("STAGES", {},
                       ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeY)) {
            for (int i = 0; i < DebugStateType::RegDump::MaxShaderStages; i++) {
                if (data.regs.stage_enable.IsStageEnabled(i)) {
                    const bool selected = selected_shader == i;
                    if (selected) {
                        PushStyleColor(ImGuiCol_Button, ImVec4{1.0f, 0.7f, 0.7f, 1.0f});
                    }
                    if (Button(names[i], {40.0f, 40.0f})) {
                        SelectShader(i);
                    }
                    if (selected) {
                        PopStyleColor();
                    }
                }
                SameLine();
            }
            EndChild();
        }
    }
    char dock_name[64];
    snprintf(dock_name, sizeof(dock_name), "BatchView###reg_dump_%d/dock_space", id);
    auto root_dock_id = ImHashStr(dock_name);
    DockSpace(root_dock_id);
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

        if (Begin(name, &show_user_data, ImGuiWindowFlags_NoScrollbar)) {
            auto shader = get_shader();
            if (!shader) {
                Text("Stage not selected");
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
                Text("Stage not selected");
            } else {
                shader->dis_view.Render("Disassembly", GetContentRegionAvail());
            }
        }
        End();
    }

    if (show_registers) {
        snprintf(name, sizeof(name), "Regs###reg_dump_%d/regs", id);
        if (Begin(name, &show_registers)) {
            if (data.is_compute) {
                DrawComputeRegs();
            } else {
                DrawGraphicsRegs();
            }
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