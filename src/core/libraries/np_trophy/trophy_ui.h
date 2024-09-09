// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <variant>

#include "common/fixed_value.h"
#include "common/types.h"
#include "core/libraries/np_trophy/np_trophy.h"
#include "imgui/imgui_layer.h"

namespace Libraries::NpTrophy {

enum TrophyType {
    UNKNOWN,
    PLATINUM,
    GOLD,
    SILVER,
    BRONZE,
};

class TrophyUI final : public ImGui::Layer {
    bool first_render{false};

    int trophyId = -1;
    std::string trophyName;
    std::string trophyDescription;
    TrophyType trophyType;

public:
    explicit TrophyUI(int trophyId, std::string trophyName, TrophyType trophyType);
    TrophyUI();
    ~TrophyUI() override;

    void Finish();

    void Draw() override;

    bool ShouldGrabGamepad() override {
        return false;
    }
};

}; // namespace Libraries::NpTrophy