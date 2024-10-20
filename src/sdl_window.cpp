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
#include "common/elf_info.h"
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

Uint32 getMouseWheelEvent(const SDL_Event* event) {
    if (event->type != SDL_EVENT_MOUSE_WHEEL)
        return 0;
    // std::cout << "We got a wheel event! ";
    if (event->wheel.y > 0) {
        return SDL_MOUSE_WHEEL_UP;
    } else if (event->wheel.y < 0) {
        return SDL_MOUSE_WHEEL_DOWN;
    } else if (event->wheel.x > 0) {
        return SDL_MOUSE_WHEEL_RIGHT;
    } else if (event->wheel.x < 0) {
        return SDL_MOUSE_WHEEL_LEFT;
    }
    return 0;
}

namespace KBMConfig {
using Libraries::Pad::OrbisPadButtonDataOffset;

// i wrapped it in a function so I can collapse it
std::string getDefaultKeyboardConfig() {
    std::string default_config =
        R"(## SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
## SPDX-License-Identifier: GPL-2.0-or-later
 
#This is the default keybinding config
#To change per-game configs, modify the CUSAXXXXX.ini files
#To change the default config that applies to new games without already existing configs, modify default.ini
#If you don't like certain mappings, delete, change or comment them out.
#You can add any amount of KBM keybinds to a single controller input,
#but you can use each KBM keybind for one controller input.

#Keybinds used by the emulator (these are unchangeable):
#F11 : fullscreen
#F10 : FPS counter
#F9  : toggle mouse-to-joystick input 
#       (it overwrites everything else to that joystick, so this is required)
#F8  : reparse keyboard input(this)

#This is a mapping for Bloodborne, inspired by other Souls titles on PC.

#Specifies which joystick the mouse movement controls.
mouse_to_joystick = right;

#Use healing item, change status in inventory
triangle = f;
#Dodge, back in inventory
circle = space;
#Interact, select item in inventory
cross = e;
#Use quick item, remove item in inventory
square = r;

#Emergency extra bullets
up = w, lalt;
up = mousewheelup;
#Change quick item
down = s, lalt;
down = mousewheeldown;
#Change weapon in left hand
left = a, lalt;
left = mousewheelleft;
#Change weapon in right hand
right = d, lalt;
right = mousewheelright;
#Change into 'inventory mode', so you don't have to hold lalt every time you go into menus
modkey_toggle = i, lalt;

#Menu
options = escape;
#Gestures
touchpad = g;

#Transform
l1 = rightbutton, lshift;
#Shoot
r1 = leftbutton;
#Light attack
l2 = rightbutton;
#Heavy attack
r2 = leftbutton, lshift;
#Does nothing
l3 = x;
#Center cam, lock on
r3 = q;
r3 = middlebutton;

#Axis mappings
#Move
axis_left_x_minus = a;
axis_left_x_plus = d;
axis_left_y_minus = w;
axis_left_y_plus = s;
#Change to 'walk mode' by holding the following key:
leftjoystick_halfmode = lctrl;
)";
    return default_config;
}

// Button map: maps key+modifier to controller button
std::map<KeyBinding, u32> button_map = {};
std::map<KeyBinding, AxisMapping> axis_map = {};
std::map<SDL_Keycode, std::pair<SDL_Keymod, bool>> key_to_modkey_toggle_map = {};

// Flags and values for varying purposes
int mouse_joystick_binding = 0;
float mouse_deadzone_offset = 0.5, mouse_speed = 1, mouse_speed_offset = 0.125;
Uint32 mouse_polling_id = 0;
bool mouse_enabled = false, leftjoystick_halfmode = false, rightjoystick_halfmode = false;

KeyBinding::KeyBinding(const SDL_Event* event) {
    modifier = getCustomModState();
    key = 0;
    // std::cout << "Someone called the new binding ctor!\n";
    if (event->type == SDL_EVENT_KEY_DOWN || event->type == SDL_EVENT_KEY_UP) {
        key = event->key.key;
    } else if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
               event->type == SDL_EVENT_MOUSE_BUTTON_UP) {
        key = event->button.button;
    } else if (event->type == SDL_EVENT_MOUSE_WHEEL) {
        key = getMouseWheelEvent(event);
    } else {
        std::cout << "We don't support this event type!\n";
    }
}

bool KeyBinding::operator<(const KeyBinding& other) const {
    return std::tie(key, modifier) < std::tie(other.key, other.modifier);
}

