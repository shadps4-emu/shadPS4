// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include <variant>
#include <vector>

#include "common/fixed_value.h"
#include "common/types.h"
#include "core/libraries/np_trophy/np_trophy.h"
#include "imgui/imgui_layer.h"

namespace Libraries::NpTrophy {

struct TrophyInfo {
    std::filesystem::path trophy_icon_path;
    std::string trophy_name;
};

class TrophyUI final : public ImGui::Layer {
    std::vector<TrophyInfo> trophy_queue;

public:
    TrophyUI();
    ~TrophyUI() override;

    void AddTrophyToQueue(std::filesystem::path trophyIconPath, std::string trophyName);

    void Finish();

    void Draw() override;
};

}; // namespace Libraries::NpTrophy