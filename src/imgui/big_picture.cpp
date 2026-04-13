//  SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#include <SDL3/SDL.h>
#include <imgui.h>

#include "big_picture.h"
#include "common/logging/log.h"
#include "core/devtools/layer.h"
#include "core/file_format/psf.h"
#include "emulator.h"
#include "imgui/renderer/imgui_impl_sdl3_bpm.h"
#include "imgui/renderer/imgui_impl_sdlrenderer3.h"

#include "imgui_fonts/notosansjp_regular.ttf.g.cpp"
#include "imgui_fonts/proggyvector_regular.ttf.g.cpp"

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
        SDL_Quit();
        return;
    }

    if (!SDL_Init(SDL_INIT_GAMEPAD)) {
        LOG_ERROR(ImGui, "SDL_INIT_GAMEPAD Error: {}", SDL_GetError());
        SDL_Quit();
        return;
    }

    window = SDL_CreateWindow("shadPS4 Big Picture Mode", EmulatorSettings.GetWindowWidth(),
                              EmulatorSettings.GetWindowHeight(), SDL_WINDOW_RESIZABLE);
    renderer = SDL_CreateRenderer(window, nullptr);

    if (EmulatorSettings.IsFullScreen()) {
        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
    }

    // Check if window creation failed
    if (window == nullptr) {
        LOG_ERROR(ImGui, "SDL Window Creation Error: {}", SDL_GetError());
        SDL_DestroyRenderer(renderer);
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

    ImFontConfig config;
    config.OversampleH = 3;
    config.OversampleV = 3;
    config.MergeMode = true;

    ImFontConfig config2;
    config.OversampleH = 3;
    config.OversampleV = 3;

    // tm symbol
    static const ImWchar icon_ranges[] = {0x2122, 0x2122, 0x3000, 0x30FF, 0};

    ImFontGlyphRangesBuilder rb{};
    rb.AddRanges(io.Fonts->GetGlyphRangesDefault());
    rb.AddRanges(io.Fonts->GetGlyphRangesGreek());
    rb.AddRanges(io.Fonts->GetGlyphRangesKorean());
    rb.AddRanges(io.Fonts->GetGlyphRangesJapanese());
    rb.AddRanges(io.Fonts->GetGlyphRangesCyrillic());

    ImVector<ImWchar> ranges{};
    rb.BuildRanges(&ranges);

    ImFont* myFont = io.Fonts->AddFontFromMemoryCompressedTTF(
        imgui_font_notosansjp_regular_compressed_data,
        imgui_font_notosansjp_regular_compressed_size, 32.0f, &config2, icon_ranges);

    io.Fonts->AddFontFromMemoryCompressedTTF(imgui_font_notosansjp_regular_compressed_data,
                                             imgui_font_notosansjp_regular_compressed_size, 32.0f,
                                             &config, ranges.Data);

    io.Fonts->AddFontFromMemoryCompressedTTF(imgui_font_proggyvector_regular_compressed_data,
                                             imgui_font_proggyvector_regular_compressed_size, 32.0f,
                                             &config, io.Fonts->GetGlyphRangesDefault());
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);      // black
    colors[ImGuiCol_Header] = ImVec4(0.20f, 0.40f, 0.70f, 1.00f);        // blue
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 0.50f, 0.85f, 1.00f); // lighter blue

    style.WindowRounding = 0.0f;
    style.FrameRounding = 5.0f;
    style.ItemSpacing = ImVec2(10.0f * uiScale, 10.0f * uiScale);
    style.FramePadding = ImVec2(10.0f * uiScale, 10.0f * uiScale);
    style.WindowBorderSize = 0.0f;
    style.WindowPadding = ImVec2(20.0f * uiScale, 20.0f * uiScale);

    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);
    GetGameInfo();

    uiScale = static_cast<float>(EmulatorSettings.GetBigPictureScale() / 1000.f);
    float tempScale = uiScale;

    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT) {
                done = true;
            }
        }

        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration;

        ImGui::PushFont(myFont);
        ImGui::Begin("Game Window", &done, window_flags);
        ImGui::SetWindowFontScale(uiScale);

        ImGuiWindowFlags child_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                       ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NavFlattened;

        if (ImGui::IsWindowAppearing()) {
            ImGui::SetNextWindowFocus();
        }

        ImGui::BeginChild("ContentRegion", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), true,
                          child_flags);
        Overlay::TextCentered("Select Game");
        ImGui::Dummy(ImVec2(0.0f, 10.f * uiScale));

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

        // Align buttons right
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

            if (ImGui::IsWindowAppearing()) {
                ImGui::SetItemDefaultFocus();
            }

            ImGui::EndPopup();
        }

        ImGui::PopFont();
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
    const float maxAvailableWidth = ImGui::GetContentRegionAvail().x;
    const float itemSpacing = style.ItemSpacing.x; // already scaled
    const float padding = 10.0f * uiScale;
    float rowContentWidth = gameImageSize * uiScale + itemSpacing;

    // Use same line if content fits horizontally, move to next line if not
    for (int i = 0; i < gameVec.size(); i++) {
        ImGui::BeginGroup();

        std::string ButtonName = "Button" + std::to_string(i);
        const char* ButtonNameChar = ButtonName.c_str();

        if (ImGui::ImageButton(ButtonNameChar, (ImTextureID)gameVec[i].iconTexture,
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
            ImGui::Dummy(ImVec2(0.0f, padding));
            rowContentWidth = gameImageSize * uiScale + itemSpacing;
        }
    }
}

std::filesystem::path UpdateChecker(const std::string sceItem, std::filesystem::path game_folder) {
    std::filesystem::path outputPath;
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

    return outputPath;
}

void GetGameInfo() {
    gameVec.clear();
    for (const auto& installLoc : EmulatorSettings.GetAllGameInstallDirs()) {
        if (installLoc.enabled && std::filesystem::exists(installLoc.path)) {
            for (const auto& entry : std::filesystem::directory_iterator(installLoc.path)) {
                if (entry.path().filename().string().ends_with("-UPDATE") ||
                    entry.path().filename().string().ends_with("-patch") || !entry.is_directory()) {
                    continue;
                }

                Game game;
                game.ebootPath = entry.path() / "eboot.bin";

                const std::string iconFileName = "icon0.png";
                std::filesystem::path iconPath = UpdateChecker(iconFileName, entry.path());
                game.iconTexture = IMG_LoadTexture(renderer, iconPath.string().c_str());

                PSF psf;
                const std::string sfoFileName = "param.sfo";
                std::filesystem::path sfoPath = UpdateChecker(sfoFileName, entry.path());

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
