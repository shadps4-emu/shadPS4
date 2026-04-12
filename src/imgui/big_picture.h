// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <filesystem>
#include <SDL3_image/SDL_image.h>

namespace BigPictureMode {

struct Game {
    SDL_Texture* iconTexture;
    std::filesystem::path ebootPath;
    std::string title;
};

void Launch();
void SetGameIcons();
void GetGameInfo();
std::filesystem::path UpdateChecker(const std::string sceItem, std::filesystem::path game_folder);

} // namespace BigPictureMode
