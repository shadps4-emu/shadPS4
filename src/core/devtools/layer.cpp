// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "layer.h"

#include <chrono>
#include <fstream>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_video.h>
#include <emulator.h>

#include <SDL3/SDL.h>
#include <imgui.h>
#include <toml.hpp>

#ifdef ENABLE_QT_GUI
#include "qt_gui/main_window.h"
#endif
#include "common/config.h"
#include "common/memory_patcher.h"
#include "common/singleton.h"
#include "common/types.h"
#include "core/debug_state.h"
#include "core/emulator_state.h"
#include "core/libraries/pad/pad.h"
#include "core/libraries/videoout/video_out.h"
#include "imgui/imgui_std.h"
#include "imgui_internal.h"
#include "input/input_handler.h"
#include "options.h"
#include "sdl_window.h"

#include <cmrc/cmrc.hpp>
#include "core/file_format/psf.h"
#include "core/file_sys/fs.h"
#include "core/ipc/ipc.h"
#include "imgui/imgui_texture.h"
#include "video_core/renderer_vulkan/vk_presenter.h"
#include "widget/frame_dump.h"
#include "widget/frame_graph.h"
#include "widget/gpu_memory.h"
#include "widget/memory_map.h"
#include "widget/module_list.h"
#include "widget/shader_list.h"
CMRC_DECLARE(res);

extern std::unique_ptr<Vulkan::Presenter> presenter;
extern Frontend::WindowSDL* g_window;

using Btn = Libraries::Pad::OrbisPadButtonDataOffset;

std::string current_filter = Config::getLogFilter();
std::string filter = Config::getLogFilter();
static char filter_buf[256] = "";

static bool show_virtual_keyboard = false;
static bool should_focus = false;

static bool show_volume = false;
static float volume_start_time;

using namespace ImGui;
using namespace ::Core::Devtools;
using L = ::Core::Devtools::Layer;

static bool show_simple_fps = false;
static bool visibility_toggled = false;
static float fps_anchor_width = FLT_MAX;
static bool show_pause_status = false;
static bool show_quit_window = false;

static bool show_hotkeys_tip = true;
static bool show_hotkeys_pause = true;
static bool show_hotkeys_tip_manual = false;
static bool fullscreen_tip_manual = false;
static bool show_fullscreen_tip = true;
static float fullscreen_tip_timer = 10.0f;
static float hotkeys_tip_timer = 10.0f;
static int quit_menu_selection = 0;
static bool showTrophyViewer = false;

struct CachedTrophy {
    std::string name;
    std::string trophyType;
    std::string id;
    bool unlocked;
    std::filesystem::path iconPath;
    ImGui::RefCountedTexture iconTex;
    ImGui::RefCountedTexture typeTex;
};

struct CachedTrophyFile {
    std::string id;
    std::vector<CachedTrophy> trophies;
};

struct TrophyGameData {
    std::vector<CachedTrophyFile> files;
    int totalCount = 0;
    int unlockedCount = 0;
};

static void SetupTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 8.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 4.0f;
    style.WindowPadding = ImVec2(10, 10);
    style.FramePadding = ImVec2(8, 6);
    style.ItemSpacing = ImVec2(8, 6);

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.12f, 0.98f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.10f, 0.98f);
    colors[ImGuiCol_Border] = ImVec4(0.20f, 0.22f, 0.25f, 0.50f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.18f, 0.22f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.24f, 0.30f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.25f, 0.28f, 0.35f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.36f, 0.69f, 1.00f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Tab] = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
}

namespace Overlay {

void TextCentered(const std::string& text) {
    float window_width = GetContentRegionAvail().x;
    float text_width = CalcTextSize(text.c_str()).x;
    float text_indentation = (window_width - text_width) * 0.5f;

    SetCursorPosX(text_indentation);
    Text("%s", text.c_str());
}

void ToggleSimpleFps() {
    show_simple_fps = !show_simple_fps;
    visibility_toggled = true;
}
void ToggleQuitWindow() {

    show_quit_window = !show_quit_window;

    if (show_quit_window) {
        quit_menu_selection = 0;

        if (!DebugState.IsGuestThreadsPaused()) {
            DebugState.PauseGuestThreads();
        }

    } else {
        if (DebugState.IsGuestThreadsPaused()) {
            DebugState.ResumeGuestThreads();
        }
    }
}

void ShowVolume() {
    if (!show_volume) {
        volume_start_time = ImGui::GetTime();
    } else {
        volume_start_time = ImGui::GetTime() - 0.3f;
    }
    show_volume = true;
}

void TogglePauseWindow() {
    if (show_hotkeys_tip) {
        show_hotkeys_pause = false;
    } else {
        show_hotkeys_pause = true;
    }
}
} // namespace Overlay

static float fps_scale = 1.0f;
static int dump_frame_count = 1;

static Widget::FrameGraph frame_graph;
static std::vector<Widget::FrameDumpViewer> frame_viewers;

static float debug_popup_timing = 3.0f;

static bool just_opened_options = false;

static Widget::MemoryMapViewer memory_map;
static Widget::ShaderList shader_list;
static Widget::ModuleList module_list;
static Widget::GpuMemoryViewer gpu_memory;

// clang-format off
static std::string help_text =
#include "help.txt"
    ;
// clang-format on

void L::TextCentered(const std::string& text) {
    ImVec2 textSize = ImGui::CalcTextSize(text.c_str());
    float windowWidth = ImGui::GetWindowSize().x;
    float cursorX = (windowWidth - textSize.x) * 0.5f;

    ImGui::SetCursorPosX(cursorX);
    ImGui::TextUnformatted(text.c_str());
}

bool L::ShouldKeepDrawing() {
    return DebugState.IsShowingDebugMenuBar();
}

void L::DrawMenuBar() {
    const auto& ctx = *GImGui;
    const auto& io = ctx.IO;

    auto isSystemPaused = DebugState.IsGuestThreadsPaused();

    bool open_popup_options = false;
    bool open_popup_help = false;

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
            MenuItem("Show loaded shaders", nullptr, &shader_list.open);
            MenuItem("Show GPU memory usage", nullptr, &gpu_memory.open);
            if (BeginMenu("Dump frames")) {
                SliderInt("Count", &dump_frame_count, 1, 5);
                if (MenuItem("Dump", "Ctrl+Alt+F9", nullptr, !DebugState.DumpingCurrentFrame())) {
                    DebugState.RequestFrameDump(dump_frame_count);
                }
                ImGui::EndMenu();
            }
            open_popup_options = MenuItem("Options");
            open_popup_help = MenuItem("Help & Tips");
            ImGui::EndMenu();
        }
        if (BeginMenu("Display")) {
            auto& pp_settings = presenter->GetPPSettingsRef();
            if (BeginMenu("Brightness")) {
                SliderFloat("Gamma", &pp_settings.gamma, 0.1f, 2.0f);
                ImGui::EndMenu();
            }
            if (BeginMenu("FSR")) {
                auto& fsr = presenter->GetFsrSettingsRef();

                static bool first_open = true;
                if (first_open) {
                    fsr.enable = Config::getFsrEnabled();
                    fsr.use_rcas = Config::getRcasEnabled();
                    fsr.rcasAttenuation =
                        static_cast<float>(Config::getRcasAttenuation()) / 1000.0f;
                    first_open = false;
                }

                Checkbox("FSR Enabled", &fsr.enable);
                BeginDisabled(!fsr.enable);
                {
                    Checkbox("RCAS", &fsr.use_rcas);
                    BeginDisabled(!fsr.use_rcas);
                    {
                        SliderFloat("RCAS Attenuation", &fsr.rcasAttenuation, 0.0, 3.0);
                    }
                    EndDisabled();
                }
                EndDisabled();

                if (Button("Save")) {
                    Config::setFsrEnabled(fsr.enable);
                    Config::setRcasEnabled(fsr.use_rcas);
                    Config::setRcasAttenuation(static_cast<int>(fsr.rcasAttenuation * 1000));
                    Config::save(Common::FS::GetUserPath(Common::FS::PathType::UserDir) /
                                 "config.toml");
                    first_open = true;
                    CloseCurrentPopup();
                }

                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        if (BeginMenu("Debug")) {
            MenuItem("Memory map", nullptr, &memory_map.open);
            MenuItem("Module list", nullptr, &module_list.open);
            ImGui::EndMenu();
        }

        SameLine(ImGui::GetWindowWidth() - 30.0f);
        if (Button("X", ImVec2(25, 25))) {
            DebugState.IsShowingDebugMenuBar() = false;
        }

        EndMainMenuBar();
    }
    if (open_popup_options) {
        OpenPopup("GPU Tools Options");
        just_opened_options = true;
    }
    if (open_popup_help) {
        OpenPopup("HelpTips");
    }
}

