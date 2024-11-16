// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "input_handler.h"

#include "fstream"
#include "iostream"
#include "list"
#include "map"
#include "sstream"
#include "string"
#include "unordered_map"
#include "vector"

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_timer.h"

#include "common/config.h"
#include "common/elf_info.h"
#include "common/io_file.h"
#include "common/path_util.h"
#include "common/version.h"
#include "input/controller.h"

namespace Input {
/*
Project structure:
n to m connection between inputs and outputs
Keyup and keydown events update a dynamic list* of u32 'flags' (what is currently in the list is
'pressed') On every event, after flag updates, we check for every input binding -> controller output
pair if all their flags are 'on' If not, disable; if so, enable them. For axes, we gather their data
into a struct cumulatively from all inputs, then after we checked all of those, we update them all
at once. Wheel inputs generate a timer that doesn't turn off their outputs automatically, but push a
userevent to do so.

What structs are needed?
InputBinding(key1, key2, key3)
ControllerOutput(button, axis) - we only need a const array of these, and one of the attr-s is
always 0 BindingConnection(inputBinding (member), controllerOutput (ref to the array element))
*/

// Flags and values for varying purposes
// todo: can we change these?
int mouse_joystick_binding = 0;
float mouse_deadzone_offset = 0.5, mouse_speed = 1, mouse_speed_offset = 0.1250;
Uint32 mouse_polling_id = 0;
bool mouse_enabled = false, leftjoystick_halfmode = false, rightjoystick_halfmode = false;

std::list<std::pair<u32, bool>> pressed_keys;
std::list<u32> toggled_keys;
std::list<BindingConnection> connections = std::list<BindingConnection>();

ControllerOutput output_array[] = {
    // Button mappings
    ControllerOutput(OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_TRIANGLE),
    ControllerOutput(OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_CIRCLE),
    ControllerOutput(OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_CROSS),
    ControllerOutput(OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_SQUARE),
    ControllerOutput(OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_L1),
    ControllerOutput(OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_L2),
    ControllerOutput(OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_R1),
    ControllerOutput(OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_R2),
    ControllerOutput(OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_L3),
    ControllerOutput(OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_R3),
    ControllerOutput(OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_OPTIONS),
    ControllerOutput(OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_TOUCH_PAD),
    ControllerOutput(OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_UP),
    ControllerOutput(OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_DOWN),
    ControllerOutput(OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_LEFT),
    ControllerOutput(OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_RIGHT),

    // Axis mappings
    ControllerOutput(0, Input::Axis::LeftX), ControllerOutput(0, Input::Axis::LeftY),
    ControllerOutput(0, Input::Axis::RightX), ControllerOutput(0, Input::Axis::RightY),
    ControllerOutput(0, Input::Axis::TriggerLeft), ControllerOutput(0, Input::Axis::TriggerRight),

    ControllerOutput(LEFTJOYSTICK_HALFMODE), ControllerOutput(RIGHTJOYSTICK_HALFMODE),
    ControllerOutput(KEY_TOGGLE),

    // End marker to signify the end of the array
    ControllerOutput(0, Input::Axis::AxisMax)};

// We had to go through 3 files of indirection just to update a flag
void ToggleMouseEnabled() {
    mouse_enabled ^= true;
}
// parsing related functions

// syntax: 'name, name,name' or 'name,name' or 'name'
InputBinding GetBindingFromString(std::string& line) {
    u32 key1 = 0, key2 = 0, key3 = 0;

    // Split the string by commas
    std::vector<std::string> tokens;
    std::stringstream ss(line);
    std::string token;

    while (std::getline(ss, token, ',')) {
        tokens.push_back(token);
    }

    // Check for invalid tokens and map valid ones to keys
    for (const auto& t : tokens) {
        if (string_to_keyboard_key_map.find(t) == string_to_keyboard_key_map.end()) {
            return InputBinding(0, 0, 0); // Skip by setting all keys to 0
        }
    }

    // Assign values to keys if all tokens were valid
    if (tokens.size() > 0)
        key1 = string_to_keyboard_key_map.at(tokens[0]);
    if (tokens.size() > 1)
        key2 = string_to_keyboard_key_map.at(tokens[1]);
    if (tokens.size() > 2)
        key3 = string_to_keyboard_key_map.at(tokens[2]);

    return InputBinding(key1, key2, key3);
}

// function that takes a controlleroutput, and returns the array's corresponding element's pointer
ControllerOutput* GetOutputPointer(const ControllerOutput& parsed) {
    // i wonder how long until someone notices this or I get rid of it
    for (int i = 0; i[output_array] != ControllerOutput(0, Axis::AxisMax); i++) {
        if (i[output_array] == parsed) {
            return &output_array[i];
        }
    }
    return nullptr;
}

void ParseInputConfig(const std::string game_id = "") {

    const auto config_file = Config::GetFoolproofKbmConfigFile(game_id);

    // todo: change usages of this to GetFoolproofKbmConfigFile (in the gui)
    if (game_id == "") {
        return;
    }

    // todo
    // we reset these here so in case the user fucks up or doesn't include this we can fall back to
    // default
    connections.clear();
    mouse_deadzone_offset = 0.5;
    mouse_speed = 1;
    mouse_speed_offset = 0.125;
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
            LOG_ERROR(Input, "Invalid format at line: {}, data: \"{}\", skipping line.", lineCount,
                      line);
            continue;
        }

