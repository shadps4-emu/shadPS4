//  SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <imgui.h>

#include "common/logging/log.h"
#include "core/devtools/layer.h"
#include "emulator.h"
#include "imgui/renderer/imgui_impl_sdl3_bpm.h"
#include "imgui/renderer/imgui_impl_sdlrenderer3.h"

static bool done = false;
static bool runGame = false;

static float uiScale = 1.0f;
static int scaleSelected = 1;

static SDL_Window* window = nullptr;
static SDL_Renderer* renderer = nullptr;

namespace BigPictureMode {

void Launch() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        LOG_ERROR(ImGui, "SDL_INIT_VIDEO Error: {}", SDL_GetError());
        return;
    }

    if (!SDL_Init(SDL_INIT_GAMEPAD)) {
        LOG_ERROR(ImGui, "SDL_INIT_GAMEPAD Error: {}", SDL_GetError());
        return;
    }

    window = SDL_CreateWindow("Big Picture", 1280, 720, SDL_WINDOW_RESIZABLE);
    renderer = SDL_CreateRenderer(window, nullptr);

    // Check if window creation failed
    if (window == nullptr) {
        LOG_ERROR(ImGui, "SDL_Init Error: {}", SDL_GetError());
        SDL_Quit();
        return;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

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

        colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
        colors[ImGuiCol_Header] = ImVec4(0.20f, 0.40f, 0.70f, 1.00f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 0.50f, 0.85f, 1.00f);

        style.WindowRounding = 0.0f;
        style.FrameRounding = 5.0f;
        style.ItemSpacing = ImVec2(10.0f * uiScale, 10.0f * uiScale);
        style.FramePadding = ImVec2(10.0f * uiScale, 10.0f * uiScale);
        style.WindowBorderSize = 0.0f;
        style.WindowPadding = ImVec2(20.0f * uiScale, 20.0f * uiScale); // Default internal padding

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration;

        ImGui::Begin("Game Window", &done, window_flags);
        ImGui::SetWindowFontScale(1.5f * uiScale);

        ImGuiWindowFlags child_flags =
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;

        if (ImGui::IsWindowAppearing())
            ImGui::SetNextWindowFocus();

        ImGui::BeginChild("ContentRegion", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()),
                          child_flags);

        Overlay::TextCentered("Select Game");
        ImGui::Dummy(ImVec2(0.0f, 20.0f * uiScale));

        ImGui::BeginGroup();
        SDL_Texture* my_texture = IMG_LoadTexture(
            renderer, "D:/Github/shadPS4/Build/Desktop_Qt_6_10_1_MSVC2022_64bit-Release/icon0.png");

        if (ImGui::IsWindowAppearing()) {
            ImGui::SetKeyboardFocusHere();
        }

        if (ImGui::ImageButton("Button1", (ImTextureID)my_texture,
                               ImVec2(200 * uiScale, 200 * uiScale))) {
            runGame = true;
            done = true;
        }

        if (ImGui::IsWindowAppearing()) {
            ImGui::SetNavCursorVisible(true);
        }

        ImGui::TextWrapped("Bloodborne");
        ImGui::EndGroup();

        ImGui::SameLine(0.0f, 20.0f * uiScale);

        ImGui::BeginGroup();
        SDL_Texture* my_texture2 = IMG_LoadTexture(
            renderer, "D:/Github/shadPS4/Build/Desktop_Qt_6_10_1_MSVC2022_64bit-Release/icon0.png");
        if (ImGui::ImageButton("Button2", (ImTextureID)my_texture2,
                               ImVec2(200 * uiScale, 200 * uiScale))) {
            printf("to launch");
        }
        ImGui::TextWrapped("Bloodborne");
        ImGui::EndGroup();

        ImGui::Dummy(ImVec2(0.0f, 20.0f * uiScale));

        ImGui::BeginGroup();
        SDL_Texture* my_texture3 = IMG_LoadTexture(
            renderer, "D:/Github/shadPS4/Build/Desktop_Qt_6_10_1_MSVC2022_64bit-Release/icon0.png");
        if (ImGui::ImageButton("Button3", (ImTextureID)my_texture3,
                               ImVec2(200 * uiScale, 200 * uiScale))) {
            printf("to launch");
        }
        ImGui::TextWrapped("Bloodborne");
        ImGui::EndGroup();

        ImGui::EndChild();

        ImGui::Separator();

        if (ImGui::RadioButton("Small", &scaleSelected, 0)) {
            uiScale = 0.75f;
        }
        ImGui::SameLine();

        if (ImGui::RadioButton("Medium", &scaleSelected, 1)) {
            uiScale = 1.0f;
        }
        ImGui::SameLine();

        if (ImGui::RadioButton("Large", &scaleSelected, 2)) {
            uiScale = 1.25f;
        }

        ImGui::SameLine();

        float buttonsWidth =
            ImGui::CalcTextSize("Settings (Under Construction)").x + ImGui::CalcTextSize("Exit").x +
            ImGui::GetStyle().FramePadding.x * 4.0f + ImGui::GetStyle().ItemSpacing.x;
        ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - buttonsWidth);

        if (ImGui::Button("Settings (Under Construction)")) {
            // Todo
        }

        ImGui::SameLine();

        if (ImGui::Button("Exit")) {
            ImGui::OpenPopup("Confirm Exit");
        }

        // Ensure the popup is centered relative to the viewport
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        if (ImGui::BeginPopupModal("Confirm Exit", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("This will exit shadPS4!\nAre you sure?");
            ImGui::Separator();

            if (ImGui::Button("OK", ImVec2(120 * uiScale, 0))) {
                ImGui::CloseCurrentPopup();
                done = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120 * uiScale, 0))) {
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::IsWindowAppearing())
                ImGui::SetItemDefaultFocus();
            ImGui::EndPopup();
        }

        if (ImGui::IsKeyPressed(ImGuiKey_GamepadL1)) {
            printf("pressed");
        }

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
    SDL_DestroyRenderer(renderer);
    SDL_Quit();

    if (runGame) {
        auto* emulator = Common::Singleton<Core::Emulator>::Instance();
        emulator->Run("D:/Game Install/CUSA03173/eboot.bin");
    }
}

} // namespace BigPictureMode