void L::DrawAdvanced() {
    DrawMenuBar();

    const auto& ctx = *GImGui;
    const auto& io = ctx.IO;

    frame_graph.Draw();

    if (DebugState.should_show_frame_dump && DebugState.waiting_reg_dumps.empty()) {
        DebugState.should_show_frame_dump = false;
        std::unique_lock lock{DebugState.frame_dump_list_mutex};
        while (!DebugState.frame_dump_list.empty()) {
            const auto& frame_dump = DebugState.frame_dump_list.back();
            frame_viewers.emplace_back(frame_dump);
            DebugState.frame_dump_list.pop_back();
        }
        static bool first_time = true;
        if (first_time) {
            first_time = false;
            DebugState.ShowDebugMessage("Tip: You can shift+click any\n"
                                        "popup to open a new window");
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
            debug_popup_timing -= DebugState.IsGuestThreadsPaused() ? 0.0f : io.DeltaTime;
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

    bool close_popup_options = true;
    if (BeginPopupModal("GPU Tools Options", &close_popup_options,
                        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
        static char disassembler_cli_isa[512];
        static char disassembler_cli_spv[512];
        static bool frame_dump_render_on_collapse;

        if (just_opened_options) {
            just_opened_options = false;
            auto s = Options.disassembler_cli_isa.copy(disassembler_cli_isa,
                                                       sizeof(disassembler_cli_isa) - 1);
            disassembler_cli_isa[s] = '\0';
            s = Options.disassembler_cli_spv.copy(disassembler_cli_spv,
                                                  sizeof(disassembler_cli_spv) - 1);
            disassembler_cli_spv[s] = '\0';
            frame_dump_render_on_collapse = Options.frame_dump_render_on_collapse;
        }

        InputText("Shader isa disassembler: ", disassembler_cli_isa, sizeof(disassembler_cli_isa));
        if (IsItemHovered()) {
            SetTooltip(R"(Command to disassemble shaders. Example: dis.exe --raw "{src}")");
        }
        InputText("Shader SPIRV disassembler: ", disassembler_cli_spv,
                  sizeof(disassembler_cli_spv));
        if (IsItemHovered()) {
            SetTooltip(R"(Command to disassemble shaders. Example: spirv-cross -V "{src}")");
        }
        Checkbox("Show frame dump popups even when collapsed", &frame_dump_render_on_collapse);
        if (IsItemHovered()) {
            SetTooltip("When a frame dump is collapsed, it will keep\n"
                       "showing all opened popups related to it");
        }

        if (Button("Save")) {
            Options.disassembler_cli_isa = disassembler_cli_isa;
            Options.disassembler_cli_spv = disassembler_cli_spv;
            Options.frame_dump_render_on_collapse = frame_dump_render_on_collapse;
            SaveIniSettingsToDisk(io.IniFilename);
            CloseCurrentPopup();
        }
        EndPopup();
    }

    if (BeginPopup("HelpTips", ImGuiWindowFlags_AlwaysAutoResize |
                                   ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove)) {
        CentralizeWindow();

        PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{10.0f});
        PushTextWrapPos(600.0f);

        const char* begin = help_text.data();
        TextUnformatted(begin, begin + help_text.size());

        PopTextWrapPos();
        PopStyleVar();

        EndPopup();
    }

    if (memory_map.open) {
        memory_map.Draw();
    }
    if (shader_list.open) {
        shader_list.Draw();
    }
    if (module_list.open) {
        module_list.Draw();
    }
    if (gpu_memory.open) {
        gpu_memory.Draw();
    }
}

void L::DrawSimple() {
    const float frameRate = DebugState.Framerate;

    if (frameRate < 10.0f) {
        PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
    } else if (frameRate < 30.0f) {
        PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.5f, 0.0f, 1.0f));
    } else if (frameRate <= 60.0f) {
        PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
    } else {
        PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.7f, 1.0f, 1.0f));
    }

    const float ms = frameRate > 0.0f ? (1000.0f / frameRate) : 0.0f;
    Text("%d FPS (%.1f ms)", static_cast<int>(std::round(frameRate)), ms);

    PopStyleColor();
}

static void LoadSettings(const char* line) {
    int i;
    float f;
    if (sscanf(line, "fps_scale=%f", &f) == 1) {
        fps_scale = f;
        return;
    }
    if (sscanf(line, "show_advanced_debug=%d", &i) == 1) {
        DebugState.IsShowingDebugMenuBar() = i != 0;
        return;
    }
    if (sscanf(line, "show_frame_graph=%d", &i) == 1) {
        frame_graph.is_open = i != 0;
        return;
    }
    if (sscanf(line, "show_shader_list=%d", &i) == 1) {
        shader_list.open = i != 0;
        return;
    }
    if (sscanf(line, "show_memory_map=%d", &i) == 1) {
        memory_map.open = i != 0;
        return;
    }
    if (sscanf(line, "show_module_list=%d", &i) == 1) {
        module_list.open = i != 0;
        return;
    }
    if (sscanf(line, "show_gpu_memory=%d", &i) == 1) {
        gpu_memory.open = i != 0;
        return;
    }
    if (sscanf(line, "dump_frame_count=%d", &i) == 1) {
        dump_frame_count = i;
        return;
    }
}

void L::SetupSettings() {
    frame_graph.is_open = true;
    show_simple_fps = Config::getShowFpsCounter();

    using SettingLoader = void (*)(const char*);

    ImGuiSettingsHandler handler{};
    handler.TypeName = "DevtoolsLayer";
    handler.TypeHash = ImHashStr(handler.TypeName);
    handler.ReadOpenFn = [](ImGuiContext*, ImGuiSettingsHandler*, const char* name) {
        if (std::string_view("Data") == name) {
            static_assert(std::is_same_v<decltype(&LoadSettings), SettingLoader>);
            return (void*)&LoadSettings;
        }
        if (std::string_view("CmdList") == name) {
            static_assert(
                std::is_same_v<decltype(&Widget::CmdListViewer::LoadConfig), SettingLoader>);
            return (void*)&Widget::CmdListViewer::LoadConfig;
        }
        if (std::string_view("Options") == name) {
            static_assert(std::is_same_v<decltype(&LoadOptionsConfig), SettingLoader>);
            return (void*)&LoadOptionsConfig;
        }
        return (void*)nullptr;
    };
    handler.ReadLineFn = [](ImGuiContext*, ImGuiSettingsHandler*, void* handle, const char* line) {
        if (handle != nullptr) {
            reinterpret_cast<SettingLoader>(handle)(line);
        }
    };
    handler.WriteAllFn = [](ImGuiContext*, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf) {
        buf->appendf("[%s][Data]\n", handler->TypeName);
        buf->appendf("fps_scale=%f\n", fps_scale);
        buf->appendf("show_advanced_debug=%d\n", DebugState.IsShowingDebugMenuBar());
        buf->appendf("show_frame_graph=%d\n", frame_graph.is_open);
        buf->appendf("show_shader_list=%d\n", shader_list.open);
        buf->appendf("show_memory_map=%d\n", memory_map.open);
        buf->appendf("show_module_list=%d\n", module_list.open);
        buf->appendf("show_gpu_memory=%d\n", gpu_memory.open);
        buf->appendf("dump_frame_count=%d\n", dump_frame_count);
        buf->append("\n");
        buf->appendf("[%s][CmdList]\n", handler->TypeName);
        Widget::CmdListViewer::SerializeConfig(buf);
        buf->append("\n");
        buf->appendf("[%s][Options]\n", handler->TypeName);
        SerializeOptionsConfig(buf);
        buf->append("\n");
    };
    AddSettingsHandler(&handler);

    const ImGuiID dock_id = ImHashStr("FrameDumpDock");
    DockBuilderAddNode(dock_id, 0);
    DockBuilderSetNodePos(dock_id, ImVec2{450.0, 150.0});
    DockBuilderSetNodeSize(dock_id, ImVec2{400.0, 500.0});
    DockBuilderFinish(dock_id);
}

void SetSimpleFps(bool enabled) {
    show_simple_fps = enabled;
    visibility_toggled = true;
}

void DrawFullscreenHotkeysWindow(bool& is_open) {
    if (!is_open)
        return;

    constexpr ImVec2 hotkeys_pos = {10, 10};
    ImGui::SetNextWindowPos(hotkeys_pos, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.90f);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNavInputs |
                             ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove;

    if (ImGui::Begin("HotkeysBoot", &is_open, flags)) {
        ImGui::SetWindowFontScale(1.0f);
        ImGui::SeparatorText("Controls");

        bool swap = Config::getXCircleButtonSwap();
        std::string pause_hk =
            std::string("F9 or Share/PS/GUIDE+") + (swap ? "Circle/B" : "Cross/A");
        std::string stop_hk =
            std::string("ESC or Share/PS/GUIDE+") + (swap ? "Cross/A" : "Circle/B");

        struct HotkeyItem {
            const char* action;
            const char* keys;
        };

        HotkeyItem hotkeys[] = {{"Pause/Resume", pause_hk.c_str()},
                                {"Stop", stop_hk.c_str()},
                                {"Fullscreen", "F11 or Share/PS/GUIDE+R2"},
                                {"Developer Tools", "Ctrl+F10 or Share/PS/GUIDE+Up"},
                                {"Show FPS", "F10 or Share/PS/GUIDE+L2"},
                                {"Settings", "F3 or Share/PS/GUIDE+Triangle/X"},
                                {"Mute Game", "F1 or Share/PS/GUIDE+Right"},
                                {"View Trophies", "F2 or Share/PS/GUIDE+Left"}};

        if (ImGui::BeginTable("HotkeysTable", 2)) {
            for (const auto& hk : hotkeys) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "%s", hk.action);
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(hk.keys);
            }
            ImGui::EndTable();
        }
    }
    ImGui::End();
}

void DrawFullscreenHotkeysPause(bool& is_open) {
    if (!is_open)
        return;

    constexpr ImVec2 hotkeys_pos = {10, 10};
    ImGui::SetNextWindowPos(hotkeys_pos, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.90f);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNavInputs |
                             ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove;

    if (ImGui::Begin("HotkeysPause", &is_open, flags)) {
        ImGui::SetWindowFontScale(1.0f);
        ImGui::SeparatorText("Pause Controls");

        bool swap = Config::getXCircleButtonSwap();
        std::string pause_hk =
            std::string("F9 or Share/PS/GUIDE+") + (swap ? "Circle/B" : "Cross/A");
        std::string stop_hk =
            std::string("ESC or Share/PS/GUIDE+") + (swap ? "Cross/A" : "Circle/B");

        struct HotkeyItem {
            const char* action;
            const char* keys;
        };

        HotkeyItem hotkeys[] = {{"Pause/Resume", pause_hk.c_str()},
                                {"Stop", stop_hk.c_str()},
                                {"Fullscreen", "F11 or Share/PS/GUIDE+R2"},
                                {"Developer Tools", "Ctrl+F10 or Share/PS/GUIDE+Up"},
                                {"Show FPS", "F10 or Share/PS/GUIDE+L2"},
                                {"Settings", "F3 or Share/PS/GUIDE+Triangle/X"},
                                {"Mute Game", "F1 or Share/PS/GUIDE+Right"},
                                {"View Trophies", "F2 or Share/PS/GUIDE+Left"}};

        if (ImGui::BeginTable("HotkeysPauseTable", 2)) {
            for (const auto& hk : hotkeys) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "%s", hk.action);
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(hk.keys);
            }
            ImGui::EndTable();
        }
    }
    ImGui::End();
}

