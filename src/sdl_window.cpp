// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>
#include "common/assert.h"
#include "common/config.h"
#include "common/io_file.h"
#include "common/path_util.h"
#include "common/version.h"
#include "core/libraries/pad/pad.h"
#include "imgui/renderer/imgui_core.h"
#include "input/controller.h"
#include "sdl_window.h"
#include "video_core/renderdoc.h"

#ifdef __APPLE__
#include <SDL3/SDL_metal.h>
#endif

namespace Frontend {
using Libraries::Pad::OrbisPadButtonDataOffset;

bool KeyBinding::operator<(const KeyBinding& other) const {
    return std::tie(key, modifier) < std::tie(other.key, other.modifier);
}

// modifiers are bitwise or-d together, so we need to check if ours is in that
template <typename T>
typename std::map<KeyBinding, T>::const_iterator FindKeyAllowingPartialModifiers(
    const std::map<KeyBinding, T>& map, KeyBinding binding) {
    for (typename std::map<KeyBinding, T>::const_iterator it = map.cbegin(); it != map.cend();
         it++) {
        if ((it->first.key == binding.key) && (it->first.modifier & binding.modifier) != 0) {
            return it;
        }
    }
    return map.end(); // Return end if no match is found
}
template <typename T>
typename std::map<KeyBinding, T>::const_iterator FindKeyAllowingOnlyNoModifiers(
    const std::map<KeyBinding, T>& map, KeyBinding binding) {
    for (typename std::map<KeyBinding, T>::const_iterator it = map.cbegin(); it != map.cend();
         it++) {
        if (it->first.key == binding.key && it->first.modifier == SDL_KMOD_NONE) {
            return it;
        }
    }
    return map.end(); // Return end if no match is found
}

// Axis map: maps key+modifier to controller axis and axis value
struct AxisMapping {
    Input::Axis axis;
    int value; // Value to set for key press (+127 or -127 for movement)
};

// i strongly suggest you collapse these maps
std::map<std::string, u32> string_to_cbutton_map = {
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
    {"right", OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_RIGHT}};
std::map<std::string, AxisMapping> string_to_axis_map = {
    {"axis_left_x_plus", {Input::Axis::LeftX, 127}},
    {"axis_left_x_minus", {Input::Axis::LeftX, -127}},
    {"axis_left_y_plus", {Input::Axis::LeftY, 127}},
    {"axis_left_y_minus", {Input::Axis::LeftY, -127}},
    {"axis_right_x_plus", {Input::Axis::RightX, 127}},
    {"axis_right_x_minus", {Input::Axis::RightX, -127}},
    {"axis_right_y_plus", {Input::Axis::RightY, 127}},
    {"axis_right_y_minus", {Input::Axis::RightY, -127}},
};
std::map<std::string, u32> string_to_keyboard_key_map = {
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
    {",", SDLK_COMMA},
    {".", SDLK_PERIOD},
    {"?", SDLK_QUESTION},
    {";", SDLK_SEMICOLON},
    {"-", SDLK_MINUS},
    {"_", SDLK_UNDERSCORE},
    {"(", SDLK_LEFTPAREN},
    {")", SDLK_RIGHTPAREN},
    {"[", SDLK_LEFTBRACKET},
    {"]", SDLK_RIGHTBRACKET},
    {"{", SDLK_LEFTBRACE},
    {"}", SDLK_RIGHTBRACE},
    {"\\", SDLK_BACKSLASH},
    {"/", SDLK_SLASH},
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
    {"leftbutton", SDL_BUTTON_LEFT},
    {"rightbutton", SDL_BUTTON_RIGHT},
    {"middlebutton", SDL_BUTTON_MIDDLE},
};
std::map<std::string, u32> string_to_keyboard_mod_key_map = {
    {"lshift", SDL_KMOD_LSHIFT}, {"rshift", SDL_KMOD_RSHIFT}, {"lctrl", SDL_KMOD_LCTRL},
    {"rctrl", SDL_KMOD_RCTRL},   {"lalt", SDL_KMOD_LALT},     {"ralt", SDL_KMOD_RALT},
    {"shift", SDL_KMOD_SHIFT},   {"ctrl", SDL_KMOD_CTRL},     {"alt", SDL_KMOD_ALT},
    {"l_meta", SDL_KMOD_LGUI},   {"r_meta", SDL_KMOD_RGUI},   {"meta", SDL_KMOD_GUI},
    {"lwin", SDL_KMOD_LGUI},     {"rwin", SDL_KMOD_RGUI},     {"win", SDL_KMOD_GUI},
    {"none", SDL_KMOD_NONE}, // if you want to be fancy
};

// Button map: maps key+modifier to controller button
std::map<KeyBinding, u32> button_map = {};
std::map<KeyBinding, AxisMapping> axis_map = {};

int mouse_joystick_binding = 0;
Uint32 mouse_polling_id = 0;
bool mouse_enabled = true;
void WindowSDL::parseInputConfig(const std::string& filename) {
    
    // Read configuration file.
    // std::cout << "Reading keyboard config...\n";
    const auto user_dir = Common::FS::GetUserPath(Common::FS::PathType::UserDir);
    std::ifstream file(user_dir / filename);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return;
    }

