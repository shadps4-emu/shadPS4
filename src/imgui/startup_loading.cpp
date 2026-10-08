// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "startup_loading.h"

namespace ImGui {

void DrawStartupLoading(ImVec2 position, ImVec2 size, std::string_view game_title,
                        ::Core::Startup::Stage stage, std::int64_t elapsed_ms) {
    using ::Core::Startup::Stage;
    if (stage == Stage::Inactive || stage == Stage::Complete || size.x < 1 || size.y < 1) {
        return;
    }

    const char* activity = "Preparing the emulator...";
    switch (stage) {
    case Stage::Executable:
        activity = "Loading game executable...";
        break;
    case Stage::Modules:
        activity = "Loading system modules...";
        break;
    case Stage::Launch:
        activity = "Waiting for the launcher...";
        break;
    case Stage::FirstFrame:
        activity = "Waiting for game video...";
        break;
    default:
        break;
    }

    const float scale = std::min({size.x / 1280.0f, size.y / 720.0f, 2.0f});
    const ImVec2 panel_size{760.0f * scale, 180.0f * scale};
    const ImVec2 origin{position.x + (size.x - panel_size.x) * 0.5f,
                        position.y + (size.y - panel_size.y) * 0.5f};
    auto* draw = GetWindowDrawList();
    draw->AddRectFilled(origin, {origin.x + panel_size.x, origin.y + panel_size.y},
                        IM_COL32(16, 20, 29, 240), 14.0f * scale);

    const ImVec2 spinner{origin.x + 42.0f * scale, origin.y + 45.0f * scale};
    const float rotation = static_cast<float>(std::fmod(GetTime() * 4.0, 6.283185307));
    for (int dot = 0; dot < 10; ++dot) {
        const float angle = rotation + static_cast<float>(dot) * 0.628318531f;
        draw->AddCircleFilled({spinner.x + std::cos(angle) * 15.0f * scale,
                               spinner.y + std::sin(angle) * 15.0f * scale},
                              2.8f * scale, IM_COL32(116, 177, 255, 45 + dot * 21));
    }

    auto* font = GetFont();
    const float left = origin.x + 78.0f * scale;
    draw->AddText(font, 28.0f * scale, {left, origin.y + 27.0f * scale},
                  IM_COL32(245, 247, 252, 255), "Loading game...");

    if (game_title.empty()) {
        game_title = "shadPS4";
    }
    float title_size = 22.0f * scale;
    const auto title_width = font->CalcTextSizeA(title_size, 100000.0f, 0.0f, game_title.data(),
                                                 game_title.data() + game_title.size())
                                 .x;
    const float available = panel_size.x - 106.0f * scale;
    if (title_width > available) {
        title_size *= available / title_width;
    }
    draw->AddText(font, title_size, {left, origin.y + 66.0f * scale}, IM_COL32(225, 230, 240, 255),
                  game_title.data(), game_title.data() + game_title.size());
    draw->AddText(font, 20.0f * scale, {left, origin.y + 101.0f * scale},
                  IM_COL32(183, 193, 211, 255), activity);

    char elapsed[96];
    const auto seconds = std::max<std::int64_t>(0, elapsed_ms) / 1000;
    std::snprintf(elapsed, sizeof(elapsed), "%lld:%02lld elapsed  |  %s",
                  static_cast<long long>(seconds / 60), static_cast<long long>(seconds % 60),
                  seconds < 30 ? "Please wait" : "Still waiting for game video");
    draw->AddText(font, 18.0f * scale, {left, origin.y + 136.0f * scale},
                  IM_COL32(148, 162, 184, 255), elapsed);
}

} // namespace ImGui