SDL_Keymod KeyBinding::getCustomModState() {
    SDL_Keymod state = SDL_GetModState();
    for (auto mod_flag : KBMConfig::key_to_modkey_toggle_map) {
        if (mod_flag.second.second) {
            state |= mod_flag.second.first;
        }
    }
    return state;
}

void parseInputConfig(const std::string game_id = "") {
    // Read configuration file of the game, and if it doesn't exist, generate it from default
    // If that doesn't exist either, generate that from getDefaultConfig() and try again
    // If even the folder is missing, we start with that.
    const auto config_dir = Common::FS::GetUserPath(Common::FS::PathType::UserDir) / "kbmConfig";
    const auto config_file = config_dir / (game_id + ".ini");
    const auto default_config_file = config_dir / "default.ini";

    // Ensure the config directory exists
    if (!std::filesystem::exists(config_dir)) {
        std::filesystem::create_directories(config_dir);
    }

    // Try loading the game-specific config file
    if (!std::filesystem::exists(config_file)) {
        // If game-specific config doesn't exist, check for the default config
        if (!std::filesystem::exists(default_config_file)) {
            // If the default config is also missing, create it from getDefaultConfig()
            const auto default_config = getDefaultKeyboardConfig();
            std::ofstream default_config_stream(default_config_file);
            if (default_config_stream) {
                default_config_stream << default_config;
            }
        }

        // If default config now exists, copy it to the game-specific config file
        if (std::filesystem::exists(default_config_file) && !game_id.empty()) {
            std::filesystem::copy(default_config_file, config_file);
        }
    }
    // if we just called the function to generate the directory and the default .ini
    if (game_id.empty()) {
        return;
    }

    // we reset these here so in case the user fucks up or doesn't include this we can fall back to
    // default
    mouse_deadzone_offset = 0.5;
    mouse_speed = 1;
    mouse_speed_offset = 0.125;
    button_map.clear();
    axis_map.clear();
    key_to_modkey_toggle_map.clear();
    int lineCount = 0;

    std::ifstream file(config_file);
    std::string line = "";
    while (std::getline(file, line)) {
        lineCount++;
        // strip the ; and whitespace
        line.erase(std::remove(line.begin(), line.end(), ' '), line.end());
        if (line[line.length() - 1] == ';') {
            line = line.substr(0, line.length() - 1);
        }
        // Ignore comment lines
        if (line.empty() || line[0] == '#') {
            continue;
        }
        // Split the line by '='
        std::size_t equal_pos = line.find('=');
        if (equal_pos == std::string::npos) {
            std::cerr << "Invalid line format at line: " << lineCount << " data: " << line
                      << std::endl;
            continue;
        }

        std::string before_equals = line.substr(0, equal_pos);
        std::string after_equals = line.substr(equal_pos + 1);
        std::size_t comma_pos = after_equals.find(',');
        KeyBinding binding = {0, SDL_KMOD_NONE};

        // special check for mouse to joystick input
        if (before_equals == "mouse_to_joystick") {
            if (after_equals == "left") {
                mouse_joystick_binding = 1;
            } else if (after_equals == "right") {
                mouse_joystick_binding = 2;
            } else {
                mouse_joystick_binding = 0; // default to 'none' or invalid
            }
            continue;
        }
        // mod key toggle
        if (before_equals == "modkey_toggle") {
            if (comma_pos != std::string::npos) {
                auto k = string_to_keyboard_key_map.find(after_equals.substr(0, comma_pos));
                auto m = string_to_keyboard_mod_key_map.find(after_equals.substr(comma_pos + 1));
                if (k != string_to_keyboard_key_map.end() &&
                    m != string_to_keyboard_mod_key_map.end()) {
                    key_to_modkey_toggle_map[k->second] = {m->second, false};
                    continue;
                }
            }
            std::cerr << "Invalid line format at line: " << lineCount << " data: " << line
                      << std::endl;
            continue;
        }
        // first we parse the binding, and if its wrong, we skip to the next line
        if (comma_pos != std::string::npos) {
            // Handle key + modifier
            std::string key = after_equals.substr(0, comma_pos);
            std::string mod = after_equals.substr(comma_pos + 1);

            auto key_it = string_to_keyboard_key_map.find(key);
            auto mod_it = string_to_keyboard_mod_key_map.find(mod);

            if (key_it != string_to_keyboard_key_map.end() &&
                mod_it != string_to_keyboard_mod_key_map.end()) {
                binding.key = key_it->second;
                binding.modifier = mod_it->second;
            } else if (before_equals == "mouse_movement_params") {
                // handle mouse movement params
                float p1 = 0.5, p2 = 1, p3 = 0.125;
                std::size_t second_comma_pos = after_equals.find(',');
                try {
                    p1 = std::stof(key);
                    p2 = std::stof(mod.substr(0, second_comma_pos));
                    p3 = std::stof(mod.substr(second_comma_pos + 1));
                    mouse_deadzone_offset = p1;
                    mouse_speed = p2;
                    mouse_speed_offset = p3;
                } catch (...) {
                    // fallback to default values
                    mouse_deadzone_offset = 0.5;
                    mouse_speed = 1;
                    mouse_speed_offset = 0.125;
                    std::cerr << "Parsing error while parsing kbm inputs at line " << lineCount
                              << " line data: " << line << "\n";
                }
                continue;
            } else {
                std::cerr << "Syntax error while parsing kbm inputs at line " << lineCount
                          << " line data: " << line << "\n";
                continue; // skip
            }
        } else {
            // Just a key without modifier
            auto key_it = string_to_keyboard_key_map.find(after_equals);
            if (key_it != string_to_keyboard_key_map.end()) {
                binding.key = key_it->second;
            } else {
                std::cerr << "Syntax error while parsing kbm inputs at line " << lineCount
                          << " line data: " << line << "\n";
                continue; // skip
            }
        }

        // Check for axis mapping (example: axis_left_x_plus)
        auto axis_it = string_to_axis_map.find(before_equals);
        auto button_it = string_to_cbutton_map.find(before_equals);
        if (axis_it != string_to_axis_map.end()) {
            axis_map[binding] = axis_it->second;
        } else if (button_it != string_to_cbutton_map.end()) {
            button_map[binding] = button_it->second;
        } else {
            std::cerr << "Syntax error while parsing kbm inputs at line " << lineCount
                      << " line data: " << line << "\n";
        }
    }
    file.close();
}

} // namespace KBMConfig