    button_map.clear();
    axis_map.clear();
    int lineCount = 0;
    std::string line;
    while (std::getline(file, line)) {
        lineCount++;
        // Ignore comment lines
        if (line.empty() || line[0] == '#') {
            continue;
        }
        // strip the ; and whitespace that we put there so that the braindead clang-format is happy
        line = line.substr(0, line.length() - 1);
        line.erase(std::remove(line.begin(), line.end(), ' '), line.end());

        // Split the line by '='
        std::size_t equal_pos = line.find('=');
        if (equal_pos == std::string::npos) {
            std::cerr << "Invalid line format: " << line << std::endl;
            continue;
        }

        std::string controller_input = line.substr(0, equal_pos);
        std::string kbm_input = line.substr(equal_pos + 1);
        KeyBinding binding = {0, SDL_KMOD_NONE}; // Initialize KeyBinding

        // quick hack to configure the mouse
        if (controller_input == "mouse_to_joystick") {
            switch (kbm_input[0]) {
            case 'l':
                mouse_joystick_binding = 1;
                break;
            case 'r':
                mouse_joystick_binding = 2;
                break;
            case 'n':
            default:
                mouse_joystick_binding = 0;
                break;
            }
            continue;
        }
        // first we parse the binding, and if its wrong, we skip to the next line
        std::size_t comma_pos = kbm_input.find(',');
        if (comma_pos != std::string::npos) {
            // Handle key + modifier
            std::string key = kbm_input.substr(0, comma_pos);
            std::string mod = kbm_input.substr(comma_pos + 1);

            auto key_it = string_to_keyboard_key_map.find(key);
            auto mod_it = string_to_keyboard_mod_key_map.find(mod);

            if (key_it != string_to_keyboard_key_map.end() &&
                mod_it != string_to_keyboard_mod_key_map.end()) {
                binding.key = key_it->second;
                binding.modifier = mod_it->second;
            } else {
                std::cerr << "Syntax error while parsing kbm inputs at line " << lineCount
                          << " line data: " << line << "\n";
                continue; // skip
            }
        } else {
            // Just a key without modifier
            auto key_it = string_to_keyboard_key_map.find(kbm_input);
            if (key_it != string_to_keyboard_key_map.end()) {
                binding.key = key_it->second;
            } else {
                std::cerr << "Syntax error while parsing kbm inputs at line " << lineCount
                          << " line data: " << line << "\n";
                continue; // skip
            }
        }

        // Check for axis mapping (example: axis_left_x_plus)
        auto axis_it = string_to_axis_map.find(controller_input);
        auto button_it = string_to_cbutton_map.find(controller_input);
        if (axis_it != string_to_axis_map.end()) {
            axis_map[binding] = axis_it->second;
        } else if (button_it != string_to_cbutton_map.end()) {
            button_map[binding] = button_it->second;
        } else {
            std::cerr << "Syntax error while parsing kbm inputs at line " << lineCount
                      << " line data: " << line << "\n";
            continue; // skip
        }
    }
    file.close();
}

