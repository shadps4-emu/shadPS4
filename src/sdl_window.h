// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <map>
#include <string>
#include "common/types.h"
#include "core/libraries/pad/pad.h"
#include "input/controller.h"

#include <SDL3/SDL_events.h>

// +1 and +2 is taken
#define SDL_MOUSE_WHEEL_UP SDL_EVENT_MOUSE_WHEEL + 3
#define SDL_MOUSE_WHEEL_DOWN SDL_EVENT_MOUSE_WHEEL + 4
#define SDL_MOUSE_WHEEL_LEFT SDL_EVENT_MOUSE_WHEEL + 5
#define SDL_MOUSE_WHEEL_RIGHT SDL_EVENT_MOUSE_WHEEL + 6

#define LEFTJOYSTICK_HALFMODE 0x00010000
#define RIGHTJOYSTICK_HALFMODE 0x00020000

struct SDL_Window;
struct SDL_Gamepad;
union SDL_Event;

namespace Input {
class GameController;
}

namespace KBMConfig {

class KeyBinding {
public:
    Uint32 key;
    SDL_Keymod modifier;
    KeyBinding(SDL_Keycode k, SDL_Keymod m) : key(k), modifier(m){};
    KeyBinding(const SDL_Event* event);
    bool operator<(const KeyBinding& other) const;
    ~KeyBinding(){};
    static SDL_Keymod getCustomModState();
};

struct AxisMapping {
    Input::Axis axis;
    int value; // Value to set for key press (+127 or -127 for movement)
};

// Define a struct to hold any necessary timing information for delayed actions
struct DelayedAction {
    Uint64 triggerTime; // When the action should be triggered
    SDL_Event event;    //  Event data
};

std::string getDefaultKeyboardConfig();
void parseInputConfig(const std::string game_id);

using Libraries::Pad::OrbisPadButtonDataOffset;
// i strongly suggest you collapse these maps
const std::map<std::string, u32> string_to_cbutton_map = {
    {"triangle", OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_TRIANGLE},
    {"circle", OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_CIRCLE},
    {"cross", OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_CROSS},
    {"square", OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_SQUARE},
    {"l1", OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_L1},
    {"l2", OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_L2},
    {"r1", OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_R1},
    {"r2", OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_R2},
    {"l3", OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_L3},
    {"r3", OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_R3},
    {"options", OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_OPTIONS},
    {"touchpad", OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_TOUCH_PAD},
    {"up", OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_UP},
    {"down", OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_DOWN},
    {"left", OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_LEFT},
    {"right", OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_RIGHT},
    {"leftjoystick_halfmode", LEFTJOYSTICK_HALFMODE},
    {"rightjoystick_halfmode", RIGHTJOYSTICK_HALFMODE},
};
const std::map<std::string, AxisMapping> string_to_axis_map = {
    {"axis_left_x_plus", {Input::Axis::LeftX, 127}},
    {"axis_left_x_minus", {Input::Axis::LeftX, -127}},
    {"axis_left_y_plus", {Input::Axis::LeftY, 127}},
    {"axis_left_y_minus", {Input::Axis::LeftY, -127}},
    {"axis_right_x_plus", {Input::Axis::RightX, 127}},
    {"axis_right_x_minus", {Input::Axis::RightX, -127}},
    {"axis_right_y_plus", {Input::Axis::RightY, 127}},
    {"axis_right_y_minus", {Input::Axis::RightY, -127}},
};
const std::map<std::string, u32> string_to_keyboard_key_map = {
    {"a", SDLK_A},
    {"b", SDLK_B},
    {"c", SDLK_C},
    {"d", SDLK_D},
    {"e", SDLK_E},
    {"f", SDLK_F},
    {"g", SDLK_G},
    {"h", SDLK_H},
    {"i", SDLK_I},
    {"j", SDLK_J},
    {"k", SDLK_K},
    {"l", SDLK_L},
    {"m", SDLK_M},
    {"n", SDLK_N},
    {"o", SDLK_O},
    {"p", SDLK_P},
    {"q", SDLK_Q},
    {"r", SDLK_R},
    {"s", SDLK_S},
    {"t", SDLK_T},
    {"u", SDLK_U},
    {"v", SDLK_V},
    {"w", SDLK_W},
    {"x", SDLK_X},
    {"y", SDLK_Y},
    {"z", SDLK_Z},
    {"0", SDLK_0},
    {"1", SDLK_1},
    {"2", SDLK_2},
    {"3", SDLK_3},
    {"4", SDLK_4},
    {"5", SDLK_5},
    {"6", SDLK_6},
    {"7", SDLK_7},
    {"8", SDLK_8},
    {"9", SDLK_9},
    {"kp0", SDLK_KP_0},
    {"kp1", SDLK_KP_1},
    {"kp2", SDLK_KP_2},
    {"kp3", SDLK_KP_3},
    {"kp4", SDLK_KP_4},
    {"kp5", SDLK_KP_5},
    {"kp6", SDLK_KP_6},
    {"kp7", SDLK_KP_7},
    {"kp8", SDLK_KP_8},
    {"kp9", SDLK_KP_9},
    {"comma", SDLK_COMMA},
    {"period", SDLK_PERIOD},
    {"question", SDLK_QUESTION},
    {"semicolon", SDLK_SEMICOLON},
    {"minus", SDLK_MINUS},
    {"underscore", SDLK_UNDERSCORE},
    {"lparenthesis", SDLK_LEFTPAREN},
    {"rparenthesis", SDLK_RIGHTPAREN},
    {"lbracket", SDLK_LEFTBRACKET},
    {"rbracket", SDLK_RIGHTBRACKET},
    {"lbrace", SDLK_LEFTBRACE},
    {"rbrace", SDLK_RIGHTBRACE},
    {"backslash", SDLK_BACKSLASH},
    {"dash", SDLK_SLASH},
    {"enter", SDLK_RETURN},
    {"space", SDLK_SPACE},
    {"tab", SDLK_TAB},
    {"backspace", SDLK_BACKSPACE},
    {"escape", SDLK_ESCAPE},
    {"left", SDLK_LEFT},
    {"right", SDLK_RIGHT},
    {"up", SDLK_UP},
    {"down", SDLK_DOWN},
    {"lctrl", SDLK_LCTRL},
    {"rctrl", SDLK_RCTRL},
    {"lshift", SDLK_LSHIFT},
    {"rshift", SDLK_RSHIFT},
    {"lalt", SDLK_LALT},
    {"ralt", SDLK_RALT},
    {"lmeta", SDLK_LGUI},
    {"rmeta", SDLK_RGUI},
    {"lwin", SDLK_LGUI},
    {"rwin", SDLK_RGUI},
    {"home", SDLK_HOME},
    {"end", SDLK_END},
    {"pgup", SDLK_PAGEUP},
    {"pgdown", SDLK_PAGEDOWN},
    {"leftbutton", SDL_BUTTON_LEFT},
    {"rightbutton", SDL_BUTTON_RIGHT},
    {"middlebutton", SDL_BUTTON_MIDDLE},
    {"sidebuttonback", SDL_BUTTON_X1},
    {"sidebuttonforward", SDL_BUTTON_X2},
    {"mousewheelup", SDL_MOUSE_WHEEL_UP},
    {"mousewheeldown", SDL_MOUSE_WHEEL_DOWN},
    {"mousewheelleft", SDL_MOUSE_WHEEL_LEFT},
    {"mousewheelright", SDL_MOUSE_WHEEL_RIGHT},
    {"kpperiod", SDLK_KP_PERIOD},
    {"kpcomma", SDLK_KP_COMMA},
    {"kpdivide", SDLK_KP_DIVIDE},
    {"kpmultiply", SDLK_KP_MULTIPLY},
    {"kpminus", SDLK_KP_MINUS},
    {"kpplus", SDLK_KP_PLUS},
    {"kpenter", SDLK_KP_ENTER},
    {"kpequals", SDLK_KP_EQUALS},
    {"capslock", SDLK_CAPSLOCK},
};
const std::map<std::string, u32> string_to_keyboard_mod_key_map = {
    {"lshift", SDL_KMOD_LSHIFT}, {"rshift", SDL_KMOD_RSHIFT},
    {"lctrl", SDL_KMOD_LCTRL},   {"rctrl", SDL_KMOD_RCTRL},
    {"lalt", SDL_KMOD_LALT},     {"ralt", SDL_KMOD_RALT},
    {"shift", SDL_KMOD_SHIFT},   {"ctrl", SDL_KMOD_CTRL},
    {"alt", SDL_KMOD_ALT},       {"l_meta", SDL_KMOD_LGUI},
    {"r_meta", SDL_KMOD_RGUI},   {"meta", SDL_KMOD_GUI},
    {"lwin", SDL_KMOD_LGUI},     {"rwin", SDL_KMOD_RGUI},
    {"win", SDL_KMOD_GUI},       {"capslock", SDL_KMOD_CAPS},
    {"numlock", SDL_KMOD_NUM},   {"none", SDL_KMOD_NONE}, // if you want to be fancy
};

// Button map: maps key+modifier to controller button
extern std::map<KeyBinding, u32> button_map;
extern std::map<KeyBinding, AxisMapping> axis_map;
extern std::map<SDL_Keycode, std::pair<SDL_Keymod, bool>> key_to_modkey_toggle_map;

} // namespace KBMConfig

