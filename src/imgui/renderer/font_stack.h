// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <imgui.h>

namespace ImGui::FontStack {

ImFont* AddPrimaryUiFont(ImFontAtlas* atlas, float font_size, int console_language,
                         const ImFontConfig& base_cfg, bool include_cjk_fallback);

} // namespace ImGui::FontStack