        std::string output_string = line.substr(0, equal_pos);
        std::string input_string = line.substr(equal_pos + 1);
        std::size_t comma_pos = input_string.find(',');

        // special check for mouse to joystick input
        if (output_string == "mouse_to_joystick") {
            if (input_string == "left") {
                mouse_joystick_binding = 1;
            } else if (input_string == "right") {
                mouse_joystick_binding = 2;
            } else {
                mouse_joystick_binding = 0; // default to 'none' or invalid
            }
            continue;
        }
        // key toggle
        if (output_string == "key_toggle") {
            if (comma_pos != std::string::npos) {
                // handle key-to-key toggling (separate list?)
                InputBinding toggle_keys = GetBindingFromString(input_string);
                if (toggle_keys.KeyCount() != 2) {
                    LOG_ERROR(Input,
                              "Syntax error: Please provide exactly 2 keys: "
                              "first is the toggler, the second is the key to toggle: {}",
                              line);
                    continue;
                }
                ControllerOutput* toggle_out = GetOutputPointer(ControllerOutput(KEY_TOGGLE));
                BindingConnection toggle_connection =
                    BindingConnection(InputBinding(toggle_keys.key2), toggle_out, toggle_keys.key3);
                connections.insert(connections.end(), toggle_connection);
                continue;
            }
            LOG_ERROR(Input, "Invalid format at line: {}, data: \"{}\", skipping line.", lineCount,
                      line);
            continue;
        }
        if (output_string == "mouse_movement_params") {
            std::stringstream ss(input_string);
            char comma; // To hold the comma separators between the floats
            ss >> mouse_deadzone_offset >> comma >> mouse_speed >> comma >> mouse_speed_offset;

            // Check for invalid input (in case there's an unexpected format)
            if (ss.fail()) {
                LOG_ERROR(Input, "Failed to parse mouse movement parameters from line: {}", line);
            } else {
                // LOG_DEBUG(Input, "Mouse movement parameters parsed: {} {} {}",
                // mouse_deadzone_offset, mouse_speed, mouse_speed_offset);
            }

            continue;
        }

        // normal cases
        InputBinding binding = GetBindingFromString(input_string);
        BindingConnection connection(0, nullptr);
        auto button_it = string_to_cbutton_map.find(output_string);
        auto axis_it = string_to_axis_map.find(output_string);

        if (binding.IsEmpty()) {
            LOG_DEBUG(Input, "Invalid format at line: {}, data: \"{}\", skipping line.", lineCount,
                      line);
            continue;
        }
        if (button_it != string_to_cbutton_map.end()) {
            connection =
                BindingConnection(binding, GetOutputPointer(ControllerOutput(button_it->second)));
            connections.insert(connections.end(), connection);

        } else if (axis_it != string_to_axis_map.end()) {
            connection = BindingConnection(
                binding, GetOutputPointer(ControllerOutput(0, axis_it->second.axis)),
                axis_it->second.value);
            connections.insert(connections.end(), connection);
        } else {
            LOG_DEBUG(Input, "Invalid format at line: {}, data: \"{}\", skipping line.", lineCount,
                      line);
            continue;
        }
        // LOG_INFO(Input, "Succesfully parsed line {}", lineCount);
    }
    file.close();
    connections.sort();
    LOG_DEBUG(Input, "Done parsing the input config!");
}

u32 GetMouseWheelEvent(const SDL_Event& event) {
    if (event.type != SDL_EVENT_MOUSE_WHEEL && event.type != SDL_EVENT_MOUSE_WHEEL_OFF) {
        LOG_DEBUG(Input, "Something went wrong with wheel input parsing!");
        return 0;
    }
    if (event.wheel.y > 0) {
        return SDL_MOUSE_WHEEL_UP;
    } else if (event.wheel.y < 0) {
        return SDL_MOUSE_WHEEL_DOWN;
    } else if (event.wheel.x > 0) {
        return SDL_MOUSE_WHEEL_RIGHT;
    } else if (event.wheel.x < 0) {
        return SDL_MOUSE_WHEEL_LEFT;
    }
    return (u32)-1;
}

