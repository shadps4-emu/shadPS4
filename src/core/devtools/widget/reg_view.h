//  SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/debug_state.h"
#include "core/devtools/widget/imgui_memory_editor.h"
#include "core/devtools/widget/reg_popup.h"
#include "core/devtools/widget/text_editor.h"

namespace Core::Devtools::Widget {

struct ShaderCache {
    MemoryEditor hex_view;
    TextEditor dis_view;
    AmdGpu::UserData user_data;
};

class RegView {
    int id;

    std::string title;
    DebugStateType::RegDump data;
    u32 batch_id{~0u};
    ImVec2 last_pos;

    std::unordered_map<int, ShaderCache> shader_decomp;
    int selected_shader{-1};
    RegPopup default_reg_popup;
    int last_selected_cb{-1};
    std::vector<RegPopup> extra_reg_popup;

    bool show_registers{true};
    bool show_user_data{true};
    bool show_disassembly{true};

    void ProcessShader(int shader_id);

    void SelectShader(int shader_id);

    void DrawComputeRegs();

    void DrawGraphicsRegs();

public:
    bool open = false;
    bool moved = false;

    RegView();

    void SetData(DebugStateType::RegDump data, const std::string& base_title, u32 batch_id);

    void SetPos(ImVec2 pos);

    void Draw();
};

} // namespace Core::Devtools::Widget
