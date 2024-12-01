//  SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/debug_state.h"
#include "text_editor.h"

namespace Core::Devtools::Widget {

class ShaderList {
    int selected_shader = -1;
    TextEditor isa_editor{};
    TextEditor spv_editor{};
    bool loaded_data = false;
    bool showing_isa = false;

    void DrawShader(DebugStateType::ShaderDump& value);

public:
    ShaderList();

    bool open = false;

    void Draw();
};

} // namespace Core::Devtools::Widget