Uint32 WindowSDL::keyRepeatCallback(void* param, Uint32 id, Uint32 interval) {
    auto* data = (std::pair<WindowSDL*, SDL_Event*>*)param;
    KeyBinding binding = {data->second->key.key, SDL_GetModState()};
    data->first->updateModKeyedInputsManually(binding);
    // data->first->onKeyPress(data->second);
    delete data->second;
    delete data;
    return 0; // Return 0 to stop the timer after firing once
}
Uint32 WindowSDL::mousePolling(void* param, Uint32 id, Uint32 interval) {
    auto* data = (WindowSDL*)param;
    data->updateMouse();
    return 33; // Return 0 to stop the timer after firing once
}

void WindowSDL::updateMouse() {
    if(!mouse_enabled) return;
    Input::Axis axis_x, axis_y;
    switch (mouse_joystick_binding) {
    case 1:
        axis_x = Input::Axis::LeftX;
        axis_y = Input::Axis::LeftY;
        break;
    case 2:
        axis_x = Input::Axis::RightX;
        axis_y = Input::Axis::RightY;
        break;
    case 0:
    default:
        return; // no update needed
    }

    float d_x = 0, d_y = 0;
    SDL_GetRelativeMouseState(&d_x, &d_y);

    float angle = atan2(d_y, d_x);
    float a_x = cos(angle) * 128.0, a_y = sin(angle) * 128.0;

    if (d_x != 0 && d_y != 0) {
        controller->Axis(0, Input::Axis::RightX, Input::GetAxis(-0x80, 0x80, a_x));
        controller->Axis(0, Input::Axis::RightY, Input::GetAxis(-0x80, 0x80, a_y));
    } else {
        controller->Axis(0, Input::Axis::RightX, Input::GetAxis(-0x80, 0x80, 0));
        controller->Axis(0, Input::Axis::RightY, Input::GetAxis(-0x80, 0x80, 0));
    }
}

WindowSDL::WindowSDL(s32 width_, s32 height_, Input::GameController* controller_,
                     std::string_view window_title)
    : width{width_}, height{height_}, controller{controller_} {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        UNREACHABLE_MSG("Failed to initialize SDL video subsystem: {}", SDL_GetError());
    }
    SDL_InitSubSystem(SDL_INIT_AUDIO);

    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING,
                          std::string(window_title).c_str());
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_X_NUMBER, SDL_WINDOWPOS_CENTERED);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_Y_NUMBER, SDL_WINDOWPOS_CENTERED);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, width);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, height);
    SDL_SetNumberProperty(props, "flags", SDL_WINDOW_VULKAN);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, true);
    window = SDL_CreateWindowWithProperties(props);
    SDL_DestroyProperties(props);
    if (window == nullptr) {
        UNREACHABLE_MSG("Failed to create window handle: {}", SDL_GetError());
    }

    SDL_SetWindowFullscreen(window, Config::isFullscreenMode());

    SDL_InitSubSystem(SDL_INIT_GAMEPAD);
    controller->TryOpenSDLController();

