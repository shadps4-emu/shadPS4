// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "input_handler.h"

#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_timer.h>

#include "common/config.h"
#include "common/io_file.h"
#include "common/path_util.h"
#include "common/version.h"
#include "common/elf_info.h"
#include "input/controller.h"

namespace Input {
/*
Project structure:
n to m connection between inputs and outputs
Keyup and keydown events update a dynamic list* of u32 'flags' (what is currently in the list is 'pressed')
On every event, after flag updates, we check for every input binding -> controller output pair if all their flags are 'on'
If not, disable; if so, enable them.
For axes, we gather their data into a struct cumulatively from all inputs, 
    then after we checked all of those, we update them all at once.
Wheel inputs generate a timer that doesn't turn off their outputs automatically, but push a userevent to do so.

What structs are needed?
InputBinding(key1, key2, key3)
ControllerOutput(button, axis) - we only need a const array of these, and one of the attr-s is always 0
BindingConnection(inputBinding (member), controllerOutput (ref to the array element))
*/

// Flags and values for varying purposes
// todo: can we change these?
int mouse_joystick_binding = 0;
float mouse_deadzone_offset = 0.5, mouse_speed = 1, mouse_speed_offset = 0.1250;
Uint32 mouse_polling_id = 0;
bool mouse_enabled = false, leftjoystick_halfmode = false, rightjoystick_halfmode = false;

std::string_view getDefaultKeyboardConfig() {
    static std::string_view default_config =
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

void parseInputConfig(const std::string game_id = "") {
    // Read configuration file of the game, and if it doesn't exist, generate it from default
    // If that doesn't exist either, generate that from getDefaultConfig() and try again
    // If even the folder is missing, we start with that.

    // maybe extract this?
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
    //old_button_map.clear();
    //old_axis_map.clear();
    //old_key_to_modkey_toggle_map.clear();
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
        
        // new data type construcor here
        // todo

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
                // handle key-to-key toggling (separate list?)
                // todo
            }
            std::cerr << "Invalid line format at line: " << lineCount << " data: " << line
                      << std::endl;
            continue;
        }
        // todo
        /* og parsing
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
            old_axis_map[binding] = axis_it->second;
        } else if (button_it != string_to_cbutton_map.end()) {
            old_button_map[binding] = button_it->second;
        } else {
            std::cerr << "Syntax error while parsing kbm inputs at line " << lineCount
                      << " line data: " << line << "\n";
        }
        */
    }
    file.close();
}

// todo: add an init for this
GameController* ControllerOutput::controller = nullptr;
void ControllerOutput::setControllerOutputController(GameController* c) {
    ControllerOutput::controller = c;
}

void ControllerOutput::update(bool pressed, int axis_direction) {
    float touchpad_x = 0;
    Input::Axis axis = Input::Axis::AxisMax;
    if(button != 0){
        switch (button) {
        case OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_L2:
        case OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_R2:
            axis = (button == OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_R2) ? Input::Axis::TriggerRight
                                                                            : Input::Axis::TriggerLeft;
            controller->Axis(0, axis, Input::GetAxis(0, 0x80, pressed ? 255 : 0));
            break;
        case OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_TOUCH_PAD:
            touchpad_x = Config::getBackButtonBehavior() == "left"    ? 0.25f
                        : Config::getBackButtonBehavior() == "right" ? 0.75f
                                                                    : 0.5f;
            controller->SetTouchpadState(0, true, touchpad_x, 0.5f);
            controller->CheckButton(0, button, pressed);
            break;
        default: // is a normal key
            controller->CheckButton(0, button, pressed);
            break;
        }
    } else if (axis != Axis::AxisMax) {
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
        int output_value = (pressed ? axis_value : 0) * multiplier;
        int ax = Input::GetAxis(-0x80, 0x80, output_value);
        controller->Axis(0, axis, ax);
    } else {
        LOG_ERROR(Input, "Controller output with no values detected!");
    }
}

void updateMouse(GameController* controller) {
    if (!mouse_enabled)
        return;
    Axis axis_x, axis_y;
    switch (mouse_joystick_binding) {
    case 1:
        axis_x = Axis::LeftX;
        axis_y = Axis::LeftY;
        break;
    case 2:
        axis_x = Axis::RightX;
        axis_y = Axis::RightY;
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

Uint32 mousePolling(void* param, Uint32 id, Uint32 interval) {
    auto* data = (GameController*)param;
    updateMouse(data);
    return interval;
}

}