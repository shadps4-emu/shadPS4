// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

// Based on imgui_impl_sdl3.h from Dear ImGui repository

#pragma once
#include "imgui.h" // IMGUI_IMPL_API
#ifndef IMGUI_DISABLE

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Gamepad;
typedef union SDL_Event SDL_Event;

// Follow "Getting Started" link and check examples/ folder to learn about using backends!
IMGUI_IMPL_API bool ImGui_ImplSDL3_InitForOpenGL(SDL_Window* window, void* sdl_gl_context);
IMGUI_IMPL_API bool ImGui_ImplSDL3_InitForVulkan(SDL_Window* window);
IMGUI_IMPL_API bool ImGui_ImplSDL3_InitForD3D(SDL_Window* window);
IMGUI_IMPL_API bool ImGui_ImplSDL3_InitForMetal(SDL_Window* window);
IMGUI_IMPL_API bool ImGui_ImplSDL3_InitForSDLRenderer(SDL_Window* window, SDL_Renderer* renderer);
IMGUI_IMPL_API bool ImGui_ImplSDL3_InitForSDLGPU(SDL_Window* window);
IMGUI_IMPL_API bool ImGui_ImplSDL3_InitForOther(SDL_Window* window);
IMGUI_IMPL_API void ImGui_ImplSDL3_Shutdown();
IMGUI_IMPL_API void ImGui_ImplSDL3_NewFrame();
IMGUI_IMPL_API bool ImGui_ImplSDL3_ProcessEvent(const SDL_Event* event);

// Gamepad selection automatically starts in AutoFirst mode, picking first available SDL_Gamepad.You
// may override this. When using manual mode, caller is responsible for opening/closing gamepad.
enum ImGui_ImplSDL3_GamepadMode {
    ImGui_ImplSDL3_GamepadMode_AutoFirst,
    ImGui_ImplSDL3_GamepadMode_AutoAll,
    ImGui_ImplSDL3_GamepadMode_Manual
};

IMGUI_IMPL_API void ImGui_ImplSDL3_SetGamepadMode(ImGui_ImplSDL3_GamepadMode mode,
                                                  SDL_Gamepad** manual_gamepads_array = nullptr,
                                                  int manual_gamepads_count = -1);

#endif // #ifndef IMGUI_DISABLE