#if defined(SDL_PLATFORM_WIN32)
    window_info.type = WindowSystemType::Windows;
    window_info.render_surface = SDL_GetPointerProperty(SDL_GetWindowProperties(window),
                                                        SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
#elif defined(SDL_PLATFORM_LINUX)
    if (SDL_strcmp(SDL_GetCurrentVideoDriver(), "x11") == 0) {
        window_info.type = WindowSystemType::X11;
        window_info.display_connection = SDL_GetPointerProperty(
            SDL_GetWindowProperties(window), SDL_PROP_WINDOW_X11_DISPLAY_POINTER, NULL);
        window_info.render_surface = (void*)SDL_GetNumberProperty(
            SDL_GetWindowProperties(window), SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
    } else if (SDL_strcmp(SDL_GetCurrentVideoDriver(), "wayland") == 0) {
        window_info.type = WindowSystemType::Wayland;
        window_info.display_connection = SDL_GetPointerProperty(
            SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, NULL);
        window_info.render_surface = SDL_GetPointerProperty(
            SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, NULL);
    }
#elif defined(SDL_PLATFORM_MACOS)
    window_info.type = WindowSystemType::Metal;
    window_info.render_surface = SDL_Metal_GetLayer(SDL_Metal_CreateView(window));
#endif
    // initialize kbm controls
    parseInputConfig("keyboardInputConfig.ini");
}

WindowSDL::~WindowSDL() = default;

void WindowSDL::waitEvent() {
    // Called on main thread
    SDL_Event event;
    if (mouse_polling_id == 0) {
        mouse_polling_id = SDL_AddTimer(33, mousePolling, (void*)this);
    }
    
    if (!SDL_WaitEvent(&event)) {
        return;
    }

    if (ImGui::Core::ProcessEvent(&event)) {
        return;
    }
    SDL_Event* event_copy = new SDL_Event();
    *event_copy = event;
    std::pair<WindowSDL*, SDL_Event*>* payload_to_timer =
        new std::pair<WindowSDL*, SDL_Event*>(this, event_copy);

    switch (event.type) {
    case SDL_EVENT_WINDOW_RESIZED:
    case SDL_EVENT_WINDOW_MAXIMIZED:
    case SDL_EVENT_WINDOW_RESTORED:
        onResize();
        break;
    case SDL_EVENT_WINDOW_MINIMIZED:
    case SDL_EVENT_WINDOW_EXPOSED:
        is_shown = event.type == SDL_EVENT_WINDOW_EXPOSED;
        onResize();
        break;
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:
    case SDL_EVENT_MOUSE_BUTTON_UP:
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        SDL_AddTimer(33, keyRepeatCallback, (void*)payload_to_timer);
        onKeyPress(&event);
        break;
    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
    case SDL_EVENT_GAMEPAD_BUTTON_UP:
    case SDL_EVENT_GAMEPAD_AXIS_MOTION:
    case SDL_EVENT_GAMEPAD_ADDED:
    case SDL_EVENT_GAMEPAD_REMOVED:
    case SDL_EVENT_GAMEPAD_TOUCHPAD_DOWN:
    case SDL_EVENT_GAMEPAD_TOUCHPAD_UP:
    case SDL_EVENT_GAMEPAD_TOUCHPAD_MOTION:
        onGamepadEvent(&event);
        break;
    case SDL_EVENT_QUIT:
        is_open = false;
        break;
    default:
        break;
    }
}
void WindowSDL::onResize() {
    SDL_GetWindowSizeInPixels(window, &width, &height);
    ImGui::Core::OnResize();
}

void WindowSDL::updateButton(KeyBinding& binding, u32 button, bool is_pressed) {
    float x;
    Input::Axis axis;
    switch (button) {
    case OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_L2:
    case OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_R2:
        axis = (button == OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_R2) ? Input::Axis::TriggerRight
                                                                         : Input::Axis::TriggerLeft;
        // int axis_value = is_pressed ? 255 : 0;
        // int ax = Input::GetAxis(0, 0x80, is_pressed ? 255 : 0);
        controller->Axis(0, axis, Input::GetAxis(0, 0x80, is_pressed ? 255 : 0));
        break;
    case OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_TOUCH_PAD:
        x = Config::getBackButtonBehavior() == "left"    ? 0.25f
            : Config::getBackButtonBehavior() == "right" ? 0.75f
                                                         : 0.5f;
        controller->SetTouchpadState(0, true, x, 0.5f);
        controller->CheckButton(0, button, is_pressed);
        break;
    default: // is a normal key
        controller->CheckButton(0, button, is_pressed);
        break;
    }
}

void WindowSDL::onKeyPress(const SDL_Event* event) {
    // Extract key and modifier
    KeyBinding binding = {0, SDL_GetModState()};
    if (event->type == SDL_EVENT_KEY_DOWN || event->type == SDL_EVENT_KEY_UP) {
        binding.key = event->key.key; // For keyboard events
    } else if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
               event->type == SDL_EVENT_MOUSE_BUTTON_UP) {
        binding.key = event->button.button; // For mouse button events
    } else {
        std::cout << "Bro something is very wrong with the waitevent switch case as this is the "
                     "only 4 possible cases\n";
        return;
    }

    u32 button = 0;
    Input::Axis axis = Input::Axis::AxisMax;
    int axis_value = 0;

    // Handle window controls outside of the input maps
    if (event->type == SDL_EVENT_KEY_DOWN) {
        // Toggle capture of the mouse
        if (binding.key == SDLK_F9) {
            SDL_SetWindowRelativeMouseMode(this->GetSdlWindow(),
                                           !SDL_GetWindowRelativeMouseMode(this->GetSdlWindow()));
        }
        // Reparse kbm inputs
        if (binding.key == SDLK_F8) {
            parseInputConfig("keyboardInputConfig.ini");
        }
        // Toggle mouse movement input
        if (binding.key == SDLK_F7) {
            mouse_enabled = !mouse_enabled;
        }
        // Toggle fullscreen
        if (binding.key == SDLK_F11) {
            SDL_WindowFlags flag = SDL_GetWindowFlags(window);
            bool is_fullscreen = flag & SDL_WINDOW_FULLSCREEN;
            SDL_SetWindowFullscreen(window, !is_fullscreen);
        }
        // Trigger rdoc capture
        if (binding.key == SDLK_F12) {
            VideoCore::TriggerCapture();
        }
    }

    // Check if the current key+modifier is a button mapping
    bool button_found = false;
    auto button_it = FindKeyAllowingPartialModifiers(button_map, binding);
    if (button_it == button_map.end()) {
        button_it = FindKeyAllowingOnlyNoModifiers(button_map, binding);
    }
    if (button_it != button_map.end()) {
        button_found = true;
        button = button_it->second;
        WindowSDL::updateButton(binding, button,
                                event->type == SDL_EVENT_KEY_DOWN ||
                                    event->type == SDL_EVENT_MOUSE_BUTTON_DOWN);
    }
    // Check if the current key+modifier is an axis mapping
    auto axis_it = FindKeyAllowingPartialModifiers(axis_map, binding);
    if (axis_it == axis_map.end() && !button_found) {
        axis_it = FindKeyAllowingOnlyNoModifiers(axis_map, binding);
    }
    if (axis_it != axis_map.end()) {
        axis = axis_it->second.axis;
        axis_value =
            (event->type == SDL_EVENT_KEY_DOWN || event->type == SDL_EVENT_MOUSE_BUTTON_DOWN)
                ? axis_it->second.value
                : 0;
        int ax = Input::GetAxis(-0x80, 0x80, axis_value);
        controller->Axis(0, axis, ax);
    }
}