u32 InputBinding::GetInputIDFromEvent(const SDL_Event& e) {
    switch (e.type) {
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:
        return e.key.key;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
        return (u32)e.button.button;
    case SDL_EVENT_MOUSE_WHEEL:
    case SDL_EVENT_MOUSE_WHEEL_OFF:
        return GetMouseWheelEvent(e);
    default:
        return (u32)-1;
    }
}

GameController* ControllerOutput::controller = nullptr;
void ControllerOutput::SetControllerOutputController(GameController* c) {
    ControllerOutput::controller = c;
}

void ToggleKeyInList(u32 key) {
    auto it = std::find(toggled_keys.begin(), toggled_keys.end(), key);
    if (it == toggled_keys.end()) {
        toggled_keys.insert(toggled_keys.end(), key);
        LOG_DEBUG(Input, "Added {} to toggled keys", key);
    } else {
        toggled_keys.erase(it);
        LOG_DEBUG(Input, "Removed {} from toggled keys", key);
    }
}

void ControllerOutput::Update(bool pressed, u32 param) {
    float touchpad_x = 0;
    if (button != 0) {
        switch (button) {
        case OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_TOUCH_PAD:
            touchpad_x = Config::getBackButtonBehavior() == "left"    ? 0.25f
                         : Config::getBackButtonBehavior() == "right" ? 0.75f
                                                                      : 0.5f;
            controller->SetTouchpadState(0, true, touchpad_x, 0.5f);
            controller->CheckButton(0, button, pressed);
            break;
        case LEFTJOYSTICK_HALFMODE:
            leftjoystick_halfmode = pressed;
            break;
        case RIGHTJOYSTICK_HALFMODE:
            rightjoystick_halfmode = pressed;
            break;
        case KEY_TOGGLE:
            if (pressed) {
                ToggleKeyInList(param);
            }
            break;
        default: // is a normal key (hopefully)
            controller->CheckButton(0, button, pressed);
            break;
        }
    } else if (axis != Axis::AxisMax) {
        float multiplier = 1.0;
        switch (axis) {
        case Axis::LeftX:
        case Axis::LeftY:
            multiplier = leftjoystick_halfmode ? 0.5 : 1.0;
            // LOG_DEBUG(Input, "Joystick multiplier set to {}", multiplier);
            break;
        case Axis::RightX:
        case Axis::RightY:
            multiplier = rightjoystick_halfmode ? 0.5 : 1.0;
            // LOG_DEBUG(Input, "Joystick multiplier set to {}", multiplier);
            break;
        case Axis::TriggerLeft:
        case Axis::TriggerRight:
            // todo: verify this works (This probably works from testing,
            // but needs extra info (multiple input to the same trigger?))
            axis_value = SDL_clamp((pressed ? (s32)param : 0), 0, 127);
            controller->Axis(0, axis, GetAxis(0, 0x80, axis_value));
            return;
        default:
            break;
        }
        axis_value = SDL_clamp((pressed ? (s32)param : 0), -127, 127);
        int ax = GetAxis(-0x80, 0x80, axis_value * multiplier);
        controller->Axis(0, axis, ax);
    } else {
        LOG_DEBUG(Input, "Controller output with no values detected!");
    }
}
void ControllerOutput::AddUpdate(bool pressed, u32 param) {

    float touchpad_x = 0;
    if (button != 0) {
        if (!pressed) {
            return;
        }
        switch (button) {
        case OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_TOUCH_PAD:
            touchpad_x = Config::getBackButtonBehavior() == "left"    ? 0.25f
                         : Config::getBackButtonBehavior() == "right" ? 0.75f
                                                                      : 0.5f;
            controller->SetTouchpadState(0, true, touchpad_x, 0.5f);
            controller->CheckButton(0, button, pressed);
            break;
        case LEFTJOYSTICK_HALFMODE:
            leftjoystick_halfmode |= pressed;
            break;
        case RIGHTJOYSTICK_HALFMODE:
            rightjoystick_halfmode |= pressed;
            break;
        case KEY_TOGGLE:
            if (pressed) {
                ToggleKeyInList(param);
            }
            break;
        default: // is a normal key (hopefully)
            controller->CheckButton(0, button, pressed);
            break;
        }
    } else if (axis != Axis::AxisMax) {
        float multiplier = 1.0;
        switch (axis) {
        case Axis::LeftX:
        case Axis::LeftY:
            multiplier = leftjoystick_halfmode ? 0.5 : 1.0;
            LOG_DEBUG(Input, "Left joystick multiplier set to {}", multiplier);
            break;
        case Axis::RightX:
        case Axis::RightY:
            multiplier = rightjoystick_halfmode ? 0.5 : 1.0;
            LOG_DEBUG(Input, " Right joystick multiplier set to {}", multiplier);
            break;
        case Axis::TriggerLeft:
        case Axis::TriggerRight:
            // todo: verify this works
            axis_value = SDL_clamp((pressed ? (s32)param : 0) + axis_value, 0, 127);
            controller->Axis(0, axis, GetAxis(0, 0x80, axis_value));
            return;
        default:
            break;
        }
        axis_value = SDL_clamp(((pressed ? (s32)param : 0) + axis_value), -127, 127);
        controller->Axis(0, axis, GetAxis(-0x80, 0x80, axis_value * multiplier));
        // LOG_INFO(Input, "Axis value delta: {} final value: {}", (pressed ? a_value : 0),
        // axis_value);
    } else {
        LOG_DEBUG(Input, "Controller output with no values detected!");
    }
}

