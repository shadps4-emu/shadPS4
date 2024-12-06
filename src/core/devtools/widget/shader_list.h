//  SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/debug_state.h"
#include "text_editor.h"

#include <filesystem>

namespace Core::Devtools::Widget {

class ShaderList {
    struct Selection {
        int index;
        TextEditor isa_editor{};
        TextEditor glsl_editor{};
        bool open = true;
        bool loaded_data = false;
        bool showing_bin = false;
        bool showing_patch = false;

        std::filesystem::path patch_path;
        std::filesystem::path patch_bin_path;

        explicit Selection(int index);
        ~Selection();

        bool DrawShader(DebugStateType::ShaderDump& value);
    };

    std::vector<Selection> open_shaders{};

public:
    bool open = false;

    void Draw();
};

} // namespace Core::Devtools::Widget