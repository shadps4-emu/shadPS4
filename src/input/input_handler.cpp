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

Things to always test before pushing like a dumbass:
Button outputs
Axis outputs
Input hierarchy
Multi key inputs
Mouse to joystick
Key toggle
Joystick halfmode

Don't be an idiot and test only the changed part expecting everything else to not be broken
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
    // Important: these have to be the first, or else they will update in the wrong order
    ControllerOutput((OrbisPadButtonDataOffset)LEFTJOYSTICK_HALFMODE),
    ControllerOutput((OrbisPadButtonDataOffset)RIGHTJOYSTICK_HALFMODE),
    ControllerOutput((OrbisPadButtonDataOffset)KEY_TOGGLE),

    // Button mappings
    ControllerOutput(OrbisPadButtonDataOffset::Triangle),
    ControllerOutput(OrbisPadButtonDataOffset::Circle),
    ControllerOutput(OrbisPadButtonDataOffset::Cross),
    ControllerOutput(OrbisPadButtonDataOffset::Square),
    ControllerOutput(OrbisPadButtonDataOffset::L1),
    ControllerOutput(OrbisPadButtonDataOffset::L3),
    ControllerOutput(OrbisPadButtonDataOffset::R1),
    ControllerOutput(OrbisPadButtonDataOffset::R3),
    ControllerOutput(OrbisPadButtonDataOffset::Options),
    ControllerOutput(OrbisPadButtonDataOffset::TouchPad),
    ControllerOutput(OrbisPadButtonDataOffset::Up),
    ControllerOutput(OrbisPadButtonDataOffset::Down),
    ControllerOutput(OrbisPadButtonDataOffset::Left),
    ControllerOutput(OrbisPadButtonDataOffset::Right),

    // Axis mappings
    ControllerOutput(OrbisPadButtonDataOffset::None, Axis::LeftX),
    ControllerOutput(OrbisPadButtonDataOffset::None, Axis::LeftY),
    ControllerOutput(OrbisPadButtonDataOffset::None, Axis::RightX),
    ControllerOutput(OrbisPadButtonDataOffset::None, Axis::RightY),
    ControllerOutput(OrbisPadButtonDataOffset::None, Axis::TriggerLeft),
    ControllerOutput(OrbisPadButtonDataOffset::None, Axis::TriggerRight),

    // the "sorry, you gave an incorrect value, now we bind it to nothing" output
    ControllerOutput(OrbisPadButtonDataOffset::None, Axis::AxisMax),
};

// We had to go through 3 files of indirection just to update a flag
void ToggleMouseEnabled() {
    mouse_enabled ^= true;
}

// parsing related functions
u32 GetAxisInputId(AxisMapping a) {
    // LOG_INFO(Input, "Parsing an axis...");
    if (a.axis == Axis::AxisMax || a.value != 0) {
        LOG_ERROR(Input, "Invalid axis given!");
        return 0;
    }
    u32 value = (u32)a.axis + 0x80000000;
    return value;
}

