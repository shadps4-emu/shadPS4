//  SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#include "layer.h"

#include "imgui/imgui_std.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <cmath>
#include <numbers>

namespace Core::Devtools {

void Layer::DrawNullGpuNotice() {

    auto* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) {
        return;
    }

    constexpr std::string_view mainNotice = "Null GPU is enabled";
    constexpr std::string_view detailsNotice =
        "Disable the nullGpu config to show the game display";

    auto displaySize = window->Size;

    ImVec2 targetSize = displaySize * 0.7f;

    float minFontSize = 1.0f;
    float maxFontSize = 200.0f;
    float optimalFontSize = minFontSize;

    static auto lastSize = ImVec2(-1, -1);
    static float lastFontSize = -1.0f;

    auto* font = ImGui::GetIO().Fonts->Fonts[IMGUI_FONT_TEXT_BIG];

    if (lastSize != targetSize) {
        while (maxFontSize - minFontSize > 0.1f) {
            float testFontSize = (minFontSize + maxFontSize) / 2.0f;

            ImVec2 textSize = font->CalcTextSizeA(testFontSize, FLT_MAX, 0.0f, &mainNotice.front(),
                                                  &mainNotice.back() + 1);

            if (textSize.x <= targetSize.x && textSize.y <= targetSize.y) {
                optimalFontSize = testFontSize;
                minFontSize = testFontSize;
            } else {
                maxFontSize = testFontSize;
            }
        }
        lastSize = targetSize;
        lastFontSize = optimalFontSize;
    } else {
        optimalFontSize = lastFontSize;
    }
    ImVec2 textSize = font->CalcTextSizeA(optimalFontSize, FLT_MAX, 0.0f, &mainNotice.front(),
                                          &mainNotice.back() + 1);

    ImVec2 textPos = (displaySize - textSize) * 0.5f + window->Pos;

    const float scale = optimalFontSize / font->FontSize;
    double timeAnim = -std::numbers::pi * ImGui::GetTime();
    int i = 0;
    for (auto ch : mainNotice) {
        double colorTime = sin(timeAnim + i * std::numbers::pi / 6.0) / 2.0 + 0.5;
        int color = (int)(200 * colorTime) + 55;

        double posTime = sin(timeAnim + i * std::numbers::pi / 15.0) / 2.0 + 0.5;

        auto pos = textPos;
        pos.y += 10.0 * (posTime < 0.5 ? std::pow(2.0, 20.0 * posTime - 10.0) / 2.0
                                       : (2.0 - std::pow(2.0, -20.0 * posTime + 10.0)) / 2.0);

        window->DrawList->AddText(font, optimalFontSize, pos, IM_COL32(color, color, color, 255),
                                  &ch, &ch + 1);
        textPos.x += font->FindGlyph(ch)->AdvanceX * scale;
        i++;
    }

    font = ImGui::GetIO().Fonts->Fonts[IMGUI_FONT_TEXT];

    textPos.y += textSize.y + 1.0;

    optimalFontSize *= 0.2;
    textSize = font->CalcTextSizeA(optimalFontSize, FLT_MAX, 0.0f, &detailsNotice.front(),
                                   &detailsNotice.back() + 1);
    textPos.x = window->Pos.x + (window->Size.x - textSize.x) * 0.5f;
    window->DrawList->AddText(font, optimalFontSize, textPos, IM_COL32(200, 200, 200, 255),
                              &detailsNotice.front(), &detailsNotice.back() + 1);
}

} // namespace Core::Devtools
