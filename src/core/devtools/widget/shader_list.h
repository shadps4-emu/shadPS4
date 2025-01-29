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
        Selection(const Selection& other) = delete;
        Selection(Selection&& other) noexcept;
        Selection& operator=(Selection other);

        void ReloadShader(DebugStateType::ShaderDump& value);

        bool DrawShader(DebugStateType::ShaderDump& value);

        int index{-1};
        std::unique_ptr<TextEditor> isa_editor{};
        std::unique_ptr<TextEditor> glsl_editor{};
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