// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <imgui.h>

#include "common/config.h"
#include "common/singleton.h"
#include "common/types.h"
#include "core/debug_state.h"
#include "imgui/imgui_std.h"
#include "imgui_internal.h"
#include "layer.h"
#include "widget/frame_dump.h"
#include "widget/frame_graph.h"

using namespace ImGui;
using namespace Core::Devtools;
using L = Core::Devtools::Layer;

static bool show_simple_fps = false;
static float fps_scale = 1.0f;

static bool show_advanced_debug = false;

static int dump_frame_count = 1;

static Widget::FrameGraph frame_graph;
static std::vector<Widget::FrameDumpViewer> frame_viewers;

static float debug_popup_timing = 3.0f;

void L::DrawMenuBar() {
    const auto& ctx = *GImGui;
    const auto& io = ctx.IO;

    auto isSystemPaused = DebugState.IsGuestThreadsPaused();

    if (BeginMainMenuBar()) {
        if (BeginMenu("Options")) {
            if (MenuItemEx("Emulator Paused", nullptr, nullptr, isSystemPaused)) {
                if (isSystemPaused) {
                    DebugState.ResumeGuestThreads();
                } else {
                    DebugState.PauseGuestThreads();
                }
            }
            ImGui::EndMenu();
        }
        if (BeginMenu("GPU Tools")) {
            MenuItem("Show frame info", nullptr, &frame_graph.is_open);
            if (BeginMenu("Dump frames")) {
                SliderInt("Count", &dump_frame_count, 1, 5);
                if (MenuItem("Dump", "Ctrl+Alt+F9", nullptr, !DebugState.DumpingCurrentFrame())) {
                    DebugState.RequestFrameDump(dump_frame_count);
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        EndMainMenuBar();
    }

    if (IsKeyPressed(ImGuiKey_F9, false)) {
        if (io.KeyCtrl && io.KeyAlt) {
            if (!DebugState.ShouldPauseInSubmit()) {
                DebugState.RequestFrameDump(dump_frame_count);
            }
        }
        if (!io.KeyCtrl && !io.KeyAlt) {
            if (isSystemPaused) {
                DebugState.ResumeGuestThreads();
            } else {
                DebugState.PauseGuestThreads();
            }
        }
    }
}

void L::DrawAdvanced() {
    DrawMenuBar();

    const auto& ctx = *GImGui;
    const auto& io = ctx.IO;

    auto isSystemPaused = DebugState.IsGuestThreadsPaused();

    frame_graph.Draw();

    if (isSystemPaused) {
        GetForegroundDrawList(GetMainViewport())
            ->AddText({10.0f, io.DisplaySize.y - 40.0f}, IM_COL32_WHITE, "Emulator paused");
    }

    if (DebugState.should_show_frame_dump) {
        DebugState.should_show_frame_dump = false;
        std::unique_lock lock{DebugState.frame_dump_list_mutex};
        while (!DebugState.frame_dump_list.empty()) {
            auto frame_dump = std::move(DebugState.frame_dump_list.back());
            DebugState.frame_dump_list.pop_back();
            frame_viewers.emplace_back(frame_dump);
        }
    }

    for (auto it = frame_viewers.begin(); it != frame_viewers.end();) {
        if (it->is_open) {
            it->Draw();
            ++it;
        } else {
            it = frame_viewers.erase(it);
        }
    }

    if (!DebugState.debug_message_popup.empty()) {
        if (debug_popup_timing > 0.0f) {
            debug_popup_timing -= io.DeltaTime;
            if (Begin("##devtools_msg", nullptr,
                      ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings |
                          ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoMove)) {
                BringWindowToDisplayFront(GetCurrentWindow());
                const auto display_size = io.DisplaySize;
                const auto& msg = DebugState.debug_message_popup.front();
                const auto padding = GetStyle().WindowPadding;
                const auto txt_size = CalcTextSize(&msg.front(), &msg.back() + 1, false, 250.0f);
                SetWindowPos({display_size.x - padding.x * 2.0f - txt_size.x, 50.0f});
                SetWindowSize({txt_size.x + padding.x * 2.0f, txt_size.y + padding.y * 2.0f});
                PushTextWrapPos(250.0f);
                TextEx(&msg.front(), &msg.back() + 1);
                PopTextWrapPos();
            }
            End();
        } else {
            DebugState.debug_message_popup.pop();
            debug_popup_timing = 3.0f;
        }
    }
}

void L::DrawSimple() {
    const auto io = GetIO();
    Text("Frame time: %.3f ms (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
}

void L::SetupSettings() {
    frame_graph.is_open = true;

    ImGuiSettingsHandler handler{};
    handler.TypeName = "DevtoolsLayer";
    handler.TypeHash = ImHashStr(handler.TypeName);
    handler.ReadOpenFn = [](ImGuiContext*, ImGuiSettingsHandler*, const char* name) {
        return std::string_view("Data") == name ? (void*)1 : nullptr;
    };
    handler.ReadLineFn = [](ImGuiContext*, ImGuiSettingsHandler*, void*, const char* line) {
        int v;
        float f;
        if (sscanf(line, "fps_scale=%f", &f) == 1) {
            fps_scale = f;
        } else if (sscanf(line, "show_advanced_debug=%d", &v) == 1) {
            show_advanced_debug = v != 0;
        } else if (sscanf(line, "show_frame_graph=%d", &v) == 1) {
            frame_graph.is_open = v != 0;
        } else if (sscanf(line, "dump_frame_count=%d", &v) == 1) {
            dump_frame_count = v;
        }
    };
    handler.WriteAllFn = [](ImGuiContext*, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf) {
        buf->appendf("[%s][Data]\n", handler->TypeName);
        buf->appendf("fps_scale=%f\n", fps_scale);
        buf->appendf("show_advanced_debug=%d\n", show_advanced_debug);
        buf->appendf("show_frame_graph=%d\n", frame_graph.is_open);
        buf->appendf("dump_frame_count=%d\n", dump_frame_count);
        buf->append("\n");
    };
    AddSettingsHandler(&handler);

    const ImGuiID dock_id = ImHashStr("FrameDumpDock");
    DockBuilderAddNode(dock_id, 0);
    DockBuilderSetNodePos(dock_id, ImVec2{50.0, 50.0});
    DockBuilderFinish(dock_id);
}

void L::Draw() {
    const auto io = GetIO();
    PushID("DevtoolsLayer");

    if (!DebugState.IsGuestThreadsPaused()) {
        const auto fn = DebugState.flip_frame_count.load();
        frame_graph.AddFrame(fn, io.DeltaTime);
    }

    if (IsKeyPressed(ImGuiKey_F10, false)) {
        if (io.KeyCtrl) {
            show_advanced_debug = !show_advanced_debug;
        } else {
            show_simple_fps = !show_simple_fps;
        }
    }

    if (show_simple_fps) {
        if (Begin("Video Info", nullptr,
                  ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDecoration |
                      ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDocking)) {
            SetWindowPos("Video Info", {999999.0f, 0.0f}, ImGuiCond_FirstUseEver);
            if (BeginPopupContextWindow()) {
#define M(label, value)                                                                            \
    if (MenuItem(label, nullptr, fps_scale == value))                                              \
    fps_scale = value
                M("0.5x", 0.5f);
                M("1.0x", 1.0f);
                M("1.5x", 1.5f);
                M("2.0x", 2.0f);
                M("2.5x", 2.5f);
                EndPopup();
#undef M
            }
            KeepWindowInside();
            SetWindowFontScale(fps_scale);
            DrawSimple();
        }
        End();
    }

    if (show_advanced_debug) {
        PushFont(io.Fonts->Fonts[IMGUI_FONT_MONO]);
        PushID("DevtoolsLayer");
        DrawAdvanced();
        PopID();
        PopFont();
    }

    PopID();
}