namespace Frontend {

enum class WindowSystemType : u8 {
    Headless,
    Windows,
    X11,
    Wayland,
    Metal,
};

struct WindowSystemInfo {
    // Connection to a display server. This is used on X11 and Wayland platforms.
    void* display_connection = nullptr;

    // Render surface. This is a pointer to the native window handle, which depends
    // on the platform. e.g. HWND for Windows, Window for X11. If the surface is
    // set to nullptr, the video backend will run in headless mode.
    void* render_surface = nullptr;

    // Scale of the render surface. For hidpi systems, this will be >1.
    float render_surface_scale = 1.0f;

    // Window system type. Determines which GL context or Vulkan WSI is used.
    WindowSystemType type = WindowSystemType::Headless;
};

class WindowSDL {
public:
    explicit WindowSDL(s32 width, s32 height, Input::GameController* controller,
                       std::string_view window_title);
    ~WindowSDL();

    s32 getWidth() const {
        return width;
    }

    s32 getHeight() const {
        return height;
    }

    bool isOpen() const {
        return is_open;
    }

    [[nodiscard]] SDL_Window* GetSdlWindow() const {
        return window;
    }

    WindowSystemInfo getWindowInfo() const {
        return window_info;
    }

    void waitEvent();
    void updateMouse();

    void initTimers();

private:
    void onResize();
    void onKeyboardMouseEvent(const SDL_Event* event);
    void onGamepadEvent(const SDL_Event* event);
    int sdlGamepadToOrbisButton(u8 button);

    void updateModKeyedInputsManually(KBMConfig::KeyBinding& binding);
    void updateButton(KBMConfig::KeyBinding& binding, u32 button, bool isPressed);
    static Uint32 keyRepeatCallback(void* param, Uint32 id, Uint32 interval);
    static Uint32 mousePolling(void* param, Uint32 id, Uint32 interval);
    void handleDelayedActions();

private:
    s32 width;
    s32 height;
    Input::GameController* controller;
    WindowSystemInfo window_info{};
    SDL_Window* window{};
    bool is_shown{};
    bool is_open{true};
};

} // namespace Frontend
