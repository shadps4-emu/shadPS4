// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <SDL3/SDL_audio.h>

#include "imgui/imgui_layer.h"
#include "imgui/imgui_texture.h"

namespace Libraries::Np::NpTrophy {

class TrophyUI final : public ImGui::Layer {
public:
    TrophyUI(const std::filesystem::path& iconPath, const std::string& title,
             const std::string& message, const std::string_view& rarity = "");
    ~TrophyUI() override;

    void Finish();

    void Draw() override;

private:
    void PlayMp3(std::span<const u8> mp3Data);
    void PlayWav(std::span<const u8> wavData);

    std::string trophy_title;
    std::string trophy_name;
    std::string_view trophy_type;
    ImGui::RefCountedTexture trophy_icon;
    ImGui::RefCountedTexture default_trophy_icon;
    ImGui::RefCountedTexture trophy_type_icon;
    ImGui::RefCountedTexture notification_bg_icon;
    ImGui::RefCountedTexture ps_button_icon;
    ImGui::RefCountedTexture ps_button_appear_icon;
    bool is_white_bg = false;
    SDL_AudioStream* stream = nullptr;
    SDL_AudioDeviceID audioDevice = 0;
    float elapsed_time = 0.0f;
    double trophy_timer = 5.0;
    std::string side = "left";
};

struct TrophyInfo {
    std::filesystem::path trophy_icon_path;
    std::string title;
    std::string trophy_name;
    std::string_view trophy_type;
};

void AddTrophyToQueue(const std::filesystem::path& trophyIconPath, const std::string& trophyName,
                      const std::string_view& rarity);

}; // namespace Libraries::Np::NpTrophy