u32 GetOrbisToSdlButtonKeycode(OrbisPadButtonDataOffset cbutton) {
    switch (cbutton) {
    case OrbisPadButtonDataOffset::Circle:
        return SDL_GAMEPAD_BUTTON_EAST;
    case OrbisPadButtonDataOffset::Triangle:
        return SDL_GAMEPAD_BUTTON_NORTH;
    case OrbisPadButtonDataOffset::Square:
        return SDL_GAMEPAD_BUTTON_WEST;
    case OrbisPadButtonDataOffset::Cross:
        return SDL_GAMEPAD_BUTTON_SOUTH;
    case OrbisPadButtonDataOffset::L1:
        return SDL_GAMEPAD_BUTTON_LEFT_SHOULDER;
    case OrbisPadButtonDataOffset::R1:
        return SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER;
    case OrbisPadButtonDataOffset::L3:
        return SDL_GAMEPAD_BUTTON_LEFT_STICK;
    case OrbisPadButtonDataOffset::R3:
        return SDL_GAMEPAD_BUTTON_RIGHT_STICK;
    case OrbisPadButtonDataOffset::Up:
        return SDL_GAMEPAD_BUTTON_DPAD_UP;
    case OrbisPadButtonDataOffset::Down:
        return SDL_GAMEPAD_BUTTON_DPAD_DOWN;
    case OrbisPadButtonDataOffset::Left:
        return SDL_GAMEPAD_BUTTON_DPAD_LEFT;
    case OrbisPadButtonDataOffset::Right:
        return SDL_GAMEPAD_BUTTON_DPAD_RIGHT;
    case OrbisPadButtonDataOffset::Options:
        return SDL_GAMEPAD_BUTTON_START;
    case (OrbisPadButtonDataOffset)BACK_BUTTON:
        return SDL_GAMEPAD_BUTTON_BACK;

    default:
        return ((u32)-1) - 0x10000000;
    }
}
u32 GetControllerButtonInputId(u32 cbutton) {
    if ((cbutton & ((u32)OrbisPadButtonDataOffset::TouchPad | LEFTJOYSTICK_HALFMODE |
                    RIGHTJOYSTICK_HALFMODE)) != 0) {
        return (u32)-1;
    }
    return GetOrbisToSdlButtonKeycode((OrbisPadButtonDataOffset)cbutton) + 0x10000000;
}

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

    // Check and process tokens
    for (const auto& t : tokens) {
        if (string_to_keyboard_key_map.find(t) != string_to_keyboard_key_map.end()) {
            // Map to keyboard key
            u32 key_id = string_to_keyboard_key_map.at(t);
            if (!key1)
                key1 = key_id;
            else if (!key2)
                key2 = key_id;
            else if (!key3)
                key3 = key_id;
        } else if (string_to_axis_map.find(t) != string_to_axis_map.end()) {
            // Map to axis input ID
            u32 axis_id = GetAxisInputId(string_to_axis_map.at(t));
            if (axis_id == (u32)-1) {
                return InputBinding(0, 0, 0);
            }
            if (!key1)
                key1 = axis_id;
            else if (!key2)
                key2 = axis_id;
            else if (!key3)
                key3 = axis_id;
        } else if (string_to_cbutton_map.find(t) != string_to_cbutton_map.end()) {
            // Map to controller button input ID
            u32 cbutton_id = GetControllerButtonInputId((u32)string_to_cbutton_map.at(t));
            if (cbutton_id == (u32)-1) {
                return InputBinding(0, 0, 0);
            }
            if (!key1)
                key1 = cbutton_id;
            else if (!key2)
                key2 = cbutton_id;
            else if (!key3)
                key3 = cbutton_id;
        } else {
            // Invalid token found; return default binding
            return InputBinding(0, 0, 0);
        }
    }

    return InputBinding(key1, key2, key3);
}

// function that takes a controlleroutput, and returns the array's corresponding element's pointer
ControllerOutput* GetOutputPointer(const ControllerOutput& parsed) {
    // i wonder how long until someone notices this or I get rid of it
    int i = 0;
    for (; i[output_array] != ControllerOutput(OrbisPadButtonDataOffset::None, Axis::AxisMax);
         i++) {
        if (i[output_array] == parsed) {
            return &output_array[i];
        }
    }
    return &output_array[i];
}

