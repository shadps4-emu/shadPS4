//  SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/debug_state.h"
#include "text_editor.h"

#include <filesystem>

namespace Core::Devtools::Widget {

class ShaderList {
    struct Selection {
        explicit Selection(int index);
        ~Selection();

        void ReloadShader(DebugStateType::ShaderDump& value);

        bool DrawShader(DebugStateType::ShaderDump& value);

        int index;
        TextEditor isa_editor{};
        TextEditor glsl_editor{};
        bool open = true;
        bool showing_bin = false;

        std::filesystem::path patch_path;
        std::filesystem::path patch_bin_path;
    };

    std::vector<Selection> open_shaders{};

    char search_box[128]{};

public:
    bool open = false;

    void Draw();
};

} // namespace Core::Devtools::Widget