// if we don't do this, then if we activate a mod keyed input and let go of the mod key first,
// the button will be stuck on the "on" state becuse the "turn off" signal would only come from
// the other key being unpressed
void WindowSDL::updateModKeyedInputsManually(Frontend::KeyBinding& binding) {
    bool mod_keyed_input_found = false;
    for (auto input : button_map) {
        if (input.first.modifier != SDL_KMOD_NONE) {
            if ((input.first.modifier & binding.modifier) == 0) {
                WindowSDL::updateButton(binding, input.second, false);
            } else if (input.first.key == binding.key) {
                mod_keyed_input_found = true;
            }
        }
    }
    for (auto input : axis_map) {
        if (input.first.modifier != SDL_KMOD_NONE) {
            if ((input.first.modifier & binding.modifier) == 0) {
                controller->Axis(0, input.second.axis, Input::GetAxis(-0x80, 0x80, 0));
            } else if (input.first.key == binding.key) {
                mod_keyed_input_found = true;
            }
        }
    }
    // if both non mod keyed and mod keyed inputs are used and you press the key and then the mod
    // key in a single frame, both will activate but the simple one will not deactivate, unless i
    // use this stupid looking workaround
    if (!mod_keyed_input_found)
        return; // in this case the fix for the fix for the wrong update order is not needed
    for (auto input : button_map) {
        if (input.first.modifier == SDL_KMOD_NONE) {
            WindowSDL::updateButton(binding, input.second, false);
        }
    }
    for (auto input : axis_map) {
        if (input.first.modifier == SDL_KMOD_NONE) {
            controller->Axis(0, input.second.axis, Input::GetAxis(-0x80, 0x80, 0));
        }
    }
    // also this sometimes leads to janky inputs but whoever decides to intentionally create a state
    // where this is needed should not deserve a smooth experience anyway
}