void L::SaveConfigWithOverrides(const std::filesystem::path& path, bool perGame = false,
                                const std::string& gameSerial = "") {
    toml::value overrides = toml::table{};

    overrides["General"]["logFilter"] = Config::getLogFilter();
    overrides["General"]["enableAutoBackup"] = Config::getEnableAutoBackup();
    overrides["General"]["isPSNSignedIn"] = Config::getShadNetEnabled(0);
    overrides["General"]["muteEnabled"] = Config::isMuteEnabled();
    overrides["General"]["isConnectedToNetwork"] = Config::getIsConnectedToNetwork();
    overrides["General"]["isDevKit"] = Config::isDevKitConsole();
    overrides["General"]["isPS4Pro"] = Config::isNeoModeConsole();
    overrides["General"]["extraDmemInMbytes"] = Config::getExtraDmemInMbytes();

    overrides["GPU"]["allowHDR"] = Config::allowHDR();
    overrides["GPU"]["vblankFrequency"] = Config::vblankFreq();
    overrides["GPU"]["fsrEnabled"] = Config::getFsrEnabled();
    overrides["GPU"]["rcasEnabled"] = Config::getRcasEnabled();
    overrides["GPU"]["rcasAttenuation"] = Config::getRcasAttenuation();
    overrides["GPU"]["readbackLinearImages"] = Config::getReadbackLinearImages();
    overrides["GPU"]["shaderSkipsEnabled"] = Config::getShaderSkipsEnabled();
    overrides["GPU"]["directMemoryAccess"] = Config::directMemoryAccess();
    overrides["GPU"]["readbackSpeedMode"] = static_cast<int>(Config::readbackSpeed());
    overrides["GPU"]["presentMode"] = Config::getPresentMode();

    overrides["Logging"]["logType"] = Config::getLogType();

    std::filesystem::create_directories(path.parent_path());

    std::ofstream ofs(path, std::ios::trunc);
    if (ofs && (ofs << overrides)) {
        ofs.close();
    }
}

void DrawFullscreenSettingsWindow(bool& is_open) {
    if (!is_open)
        return;

    auto DrawYesNo = [](const char* label, bool value) {
        ImGui::Text("%s:", label);
        ImGui::SameLine();
        ImGui::TextColored(value ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0, 0, 1), value ? "Yes" : "No");
    };

    constexpr ImVec2 settings_pos = {10, 300};
    ImGui::SetNextWindowPos(settings_pos, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(0.95f);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNavInputs;

    if (ImGui::Begin("Settings", &is_open, flags)) {
        ImGui::SeparatorText("Status");
        ImGui::Text("Network:");
        ImGui::SameLine();
        ImGui::TextColored(Config::getIsConnectedToNetwork() ? ImVec4(0, 1, 0, 1)
                                                             : ImVec4(1, 0, 0, 1),
                           Config::getIsConnectedToNetwork() ? "Connected" : "Disconnected");

        DrawYesNo("PSN", Config::getShadNetEnabled(0));
        ImGui::SeparatorText("Configuration");

        if (ImGui::BeginTable("SettingsGrid", 2, ImGuiTableFlags_BordersInnerV)) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            DrawYesNo("HDR Allowed", Config::allowHDR());
            DrawYesNo("FSR Enabled", Config::getFsrEnabled());
            if (Config::getFsrEnabled()) {
                DrawYesNo("RCAS", Config::getRcasEnabled());
                ImGui::Text("RCAS Sharpness: %.2f", presenter->GetFsrSettingsRef().rcasAttenuation);
            }
            ImGui::Text("VBlank: %d Hz", Config::vblankFreq());
            ImGui::Text("Present: %s", Config::getPresentMode().c_str());

            ImGui::TableSetColumnIndex(1);

            DrawYesNo("Linear Readbacks", Config::getReadbackLinearImages());
            DrawYesNo("DMA Access", Config::directMemoryAccess());

            const char* readbackStr = "Unknown";
            switch (Config::readbackSpeed()) {
            case Config::ReadbackSpeed::Disable:
                readbackStr = "Disable";
                break;
            case Config::ReadbackSpeed::Unsafe:
                readbackStr = "Unsafe";
                break;
            case Config::ReadbackSpeed::Low:
                readbackStr = "Low";
                break;
            case Config::ReadbackSpeed::Fast:
                readbackStr = "Fast";
                break;
            case Config::ReadbackSpeed::Default:
                readbackStr = "Default";
                break;
            }
            ImGui::Text("Readback: %s", readbackStr);
            DrawYesNo("Auto Backup", Config::getEnableAutoBackup());
            DrawYesNo("Shader Skips", Config::getShaderSkipsEnabled());

            ImGui::EndTable();
        }

        ImGui::Separator();
        ImGui::TextDisabled("Log Type: %s | Filter: %s", Config::getLogType().c_str(),
                            Config::getLogFilter().c_str());
    }
    ImGui::End();
}

void DrawVirtualKeyboard() {
    if (!show_virtual_keyboard)
        return;

    static bool first_letter_caps = true;
    static bool caps_lock = false;

    auto push_to_box = [&]() { Config::setLogFilter(std::string(filter_buf)); };
    auto clear_all = [&]() {
        filter_buf[0] = '\0';
        first_letter_caps = true;
        caps_lock = false;
        push_to_box();
    };

    ImGui::SetNextWindowSize(ImVec2(600, 300), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Virtual Keyboard", &show_virtual_keyboard)) {

        ImGui::SetNextItemWidth(-1);
        ImGui::InputText("##Display", filter_buf, sizeof(filter_buf), ImGuiInputTextFlags_ReadOnly);
        ImGui::Spacing();

        static const char* keys_lower[] = {
            "q", "w", "e", "r", "t", "y",  "u",     "i",    "o",    "p",    "a", "s",
            "d", "f", "g", "h", "j", "k",  "l",     "z",    "x",    "c",    "v", "b",
            "n", "m", ".", "*", ":", "*:", "SPACE", "BACK", "CAPS", "SHIFT"};

        static const char* keys_upper[] = {
            "Q", "W", "E", "R", "T", "Y",  "U",     "I",    "O",    "P",    "A", "S",
            "D", "F", "G", "H", "J", "K",  "L",     "Z",    "X",    "C",    "V", "B",
            "N", "M", ".", "*", ":", "*:", "SPACE", "BACK", "CAPS", "SHIFT"};

        const char** keys = (caps_lock || first_letter_caps) ? keys_upper : keys_lower;

        int columns = 10;
        if (ImGui::BeginTable("KeyGrid", columns)) {
            for (int i = 0; i < IM_ARRAYSIZE(keys_lower); i++) {
                ImGui::TableNextColumn();
                const char* key = keys[i];

                if (ImGui::Button(key, ImVec2(-FLT_MIN, 40))) {
                    if (strcmp(key, "SPACE") == 0) {
                        strncat(filter_buf, " ", sizeof(filter_buf) - strlen(filter_buf) - 1);
                    } else if (strcmp(key, "BACK") == 0) {
                        int len = strlen(filter_buf);
                        if (len > 0)
                            filter_buf[len - 1] = '\0';
                        if (strlen(filter_buf) == 0)
                            first_letter_caps = true;
                    } else if (strcmp(key, "CAPS") == 0) {
                        caps_lock = !caps_lock;
                    } else if (strcmp(key, "SHIFT") == 0) {
                        caps_lock = true;
                        first_letter_caps = false;
                    } else if (strcmp(key, "*:") == 0) {
                        strncat(filter_buf, "*:", sizeof(filter_buf) - strlen(filter_buf) - 1);
                    } else {
                        char c = key[0];
                        if (!caps_lock && !first_letter_caps && isalpha(c))
                            c = tolower(c);

                        strncat(filter_buf, &c, 1);

                        if (first_letter_caps)
                            first_letter_caps = false;

                        if (caps_lock && strcmp(key, "CAPS") != 0)
                            caps_lock = false;
                    }
                    push_to_box();
                }
            }
            ImGui::EndTable();
        }

        ImGui::Separator();
        if (ImGui::Button("Close Keyboard", ImVec2(150, 40))) {
            show_virtual_keyboard = false;
            Config::setLogFilter(std::string(filter_buf));
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear Text", ImVec2(150, 40))) {
            clear_all();
        }
    }
    ImGui::End();
}

