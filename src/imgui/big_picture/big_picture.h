// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <filesystem>
#include <SDL3/SDL.h>

namespace BigPictureMode {

struct Game {
    SDL_Texture* iconTexture;
    std::filesystem::path ebootPath;
    std::string title;
    std::string serial;
    bool focusState;
};

void Launch(char* executableName);
void SetGameIcons(std::vector<Game>& games);
void GetGameInfo(std::vector<Game>& games, bool AddGlobalSettings, SDL_Texture* texture = {});
std::filesystem::path UpdateChecker(const std::string sceItem, std::filesystem::path game_folder);

void LoadTextureDataFromFile(std::filesystem::path filePath, SDL_Texture*& texture,
                             SDL_Renderer* renderer);
void LoadTextureData(std::vector<char> data, SDL_Texture*& texture, SDL_Renderer* renderer);

} // namespace BigPictureMode
