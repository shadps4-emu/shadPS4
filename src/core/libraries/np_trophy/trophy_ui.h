// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include <variant>
#include <queue>

#include "common/fixed_value.h"
#include "common/types.h"
#include "core/libraries/np_trophy/np_trophy.h"
#include "imgui/imgui_layer.h"
#include "imgui/imgui_texture.h"

namespace Libraries::NpTrophy {

class TrophyUI final : public ImGui::Layer {
public:
    TrophyUI(const std::filesystem::path& trophyIconPath, const std::string& trophyName,
             const std::string_view& rarity);
    ~TrophyUI() override;

    void Finish();

    void Draw() override;

private:
    std::string trophy_name;
    std::string_view trophy_type;
    ImGui::RefCountedTexture trophy_icon;
    ImGui::RefCountedTexture trophy_type_icon;
};

struct TrophyInfo {
    std::filesystem::path trophy_icon_path;
    std::string trophy_name;
    std::string_view trophy_type;
};

void AddTrophyToQueue(const std::filesystem::path& trophyIconPath, const std::string& trophyName,
                      const std::string_view& rarity);

}; // namespace Libraries::NpTrophy