void ParseInputConfig(const std::string game_id = "") {

    const auto config_file = Config::GetFoolproofKbmConfigFile(game_id);

    // todo: change usages of this to GetFoolproofKbmConfigFile (in the gui)
    if (game_id == "") {
        return;
    }

    // we reset these here so in case the user fucks up or doesn't include this,
    // we can fall back to default
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
            LOG_WARNING(Input, "Invalid format at line: {}, data: \"{}\", skipping line.",
                        lineCount, line);
            continue;
        }

        std::string output_string = line.substr(0, equal_pos);
        std::string input_string = line.substr(equal_pos + 1);
        std::size_t comma_pos = input_string.find(',');

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
        if (output_string == "key_toggle") {
            if (comma_pos != std::string::npos) {
                // handle key-to-key toggling (separate list?)
                InputBinding toggle_keys = GetBindingFromString(input_string);
                if (toggle_keys.KeyCount() != 2) {
                    LOG_WARNING(Input,
                                "Syntax error: Please provide exactly 2 keys: "
                                "first is the toggler, the second is the key to toggle: {}",
                                line);
                    continue;
                }
                ControllerOutput* toggle_out =
                    GetOutputPointer(ControllerOutput((OrbisPadButtonDataOffset)KEY_TOGGLE));
                BindingConnection toggle_connection =
                    BindingConnection(InputBinding(toggle_keys.key2), toggle_out, toggle_keys.key3);
                connections.insert(connections.end(), toggle_connection);
                continue;
            }
            LOG_WARNING(Input, "Invalid format at line: {}, data: \"{}\", skipping line.",
                        lineCount, line);
            continue;
        }
        if (output_string == "mouse_movement_params") {
            std::stringstream ss(input_string);
            char comma; // To hold the comma separators between the floats
            ss >> mouse_deadzone_offset >> comma >> mouse_speed >> comma >> mouse_speed_offset;

            // Check for invalid input (in case there's an unexpected format)
            if (ss.fail()) {
                LOG_WARNING(Input, "Failed to parse mouse movement parameters from line: {}", line);
            }
            continue;
        }

        // normal cases
        InputBinding binding = GetBindingFromString(input_string);
        BindingConnection connection(0, nullptr);
        auto button_it = string_to_cbutton_map.find(output_string);
        auto axis_it = string_to_axis_map.find(output_string);

        if (binding.IsEmpty()) {
            LOG_WARNING(Input, "Invalid format at line: {}, data: \"{}\", skipping line.",
                        lineCount, line);
            continue;
        }
        if (button_it != string_to_cbutton_map.end()) {
            connection =
                BindingConnection(binding, GetOutputPointer(ControllerOutput(button_it->second)));
            connections.insert(connections.end(), connection);

        } else if (axis_it != string_to_axis_map.end()) {
            int value_to_set = (binding.key3 & 0x80000000) != 0 ? 0
                               : (axis_it->second.axis == Axis::TriggerLeft ||
                                  axis_it->second.axis == Axis::TriggerRight)
                                   ? 127
                                   : axis_it->second.value;
            connection =
                BindingConnection(binding,
                                  GetOutputPointer(ControllerOutput(OrbisPadButtonDataOffset::None,
                                                                    axis_it->second.axis)),
                                  value_to_set);
            connections.insert(connections.end(), connection);
        } else {
            LOG_WARNING(Input, "Invalid format at line: {}, data: \"{}\", skipping line.",
                        lineCount, line);
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
        return (u32)-1;
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
    int value_mask;
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
    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
    case SDL_EVENT_GAMEPAD_BUTTON_UP:
        return (u32)e.gbutton.button + 0x10000000; // I believe this range is unused
    case SDL_EVENT_GAMEPAD_AXIS_MOTION:
        // todo: somehow put this value into the correct connection
        // solution 1: add it to the keycode as a 0x0FF00000 (a bit hacky but works I guess?)
        // I guess in software developement, there really is nothing more permanent than a temporary
        // solution
        value_mask = (u32)((e.gaxis.value / 256 + 128) << 20); // +-32000 to +-128 to 0-255
        return (u32)e.gaxis.axis + 0x80000000 +
               value_mask; // they are pushed to the end of the sorted array
    default:
        return (u32)-1;
    }
}

GameController* ControllerOutput::controller = nullptr;
void ControllerOutput::SetControllerOutputController(GameController* c) {
    ControllerOutput::controller = c;
}

void ToggleKeyInList(u32 key) {
    if ((key & 0x80000000) != 0) {
        LOG_ERROR(Input, "Toggling analog inputs is not supported!");
        return;
    }
    auto it = std::find(toggled_keys.begin(), toggled_keys.end(), key);
    if (it == toggled_keys.end()) {
        toggled_keys.insert(toggled_keys.end(), key);
        LOG_DEBUG(Input, "Added {} to toggled keys", key);
    } else {
        toggled_keys.erase(it);
        LOG_DEBUG(Input, "Removed {} from toggled keys", key);
    }
}

