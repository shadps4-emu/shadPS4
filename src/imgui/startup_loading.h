// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string_view>
#include <imgui.h>

#include "core/startup_progress.h"

namespace ImGui {

void DrawStartupLoading(ImVec2 position, ImVec2 size, std::string_view game_title,
                        ::Core::Startup::Stage stage, std::int64_t elapsed_ms);

} // namespace ImGui