namespace Frontend {
using Libraries::Pad::OrbisPadButtonDataOffset;

using namespace KBMConfig;
using KBMConfig::AxisMapping;
using KBMConfig::KeyBinding;

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

Uint32 WindowSDL::keyRepeatCallback(void* param, Uint32 id, Uint32 interval) {
    auto* data = (std::pair<WindowSDL*, SDL_Event*>*)param;
    KeyBinding binding(data->second);
    if (data->second->type == SDL_EVENT_MOUSE_WHEEL) {
        // send an off signal a frame later
        auto button_it = button_map.find(binding);
        auto axis_it = axis_map.find(binding);
        if (button_it != button_map.end()) {
            data->first->updateButton(binding, button_it->second, false);
        } else if (axis_it != axis_map.end()) {
            data->first->controller->Axis(0, axis_it->second.axis, Input::GetAxis(-0x80, 0x80, 0));
        }
        return 0;
    }
    data->first->updateModKeyedInputsManually(binding);
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
    if (!mouse_enabled)
        return;
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

    float output_speed =
        SDL_clamp((sqrt(d_x * d_x + d_y * d_y) + mouse_speed_offset * 128) * mouse_speed,
                  mouse_deadzone_offset * 128, 128.0);
    // std::cout << "speed: " << mouse_speed << "\n";

    float angle = atan2(d_y, d_x);
    float a_x = cos(angle) * output_speed, a_y = sin(angle) * output_speed;

    if (d_x != 0 && d_y != 0) {
        controller->Axis(0, axis_x, Input::GetAxis(-0x80, 0x80, a_x));
        controller->Axis(0, axis_y, Input::GetAxis(-0x80, 0x80, a_y));
    } else {
        controller->Axis(0, axis_x, Input::GetAxis(-0x80, 0x80, 0));
        controller->Axis(0, axis_y, Input::GetAxis(-0x80, 0x80, 0));
    }
}

static Uint32 SDLCALL PollController(void* userdata, SDL_TimerID timer_id, Uint32 interval) {
    auto* controller = reinterpret_cast<Input::GameController*>(userdata);
    return controller->Poll();
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
    parseInputConfig(std::string(Common::ElfInfo::Instance().GameSerial()));
}

WindowSDL::~WindowSDL() = default;

void WindowSDL::waitEvent() {
    // Called on main thread
    SDL_Event event{};

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
    case SDL_EVENT_MOUSE_WHEEL:
    case SDL_EVENT_MOUSE_BUTTON_UP:
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        // native mouse update function goes here
        // as seen in pr #633
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:
        SDL_AddTimer(33, keyRepeatCallback, (void*)payload_to_timer);
        onKeyboardMouseEvent(&event);
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

void WindowSDL::initTimers() {
    SDL_AddTimer(100, &PollController, controller);
    SDL_AddTimer(33, mousePolling, (void*)this);
}

void WindowSDL::onResize() {
    SDL_GetWindowSizeInPixels(window, &width, &height);
    ImGui::Core::OnResize();
}

// for L2/R2, touchpad and normal buttons
void WindowSDL::updateButton(KeyBinding& binding, u32 button, bool is_pressed) {
    float touchpad_x = 0;
    Input::Axis axis = Input::Axis::AxisMax;
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
        touchpad_x = Config::getBackButtonBehavior() == "left"    ? 0.25f
                     : Config::getBackButtonBehavior() == "right" ? 0.75f
                                                                  : 0.5f;
        controller->SetTouchpadState(0, true, touchpad_x, 0.5f);
        controller->CheckButton(0, button, is_pressed);
        break;
    default: // is a normal key
        controller->CheckButton(0, button, is_pressed);
        break;
    }
}

// previously onKeyPress
void WindowSDL::onKeyboardMouseEvent(const SDL_Event* event) {
    // Extract key and modifier
    KeyBinding binding(event);

    bool input_down = event->type == SDL_EVENT_KEY_DOWN ||
                      event->type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
                      event->type == SDL_EVENT_MOUSE_WHEEL;

    // Handle window controls outside of the input maps
    if (event->type == SDL_EVENT_KEY_DOWN) {
        // Reparse kbm inputs
        if (binding.key == SDLK_F8) {
            parseInputConfig(std::string(Common::ElfInfo::Instance().GameSerial()));
        }
        // Toggle mouse capture and movement input
        else if (binding.key == SDLK_F9) {
            mouse_enabled = !mouse_enabled;
            SDL_SetWindowRelativeMouseMode(this->GetSdlWindow(),
                                           !SDL_GetWindowRelativeMouseMode(this->GetSdlWindow()));
        }
        // Toggle fullscreen
        else if (binding.key == SDLK_F11) {
            SDL_WindowFlags flag = SDL_GetWindowFlags(window);
            bool is_fullscreen = flag & SDL_WINDOW_FULLSCREEN;
            SDL_SetWindowFullscreen(window, !is_fullscreen);
        }
        // Trigger rdoc capture
        else if (binding.key == SDLK_F12) {
            VideoCore::TriggerCapture();
        }
    }

    // Check for modifier toggle
    auto modkey_toggle_it = key_to_modkey_toggle_map.find(binding.key);
    modkey_toggle_it->second.second ^=
        (modkey_toggle_it != key_to_modkey_toggle_map.end() &&
         (binding.modifier & (~modkey_toggle_it->second.first)) == SDL_KMOD_NONE && input_down);

    // Check if the current key+modifier is a button or axis mapping
    // first only exact matches
    auto button_it = FindKeyAllowingPartialModifiers(button_map, binding);
    auto axis_it = FindKeyAllowingPartialModifiers(axis_map, binding);
    // then no mod key matches if we didn't find it in the previous pass
    if (button_it == button_map.end() && axis_it == axis_map.end()) {
        button_it = FindKeyAllowingOnlyNoModifiers(button_map, binding);
    }
    if (axis_it == axis_map.end() && button_it == button_map.end()) {
        axis_it = FindKeyAllowingOnlyNoModifiers(axis_map, binding);
    }

    if (button_it != button_map.end()) {
        // joystick_halfmode is not a button update so we handle it differently
        if (button_it->second == LEFTJOYSTICK_HALFMODE) {
            leftjoystick_halfmode = input_down;
        } else if (button_it->second == RIGHTJOYSTICK_HALFMODE) {
            rightjoystick_halfmode = input_down;
        } else {
            WindowSDL::updateButton(binding, button_it->second, input_down);
        }
    }
    if (axis_it != axis_map.end()) {
        Input::Axis axis = axis_it->second.axis;
        float multiplier = 1.0;
        switch (axis) {
        case Input::Axis::LeftX:
        case Input::Axis::LeftY:
            multiplier = leftjoystick_halfmode ? 0.5 : 1.0;
            break;
        case Input::Axis::RightX:
        case Input::Axis::RightY:
            multiplier = rightjoystick_halfmode ? 0.5 : 1.0;
            break;
        default:
            break;
        }
        int axis_value = (input_down ? axis_it->second.value : 0) * multiplier;
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