void ControllerOutput::ResetUpdate() {
    state_changed = false;
    new_button_state = false;
    new_param = 0;
}
void ControllerOutput::AddUpdate(bool pressed, bool analog, u32 param) {
    state_changed = true;
    if (button != OrbisPadButtonDataOffset::None) {
        if (analog) {
            new_button_state |= abs((s32)param) > 0x40;
        } else {
            new_button_state |= pressed;
            new_param = param;
        }

    } else if (axis != Axis::AxisMax) {
        switch (axis) {
        case Axis::TriggerLeft:
        case Axis::TriggerRight:
            // if it's a button input, then we know the value to set, so the param is 0.
            // if it's an analog input, then the param isn't 0
            // warning: doesn't work yet
            new_param = SDL_clamp((pressed ? (s32)param : 0) + new_param, 0, 127);
            break;
        default:
            // todo: do the same as above
            new_param = SDL_clamp((pressed ? (s32)param : 0) + new_param, -127, 127);
            break;
        }
    }
}
void ControllerOutput::FinalizeUpdate() {
    old_button_state = new_button_state;
    old_param = new_param;
    float touchpad_x = 0;
    if (button != OrbisPadButtonDataOffset::None) {
        switch ((u32)button) {
        case (u32)OrbisPadButtonDataOffset::TouchPad:
            touchpad_x = Config::getBackButtonBehavior() == "left"    ? 0.25f
                         : Config::getBackButtonBehavior() == "right" ? 0.75f
                                                                      : 0.5f;
            controller->SetTouchpadState(0, new_button_state, touchpad_x, 0.5f);
            controller->CheckButton(0, button, new_button_state);
            break;
        case LEFTJOYSTICK_HALFMODE:
            leftjoystick_halfmode = new_button_state;
            break;
        case RIGHTJOYSTICK_HALFMODE:
            rightjoystick_halfmode = new_button_state;
            break;
        case KEY_TOGGLE:
            if (new_button_state) {
                // LOG_DEBUG(Input, "Toggling a button...");
                ToggleKeyInList(new_param);
            }
            break;
        default: // is a normal key (hopefully)
            controller->CheckButton(0, (OrbisPadButtonDataOffset)button, new_button_state);
            break;
        }
    } else if (axis != Axis::AxisMax) {
        float multiplier = 1.0;
        switch (axis) {
        case Axis::LeftX:
        case Axis::LeftY:
            multiplier = leftjoystick_halfmode ? 0.5 : 1.0;
            break;
        case Axis::RightX:
        case Axis::RightY:
            multiplier = rightjoystick_halfmode ? 0.5 : 1.0;
            break;
        case Axis::TriggerLeft:
        case Axis::TriggerRight:
            controller->Axis(0, axis, GetAxis(0x0, 0x80, new_param));
            // Also handle button counterpart for TriggerLeft and TriggerRight
            controller->CheckButton(0,
                                    axis == Axis::TriggerLeft ? OrbisPadButtonDataOffset::L2
                                                              : OrbisPadButtonDataOffset::R2,
                                    new_param > 0x20);
            return;
        default:
            break;
        }
        controller->Axis(0, axis, GetAxis(-0x80, 0x80, new_param * multiplier));
    }
}

