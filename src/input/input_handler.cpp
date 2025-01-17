// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "input_handler.h"

#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <ranges>
#include <sstream>
#include <string>
#include <string_view>
#include <typeinfo>
#include <unordered_map>
#include <vector>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_timer.h"

#include "common/config.h"
#include "common/elf_info.h"
#include "common/io_file.h"
#include "common/path_util.h"
#include "common/version.h"
#include "input/controller.h"
#include "input/input_mouse.h"

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

bool leftjoystick_halfmode = false, rightjoystick_halfmode = false;

std::list<std::pair<InputEvent, bool>> pressed_keys;
std::list<InputID> toggled_keys;
static std::vector<BindingConnection> connections;

auto output_array = std::array{
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

    ControllerOutput(OrbisPadButtonDataOffset::None, Axis::AxisMax),
};

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
    std::array<InputID, 3> keys = {InputID(), InputID(), InputID()};

    // Check and process tokens
    for (const auto token : std::views::split(line, ',')) { // Split by comma
        const std::string t(token.begin(), token.end());
        InputID input;

        if (string_to_keyboard_key_map.find(t) != string_to_keyboard_key_map.end()) {
            input = InputID(InputType::KeyboardMouse, string_to_keyboard_key_map.at(t));
        } else if (string_to_axis_map.find(t) != string_to_axis_map.end()) {
            input = InputID(InputType::Axis, (u32)string_to_axis_map.at(t).axis);
        } else if (string_to_cbutton_map.find(t) != string_to_cbutton_map.end()) {
            input = InputID(InputType::Controller, (u32)string_to_cbutton_map.at(t));
        } else {
            // Invalid token found; return default binding
            LOG_DEBUG(Input, "Invalid token found: {}", t);
            return InputBinding();
        }

        // Assign to the first available slot
        for (auto& key : keys) {
            if (!key.IsValid()) {
                key = input;
                break;
            }
        }
    }
    LOG_DEBUG(Input, "Parsed line: {} {} {}", keys[0].ToString(), keys[1].ToString(),
              keys[2].ToString());
    return InputBinding(keys[0], keys[1], keys[2]);
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
    float mouse_deadzone_offset = 0.5;
    float mouse_speed = 1;
    float mouse_speed_offset = 0.125;
    int lineCount = 0;

    std::ifstream file(config_file);
    std::string line = "";
    while (std::getline(file, line)) {
        lineCount++;

        // Strip the ; and whitespace
        line.erase(std::remove_if(line.begin(), line.end(),
                                  [](unsigned char c) { return std::isspace(c); }),
                   line.end());

        if (line.empty()) {
            continue;
        }
        // Truncate lines starting at #
        std::size_t comment_pos = line.find('#');
        if (comment_pos != std::string::npos) {
            line = line.substr(0, comment_pos);
        }
        // Remove trailing semicolon
        if (!line.empty() && line[line.length() - 1] == ';') {
            line = line.substr(0, line.length() - 1);
        }
        if (line.empty()) {
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
                SetMouseToJoystick(1);
            } else if (input_string == "right") {
                SetMouseToJoystick(2);
            } else {
                LOG_WARNING(Input, "Invalid argument for mouse-to-joystick binding");
                SetMouseToJoystick(0);
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
                ControllerOutput* toggle_out = &*std::ranges::find(
                    output_array, ControllerOutput((OrbisPadButtonDataOffset)KEY_TOGGLE));
                BindingConnection toggle_connection = BindingConnection(
                    InputBinding(toggle_keys.keys[0]), toggle_out, 0, toggle_keys.keys[1]);
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
        BindingConnection connection(InputID(), nullptr);
        auto button_it = string_to_cbutton_map.find(output_string);
        auto axis_it = string_to_axis_map.find(output_string);

        if (binding.IsEmpty()) {
            LOG_WARNING(Input, "Invalid format at line: {}, data: \"{}\", skipping line.",
                        lineCount, line);
            continue;
        }
        if (button_it != string_to_cbutton_map.end()) {
            connection = BindingConnection(
                binding, &*std::ranges::find(output_array, ControllerOutput(button_it->second)));
            connections.insert(connections.end(), connection);

        } else if (axis_it != string_to_axis_map.end()) {
            int value_to_set = binding.keys[2].type == InputType::Axis ? 0
                               : (axis_it->second.axis == Axis::TriggerLeft ||
                                  axis_it->second.axis == Axis::TriggerRight)
                                   ? 127
                                   : axis_it->second.value;
            connection = BindingConnection(
                binding,
                &*std::ranges::find(output_array, ControllerOutput(OrbisPadButtonDataOffset::None,
                                                                   axis_it->second.axis)),
                value_to_set);
            connections.insert(connections.end(), connection);
        } else {
            LOG_WARNING(Input, "Invalid format at line: {}, data: \"{}\", skipping line.",
                        lineCount, line);
            continue;
        }
        LOG_DEBUG(Input, "Succesfully parsed line {}", lineCount);
    }
    file.close();
    std::sort(connections.begin(), connections.end());
    for (auto& c : connections) {
        LOG_DEBUG(Input, "Binding: {}", c.binding.ToString());
    }
    LOG_DEBUG(Input, "Done parsing the input config!");
}

