//  SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <imgui.h>

#include "common/logging/log.h"
#include "core/devtools/layer.h"
#include "imgui/renderer/imgui_impl_sdl3_bpm.h"
#include "imgui/renderer/imgui_impl_sdlrenderer3.h"

static bool done = false;

namespace Core::Devtools {

void Layer::DrawBigPicture() {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        LOG_ERROR(Common, "SDL_Init Error: {}", SDL_GetError());
        return;
    }

    window = SDL_CreateWindow("Big Picture", 1280, 720, SDL_WINDOW_RESIZABLE);
    renderer = SDL_CreateRenderer(window, nullptr);

    // Check if window creation failed
    if (window == nullptr) {
        LOG_ERROR(Common, "SDL_Init Error: {}", SDL_GetError());
        SDL_Quit();
        return;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
                done = true;
        }

        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        ImGuiStyle& style = ImGui::GetStyle();
        ImVec4* colors = style.Colors;

        // Darker, high-contrast palette
        colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
        colors[ImGuiCol_Header] = ImVec4(0.20f, 0.40f, 0.70f, 1.00f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 0.50f, 0.85f, 1.00f);

        // Large, rounded elements
        style.WindowRounding = 0.0f;
        style.FrameRounding = 5.0f;
        style.ItemSpacing = ImVec2(10.0f, 10.0f);
        style.FramePadding = ImVec2(10.0f, 10.0f);
        style.WindowBorderSize = 0.0f;
        style.WindowPadding = ImVec2(50.0f, 50.0f); // Default internal padding

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGuiWindowFlags window_flags =
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;

        ImGui::Begin("Game Window", nullptr, window_flags);
        ImGui::SetWindowFontScale(2.0f);

        Overlay::TextCentered("Select Game");

        ImGui::BeginGroup();
        SDL_Texture* my_texture = IMG_LoadTexture(
            renderer, "D:/Github/shadPS4/Build/Desktop_Qt_6_10_1_MSVC2022_64bit-Release/icon0.png");
        if (ImGui::ImageButton("Button1", (ImTextureID)my_texture, ImVec2(250, 250))) {
            printf("to launch");
        }
        ImGui::TextWrapped("Bloodborne");
        ImGui::EndGroup();

        ImGui::SameLine(0.0f, 20.0f);

        ImGui::BeginGroup();
        SDL_Texture* my_texture2 = IMG_LoadTexture(
            renderer, "D:/Github/shadPS4/Build/Desktop_Qt_6_10_1_MSVC2022_64bit-Release/icon0.png");
        if (ImGui::ImageButton("Button2", (ImTextureID)my_texture2, ImVec2(250, 250))) {
            printf("to launch");
        }
        ImGui::TextWrapped("Bloodborne");
        ImGui::EndGroup();

        ImGui::Dummy(ImVec2(0.0f, 20.0f));

        ImGui::BeginGroup();
        SDL_Texture* my_texture3 = IMG_LoadTexture(
            renderer, "D:/Github/shadPS4/Build/Desktop_Qt_6_10_1_MSVC2022_64bit-Release/icon0.png");
        if (ImGui::ImageButton("Button3", (ImTextureID)my_texture3, ImVec2(250, 250))) {
            printf("to launch");
        }
        ImGui::TextWrapped("Bloodborne");
        ImGui::EndGroup();

        ImGui::End();

        ImGui::Render();
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
    }

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyWindow(window);
    SDL_Quit();
}

} // namespace Core::Devtools