// Updates the list of pressed keys with the given input.
// Returns whether the list was updated or not.
bool UpdatePressedKeys(u32 value, bool is_pressed) {
    // Skip invalid inputs
    if (value == (u32)-1) {
        return false;
    }
    if ((value & 0x80000000) != 0) {
        // analog input, it gets added when it first sends an event,
        // and from there, it only changes the parameter
        // reverse iterate until we get out of the 0x8000000 range, if found,
        // update the parameter, if not, add it to the end
        u32 value_to_search = value & 0xF00FFFFF;
        for (auto& it = --pressed_keys.end(); (it->first & 0x80000000) != 0; it--) {
            if ((it->first & 0xF00FFFFF) == value_to_search) {
                it->first = value;
                // LOG_DEBUG(Input, "New value for {:X}: {:x}", value, value);
                return true;
            }
        }
        // LOG_DEBUG(Input, "Input activated for the first time, adding it to the list");
        pressed_keys.insert(pressed_keys.end(), {value, false});
        return true;
    }
    if (is_pressed) {
        // Find the correct position for insertion to maintain order
        auto it =
            std::lower_bound(pressed_keys.begin(), pressed_keys.end(), value,
                             [](const std::pair<u32, bool>& pk, u32 v) { return pk.first < v; });

        // Insert only if 'value' is not already in the list
        if (it == pressed_keys.end() || it->first != value) {
            pressed_keys.insert(it, {value, false});
            return true;
        }
    } else {
        // Remove 'value' from the list if it's not pressed
        auto it =
            std::find_if(pressed_keys.begin(), pressed_keys.end(),
                         [value](const std::pair<u32, bool>& pk) { return pk.first == value; });
        if (it != pressed_keys.end()) {
            pressed_keys.erase(it);
            return true;
        }
    }
    return false;
}
// Check if a given binding's all keys are currently active.
// For now it also extracts the analog inputs' parameters.
void IsInputActive(BindingConnection& connection, bool& active, bool& analog) {
    InputBinding i = connection.binding;
    // Extract keys from InputBinding and ignore unused (0) or toggled keys
    std::list<u32> input_keys = {i.key1, i.key2, i.key3};
    input_keys.remove(0);
    for (auto key = input_keys.begin(); key != input_keys.end();) {
        if (std::find(toggled_keys.begin(), toggled_keys.end(), *key) != toggled_keys.end()) {
            key = input_keys.erase(key); // Use the returned iterator
        } else {
            ++key; // Increment only if no erase happened
        }
    }
    if (input_keys.empty()) {
        LOG_DEBUG(Input, "No actual inputs to check, returning true");
        active = true;
        return;
    }

    // Iterator for pressed_keys, starting from the beginning
    auto pressed_it = pressed_keys.begin();

    // Store pointers to flags in pressed_keys that need to be set if all keys are active
    std::list<bool*> flags_to_set;

    // Check if all keys in input_keys are active
    for (u32 key : input_keys) {
        bool key_found = false;

        // Search for the current key in pressed_keys starting from the last checked position
        while (pressed_it != pressed_keys.end() && (pressed_it->first & 0x80000000) == 0) {
            if (pressed_it->first == key) {

                key_found = true;
                flags_to_set.push_back(&pressed_it->second);

                ++pressed_it; // Move to the next key in pressed_keys
                break;
            }
            ++pressed_it;
        }
        if (!key_found && (key & 0x80000000) != 0) {
            // reverse iterate over the analog inputs, as they can't be sorted
            auto& rev_it = --pressed_keys.end();
            for (auto rev_it = --pressed_keys.end(); (rev_it->first & 0x80000000) != 0; rev_it--) {
                if ((rev_it->first & 0xF00FFFFF) == (key & 0xF00FFFFF)) {
                    connection.parameter = (u32)((s32)((rev_it->first & 0x0FF00000) >> 20) - 128);
                    // LOG_DEBUG(Input, "Extracted the following param: {:X} from {:X}",
                    //          (s32)connection.parameter, rev_it->first);
                    key_found = true;
                    analog = true;
                    flags_to_set.push_back(&rev_it->second);
                    break;
                }
            }
        }
        if (!key_found) {
            return;
        }
    }

    bool is_fully_blocked = true;
    for (bool* flag : flags_to_set) {
        is_fully_blocked &= *flag;
    }
    if (is_fully_blocked) {
        return;
    }
    for (bool* flag : flags_to_set) {
        *flag = true;
    }

    LOG_DEBUG(Input, "Input found: {}", i.ToString());
    active = true;
    return; // All keys are active
}

void ActivateOutputsFromInputs() {
    // LOG_DEBUG(Input, "Start of an input frame...");
    for (auto& it : pressed_keys) {
        it.second = false;
    }

    // this is the cleanest looking code I've ever written, too bad it is not working
    // (i left the last part in by accident, then it turnd out to still be true even after I thought
    // everything is good) (but now it really should be fine)
    for (auto& it : output_array) {
        it.ResetUpdate();
    }
    for (auto& it : connections) {
        bool active = false, analog_input_detected = false;
        IsInputActive(it, active, analog_input_detected);
        it.output->AddUpdate(active, analog_input_detected, it.parameter);
    }
    for (auto& it : output_array) {
        it.FinalizeUpdate();
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