// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <variant>
#include <vector>
#include <string>

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

struct TrophyInfo {
    int trophyId = -1;
    std::string trophyName;
    TrophyType trophyType;
};

class TrophyUI final : public ImGui::Layer {
    bool first_render{false};
    std::vector<TrophyInfo> trophyQueue;

public:
    TrophyUI();
    ~TrophyUI() override;

    void AddTrophyToQueue(int trophyId, std::string trophyName, TrophyType trophyType);

    void Finish();

    void Draw() override;

    bool ShouldGrabGamepad() override {
        return false;
    }
};

}; // namespace Libraries::NpTrophy