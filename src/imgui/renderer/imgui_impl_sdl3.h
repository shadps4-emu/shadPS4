// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

// Based on imgui_impl_sdl3.h from Dear ImGui repository

#pragma once

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Gamepad;
typedef union SDL_Event SDL_Event;

namespace ImGui::Sdl {

bool Init(SDL_Window* window);
void Shutdown();
void NewFrame();
bool ProcessEvent(const SDL_Event* event);
void OnResize();

// Gamepad selection automatically starts in AutoFirst mode, picking first available SDL_Gamepad.
// You may override this. When using manual mode, caller is responsible for opening/closing gamepad.
enum GamepadMode {
    ImGui_ImplSDL3_GamepadMode_AutoFirst,
    ImGui_ImplSDL3_GamepadMode_AutoAll,
    ImGui_ImplSDL3_GamepadMode_Manual
};
void SetGamepadMode(GamepadMode mode, SDL_Gamepad** manual_gamepads_array = NULL,
                    int manual_gamepads_count = -1);

}; // namespace ImGui::Sdl