u32 GetMouseWheelEvent(const SDL_Event& event) {
    if (event.type != SDL_EVENT_MOUSE_WHEEL && event.type != SDL_EVENT_MOUSE_WHEEL_OFF) {
        LOG_WARNING(Input, "Something went wrong with wheel input parsing!");
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

InputEvent InputBinding::GetInputEventFromSDLEvent(const SDL_Event& e) {
    int value_mask;
    switch (e.type) {
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:
        return InputEvent(InputType::KeyboardMouse, e.key.key, e.type == SDL_EVENT_KEY_DOWN, 0);
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
        return InputEvent(InputType::KeyboardMouse, e.button.button,
                          e.type == SDL_EVENT_MOUSE_BUTTON_DOWN, 0);
    case SDL_EVENT_MOUSE_WHEEL:
    case SDL_EVENT_MOUSE_WHEEL_OFF:
        return InputEvent(InputType::KeyboardMouse, GetMouseWheelEvent(e),
                          e.type == SDL_EVENT_MOUSE_WHEEL, 0);
    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
    case SDL_EVENT_GAMEPAD_BUTTON_UP:
        return InputEvent(InputType::Controller, e.gbutton.button,
                          e.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN, 0);
    case SDL_EVENT_GAMEPAD_AXIS_MOTION:
        return InputEvent(InputType::Controller, e.gaxis.axis, true, (s8)(e.gaxis.value / 256));
    default:
        return InputEvent(InputType::Count, (u32)-1, false, 0);
    }
}

GameController* ControllerOutput::controller = nullptr;
void ControllerOutput::SetControllerOutputController(GameController* c) {
    ControllerOutput::controller = c;
}

void ToggleKeyInList(InputID input) {
    if (input.type == InputType::Axis) {
        LOG_ERROR(Input, "Toggling analog inputs is not supported!");
        return;
    }
    auto it = std::find(toggled_keys.begin(), toggled_keys.end(), input);
    if (it == toggled_keys.end()) {
        toggled_keys.insert(toggled_keys.end(), input);
        LOG_DEBUG(Input, "Added {} to toggled keys", input.ToString());
    } else {
        toggled_keys.erase(it);
        LOG_DEBUG(Input, "Removed {} from toggled keys", input.ToString());
    }
}

void ControllerOutput::ResetUpdate() {
    state_changed = false;
    new_button_state = false;
    new_param = 0;
}
void ControllerOutput::AddUpdate(InputEvent event) {
    state_changed = true;
    if ((u32)button == KEY_TOGGLE) {
        if (event.active) {
            LOG_DEBUG(Input, "Toggling a button...");
            ToggleKeyInList(event.input); // todo: fix key toggle
        }
    } else if (button != OrbisPadButtonDataOffset::None) {
        if (event.input.type == InputType::Axis) {
            new_button_state |= abs((s32)event.axis_value) > 0x40;
        } else {
            new_button_state |= event.active;
            new_param = event.axis_value;
        }

    } else if (axis != Axis::AxisMax) {
        switch (axis) {
        case Axis::TriggerLeft:
        case Axis::TriggerRight:
            // if it's a button input, then we know the value to set, so the param is 0.
            // if it's an analog input, then the param isn't 0
            new_param = (event.active ? (s32)event.axis_value : 0) + new_param;
            break;
        default:
            new_param = (event.active ? (s32)event.axis_value : 0) + new_param;
            break;
        }
    }
}
void ControllerOutput::FinalizeUpdate() {
    if (!state_changed) {
        // return;
    }
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
        // KEY_TOGGLE isn't handled here anymore, as this function doesn't have the necessary data
        // to do it, and it would be inconvenient to force it here, when AddUpdate does the job just
        // fine, and a toggle doesn't have to checked against every input that's bound to it, it's
        // enough that one is pressed
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
bool UpdatePressedKeys(InputEvent event) {
    // Skip invalid inputs
    InputID input = event.input;
    if (input.sdl_id == (u32)-1) {
        return false;
    }
    if (input.type == InputType::Axis) {
        // analog input, it gets added when it first sends an event,
        // and from there, it only changes the parameter
        const auto& it = std::find_if(
            pressed_keys.begin(), pressed_keys.end(),
            [input](const std::pair<InputEvent, bool>& e) { return e.first.input == input; });
        if (it == pressed_keys.end()) {
            pressed_keys.push_back({event, false});
        } else {
            it->first.axis_value = event.axis_value;
        }
        return true;
    }
    if (event.active) {
        // Find the correct position for insertion to maintain order
        auto it = std::lower_bound(pressed_keys.begin(), pressed_keys.end(), input,
                                   [](const std::pair<InputEvent, bool>& e, InputID i) {
                                       return e.first.input.type <= i.type &&
                                              e.first.input.sdl_id < i.sdl_id;
                                   });

        // Insert only if 'value' is not already in the list
        if (it == pressed_keys.end() || it->first.input != input) {
            pressed_keys.insert(it, {event, false});
            return true;
        }
    } else {
        // Remove 'value' from the list if it's not pressed
        auto it = std::find_if(
            pressed_keys.begin(), pressed_keys.end(),
            [input](const std::pair<InputEvent, bool>& e) { return e.first.input == input; });
        if (it != pressed_keys.end()) {
            pressed_keys.erase(it);
            return true;
        }
    }
    return false;
}
// Check if the binding's all keys are currently active.
// It also extracts the analog inputs' parameters, and updates the input hierarchy flags.
InputEvent BindingConnection::ProcessBinding() {
    // the last key is always set (if the connection isn't empty),
    // and the analog inputs are always the last one due to how they are sorted,
    // so this signifies whether or not the input is analog
    InputEvent event = InputEvent(binding.keys[3]);
    if (pressed_keys.empty()) {
        return event;
    }
    if (event.input.type != InputType::Axis) {
        // for button inputs
        event.axis_value = axis_param;
    }
    // it's a bit scuffed, but if the output is a toggle, then we put the key here
    if ((u32)output->button == KEY_TOGGLE) {
        event.input = toggle;
    }

    // Extract keys from InputBinding and ignore unused or toggled keys
    std::list<InputID> input_keys = {binding.keys[0], binding.keys[1], binding.keys[2]};
    input_keys.remove(InputID());
    for (auto key = input_keys.begin(); key != input_keys.end();) {
        if (std::find(toggled_keys.begin(), toggled_keys.end(), *key) != toggled_keys.end()) {
            key = input_keys.erase(key); // Use the returned iterator
        } else {
            ++key; // Increment only if no erase happened
        }
    }
    if (input_keys.empty()) {
        LOG_DEBUG(Input, "No actual inputs to check, returning true");
        event.active = true;
        return event;
    }

    // Iterator for pressed_keys, starting from the beginning
    auto pressed_it = pressed_keys.begin();

    // Store pointers to flags in pressed_keys that need to be set if all keys are active
    std::list<bool*> flags_to_set;

    // Check if all keys in input_keys are active
    for (InputID key : input_keys) {
        bool key_found = false;

        while (pressed_it != pressed_keys.end()) {
            if (pressed_it->first.input == key && pressed_it->second == false) {
                key_found = true;
                flags_to_set.push_back(&pressed_it->second);
                if (pressed_it->first.input.type == InputType::Axis) {
                    event.axis_value = pressed_it->first.axis_value;
                }
                ++pressed_it;
                break;
            }
            ++pressed_it;
        }
        if (!key_found) {
            return event;
        }
    }

    for (bool* flag : flags_to_set) {
        *flag = true;
    }

    LOG_DEBUG(Input, "Input found: {}", binding.ToString());
    event.active = true;
    return event; // All keys are active
}

void ActivateOutputsFromInputs() {
    // Reset values and flags
    for (auto& it : pressed_keys) {
        it.second = false;
    }
    for (auto& it : output_array) {
        it.ResetUpdate();
    }

    // Iterate over all inputs, and update their respecive outputs accordingly
    for (auto& it : connections) {
        it.output->AddUpdate(it.ProcessBinding());
    }

    // Update all outputs
    for (auto& it : output_array) {
        it.FinalizeUpdate();
    }
}

} // namespace Input
