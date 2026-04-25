//  SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#include <fstream>
#include <cmrc/cmrc.hpp>
#include <imgui.h>
#include <stb_image.h>

#include "big_picture.h"
#include "common/logging/log.h"
#include "core/devtools/layer.h"
#include "core/emulator_settings.h"
#include "core/file_format/psf.h"
#include "emulator.h"
#include "imgui/imgui_std.h"
#include "imgui/renderer/font_stack.h"
#include "imgui/renderer/imgui_impl_sdl3_bpm.h"
#include "imgui/renderer/imgui_impl_sdlrenderer3.h"
#include "settings_dialog_imgui.h"

CMRC_DECLARE(res);

namespace BigPictureMode {

const float gameImageSize = 200.f;

static bool done = false;
static bool showSettings = false;

static std::filesystem::path runEbootPath = "";
static std::vector<Game> gameVec = {};

static float uiScale = 1.0f;
static SDL_Renderer* renderer;

void Launch(char* executableName) {
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

    SDL_Window* window =
        SDL_CreateWindow("shadPS4 Big Picture Mode", 1280, 720, SDL_WINDOW_FULLSCREEN);
    renderer = SDL_CreateRenderer(window, nullptr);

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

    ImFontConfig font_cfg;
    font_cfg.OversampleH = 2;
    font_cfg.OversampleV = 1;
    ImFont* myFont = ImGui::FontStack::AddPrimaryUiFont(
        io.Fonts, 32.0f, EmulatorSettings.GetConsoleLanguage(), font_cfg, true);
    io.FontDefault = myFont;

    io.Fonts->Build();

    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);         // black
    colors[ImGuiCol_Header] = ImVec4(0.20f, 0.40f, 0.70f, 1.00f);           // blue
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 0.50f, 0.85f, 1.00f);    // lighter blue
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f); // another light  blue
    colors[ImGuiCol_SliderGrab] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);       // another light  blue

    style.WindowRounding = 0.0f;
    style.FrameRounding = 5.0f * uiScale;
    style.ItemSpacing = ImVec2(10.0f * uiScale, 10.0f * uiScale);
    style.FramePadding = ImVec2(10.0f * uiScale, 10.0f * uiScale);
    style.FrameBorderSize = 2.5f * uiScale;
    style.WindowBorderSize = 0.0f;
    style.WindowPadding = ImVec2(20.0f * uiScale, 20.0f * uiScale);
    style.GrabMinSize = 20.0f * uiScale;

    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);
    GetGameInfo(gameVec, false);

    uiScale = static_cast<float>(EmulatorSettings.GetBigPictureScale() / 1000.f);

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
        ImGui::PushFont(myFont);

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGuiWindowFlags window_flags =
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollWithMouse;

        ImGui::Begin("Game Window", &done, window_flags);
        ImGui::DrawPrettyBackground();
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

        SetGameIcons(gameVec);
        ImGui::EndChild();
        ImGui::Separator();

        ImGui::SetNextItemWidth(300.0f * uiScale);

        static float sliderScale = 1.0f;
        if (ImGui::IsWindowAppearing()) {
            sliderScale = uiScale;
        }
        ImGui::SliderFloat("UI Scale", &sliderScale, 0.25f, 3.0f);

        // Only update when user is not interacting with slider
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            uiScale = sliderScale;
        }

        ImGui::SameLine();

        // Align buttons right
        float buttonsWidth = ImGui::CalcTextSize("Settings").x + ImGui::CalcTextSize("Exit").x +
                             ImGui::GetStyle().FramePadding.x * 4.0f +
                             ImGui::GetStyle().ItemSpacing.x;
        ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - buttonsWidth);

        if (ImGui::Button("Settings")) {
            showSettings = true;
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

        if (showSettings) {
            EmulatorSettings.SetBigPictureScale(static_cast<int>(uiScale * 1000));
            EmulatorSettings.Save();
            DrawSettings(&showSettings);

            // update when settings dialog closed
            if (!showSettings) {
                uiScale = static_cast<float>(EmulatorSettings.GetBigPictureScale() / 1000.f);
                sliderScale = uiScale;
                GetGameInfo(gameVec, false);
            }
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

    if (runEbootPath != "") {
        auto* emulator = Common::Singleton<Core::Emulator>::Instance();
        emulator->executableName = executableName;
        emulator->Run(runEbootPath);
    }
}

void SetGameIcons(std::vector<Game>& games) {
    ImGuiStyle& style = ImGui::GetStyle();
    const float maxAvailableWidth = ImGui::GetContentRegionAvail().x;
    const float itemSpacing = style.ItemSpacing.x; // already scaled
    const float padding = 10.0f * uiScale;
    float rowContentWidth = gameImageSize * uiScale + itemSpacing;

    for (int i = 0; i < games.size(); i++) {
        ImGui::BeginGroup();
        std::string ButtonName = "Button" + std::to_string(i);
        const char* ButtonNameChar = ButtonName.c_str();

        bool isNextItemFocused = (ImGui::GetID(ButtonNameChar) == ImGui::GetFocusID());
        bool popColor = false;
        if (isNextItemFocused) {
            ImGui::PushStyleColor(ImGuiCol_Button,
                                  ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered]);
            popColor = true;
        }

        if (ImGui::ImageButton(ButtonNameChar, (ImTextureID)games[i].iconTexture,
                               ImVec2(gameImageSize * uiScale, gameImageSize * uiScale))) {
            done = true;
            runEbootPath = games[i].ebootPath;
        }

        if (popColor) {
            ImGui::PopStyleColor();
        }

        // Scroll to item only when newly-focused
        if (ImGui::IsItemFocused() && !games[i].focusState) {
            ImGui::SetScrollHereY(0.5f);
        }

        if (ImGui::IsWindowFocused())
            games[i].focusState = ImGui::IsItemFocused();

        ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + gameImageSize * uiScale);
        ImGui::TextWrapped("%s", games[i].title.c_str());
        ImGui::PopTextWrapPos();
        ImGui::EndGroup();

        // Use same line if content fits horizontally, move to next line if not
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

void GetGameInfo(std::vector<Game>& games, bool AddGlobalSettings, SDL_Texture* texture) {
    games.clear();
    if (AddGlobalSettings) {
        Game global;
        global.title = "Global";
        global.iconTexture = texture;
        global.focusState = false;
        games.push_back(global);
    }

    for (const auto& installLoc : EmulatorSettings.GetAllGameInstallDirs()) {
        if (installLoc.enabled && std::filesystem::exists(installLoc.path)) {
            for (const auto& entry : std::filesystem::directory_iterator(installLoc.path)) {
                if (entry.path().filename().string().ends_with("-UPDATE") ||
                    entry.path().filename().string().ends_with("-patch") || !entry.is_directory()) {
                    continue;
                }

                Game game;
                PSF psf;
                const std::string sfoFileName = "param.sfo";
                std::filesystem::path sfoPath = UpdateChecker(sfoFileName, entry.path());

                if (psf.Open(sfoPath)) {
                    if (const auto title = psf.GetString("TITLE"); title.has_value()) {
                        game.title = *title;
                    }

                    if (const auto title_id = psf.GetString("TITLE_ID"); title_id.has_value()) {
                        game.serial = *title_id;
                    }
                } else {
                    continue;
                }

                const std::string iconFileName = "icon0.png";
                std::filesystem::path iconPath = UpdateChecker(iconFileName, entry.path());
                LoadTextureDataFromFile(iconPath, game.iconTexture, renderer);

                game.ebootPath = entry.path() / "eboot.bin";
                game.focusState = false;
                games.push_back(game);
            }
        }
    }

    // Keep global settings at the start if it's added
    auto start = AddGlobalSettings ? games.begin() + 1 : (games.begin());
    std::sort(start, games.end(), [](const Game& a, const Game& b) {
        return a.title < b.title; // Alphabetical order
    });
}

void LoadTextureDataFromFile(std::filesystem::path filePath, SDL_Texture*& texture,
                             SDL_Renderer* m_renderer) {
    std::ifstream file(filePath, std::ios::binary);
    std::vector<char> data =
        std::vector<char>(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    LoadTextureData(data, texture, m_renderer);
}

void LoadTextureData(std::vector<char> data, SDL_Texture*& texture, SDL_Renderer* m_renderer) {
    int image_width = 0;
    int image_height = 0;
    int channels = 4;
    unsigned char* image_data = stbi_load_from_memory(
        (const unsigned char*)data.data(), (int)data.size(), &image_width, &image_height, NULL, 4);
    if (image_data == nullptr) {
        LOG_ERROR(ImGui, "Failed to load image: {}", stbi_failure_reason());
    }

    SDL_Surface* surface = SDL_CreateSurfaceFrom(image_width, image_height, SDL_PIXELFORMAT_RGBA32,
                                                 (void*)image_data, channels * image_width);
    if (surface == nullptr) {
        LOG_ERROR(ImGui, "Unable to create SDL surface: {}", SDL_GetError());
    }

    texture = SDL_CreateTextureFromSurface(m_renderer, surface);
    if (texture == nullptr) {
        LOG_ERROR(ImGui, "Unable to create SDL texture: {}", SDL_GetError());
    }

    SDL_DestroySurface(surface);
    stbi_image_free(image_data);
}

} // namespace BigPictureMode
