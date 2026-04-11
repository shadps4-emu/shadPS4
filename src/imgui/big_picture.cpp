//  SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <imgui.h>

#include "big_picture.h"
#include "common/logging/log.h"
#include "core/devtools/layer.h"
#include "core/file_format/psf.h"
#include "emulator.h"
#include "imgui/renderer/imgui_impl_sdl3_bpm.h"
#include "imgui/renderer/imgui_impl_sdlrenderer3.h"

namespace BigPictureMode {

const float gameImageSize = 200.f;

static bool done = false;
static bool runGame = false;
static std::filesystem::path runEbootPath = "";
static std::vector<Game> gameVec = {};
static std::vector<bool> focusState = {};

static float uiScale = 1.0f;
static int scaleSelected = 1;

static SDL_Window* window = nullptr;
static SDL_Renderer* renderer = nullptr;

void Launch() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        LOG_ERROR(ImGui, "SDL_INIT_VIDEO Error: {}", SDL_GetError());
        return;
    }

    if (!SDL_Init(SDL_INIT_GAMEPAD)) {
        LOG_ERROR(ImGui, "SDL_INIT_GAMEPAD Error: {}", SDL_GetError());
        return;
    }

    window = SDL_CreateWindow("Big Picture", EmulatorSettings.GetWindowWidth(),
                              EmulatorSettings.GetWindowHeight(), SDL_WINDOW_RESIZABLE);
    renderer = SDL_CreateRenderer(window, nullptr);

    if (EmulatorSettings.IsFullScreen()) {
        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
    }

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
    io.ConfigNavCursorVisibleAlways = true;

    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    GetGameInfo();

    uiScale = static_cast<float>(EmulatorSettings.GetBigPictureScale() / 1000.f);
    float tempScale = uiScale;

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

        if (ImGui::IsWindowAppearing()) {
            ImGui::SetKeyboardFocusHere();
        }

        SetGameIcons();

        ImGui::EndChild();
        ImGui::Separator();

        ImGui::SetNextItemWidth(300.0f * uiScale);
        if (ImGui::SliderFloat("UI Scale", &tempScale, 0.25f, 3.0f)) {
            // Dynamically changes UI scale
        }

        // Only update when user is not interacting with slider
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            uiScale = tempScale;
            tempScale = uiScale;
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

    EmulatorSettings.SetBigPictureScale(static_cast<int>(uiScale * 1000));
    EmulatorSettings.Save();

    if (runGame) {
        auto* emulator = Common::Singleton<Core::Emulator>::Instance();
        emulator->Run(runEbootPath);
    }
}

void SetGameIcons() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImGuiIO& io = ImGui::GetIO();
    const float maxAvailableWidth = ImGui::GetContentRegionAvail().x;
    const float itemSpacing = style.ItemSpacing.x; // already scaled
    const float padding = 20.0f * uiScale;
    float rowContentWidth = gameImageSize * uiScale + itemSpacing;

    for (int i = 0; i < gameVec.size(); i++) {
        ImGui::BeginGroup();
        SDL_Texture* my_texture = IMG_LoadTexture(renderer, gameVec[i].iconPath.string().c_str());

        std::string ButtonName = "Button" + std::to_string(i);
        const char* ButtonNameChar = ButtonName.c_str();

        if (ImGui::ImageButton(ButtonNameChar, (ImTextureID)my_texture,
                               ImVec2(gameImageSize * uiScale, gameImageSize * uiScale))) {
            runGame = true;
            done = true;
            runEbootPath = gameVec[i].ebootPath;
        }

        // Scroll to item only when newly-focused
        if (ImGui::IsItemFocused() && !focusState[i]) {
            ImGui::SetScrollHereY(0.5f);
        }
        focusState[i] = ImGui::IsItemFocused();

        ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + gameImageSize * uiScale);
        ImGui::TextWrapped("%s", gameVec[i].title.c_str());
        ImGui::PopTextWrapPos();
        ImGui::EndGroup();

        rowContentWidth += (gameImageSize * uiScale + itemSpacing * 2 + padding);
        if (rowContentWidth < maxAvailableWidth) {
            ImGui::SameLine(0.0f, padding);
        } else {
            rowContentWidth = gameImageSize * uiScale + itemSpacing;
        }
    }
}

void SceUpdateChecker(const std::string sceItem, std::filesystem::path& outputPath,
                      std::filesystem::path game_folder) {
    auto update_folder = game_folder;
    update_folder += "-UPDATE";

    auto patch_folder = game_folder;
    patch_folder += "-patch";

    if (std::filesystem::exists(update_folder / "sce_sys" / sceItem)) {
        outputPath = update_folder / "sce_sys" / sceItem;
    } else if (std::filesystem::exists(patch_folder / "sce_sys" / sceItem)) {
        outputPath = patch_folder / "sce_sys" / sceItem;
    } else {
        outputPath = game_folder / "sce_sys" / sceItem;
    }
}

void GetGameInfo() {
    gameVec.clear();

    for (const auto& installLoc : EmulatorSettings.GetAllGameInstallDirs()) {
        if (installLoc.enabled) {
            for (const auto& entry : std::filesystem::directory_iterator(installLoc.path)) {
                if (entry.path().filename().string().ends_with("-UPDATE") ||
                    entry.path().filename().string().ends_with("-patch") || !entry.is_directory()) {
                    continue;
                }

                Game game;
                game.ebootPath = entry.path() / "eboot.bin";

                const std::string iconFileName = "icon0.png";
                SceUpdateChecker(iconFileName, game.iconPath, entry.path());

                PSF psf;
                const std::string sfoFileName = "param.sfo";
                std::filesystem::path sfoPath;
                SceUpdateChecker(sfoFileName, sfoPath, entry.path());

                if (psf.Open(sfoPath)) {
                    if (const auto title = psf.GetString("TITLE"); title.has_value()) {
                        game.title = *title;
                    }
                } else {
                    continue;
                }

                gameVec.push_back(game);
                focusState.push_back(false);
            }
        }
    }
}

} // namespace BigPictureMode