void L::DrawPauseStatusWindow(bool& is_open) {
    if (!is_open)
        return;
    u8 pad = 0;

    static bool should_focus = false;
    static TrophyGameData cachedTrophies;
    static bool dataLoaded = false;

    constexpr ImVec2 window_size = {700, 600};
    ImGui::SetNextWindowBgAlpha(0.95f);
    ImGui::SetNextWindowSize(window_size, ImGuiCond_FirstUseEver);

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoFocusOnAppearing |
                                   ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;

    if (Input::ControllerPressedOnce(pad, {Btn::Up}) ||
        Input::ControllerPressedOnce(pad, {Btn::Down}) ||
        Input::ControllerPressedOnce(pad, {Btn::Left}) ||
        Input::ControllerPressedOnce(pad, {Btn::Right})) {
        should_focus = true;
    }

    if (should_focus)
        ImGui::SetWindowFocus("Pause Menu");

    if (!ImGui::Begin("Pause Menu", &is_open, windowFlags)) {
        ImGui::End();
        return;
    }

    ImGui::SetWindowFontScale(1.0f);
    ImGui::Spacing();

    ImGui::PushFont(io.Fonts->Fonts[0]);
    ImGui::SetWindowFontScale(1.5f);
    L::TextCentered("GAME PAUSED");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopFont();
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0.0f, 10.0f));

    if (ImGui::BeginTable("StatusBar", 3)) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        if (Config::getIsConnectedToNetwork())
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "Network: Connected");
        else
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "Network: Offline");

        ImGui::TableSetColumnIndex(1);
        ImGui::Text("FPS: %.0f", DebugState.Framerate);

        ImGui::TableSetColumnIndex(2);
        if (Config::getShadNetEnabled(0))
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "PSN: Online");
        else
            ImGui::TextDisabled("PSN: Offline");
        ImGui::EndTable();
    }
    ImGui::Dummy(ImVec2(0.0f, 10.0f));
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0.0f, 10.0f));

    float btnWidth = 160.0f;
    float btnHeight = 40.0f;
    float totalWidth = btnWidth * 6.0f + ImGui::GetStyle().ItemSpacing.x * 5.0f;
    float centerPos = (ImGui::GetContentRegionAvail().x - totalWidth) * 0.5f;

    ImGui::SetCursorPosX(centerPos);
    if (ImGui::Button("Resume Game", ImVec2(btnWidth, btnHeight))) {
        SDL_Event e;
        e.type = SDL_EVENT_TOGGLE_PAUSE;
        SDL_PushEvent(&e);
    }
    ImGui::SameLine();

    if (ImGui::Button("View Trophies", ImVec2(btnWidth, btnHeight))) {
        ImGui::OpenPopup("Quick Trophy List Viewer");
        dataLoaded = false;
    }
    ImGui::SameLine();

    if (ImGui::Button("Big Picture", ImVec2(btnWidth, btnHeight))) {
        Config::setGameRunning(true);
        Config::setIsFullscreen(true);
        SDL_Event event{};
        event.type = SDL_EVENT_QUIT + 3;
        SDL_PushEvent(&event);
    }
    ImGui::SameLine();

    if (ImGui::Button("Restart Game", ImVec2(btnWidth, btnHeight))) {
        auto mnt = Common::Singleton<FileSys::MntPoints>::Instance();
        auto eboot_path = mnt->GetHostPath("/app0/eboot.bin");
        auto emulator = Common::Singleton<Emulator>::Instance();
        emulator->Restart(eboot_path, {});
    }
    ImGui::SameLine();

    if (ImGui::Button("Restart Emulator", ImVec2(btnWidth, btnHeight))) {
        SDL_Event event{};
        event.type = SDL_EVENT_QUIT + 1;
        SDL_PushEvent(&event);
    }
    ImGui::SameLine();

    if (ImGui::Button("Stop & Quit", ImVec2(btnWidth, btnHeight))) {
        SDL_Event event{};
        event.type = SDL_EVENT_QUIT;
        SDL_PushEvent(&event);
    }

    static ImVec2 trophy_pos = ImVec2(200, 200);
    static ImVec2 trophy_size = ImVec2(600, 400);
    static float move_speed = 0.8f;
    static float resize_speed = 0.8f;

    bool using_controller = ImGui::IsKeyDown(ImGuiKey_GamepadL1);

    if (using_controller) {
        float lx = 0, ly = 0;
        if (ImGui::IsKeyDown(ImGuiKey_GamepadLStickRight))
            lx = 1;
        if (ImGui::IsKeyDown(ImGuiKey_GamepadLStickLeft))
            lx = -1;
        if (ImGui::IsKeyDown(ImGuiKey_GamepadLStickDown))
            ly = 1;
        if (ImGui::IsKeyDown(ImGuiKey_GamepadLStickUp))
            ly = -1;
        trophy_pos.x += lx * move_speed;
        trophy_pos.y += ly * move_speed;

        float rx = 0, ry = 0;
        if (ImGui::IsKeyDown(ImGuiKey_GamepadRStickRight))
            rx = 1;
        if (ImGui::IsKeyDown(ImGuiKey_GamepadRStickLeft))
            rx = -1;
        if (ImGui::IsKeyDown(ImGuiKey_GamepadRStickDown))
            ry = 1;
        if (ImGui::IsKeyDown(ImGuiKey_GamepadRStickUp))
            ry = -1;
        trophy_size.x = ImClamp(trophy_size.x + rx * resize_speed, 300.0f, 3000.0f);
        trophy_size.y = ImClamp(trophy_size.y + ry * resize_speed, 300.0f, 3000.0f);
    }

    ImGui::SetNextWindowPos(trophy_pos,
                            using_controller ? ImGuiCond_Always : ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(trophy_size,
                             using_controller ? ImGuiCond_Always : ImGuiCond_FirstUseEver);

    if (ImGui::BeginPopupModal("Quick Trophy List Viewer", nullptr,
                               ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar)) {

        if (!dataLoaded) {
            cachedTrophies.files.clear();
            cachedTrophies.totalCount = 0;
            cachedTrophies.unlockedCount = 0;
            std::string gameSerial = MemoryPatcher::g_game_serial;

            if (!gameSerial.empty()) {
                std::filesystem::path metaDir =
                    Common::FS::GetUserPath(Common::FS::PathType::MetaDataDir) / gameSerial /
                    "TrophyFiles";

                if (std::filesystem::exists(metaDir)) {
                    for (auto& dirEntry : std::filesystem::directory_iterator(metaDir)) {
                        if (!dirEntry.is_directory())
                            continue;

                        std::string xmlPath = (dirEntry.path() / "Xml/TROP.XML").string();
                        if (!std::filesystem::exists(xmlPath))
                            continue;

#ifdef ENABLE_QT_GUI
                        QFile file(QString::fromStdString(xmlPath));
                        if (file.open(QFile::ReadOnly | QFile::Text)) {
                            CachedTrophyFile currentFile;
                            currentFile.id = dirEntry.path().filename().string();

                            QXmlStreamReader reader(&file);
                            while (!reader.atEnd() && !reader.hasError()) {
                                reader.readNext();
                                if (reader.isStartElement() &&
                                    reader.name().toString() == "trophy") {
                                    CachedTrophy t;
                                    t.unlocked = false;
                                    cachedTrophies.totalCount++;
                                    t.id = reader.attributes().value("id").toString().toStdString();
                                    t.trophyType =
                                        reader.attributes().value("ttype").toString().toStdString();

                                    if (reader.attributes().hasAttribute("unlockstate") &&
                                        reader.attributes().value("unlockstate").toString() ==
                                            "true") {
                                        t.unlocked = true;
                                        cachedTrophies.unlockedCount++;
                                    }

                                    while (!(reader.isEndElement() &&
                                             reader.name().toString() == "trophy")) {
                                        reader.readNext();
                                        if (reader.isStartElement() &&
                                            reader.name().toString() == "name") {
                                            t.name = reader.readElementText().toStdString();
                                        }
                                    }
                                    currentFile.trophies.push_back(t);
                                }
                            }
                            for (auto& trophy : currentFile.trophies) {
                                std::string iconFileName = "TROP" + trophy.id + ".PNG";
                                std::filesystem::path iconsDir = dirEntry.path() / "Icons";
                                trophy.iconPath = iconsDir / iconFileName;
                                if (std::filesystem::exists(trophy.iconPath)) {
                                    trophy.iconTex =
                                        ImGui::RefCountedTexture::DecodePngFile(trophy.iconPath);
                                }
                                std::string gradeFile;
                                if (trophy.trophyType == "P")
                                    gradeFile = "src/images/platinum.png";
                                else if (trophy.trophyType == "G")
                                    gradeFile = "src/images/gold.png";
                                else if (trophy.trophyType == "S")
                                    gradeFile = "src/images/silver.png";
                                else
                                    gradeFile = "src/images/bronze.png";
                                auto resource = cmrc::res::get_filesystem();
                                try {
                                    auto resFile = resource.open(gradeFile);
                                    std::vector<u8> imgdata(resFile.begin(), resFile.end());
                                    trophy.typeTex = ImGui::RefCountedTexture::DecodePngTexture(
                                        std::move(imgdata));
                                } catch (...) {
                                }
                            }
                            cachedTrophies.files.push_back(std::move(currentFile));
                        }
#endif
                    }
                }
            }
            dataLoaded = true;
        }

        if (!using_controller) {
            trophy_pos = ImGui::GetWindowPos();
            trophy_size = ImGui::GetWindowSize();
        } else {
            ImGui::SetNextWindowPos(trophy_pos, ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(trophy_size, ImGuiCond_FirstUseEver);
        }

        ImGui::SetWindowFontScale(1.5f);
#ifdef ENABLE_QT_GUI
        TextCentered(("Trophies (" + std::to_string(cachedTrophies.unlockedCount) + "/" +
                      std::to_string(cachedTrophies.totalCount) + ")")
                         .c_str());
#else
        TextCentered("SDL build can't read trophy XML, use QT");
#endif
        ImGui::SetWindowFontScale(1.0f);
        ImGui::Separator();

#ifdef ENABLE_QT_GUI
        if (cachedTrophies.files.empty()) {
            ImGui::Text("No trophy data found for this game.");
        } else {
            ImGui::BeginChild("TrophyList", ImVec2(0, -40), true);
            for (const auto& fileData : cachedTrophies.files) {
                if (ImGui::CollapsingHeader(fileData.id.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                    if (ImGui::BeginTable("TrophyTable", 4,
                                          ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV |
                                              ImGuiTableFlags_ScrollY)) {
                        ImGui::TableSetupColumn("##grade", ImGuiTableColumnFlags_WidthFixed, 40.0f);
                        ImGui::TableSetupColumn("##icon", ImGuiTableColumnFlags_WidthFixed, 56.0f);
                        ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 60.0f);
                        ImGui::TableSetupColumn("Trophy Name", ImGuiTableColumnFlags_WidthStretch);
                        ImGui::TableHeadersRow();

                        for (const auto& trophy : fileData.trophies) {
                            ImGui::TableNextRow(ImGuiTableRowFlags_None, 52.0f);

                            ImGui::TableSetColumnIndex(0);
                            if (trophy.typeTex) {
                                auto typeImg = trophy.typeTex.GetTexture();
                                ImGui::Image(typeImg.im_id, ImVec2(36.0f, 36.0f));
                            }

                            ImGui::TableSetColumnIndex(1);
                            if (trophy.iconTex) {
                                auto img = trophy.iconTex.GetTexture();
                                if (!trophy.unlocked)
                                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.35f);
                                ImGui::Image(img.im_id, ImVec2(50.0f, 50.0f));
                                if (!trophy.unlocked)
                                    ImGui::PopStyleVar();
                            }

                            ImGui::TableSetColumnIndex(2);
                            if (trophy.unlocked) {
                                ImGui::PushStyleColor(ImGuiCol_Text,
                                                      ImVec4(0.2f, 1.0f, 0.3f, 1.0f));
                                ImGui::TextUnformatted("Unlocked");
                                ImGui::PopStyleColor();
                            } else {
                                ImGui::PushStyleColor(ImGuiCol_Text,
                                                      ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
                                ImGui::TextUnformatted("Locked");
                                ImGui::PopStyleColor();
                            }

                            ImGui::TableSetColumnIndex(3);
                            ImGui::TextWrapped("%s", trophy.name.c_str());
                        }
                        ImGui::EndTable();
                    }
                }
            }
            ImGui::EndChild();
        }
#endif

        if (ImGui::Button("Close", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    ImGui::Dummy(ImVec2(0.0f, 15.0f));
    ImGui::Separator();

    if (ImGui::BeginTabBar("PauseTabs")) {

        if (ImGui::BeginTabItem("System")) {
            ImGui::Spacing();

            static bool network_connected = Config::getIsConnectedToNetwork();
            if (ImGui::Checkbox("Connect Network", &network_connected))
                Config::setIsConnectedToNetwork(network_connected);

            bool psn = Config::getShadNetEnabled(0);
            if (ImGui::Checkbox("Shadnet Enabled", &psn))
                Config::setShadNetEnabled(0, psn);

            bool autobackup = Config::getEnableAutoBackup();
            if (ImGui::Checkbox("Auto Backup", &autobackup))
                Config::setEnableAutoBackup(autobackup);

            ImGui::SeparatorText("Console Mode");
            static bool is_devkit = Config::isDevKitConsole();
            if (ImGui::Checkbox("Devkit Mode", &is_devkit)) {
                Config::setDevKitMode(is_devkit);
            }

            static bool is_neo = Config::isNeoModeConsole();
            if (ImGui::Checkbox("PS4 Pro (Neo) Mode", &is_neo)) {
                Config::setNeoMode(is_neo);
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Graphics")) {
            ImGui::Spacing();
            if (ImGui::Checkbox("Show Tips", &show_fullscreen_tip)) {
                if (show_fullscreen_tip)
                    fullscreen_tip_timer = 10.0f;
            }

            bool hdr = Config::allowHDR();
            if (ImGui::Checkbox("HDR Allowed", &hdr))
                Config::setAllowHDR(hdr);

            int vblank = Config::vblankFreq();
            if (ImGui::SliderInt("VBlank Freq", &vblank, 1, 120))
                Config::setVblankFreq(vblank);

            ImGui::SeparatorText("Upscaling");
            bool fsr_enabled = Config::getFsrEnabled();
            if (ImGui::Checkbox("FSR Enabled", &fsr_enabled))
                Config::setFsrEnabled(fsr_enabled);

            ImGui::BeginDisabled(!fsr_enabled);
            bool rcas_enabled = Config::getRcasEnabled();
            if (ImGui::Checkbox("RCAS", &rcas_enabled))
                Config::setRcasEnabled(rcas_enabled);

            ImGui::BeginDisabled(!rcas_enabled);
            if (presenter) {
                auto& fsr = presenter->GetFsrSettingsRef();
                static float rcas_float =
                    static_cast<float>(Config::getRcasAttenuation()) / 1000.0f;
                if (ImGui::SliderFloat("Sharpness", &rcas_float, 0.0f, 3.0f, "%.2f")) {
                    fsr.rcasAttenuation = rcas_float;
                    Config::setRcasAttenuation(static_cast<int>(rcas_float * 1000));
                }
            }
            ImGui::EndDisabled();
            ImGui::EndDisabled();

            ImGui::SeparatorText("Present Mode");
            struct PresentModeOption {
                const char* label;
                const char* key;
            };
            static const char* presentModes[] = {
                "Mailbox (Vsync)",
                "Fifo (Vsync)",
                "Immediate (No Vsync)",
            };
            static const char* presentModeKeys[] = {
                "Mailbox",
                "Fifo",
                "Immediate",
            };
            int presentModeIndex = 0;
            for (int i = 0; i < IM_ARRAYSIZE(presentModeKeys); i++) {
                if (Config::getPresentMode() == presentModeKeys[i]) {
                    presentModeIndex = i;
                    break;
                }
            }
            if (ImGui::Combo("Mode", &presentModeIndex, presentModes, IM_ARRAYSIZE(presentModes))) {
                Config::setPresentMode(presentModeKeys[presentModeIndex]);
            }

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Advanced")) {
            ImGui::Spacing();
            ImGui::SeparatorText("Readbacks");
            static const char* readbackOptions[] = {"Disable", "Unsafe", "Low", "Default", "Fast"};
            static int readbackIndex = static_cast<int>(Config::readbackSpeed());
            if (ImGui::Combo("Speed", &readbackIndex, readbackOptions,
                             IM_ARRAYSIZE(readbackOptions))) {
                Config::setReadbackSpeed(static_cast<Config::ReadbackSpeed>(readbackIndex));
            }
            bool lr = Config::getReadbackLinearImages();
            if (ImGui::Checkbox("Linear Readbacks", &lr))
                Config::setReadbackLinearImages(lr);

            bool dma = Config::directMemoryAccess();
            if (ImGui::Checkbox("DMA Access", &dma))
                Config::setDirectMemoryAccess(dma);

            ImGui::SeparatorText("Memory");
            int extra_memory = Config::getExtraDmemInMbytes();
            if (ImGui::InputInt("Extra Mem (MB)", &extra_memory, 128, 512)) {
                if (extra_memory < 0)
                    extra_memory = 0;
                if (extra_memory > 9999)
                    extra_memory = 9999;
                Config::setExtraDmemInMbytes(extra_memory);
            }
            bool ss = Config::getShaderSkipsEnabled();
            if (ImGui::Checkbox("Shader Skips", &ss))
                Config::setShaderSkipsEnabled(ss);

            ImGui::SeparatorText("Logging");
            static const char* logTypes[] = {"sync", "async"};
            int logTypeIndex = (Config::getLogType() == "async") ? 1 : 0;
            if (ImGui::Combo("Log Type", &logTypeIndex, logTypes, IM_ARRAYSIZE(logTypes)))
                Config::setLogType(logTypes[logTypeIndex]);

            if (filter_buf[0] == '\0') {
                std::string current_filter = Config::getLogFilter();
                strncpy(filter_buf, current_filter.c_str(), sizeof(filter_buf) - 1);
            }
            if (ImGui::InputText("Log Filter", filter_buf, sizeof(filter_buf),
                                 ImGuiInputTextFlags_CallbackAlways,
                                 [](ImGuiInputTextCallbackData*) {
                                     show_virtual_keyboard = true;
                                     return 0;
                                 })) {
                Config::setLogFilter(std::string(filter_buf));
            }

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Change Game")) {
            ImGui::Spacing();

            struct PauseMenuGame {
                std::string title;
                std::filesystem::path ebootPath;
                ImGui::RefCountedTexture iconTex;
            };
            static std::vector<PauseMenuGame> pauseGameList;
            static bool pauseGameListLoaded = false;

            if (!pauseGameListLoaded) {
                pauseGameList.clear();

                auto gameDirs = Config::getGameDirectories();
                auto enabledDirs = Config::getGameDirectoriesEnabled();

                auto updateChecker =
                    [](const std::string& sceItem,
                       const std::filesystem::path& gameFolder) -> std::filesystem::path {
                    auto updateFolder = gameFolder;
                    updateFolder += "-UPDATE";
                    auto patchFolder = gameFolder;
                    patchFolder += "-patch";
                    if (std::filesystem::exists(updateFolder / "sce_sys" / sceItem))
                        return updateFolder / "sce_sys" / sceItem;
                    if (std::filesystem::exists(patchFolder / "sce_sys" / sceItem))
                        return patchFolder / "sce_sys" / sceItem;
                    return gameFolder / "sce_sys" / sceItem;
                };

                for (size_t i = 0; i < gameDirs.size(); i++) {
                    const auto& installLoc = gameDirs[i];
                    bool enabled = (i < enabledDirs.size()) ? enabledDirs[i] : true;
                    if (!enabled || !std::filesystem::exists(installLoc))
                        continue;

                    for (const auto& entry : std::filesystem::directory_iterator(installLoc)) {
                        std::string folderName = entry.path().filename().string();
                        if (folderName.ends_with("-UPDATE") || folderName.ends_with("-patch") ||
                            !entry.is_directory())
                            continue;

                        std::filesystem::path ebootPath = entry.path() / "eboot.bin";
                        if (!std::filesystem::exists(ebootPath))
                            continue;

                        PauseMenuGame game;
                        game.ebootPath = ebootPath;

                        PSF psf;
                        auto sfoPath = updateChecker("param.sfo", entry.path());
                        if (psf.Open(sfoPath)) {
                            if (auto title = psf.GetString("TITLE"); title.has_value())
                                game.title = *title;
                            else
                                game.title = folderName;
                        } else {
                            game.title = folderName;
                        }

                        auto iconPath = updateChecker("icon0.png", entry.path());
                        if (std::filesystem::exists(iconPath)) {
                            game.iconTex = ImGui::RefCountedTexture::DecodePngFile(iconPath);
                        }

                        pauseGameList.push_back(std::move(game));
                    }
                }
                pauseGameListLoaded = true;
            }

            ImGui::TextDisabled("Select a game below to switch. Current game will restart.");
            ImGui::Separator();
            ImGui::Spacing();

            if (pauseGameList.empty()) {
                ImGui::TextColored(ImVec4(1, 0.6f, 0.2f, 1),
                                   "No games found in configured game directories.");
            } else {
                const float iconSize = 80.0f;
                const float itemPadding = 12.0f;
                const float itemWidth = iconSize + itemPadding * 2.0f + 100.0f;
                float availWidth = ImGui::GetContentRegionAvail().x;
                int cols = std::max(1, (int)(availWidth / (iconSize + 160.0f)));

                ImGui::BeginChild("GameListScrollRegion", ImVec2(0, -30), true);

                for (int i = 0; i < (int)pauseGameList.size(); i++) {
                    const auto& game = pauseGameList[i];

                    ImGui::PushID(i);
                    ImGui::BeginGroup();

                    if (game.iconTex) {
                        auto img = game.iconTex.GetTexture();
                        if (ImGui::ImageButton("##icon", img.im_id, ImVec2(iconSize, iconSize))) {
                            auto* emulator = Common::Singleton<::Core::Emulator>::Instance();
                            emulator->Restart(game.ebootPath, {});
                        }
                    } else {
                        if (ImGui::Button("?", ImVec2(iconSize, iconSize))) {
                            auto* emulator = Common::Singleton<::Core::Emulator>::Instance();
                            emulator->Restart(game.ebootPath, {});
                        }
                    }
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("Launch: %s", game.title.c_str());
                    }

                    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + iconSize);
                    ImGui::TextWrapped("%s", game.title.c_str());
                    ImGui::PopTextWrapPos();

                    ImGui::EndGroup();
                    ImGui::PopID();

                    bool lastInRow = ((i + 1) % cols == 0) || (i + 1 == (int)pauseGameList.size());
                    if (!lastInRow) {
                        ImGui::SameLine(0.0f, itemPadding);
                    } else {
                        ImGui::Dummy(ImVec2(0.0f, itemPadding));
                    }
                }
                ImGui::EndChild();

                if (ImGui::SmallButton("Refresh Game List")) {
                    pauseGameListLoaded = false;
                }
            }

            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::Dummy(ImVec2(0, 10));
    ImGui::Separator();

    auto SaveConfig = [&](const std::filesystem::path& path) {
        Config::setLogFilter(std::string(filter_buf));
        Config::save(path);
    };

    if (ImGui::Button("Save Config...", ImVec2(150, 0)))
        ImGui::OpenPopup("Save Config As");

    if (ImGui::BeginPopupModal("Save Config As", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Where do you want to save the changes?");
        ImGui::Separator();

        if (ImGui::Button("Global Config (config.toml)", ImVec2(250, 40))) {
            SaveConfig(Common::FS::GetUserPath(Common::FS::PathType::UserDir) / "config.toml");
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::Button("Per-Game Config", ImVec2(250, 40))) {
            if (!MemoryPatcher::g_game_serial.empty()) {
                SaveConfigWithOverrides(
                    Common::FS::GetUserPath(Common::FS::PathType::CustomConfigs) /
                        (MemoryPatcher::g_game_serial + ".toml"),
                    true);
            }
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 40)))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }

    ImGui::SameLine();
    if (ImGui::Button("Save & Restart", ImVec2(150, 0)))
        ImGui::OpenPopup("Save Config As Restart Emulator");

    if (ImGui::BeginPopupModal("Save Config As Restart Emulator", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Where do you want to save the changes?");
        ImGui::Separator();

        if (ImGui::Button("Global Config (config.toml)", ImVec2(250, 40))) {
            SaveConfig(Common::FS::GetUserPath(Common::FS::PathType::UserDir) / "config.toml");
            SDL_Event event{};
            event.type = SDL_EVENT_QUIT + 1;
            SDL_PushEvent(&event);
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::Button("Per-Game Config", ImVec2(250, 40))) {
            if (!MemoryPatcher::g_game_serial.empty()) {
                SaveConfigWithOverrides(
                    Common::FS::GetUserPath(Common::FS::PathType::CustomConfigs) /
                        (MemoryPatcher::g_game_serial + ".toml"),
                    true);
            }
            SDL_Event event{};
            event.type = SDL_EVENT_QUIT + 1;
            SDL_PushEvent(&event);
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 40)))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }

    ImGui::End();
    DrawVirtualKeyboard();
    should_focus = false;
}

void L::Draw() {
    static bool theme_applied = false;
    if (!theme_applied) {
        SetupTheme();
        theme_applied = true;
    }

    u8 pad = 0;
    const auto io = GetIO();
    PushID("DevtoolsLayer");
    const bool blockHardcoded = Config::DisableHardcodedHotkeys();
    const bool useHomeButtonForHotkeys = Config::UseHomeButtonForHotkeys();
    Btn modifierButton = useHomeButtonForHotkeys ? Btn::Home : Btn::TouchPad;

    static bool showPauseHelpWindow = true;
    if (Config::getPauseOnUnfocus()) {

        if (!(SDL_GetWindowFlags(g_window->GetSDLWindow()) & SDL_WINDOW_INPUT_FOCUS)) {
            DrawPauseStatusWindow(showPauseHelpWindow);
        }
    }

    if (!blockHardcoded) {
        if (IsKeyPressed(ImGuiKey_F3, false)) {
            show_fullscreen_tip = !show_fullscreen_tip;
            fullscreen_tip_manual = true;
        }
    }

    if (!blockHardcoded) {
        if (IsKeyPressed(ImGuiKey_F2, false)) {
            showTrophyViewer = !showTrophyViewer;
        }
    }
    if (!blockHardcoded) {
        if (Input::ControllerComboPressedOnce(pad, modifierButton, Btn::Left)) {
            showTrophyViewer = !showTrophyViewer;
        }
    }

    const bool userQuitKeyboard = Input::HasUserHotkeyDefined(pad, Input::HotkeyPad::QuitPad,
                                                              Input::HotkeyInputType::Keyboard);
    const bool userQuitController = Input::HasUserHotkeyDefined(pad, Input::HotkeyPad::QuitPad,
                                                                Input::HotkeyInputType::Controller);

    // Emulated pad already handles the swap logic physically.
    // We bind statically to the logical intent.
    const Btn cancel_btn = Btn::Circle;

    if (!userQuitController && !blockHardcoded) {
        if (Input::ControllerComboPressedOnce(pad, modifierButton, cancel_btn)) {
            Overlay::ToggleQuitWindow();
            visibility_toggled = true;
        }
    }

    if (!userQuitKeyboard) {
        if (IsKeyPressed(ImGuiKey_Escape, false)) {
            Overlay::ToggleQuitWindow();
            visibility_toggled = true;
        }
    }

    const bool userPauseKeyboard = Input::HasUserHotkeyDefined(pad, Input::HotkeyPad::PausePad,
                                                               Input::HotkeyInputType::Keyboard);
    const bool userPauseController = Input::HasUserHotkeyDefined(
        pad, Input::HotkeyPad::PausePad, Input::HotkeyInputType::Controller);

    if (!userPauseKeyboard) {
        if (IsKeyPressed(ImGuiKey_F9, false)) {
            SDL_Event e;
            e.type = SDL_EVENT_TOGGLE_PAUSE;
            SDL_PushEvent(&e);
        }
    }

    const Btn confirm_btn = Btn::Cross;

    if (!userPauseController && !blockHardcoded) {
        if (Input::ControllerComboPressedOnce(pad, modifierButton, confirm_btn)) {
            SDL_Event e;
            e.type = SDL_EVENT_TOGGLE_PAUSE;
            SDL_PushEvent(&e);
        }
    }

    if (IsKeyPressed(ImGuiKey_F10, false)) {
        if (io.KeyCtrl) {
            DebugState.IsShowingDebugMenuBar() ^= true;
        }
        visibility_toggled = true;
    }

    const bool userFpsKeyboard = Input::HasUserHotkeyDefined(pad, Input::HotkeyPad::SimpleFpsPad,
                                                             Input::HotkeyInputType::Keyboard);
    const bool userFpsController = Input::HasUserHotkeyDefined(pad, Input::HotkeyPad::SimpleFpsPad,
                                                               Input::HotkeyInputType::Controller);

    if (!userFpsController && !blockHardcoded) {
        if (Input::ControllerComboPressedOnce(pad, modifierButton, Btn::L2)) {
            show_simple_fps = !show_simple_fps;
            visibility_toggled = true;
        }
    }

    if (!userFpsKeyboard) {
        if (IsKeyPressed(ImGuiKey_F10, false)) {
            show_simple_fps = !show_simple_fps;
            visibility_toggled = true;
        }
    }

    const bool userFullscreenKeyboard = Input::HasUserHotkeyDefined(
        pad, Input::HotkeyPad::FullscreenPad, Input::HotkeyInputType::Keyboard);
    const bool userFullscreenController = Input::HasUserHotkeyDefined(
        pad, Input::HotkeyPad::FullscreenPad, Input::HotkeyInputType::Controller);

    if (!userFullscreenController && !blockHardcoded) {
        if (Input::ControllerComboPressedOnce(pad, modifierButton, Btn::R2)) {
            SDL_Event toggleFullscreenEvent;
            toggleFullscreenEvent.type = SDL_EVENT_TOGGLE_FULLSCREEN;
            SDL_PushEvent(&toggleFullscreenEvent);
        }
    }

    if (!userFullscreenKeyboard) {
        if (IsKeyPressed(ImGuiKey_F11, false)) {
            SDL_Event toggleFullscreenEvent;
            toggleFullscreenEvent.type = SDL_EVENT_TOGGLE_FULLSCREEN;
            SDL_PushEvent(&toggleFullscreenEvent);
        }
    }
    if (!blockHardcoded) {

        if (Input::ControllerComboPressedOnce(pad, modifierButton, Btn::Triangle)) {
            show_fullscreen_tip = !show_fullscreen_tip;
            fullscreen_tip_manual = true;
        }
    }

#ifdef ENABLE_QT_GUI
    if (!blockHardcoded) {

        if (Input::ControllerComboPressedOnce(pad, modifierButton, Btn::Right)) {
            if (g_MainWindow)
                g_MainWindow->ToggleMute();
        }
        if (IsKeyPressed(ImGuiKey_F1, false) && !Config::hasCustomMuteHotkey()) {
            if (g_MainWindow)
                g_MainWindow->ToggleMute();
        }
    }
#endif
    if (!blockHardcoded) {

        const bool show_debug_menu_combo =
            Input::ControllerComboPressedOnce(pad, modifierButton, Btn::Up);
        if (show_debug_menu_combo) {
            DebugState.IsShowingDebugMenuBar() ^= true;
            visibility_toggled = true;
        }
    }

    if (!DebugState.IsGuestThreadsPaused()) {
        const auto fn = DebugState.flip_frame_count.load();
        frame_graph.AddFrame(fn, DebugState.FrameDeltaTime);
    }

    if (!fullscreen_tip_manual) {
        if (!Config::getScreenTipDisable()) {
            fullscreen_tip_timer -= DebugState.IsGuestThreadsPaused() ? 0.0f : io.DeltaTime;
            if (fullscreen_tip_timer <= 0.0f) {
                show_fullscreen_tip = false;
                fullscreen_tip_timer = 10.0f;
            }
        } else {
            show_fullscreen_tip = false;
        }
    }

    if (!show_hotkeys_tip_manual) {
        if (!Config::getScreenTipDisable()) {
            hotkeys_tip_timer -= DebugState.IsGuestThreadsPaused() ? 0.0f : io.DeltaTime;
            if (hotkeys_tip_timer <= 0.0f) {
                show_hotkeys_tip = false;
                hotkeys_tip_timer = 10.0f;
            }
        } else {
            show_hotkeys_tip = false;
        }
    }

    if (show_hotkeys_tip || show_hotkeys_tip_manual)
        DrawFullscreenHotkeysWindow(show_hotkeys_tip);

    if (show_fullscreen_tip || fullscreen_tip_manual)
        DrawFullscreenSettingsWindow(show_fullscreen_tip);

    if (!show_quit_window && DebugState.IsGuestThreadsPaused()) {
        DrawPauseStatusWindow(showPauseHelpWindow);
        DrawFullscreenHotkeysPause(show_hotkeys_pause);
    }

    if (show_simple_fps) {
        if (Begin("Video Info", nullptr,
                  ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDecoration |
                      ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDocking)) {
            if (const float width = GetIO().DisplaySize.x; width != fps_anchor_width) {
                visibility_toggled |= GetWindowPos().x + GetCurrentWindowRead()->SizeFull.x >=
                                      fps_anchor_width - 1.0f;
                fps_anchor_width = width;
            }
            // Set window position to top left if it was toggled on
            if (visibility_toggled) {
                SetWindowPos("Video Info", {999999.0f, 0.0f}, ImGuiCond_Always);
                visibility_toggled = false;
            }
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

    if (DebugState.IsShowingDebugMenuBar()) {
        PushFont(io.Fonts->Fonts[IMGUI_FONT_MONO]);
        PushID("DevtoolsLayer");
        DrawAdvanced();
        PopID();
        PopFont();
    }

    int menu_count = 4;

    const char* options[] = {
        "Exit Game",
        "Minimize Game",
        "Quit Emulator",
        "Cancel",
    };

    if (show_quit_window) {

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        ImDrawList* draw_list = ImGui::GetForegroundDrawList();
        ImVec2 viewport_size = ImGui::GetMainViewport()->Size;
        draw_list->AddRectFilled(ImVec2(0, 0), viewport_size, IM_COL32(0, 0, 0, 160));

        ImGui::SetNextWindowSize(ImVec2(400, 0));
        if (ImGui::Begin("Controller Exit Menu", nullptr,
                         ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDecoration |
                             ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDocking)) {

            ImGui::SetWindowFontScale(1.5f);
            L::TextCentered("QUIT MENU");
            ImGui::SetWindowFontScale(1.0f);
            ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::IsKeyPressed(ImGuiKey_DownArrow, true) ||
                ImGui::IsKeyPressed(ImGuiKey_GamepadDpadDown, true) ||
                ImGui::IsKeyPressed(ImGuiKey_GamepadLStickDown, true)) {
                quit_menu_selection = (quit_menu_selection + 1) % menu_count;
            }

            if (ImGui::IsKeyPressed(ImGuiKey_UpArrow, true) ||
                ImGui::IsKeyPressed(ImGuiKey_GamepadDpadUp, true) ||
                ImGui::IsKeyPressed(ImGuiKey_GamepadLStickUp, true)) {
                quit_menu_selection = (quit_menu_selection + menu_count - 1) % menu_count;
            }

            for (int i = 0; i < menu_count; i++) {
                if (i == quit_menu_selection) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.9f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
                    if (ImGui::Button(options[i], ImVec2(-1, 45))) {
                    }
                    ImGui::PopStyleColor(2);
                } else {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.18f, 1.0f));
                    if (ImGui::Button(options[i], ImVec2(-1, 45))) {
                        quit_menu_selection = i;
                    }
                    ImGui::PopStyleColor();
                }
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::SetWindowFontScale(0.9f);

            const bool swap_x_circle_menu = Config::getXCircleButtonSwap();
            std::string select_btn = swap_x_circle_menu ? "Circle/B" : "Cross/A";
            std::string cancel_hint = "Press " + select_btn + " to Select | Triangle / Y to Cancel";

            L::TextCentered(cancel_hint);
            ImGui::SetWindowFontScale(1.0f);

            const ImGuiKey confirm_button = ImGuiKey_GamepadFaceDown;

            if (ImGui::IsKeyPressed(ImGuiKey_Enter, false) ||
                ImGui::IsKeyPressed(confirm_button, false)) {

                switch (quit_menu_selection) {
                case 0: {
                    SDL_Event event{};
                    SDL_memset(&event, 0, sizeof(event));
                    event.type = SDL_EVENT_QUIT;
                    SDL_PushEvent(&event);
                    break;
                }

                case 1: {
                    if (g_window && g_window->GetSDLWindow())
                        SDL_MinimizeWindow(g_window->GetSDLWindow());
                    show_quit_window = false;
                    break;
                }
                case 2: {
#ifdef Q_OS_WIN
                    QProcess::startDetached("taskkill", QStringList()
                                                            << "/IM" << "shadPS4.exe" << "/F");
#elif defined(Q_OS_LINUX)
                    QProcess::startDetached("pkill", QStringList() << "Shadps4-qt");
#elif defined(Q_OS_MACOS)
                    QProcess::startDetached("pkill", QStringList() << "shadps4");
#endif
                    break;
                }

                case 3: {
                    Overlay::ToggleQuitWindow();
                    break;
                }
                }
            }
            if (ImGui::IsKeyPressed(ImGuiKey_Escape, false) ||
                ImGui::IsKeyPressed(ImGuiKey_GamepadFaceUp, false)) {
                Overlay::ToggleQuitWindow();
            }
            ImGui::End();
        }
    }

    if (showTrophyViewer) {
        ImGuiIO& io = ImGui::GetIO();
        static bool trophy_focus = false;
        static ImVec2 trophy_pos = ImVec2(200, 200);
        static ImVec2 trophy_size = ImVec2(600, 400);
        static float move_speed = 1.0f;
        static float resize_speed = 1.0f;
        static TrophyGameData cachedTrophies;
        static bool dataLoaded = false;

        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        bool using_controller = ImGui::IsKeyDown(ImGuiKey_GamepadL1);

        if (Input::ControllerPressedOnce(pad, {Btn::Up}) ||
            Input::ControllerPressedOnce(pad, {Btn::Down})) {
            trophy_focus = true;
            ImGui::SetWindowFocus("Quick Trophy List Viewer");
        }
        if (!trophy_focus && using_controller) {
            ImGui::ClearActiveID();
        }

        if (using_controller) {
            float lx = 0, ly = 0;

            if (ImGui::IsKeyDown(ImGuiKey_GamepadLStickRight))
                lx = 1;
            if (ImGui::IsKeyDown(ImGuiKey_GamepadLStickLeft))
                lx = -1;
            if (ImGui::IsKeyDown(ImGuiKey_GamepadLStickDown))
                ly = 1;
            if (ImGui::IsKeyDown(ImGuiKey_GamepadLStickUp))
                ly = -1;

            trophy_pos.x += lx * move_speed;
            trophy_pos.y += ly * move_speed;

            float rx = 0, ry = 0;

            if (ImGui::IsKeyDown(ImGuiKey_GamepadRStickRight))
                rx = 1;
            if (ImGui::IsKeyDown(ImGuiKey_GamepadRStickLeft))
                rx = -1;
            if (ImGui::IsKeyDown(ImGuiKey_GamepadRStickDown))
                ry = 1;
            if (ImGui::IsKeyDown(ImGuiKey_GamepadRStickUp))
                ry = -1;

            trophy_size.x = ImClamp(trophy_size.x + rx * resize_speed, 300.0f, 3000.0f);
            trophy_size.y = ImClamp(trophy_size.y + ry * resize_speed, 300.0f, 3000.0f);
        }

        ImGui::SetNextWindowPos(trophy_pos,
                                using_controller ? ImGuiCond_Always : ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(trophy_size,
                                 using_controller ? ImGuiCond_Always : ImGuiCond_FirstUseEver);

        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar |
                                       ImGuiWindowFlags_NoFocusOnAppearing |
                                       ImGuiWindowFlags_NoTitleBar;

        if (ImGui::Begin("Quick Trophy List Viewer", nullptr, windowFlags)) {

            if (!using_controller) {
                trophy_pos = ImGui::GetWindowPos();
                trophy_size = ImGui::GetWindowSize();
            }

            if (!dataLoaded) {
                cachedTrophies.files.clear();
                cachedTrophies.totalCount = 0;
                cachedTrophies.unlockedCount = 0;
                std::string gameSerial = MemoryPatcher::g_game_serial;

                if (!gameSerial.empty()) {
                    std::filesystem::path metaDir =
                        Common::FS::GetUserPath(Common::FS::PathType::MetaDataDir) / gameSerial /
                        "TrophyFiles";

                    if (std::filesystem::exists(metaDir)) {
                        for (auto& dirEntry : std::filesystem::directory_iterator(metaDir)) {
                            if (!dirEntry.is_directory())
                                continue;

                            std::string xmlPath = (dirEntry.path() / "Xml/TROP.XML").string();
                            if (!std::filesystem::exists(xmlPath))
                                continue;

#ifdef ENABLE_QT_GUI
                            QFile file(QString::fromStdString(xmlPath));
                            if (file.open(QFile::ReadOnly | QFile::Text)) {
                                CachedTrophyFile currentFile;
                                currentFile.id = dirEntry.path().filename().string();

                                QXmlStreamReader reader(&file);
                                while (!reader.atEnd() && !reader.hasError()) {
                                    reader.readNext();
                                    if (reader.isStartElement() &&
                                        reader.name().toString() == "trophy") {
                                        CachedTrophy t;
                                        t.unlocked = false;
                                        cachedTrophies.totalCount++;
                                        t.id = reader.attributes()
                                                   .value("id")
                                                   .toString()
                                                   .toStdString();
                                        t.trophyType = reader.attributes()
                                                           .value("ttype")
                                                           .toString()
                                                           .toStdString();

                                        if (reader.attributes().hasAttribute("unlockstate") &&
                                            reader.attributes().value("unlockstate").toString() ==
                                                "true") {
                                            t.unlocked = true;
                                            cachedTrophies.unlockedCount++;
                                        }

                                        while (!(reader.isEndElement() &&
                                                 reader.name().toString() == "trophy")) {
                                            reader.readNext();
                                            if (reader.isStartElement() &&
                                                reader.name().toString() == "name") {
                                                t.name = reader.readElementText().toStdString();
                                            }
                                        }
                                        currentFile.trophies.push_back(t);
                                    }
                                }
                                for (auto& trophy : currentFile.trophies) {
                                    std::string iconFileName = "TROP" + trophy.id + ".PNG";
                                    std::filesystem::path iconsDir = dirEntry.path() / "Icons";
                                    trophy.iconPath = iconsDir / iconFileName;
                                    if (std::filesystem::exists(trophy.iconPath)) {
                                        trophy.iconTex = ImGui::RefCountedTexture::DecodePngFile(
                                            trophy.iconPath);
                                    }
                                    std::string gradeFile;
                                    if (trophy.trophyType == "P")
                                        gradeFile = "src/images/platinum.png";
                                    else if (trophy.trophyType == "G")
                                        gradeFile = "src/images/gold.png";
                                    else if (trophy.trophyType == "S")
                                        gradeFile = "src/images/silver.png";
                                    else
                                        gradeFile = "src/images/bronze.png";
                                    auto resource = cmrc::res::get_filesystem();
                                    try {
                                        auto resFile = resource.open(gradeFile);
                                        std::vector<u8> imgdata(resFile.begin(), resFile.end());
                                        trophy.typeTex = ImGui::RefCountedTexture::DecodePngTexture(
                                            std::move(imgdata));
                                    } catch (...) {
                                    }
                                }
                                cachedTrophies.files.push_back(std::move(currentFile));
                            }
#endif
                        }
                    }
                }
                dataLoaded = true;
            }

            ImGui::SetWindowFontScale(1.5f);
#ifdef ENABLE_QT_GUI
            L::TextCentered(("Trophies (" + std::to_string(cachedTrophies.unlockedCount) + "/" +
                             std::to_string(cachedTrophies.totalCount) + ")")
                                .c_str());
#else
            L::TextCentered("SDL build can't read trophy XML, use QT");
#endif
            ImGui::SetWindowFontScale(1.0f);
            ImGui::Separator();

#ifdef ENABLE_QT_GUI
            if (cachedTrophies.files.empty()) {
                ImGui::Text("No trophy data found.");
            } else {
                ImGui::BeginChild("TrophyListFull", ImVec2(0, -40), true);
                for (const auto& fileData : cachedTrophies.files) {
                    if (ImGui::CollapsingHeader(fileData.id.c_str(),
                                                ImGuiTreeNodeFlags_DefaultOpen)) {
                        if (ImGui::BeginTable("TrophyTableFull", 4,
                                              ImGuiTableFlags_RowBg |
                                                  ImGuiTableFlags_BordersInnerV |
                                                  ImGuiTableFlags_ScrollY)) {
                            ImGui::TableSetupColumn("##grade", ImGuiTableColumnFlags_WidthFixed,
                                                    40.0f);
                            ImGui::TableSetupColumn("##icon", ImGuiTableColumnFlags_WidthFixed,
                                                    56.0f);
                            ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed,
                                                    70.0f);
                            ImGui::TableSetupColumn("Trophy Name",
                                                    ImGuiTableColumnFlags_WidthStretch);
                            ImGui::TableHeadersRow();

                            for (const auto& trophy : fileData.trophies) {
                                ImGui::TableNextRow(ImGuiTableRowFlags_None, 52.0f);

                                ImGui::TableSetColumnIndex(0);
                                if (trophy.typeTex) {
                                    auto typeImg = trophy.typeTex.GetTexture();
                                    ImGui::Image(typeImg.im_id, ImVec2(36.0f, 36.0f));
                                }

                                ImGui::TableSetColumnIndex(1);
                                if (trophy.iconTex) {
                                    auto img = trophy.iconTex.GetTexture();
                                    if (!trophy.unlocked)
                                        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.35f);
                                    ImGui::Image(img.im_id, ImVec2(50.0f, 50.0f));
                                    if (!trophy.unlocked)
                                        ImGui::PopStyleVar();
                                }

                                ImGui::TableSetColumnIndex(2);
                                if (trophy.unlocked) {
                                    ImGui::PushStyleColor(ImGuiCol_Text,
                                                          ImVec4(0.2f, 1.0f, 0.3f, 1.0f));
                                    ImGui::TextUnformatted("Unlocked");
                                    ImGui::PopStyleColor();
                                } else {
                                    ImGui::PushStyleColor(ImGuiCol_Text,
                                                          ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
                                    ImGui::TextUnformatted("Locked");
                                    ImGui::PopStyleColor();
                                }

                                ImGui::TableSetColumnIndex(3);
                                ImGui::TextWrapped("%s", trophy.name.c_str());
                            }
                            ImGui::EndTable();
                        }
                    }
                }
                ImGui::EndChild();
            }
#endif

            if (ImGui::Button("Close", ImVec2(120, 0))) {
                showTrophyViewer = false;
                trophy_focus = false;
            }

            ImGui::End();
        }
    }

    if (show_volume) {
        float current_time = ImGui::GetTime();
        float elapsed_time = current_time - volume_start_time;
        const float animation_duration = 0.3f;
        const float display_duration = 3.0f;

        if (elapsed_time >= display_duration) {
            show_volume = false;
        } else {
            float anim_progress = std::min(elapsed_time / animation_duration, 1.0f);

            const ImVec2 popup_size = ImVec2(200.0f, 60.0f);

            const ImVec2 base_pos =
                ImVec2(ImGui::GetMainViewport()->WorkPos.x +
                           (ImGui::GetMainViewport()->WorkSize.x - popup_size.x) * 0.5f,
                       ImGui::GetMainViewport()->WorkPos.y + ImGui::GetMainViewport()->WorkSize.y -
                           popup_size.y - 20.0f);

            const float y_offset = -20.0f * anim_progress;
            const ImVec2 popup_pos = ImVec2(base_pos.x, base_pos.y + y_offset);

            SetNextWindowPos(popup_pos, ImGuiCond_Always);
            SetNextWindowSize(popup_size, ImGuiCond_Always);

            if (ImGui::Begin("Volume Window", &show_volume,
                             ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDecoration |
                                 ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDocking |
                                 ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar)) {

                const float alpha = std::min(anim_progress * 3.0f, 1.0f);
                const ImU32 bg_color = IM_COL32(20, 20, 20, static_cast<ImU32>(alpha * 200));
                const ImU32 border_color = IM_COL32(100, 100, 100, static_cast<ImU32>(alpha * 255));
                const ImU32 text_color = IM_COL32(255, 255, 255, static_cast<ImU32>(alpha * 255));
                const ImU32 mute_color = IM_COL32(255, 100, 100, static_cast<ImU32>(alpha * 255));

                ImDrawList* draw_list = GetWindowDrawList();
                const ImVec2 window_min = GetWindowPos();
                const ImVec2 window_max =
                    ImVec2(window_min.x + popup_size.x, window_min.y + popup_size.y);

                draw_list->AddRectFilled(window_min, window_max, bg_color, 8.0f);
                draw_list->AddRect(window_min, window_max, border_color, 8.0f, 0, 2.0f);

                const int volume = Config::getVolumeSlider();
                const bool is_muted = Config::isMuteEnabled();

                char volume_text[32];
                if (is_muted) {
                    snprintf(volume_text, sizeof(volume_text), "Muted");
                } else {
                    snprintf(volume_text, sizeof(volume_text), "%d%%", volume);
                }

                const ImVec2 text_size = ImGui::CalcTextSize(volume_text);
                const ImVec2 text_pos =
                    ImVec2(window_min.x + (popup_size.x - text_size.x) * 0.5f,
                           window_min.y + (popup_size.y - text_size.y) * 0.5f - 10.0f);

                if (is_muted) {
                    draw_list->AddText(text_pos, mute_color, volume_text);
                } else {
                    draw_list->AddText(text_pos, text_color, volume_text);
                }

                const float bar_width = 140.0f;
                const float bar_height = 6.0f;
                const float bar_x = window_min.x + (popup_size.x - bar_width) * 0.5f;
                const float bar_y = window_min.y + popup_size.y - 25.0f;

                const ImVec2 bar_bg_min = ImVec2(bar_x, bar_y);
                const ImVec2 bar_bg_max = ImVec2(bar_x + bar_width, bar_y + bar_height);
                draw_list->AddRectFilled(bar_bg_min, bar_bg_max,
                                         IM_COL32(60, 60, 60, static_cast<ImU32>(alpha * 255)),
                                         3.0f);

                if (!is_muted && volume > 0) {
                    const float fill_width = (static_cast<float>(volume) / 300.0f) * bar_width;
                    const ImVec2 bar_fill_min = ImVec2(bar_x, bar_y);
                    const ImVec2 bar_fill_max = ImVec2(bar_x + fill_width, bar_y + bar_height);
                    draw_list->AddRectFilled(
                        bar_fill_min, bar_fill_max,
                        IM_COL32(100, 200, 100, static_cast<ImU32>(alpha * 255)), 3.0f);
                }

                draw_list->AddRect(bar_bg_min, bar_bg_max,
                                   IM_COL32(120, 120, 120, static_cast<ImU32>(alpha * 255)), 3.0f,
                                   0, 1.0f);
            }
            End();
        }
    }

    PopID();
}