void WindowSDL::onGamepadEvent(const SDL_Event* event) {
    using Libraries::Pad::OrbisPadButtonDataOffset;

    u32 button = 0;
    Input::Axis axis = Input::Axis::AxisMax;
    switch (event->type) {
    case SDL_EVENT_GAMEPAD_ADDED:
    case SDL_EVENT_GAMEPAD_REMOVED:
        controller->TryOpenSDLController();
        break;
    case SDL_EVENT_GAMEPAD_TOUCHPAD_DOWN:
    case SDL_EVENT_GAMEPAD_TOUCHPAD_UP:
    case SDL_EVENT_GAMEPAD_TOUCHPAD_MOTION:
        controller->SetTouchpadState(event->gtouchpad.finger,
                                     event->type != SDL_EVENT_GAMEPAD_TOUCHPAD_UP,
                                     event->gtouchpad.x, event->gtouchpad.y);
        break;
    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
    case SDL_EVENT_GAMEPAD_BUTTON_UP:
        button = sdlGamepadToOrbisButton(event->gbutton.button);
        if (button != 0) {
            if (event->gbutton.button == SDL_GAMEPAD_BUTTON_BACK) {
                std::string backButtonBehavior = Config::getBackButtonBehavior();
                if (backButtonBehavior != "none") {
                    float x = backButtonBehavior == "left"
                                  ? 0.25f
                                  : (backButtonBehavior == "right" ? 0.75f : 0.5f);
                    // trigger a touchpad event so that the touchpad emulation for back button works
                    controller->SetTouchpadState(0, true, x, 0.5f);
                    controller->CheckButton(0, button,
                                            event->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN);
                }
            } else {
                controller->CheckButton(0, button, event->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN);
            }
        }
        break;
    case SDL_EVENT_GAMEPAD_AXIS_MOTION:
        axis = event->gaxis.axis == SDL_GAMEPAD_AXIS_LEFTX           ? Input::Axis::LeftX
               : event->gaxis.axis == SDL_GAMEPAD_AXIS_LEFTY         ? Input::Axis::LeftY
               : event->gaxis.axis == SDL_GAMEPAD_AXIS_RIGHTX        ? Input::Axis::RightX
               : event->gaxis.axis == SDL_GAMEPAD_AXIS_RIGHTY        ? Input::Axis::RightY
               : event->gaxis.axis == SDL_GAMEPAD_AXIS_LEFT_TRIGGER  ? Input::Axis::TriggerLeft
               : event->gaxis.axis == SDL_GAMEPAD_AXIS_RIGHT_TRIGGER ? Input::Axis::TriggerRight
                                                                     : Input::Axis::AxisMax;
        if (axis != Input::Axis::AxisMax) {
            if (event->gaxis.axis == SDL_GAMEPAD_AXIS_LEFT_TRIGGER ||
                event->gaxis.axis == SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) {
                controller->Axis(0, axis, Input::GetAxis(0, 0x8000, event->gaxis.value));

            } else {
                controller->Axis(0, axis, Input::GetAxis(-0x8000, 0x8000, event->gaxis.value));
            }
        }
        break;
    }
}

int WindowSDL::sdlGamepadToOrbisButton(u8 button) {
    using Libraries::Pad::OrbisPadButtonDataOffset;

    switch (button) {
    case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
        return OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_DOWN;
    case SDL_GAMEPAD_BUTTON_DPAD_UP:
        return OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_UP;
    case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
        return OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_LEFT;
    case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
        return OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_RIGHT;
    case SDL_GAMEPAD_BUTTON_SOUTH:
        return OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_CROSS;
    case SDL_GAMEPAD_BUTTON_NORTH:
        return OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_TRIANGLE;
    case SDL_GAMEPAD_BUTTON_WEST:
        return OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_SQUARE;
    case SDL_GAMEPAD_BUTTON_EAST:
        return OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_CIRCLE;
    case SDL_GAMEPAD_BUTTON_START:
        return OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_OPTIONS;
    case SDL_GAMEPAD_BUTTON_TOUCHPAD:
        return OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_TOUCH_PAD;
    case SDL_GAMEPAD_BUTTON_BACK:
        return OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_TOUCH_PAD;
    case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:
        return OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_L1;
    case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER:
        return OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_R1;
    case SDL_GAMEPAD_BUTTON_LEFT_STICK:
        return OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_L3;
    case SDL_GAMEPAD_BUTTON_RIGHT_STICK:
        return OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_R3;
    default:
        return 0;
    }
}

} // namespace Frontend
