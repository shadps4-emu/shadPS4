//  SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include "core/debug_state.h"
#include "imgui_memory_editor.h"
#include "text_editor.h"

namespace Core::Devtools::Widget {

struct ShaderCache {
    MemoryEditor hex_view;
    TextEditor dis_view;
    Vulkan::Liverpool::UserData user_data;
};

class RegView {
    int id;

    DebugStateType::RegDump data;

    std::unordered_map<int, ShaderCache> shader_decomp;
    int selected_shader{-1};
    bool show_user_data{true};
    bool show_disassembly{true};

    void ProcessShader(int shader_id);

public:
    bool open = false;

    RegView();

    void SetData(DebugStateType::RegDump data);

    void Draw();
};

} // namespace Core::Devtools::Widget