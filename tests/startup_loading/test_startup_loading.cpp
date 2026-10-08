// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string_view>
#include <thread>
#include <vector>

#include "imgui/startup_loading.h"

void assert_fail_debug_msg(const char* message) {
    std::fprintf(stderr, "%s\n", message);
    std::abort();
}

static void Check(bool condition, const char* message) {
    if (!condition) {
        assert_fail_debug_msg(message);
    }
}

using Core::Startup::Progress;
using Core::Startup::Stage;

static void BlackFrames() {
    Progress progress;
    progress.Begin();
    std::vector<std::uint8_t> pixels(1920 * 1080 * 4);
    for (std::size_t i = 3; i < pixels.size(); i += 4) {
        pixels[i] = 255;
    }
    for (int frame = 0; frame < 40; ++frame) {
        Check(!progress.Presented(Core::Startup::HasVisibleRgb8Content(pixels), true),
              "successfully presented opaque black game buffers must keep the indicator");
    }
    Check(progress.IsActive(), "ten seconds of black readbacks must not expire the indicator");
    const std::array<std::uint8_t, 8> near_black{1, 2, 4, 255, 4, 3, 1, 128};
    Check(!Core::Startup::HasVisibleRgb8Content(near_black),
          "alpha and near-black rounding noise must not count as a visible picture");
    Check(!Core::Startup::HasVisibleRgb8Content({}), "empty readback is not visible content");
    const std::array<std::uint8_t, 3> incomplete{255, 255, 255};
    Check(!Core::Startup::HasVisibleRgb8Content(incomplete),
          "an incomplete pixel must not be read past the buffer");
    for (const auto position : {std::size_t{0}, pixels.size() / 2, pixels.size() - 4}) {
        for (int channel = 0; channel < 3; ++channel) {
            pixels[position + channel] = 32;
            Check(Core::Startup::HasVisibleRgb8Content(pixels),
                  "small colored content must be found even at the image corners");
            pixels[position + channel] = 0;
        }
    }
    pixels[pixels.size() - 4] = 255;
    const bool visible = Core::Startup::HasVisibleRgb8Content(pixels);
    Check(!progress.Presented(visible, false), "a failed visible presentation must not finish");
    Check(progress.Presented(visible, true), "visible game content must finish startup");
    pixels[pixels.size() - 4] = 0;
    Check(!progress.Presented(Core::Startup::HasVisibleRgb8Content(pixels), true),
          "later black game frames must not restart the indicator");
    Check(!progress.IsActive(), "the completed indicator must stay off");
}

static void Lifecycle() {
    Progress progress;
    const auto start = Progress::Clock::time_point{};
    Check(!progress.IsActive(), "startup must be opt-in for game launch");
    Check(!progress.Presented(true, true), "an unrelated presenter must not complete startup");
    progress.Begin(start);
    Check(progress.IsActive(), "startup must remain visible before video-out opens");
    progress.SetStage(Stage::FirstFrame);
    Check(!progress.Presented(false, true), "host blank frames must not finish startup");
    Check(!progress.Presented(true, false), "failed presentation must not finish startup");
    Check(progress.IsActive(), "keep drawing while the game has no displayed frame");
    Check(progress.ElapsedMs(start + std::chrono::seconds{20}) == 20000,
          "elapsed time must not depend on a game's frame rate");
    Check(progress.Presented(true, true), "visible game content completes startup");
    Check(!progress.Presented(true, true), "completion must be reported only once");
    progress.SetStage(Stage::Modules);
    Check(!progress.IsActive(), "late loader updates must not cover gameplay");
    Check(!progress.Presented(false, true), "later black transitions must not rearm startup");

    for (int attempt = 0; attempt < 100; ++attempt) {
        progress.Begin(start);
        std::thread loader([&] {
            for (int i = 0; i < 100; ++i) {
                progress.SetStage(Stage::Modules);
            }
        });
        progress.Presented(true, true);
        loader.join();
        Check(!progress.IsActive(), "completion must win over concurrent loader updates");
    }
}

static std::vector<ImVec2> Draw(Stage stage, ImVec2 size, const char* title, int elapsed_ms) {
    auto& io = ImGui::GetIO();
    io.DisplaySize = size;
    ImGui::NewFrame();
    ImGui::SetNextWindowPos({0, 0});
    ImGui::SetNextWindowSize(size);
    ImGui::Begin("Display", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
                     ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoSavedSettings);
    auto* draw = ImGui::GetWindowDrawList();
    const int before = draw->VtxBuffer.Size;
    ImGui::DrawStartupLoading({0, 0}, size, title, stage, elapsed_ms);
    std::vector<ImVec2> result;
    for (int i = before; i < draw->VtxBuffer.Size; ++i) {
        const auto pos = draw->VtxBuffer[i].pos;
        Check(std::isfinite(pos.x) && std::isfinite(pos.y), "UI geometry must be finite");
        Check(pos.x >= 0 && pos.y >= 0 && pos.x <= size.x && pos.y <= size.y,
              "loading panel must fit the game display");
        result.push_back(pos);
    }
    if (!result.empty()) {
        ImVec2 minimum = result.front();
        ImVec2 maximum = minimum;
        for (const auto& pos : result) {
            minimum.x = std::min(minimum.x, pos.x);
            minimum.y = std::min(minimum.y, pos.y);
            maximum.x = std::max(maximum.x, pos.x);
            maximum.y = std::max(maximum.y, pos.y);
        }
        Check(std::abs((minimum.x + maximum.x) * 0.5f - size.x * 0.5f) < 1.0f &&
                  std::abs((minimum.y + maximum.y) * 0.5f - size.y * 0.5f) < 1.0f,
              "indicator must be centered horizontally and vertically");
    }
    ImGui::End();
    ImGui::Render();
    Check(!io.WantCaptureKeyboard && !io.WantCaptureMouse,
          "loading indicator must not capture game or quit-dialog input");
    return result;
}

static void Ui() {
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.DeltaTime = 1.0f / 60.0f;
    io.Fonts->AddFontDefault();
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    Check(Draw(Stage::Inactive, {1280, 720}, "Game", 0).empty(), "inactive UI must be absent");
    const auto first = Draw(Stage::FirstFrame, {1280, 720}, "The Last Guardian", 10000);
    const auto second = Draw(Stage::FirstFrame, {1280, 720}, "The Last Guardian", 10000);
    Check(!first.empty() && first.size() == second.size(), "indicator must draw while waiting");
    bool moved = false;
    for (size_t i = 0; i < first.size(); ++i) {
        moved |= first[i].x != second[i].x || first[i].y != second[i].y;
    }
    Check(moved, "indicator must animate without a new game frame");
    Draw(Stage::Executable, {640, 360}, "", 2000);
    Draw(Stage::FirstFrame, {2560, 1080}, "Ultrawide game display", 10000);
    Draw(Stage::Modules, {3840, 2160}, "A very long game title with multiple words and subtitles",
         65000);
    Check(Draw(Stage::Complete, {1280, 720}, "Game", 20000).empty(),
          "completed startup must leave no overlay");
    ImGui::DestroyContext();
}

int main(int argc, char** argv) {
    if (argc == 2 && std::string_view{argv[1]} == "lifecycle") {
        Lifecycle();
    } else if (argc == 2 && std::string_view{argv[1]} == "black_frames") {
        BlackFrames();
    } else {
        Ui();
    }
}
