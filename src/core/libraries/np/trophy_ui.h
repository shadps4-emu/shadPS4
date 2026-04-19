// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include <SDL3/SDL_audio.h>

#include "imgui/imgui_layer.h"
#include "imgui/imgui_texture.h"

namespace Libraries::Np::NpTrophy {

class TrophyUI final : public ImGui::Layer {
public:
    TrophyUI(const std::filesystem::path& trophyIconPath, const std::string& trophyName,
             const std::string_view& rarity);
    ~TrophyUI() override;

    void Finish();

    void Draw() override;

private:
    void PlayMp3(std::vector<unsigned char> mp3Data);
    void PlayWav(std::vector<unsigned char> wavData);

    std::string trophy_name;
    std::string_view trophy_type;
    ImGui::RefCountedTexture trophy_icon;
    ImGui::RefCountedTexture trophy_type_icon;
    SDL_AudioStream* stream;
    SDL_AudioDeviceID audioDevice;
};

struct TrophyInfo {
    std::filesystem::path trophy_icon_path;
    std::string trophy_name;
    std::string_view trophy_type;
};

void AddTrophyToQueue(const std::filesystem::path& trophyIconPath, const std::string& trophyName,
                      const std::string_view& rarity);

}; // namespace Libraries::Np::NpTrophy