void UpdatePressedKeys(u32 value, bool is_pressed) {
    if (is_pressed) {
        // Find the correct position for insertion to maintain order
        auto it =
            std::lower_bound(pressed_keys.begin(), pressed_keys.end(), value,
                             [](const std::pair<u32, bool>& pk, u32 v) { return pk.first < v; });

        // Insert only if 'value' is not already in the list
        if (it == pressed_keys.end() || it->first != value) {
            pressed_keys.insert(it, {value, false});
        }
    } else {
        // Remove 'value' from the list if it's not pressed
        auto it =
            std::find_if(pressed_keys.begin(), pressed_keys.end(),
                         [value](const std::pair<u32, bool>& pk) { return pk.first == value; });
        if (it != pressed_keys.end()) {
            pressed_keys.erase(it); // Remove the key entirely from the list
        }
    }
}

// Check if a given binding's all keys are currently active.
bool IsInputActive(const InputBinding& i) {
    // Extract keys from InputBinding and ignore unused (0) keys
    std::list<uint32_t> input_keys = {i.key1, i.key2, i.key3};
    input_keys.remove(0);

    // Iterator for pressed_keys, starting from the beginning
    auto pressed_it = pressed_keys.begin();
    auto pressed_end = pressed_keys.end();

    // Store pointers to flags in pressed_keys that need to be set if all keys are active
    std::list<bool*> flags_to_set;

    // Check if all keys in input_keys are active
    for (uint32_t key : input_keys) {
        bool key_found = false;

        // Search for the current key in pressed_keys starting from the last checked position
        while (pressed_it != pressed_end && pressed_it->first <= key) {
            if (pressed_it->first == key) {
                
                key_found = true;
                flags_to_set.push_back(&pressed_it->second);
                
                ++pressed_it; // Move to the next key in pressed_keys
                break;
            }
            ++pressed_it;
        }

        // If not found in pressed_keys, check in toggled_keys
        if (!key_found &&
            std::find(toggled_keys.begin(), toggled_keys.end(), key) == toggled_keys.end()) {
            return false; // Key not found in either list
        }
    }

    bool is_fully_blocked = true;
    for (bool* flag : flags_to_set) {
        is_fully_blocked &= *flag;
    }
    if(is_fully_blocked) {
        return false;
    }
    for (bool* flag : flags_to_set) {
        *flag = true;
    }

    LOG_DEBUG(Input, "Input found: {}", i.ToString());
    return true; // All keys are active
}

void ActivateOutputsFromInputs() {
    //  reset everything
    for (auto& it : pressed_keys) {
        it.second = false;
    }
    for (auto it = connections.begin(); it != connections.end(); it++) {
        if (it->output) {
            it->output->Update(false, 0);
        } else {
            LOG_DEBUG(Input, "Null output in BindingConnection at position {}",
                      std::distance(connections.begin(), it));
        }
    }
    // iterates over the connections, and updates them depending on whether the corresponding input
    // trio is found
    for (auto& it : connections) {
        if (it.output) {
            bool active = IsInputActive(it.binding);
            it.output->AddUpdate(active, it.parameter);
        }
    }
}

void UpdateMouse(GameController* controller) {
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
        controller->Axis(0, axis_x, GetAxis(-0x80, 0x80, a_x));
        controller->Axis(0, axis_y, GetAxis(-0x80, 0x80, a_y));
    } else {
        controller->Axis(0, axis_x, GetAxis(-0x80, 0x80, 0));
        controller->Axis(0, axis_y, GetAxis(-0x80, 0x80, 0));
    }
}

Uint32 MousePolling(void* param, Uint32 id, Uint32 interval) {
    auto* data = (GameController*)param;
    UpdateMouse(data);
    return interval;
}

} // namespace Input