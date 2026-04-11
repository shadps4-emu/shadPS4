// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

// Based on imgui_impl_sdlrenderer3.h from Dear ImGui repository

#pragma once
#include "imgui.h" // IMGUI_IMPL_API
#ifndef IMGUI_DISABLE

struct SDL_Renderer;

// Follow "Getting Started" link and check examples/ folder to learn about using backends!
IMGUI_IMPL_API bool ImGui_ImplSDLRenderer3_Init(SDL_Renderer* renderer);
IMGUI_IMPL_API void ImGui_ImplSDLRenderer3_Shutdown();
IMGUI_IMPL_API void ImGui_ImplSDLRenderer3_NewFrame();
IMGUI_IMPL_API void ImGui_ImplSDLRenderer3_RenderDrawData(ImDrawData* draw_data,
                                                          SDL_Renderer* renderer);

// Called by Init/NewFrame/Shutdown
IMGUI_IMPL_API bool ImGui_ImplSDLRenderer3_CreateFontsTexture();
IMGUI_IMPL_API void ImGui_ImplSDLRenderer3_DestroyFontsTexture();
IMGUI_IMPL_API bool ImGui_ImplSDLRenderer3_CreateDeviceObjects();
IMGUI_IMPL_API void ImGui_ImplSDLRenderer3_DestroyDeviceObjects();

// [BETA] Selected render state data shared with callbacks.
// This is temporarily stored in GetPlatformIO().Renderer_RenderState during the
// ImGui_ImplSDLRenderer3_RenderDrawData() call. (Please open an issue if you feel you need access
// to more data)
struct ImGui_ImplSDLRenderer3_RenderState {
    SDL_Renderer* Renderer;
};

#endif // #ifndef IMGUI_DISABLE
