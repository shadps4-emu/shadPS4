// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <filesystem>
#include <string>
#include <variant>
#include <QObject>
#include <queue>

#include "common/fixed_value.h"
#include "common/types.h"
#include "core/libraries/np_trophy/np_trophy.h"
#include "imgui/imgui_layer.h"
#include "imgui/imgui_texture.h"

namespace Libraries::NpTrophy {

// Ensure that TrophyUI is a QObject-derived class
class TrophyUI final : public QObject, public ImGui::Layer {
Q_OBJECT // This is the necessary macro for Qt's meta-object system
    public : TrophyUI(const std::filesystem::path& trophyIconPath, const std::string& trophyName);
    ~TrophyUI() override;

    void Finish();

    void Draw() override;

private:
    std::string trophy_name;
    float trophy_timer = 5.0f;
    ImGui::RefCountedTexture trophy_icon;
};

struct TrophyInfo {
    std::filesystem::path trophy_icon_path;
    std::string trophy_name;
};

void AddTrophyToQueue(const std::filesystem::path& trophyIconPath, const std::string& trophyName);

}; // namespace Libraries::NpTrophy