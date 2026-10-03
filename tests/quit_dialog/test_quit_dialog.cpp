// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstdlib>
#include <iostream>
#include <string_view>
#include "core/devtools/quit_dialog.h"

using Core::Devtools::QuitDialog;
static QuitDialog quit_dialog;
static SDL_Event next_event{};
static int guest_events = 0;
static int raw_events = 0;
static int imgui_events = 0;
static int quit_events = 0;
static bool imgui_wants_event = false;

static void Check(bool value, const char* message) {
    if (!value) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

extern "C" bool SDL_WaitEvent(SDL_Event* event) {
    *event = next_event;
    return true;
}
extern "C" bool SDL_PushEvent(SDL_Event* event) {
    Check(event->type == SDL_EVENT_QUIT, "only the normal SDL quit event may be posted");
    ++quit_events;
    return true;
}
extern "C" const char* SDL_GetError() {
    return "test";
}

#define LOG_INFO(...) ((void)0)
#define LOG_ERROR(...) ((void)0)
namespace Overlay {
#include "quit_actions.inc"
}
namespace Libraries::Mouse {
bool PushSDLEvent(const SDL_Event&) {
    ++raw_events;
    return false;
}
} // namespace Libraries::Mouse
namespace Libraries::Keyboard {
bool PushSDLEvent(const SDL_Event&) {
    ++raw_events;
    return false;
}
} // namespace Libraries::Keyboard
namespace ImGui::Core {
static std::atomic<std::uint32_t> force_gamepad_input_capture_count{0};
#include "pad_capture.inc"
bool ProcessEvent(SDL_Event*) {
    ++imgui_events;
    return imgui_wants_event;
}
} // namespace ImGui::Core

struct WindowSDL {
    void WaitEvent();
};
#include "event_dispatch.inc"

static void Send(SDL_Event event) {
    next_event = event;
    WindowSDL{}.WaitEvent();
}
static void Button(SDL_JoystickID id, Uint8 button, bool down) {
    SDL_Event event{};
    event.type = down ? SDL_EVENT_GAMEPAD_BUTTON_DOWN : SDL_EVENT_GAMEPAD_BUTTON_UP;
    event.gbutton.which = id;
    event.gbutton.button = button;
    Send(event);
}
static void Key(SDL_Keycode key, SDL_Scancode scan, bool down, bool repeat = false) {
    SDL_Event event{};
    event.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
    event.key.key = key;
    event.key.scancode = scan;
    event.key.repeat = repeat;
    Send(event);
}
static void Open(SDL_JoystickID id = 11) {
    Button(id, SDL_GAMEPAD_BUTTON_GUIDE, true);
    Check(quit_dialog.IsVisible(), "Guide must open without waiting for an ImGui frame");
    Check(ImGui::Core::IsGamepadInputCaptured(), "open prompt must intercept guest pad reads");
    Button(id, SDL_GAMEPAD_BUTTON_GUIDE, false);
    Check(quit_dialog.IsVisible(), "Guide release must not close the prompt");
}

int main(int argc, char** argv) {
    Check(argc == 2, "expected a test case");
    const std::string_view name = argv[1];
    if (name == "second_controller") {
        // Neither slot zero nor ImGui polling/focus participates in this sequence.
        imgui_wants_event = true;
        Open(11);
        Button(11, SDL_GAMEPAD_BUTTON_SOUTH, true);
        Button(11, SDL_GAMEPAD_BUTTON_SOUTH, true);
        Button(11, SDL_GAMEPAD_BUTTON_SOUTH, false);
        Check(quit_events == 1, "second controller must confirm exactly once");
        Check(raw_events == 0 && imgui_events == 0 && guest_events == 0,
              "Guide and confirmation must never reach a competing input consumer");
        Check(ImGui::Core::IsGamepadInputCaptured(), "confirmation stays intercepted until exit");
    } else if (name == "unfocused_confirm") {
        Overlay::ToggleQuitWindow();
        Button(11, SDL_GAMEPAD_BUTTON_SOUTH, true);
        Button(11, SDL_GAMEPAD_BUTTON_SOUTH, false);
        Check(quit_events == 1 && guest_events == 0,
              "Cross must confirm the visible prompt, not reach the game when NavActive is false");
    } else if (name == "cancel_resume") {
        Open();
        SDL_Event motion{};
        motion.type = SDL_EVENT_GAMEPAD_AXIS_MOTION;
        Send(motion);
        Check(guest_events == 1 && ImGui::Core::IsGamepadInputCaptured(),
              "physical state continues updating while guest pad reads are intercepted");
        Button(11, SDL_GAMEPAD_BUTTON_EAST, true);
        Check(!quit_dialog.IsVisible() && ImGui::Core::IsGamepadInputCaptured(),
              "cancel must stay intercepted until its release");
        Button(11, SDL_GAMEPAD_BUTTON_EAST, false);
        Check(!ImGui::Core::IsGamepadInputCaptured(), "release must resume ordinary pad reads");
        Check(guest_events == 1 && quit_events == 0, "cancel must not reach the game or quit it");
        Button(11, SDL_GAMEPAD_BUTTON_SOUTH, true);
        Button(11, SDL_GAMEPAD_BUTTON_SOUTH, false);
        Check(guest_events == 3, "fresh Cross press after cancel must reach the game");
        Open();
        ImGui::Core::force_gamepad_input_capture_count = 1;
        Button(11, SDL_GAMEPAD_BUTTON_GUIDE, true);
        Button(11, SDL_GAMEPAD_BUTTON_GUIDE, false);
        Check(!quit_dialog.IsVisible() && ImGui::Core::IsGamepadInputCaptured(),
              "closing quit must preserve an independent OSK capture");
        ImGui::Core::force_gamepad_input_capture_count = 0;
        Check(!ImGui::Core::IsGamepadInputCaptured(), "no quit capture should remain");
    } else if (name == "held_confirm") {
        Button(11, SDL_GAMEPAD_BUTTON_SOUTH, true);
        Open();
        Check(quit_events == 0, "Cross held before opening must not auto-confirm");
        Button(11, SDL_GAMEPAD_BUTTON_SOUTH, false);
        Check(guest_events == 2, "preexisting held input must receive its release");
        Button(11, SDL_GAMEPAD_BUTTON_SOUTH, true);
        Check(quit_events == 1 && guest_events == 2, "fresh Cross confirms and is consumed");
    } else if (name == "keyboard") {
        Key(SDLK_RETURN, SDL_SCANCODE_RETURN, true);
        Overlay::ToggleQuitWindow();
        Key(SDLK_RETURN, SDL_SCANCODE_RETURN, true, true);
        Check(quit_events == 0, "keyboard repeat must not confirm a newly opened prompt");
        Key(SDLK_RETURN, SDL_SCANCODE_RETURN, false);
        Check(guest_events == 2, "release of preexisting Enter must still clear physical state");
        Key(SDLK_ESCAPE, SDL_SCANCODE_ESCAPE, true);
        Check(!quit_dialog.IsVisible() && ImGui::Core::IsGamepadInputCaptured(), "Escape cancels");
        Key(SDLK_ESCAPE, SDL_SCANCODE_ESCAPE, false);
        Check(!ImGui::Core::IsGamepadInputCaptured(), "Escape release clears interception");
        Overlay::ToggleQuitWindow();
        Key(SDLK_KP_ENTER, SDL_SCANCODE_KP_ENTER, true);
        Key(SDLK_KP_ENTER, SDL_SCANCODE_KP_ENTER, true, true);
        Key(SDLK_KP_ENTER, SDL_SCANCODE_KP_ENTER, false);
        Check(quit_events == 1 && guest_events == 2, "keypad Enter confirms once without leakage");
    } else if (name == "hotplug") {
        Open(11);
        Button(11, SDL_GAMEPAD_BUTTON_EAST, true);
        SDL_Event removed{};
        removed.type = SDL_EVENT_GAMEPAD_REMOVED;
        removed.gdevice.which = 11;
        Send(removed);
        Check(!ImGui::Core::IsGamepadInputCaptured(), "disconnect clears a held cancel capture");
        Check(guest_events == 1, "disconnect must continue to controller housekeeping");
        Open(12);
        Button(12, SDL_GAMEPAD_BUTTON_SOUTH, true);
        Check(quit_events == 1, "replacement controller must confirm without cached SDL handles");
    } else {
        Check(false, "unknown test case");
    }
}
