// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "input_handler.h"

#include "fstream"
#include "iostream"
#include "map"
#include "unordered_map"
#include "list"
#include "sstream"
#include "string"
#include "vector"

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_timer.h"

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

std::list<u32> pressed_keys = std::list<u32>();
std::list<BindingConnection> connections = std::list<BindingConnection>();

void toggleMouseEnabled() {
    mouse_enabled ^= true;
}
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
    ControllerOutput(0, Input::Axis::LeftX),
    ControllerOutput(0, Input::Axis::LeftY),
    ControllerOutput(0, Input::Axis::RightX),
    ControllerOutput(0, Input::Axis::RightY),
    ControllerOutput(0, Input::Axis::TriggerLeft),
    ControllerOutput(0, Input::Axis::TriggerRight),

    ControllerOutput(LEFTJOYSTICK_HALFMODE),
    ControllerOutput(RIGHTJOYSTICK_HALFMODE),

    // End marker to signify the end of the array
    ControllerOutput(0, Input::Axis::AxisMax)
};

// parsing related functions

// syntax: 'name, name,name' or 'name,name' or 'name'
InputBinding getBindingFromString(std::string& line) {
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
    if (tokens.size() > 0) key1 = string_to_keyboard_key_map.at(tokens[0]);
    if (tokens.size() > 1) key2 = string_to_keyboard_key_map.at(tokens[1]);
    if (tokens.size() > 2) key3 = string_to_keyboard_key_map.at(tokens[2]);

    return InputBinding(key1, key2, key3);
}

// function that takes a controlleroutput, and returns the array's corresponding element's pointer
ControllerOutput* getOutputPointer(const ControllerOutput& parsed) {
    // i wonder how long until someone notices this or I get rid of it
    for (int i = 0; i[output_array] != ControllerOutput(0, Axis::AxisMax); i++) {
        if(i[output_array] == parsed) {
            return &output_array[i];
        }
    }
    return nullptr;
}

void parseInputConfig(const std::string game_id = "") {
    
    const auto config_file = Config::getFoolproofKbmConfigFile(game_id);

    // todo: change usages of this to getFoolproofKbmConfigFile (in the gui)
    if(game_id == "") {
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
            LOG_ERROR(Input, "Invalid format at line: {}, data: \"{}\", skipping line.", lineCount, line);
            continue;
        }

        std::string before_equals = line.substr(0, equal_pos);
        std::string after_equals = line.substr(equal_pos + 1);
        std::size_t comma_pos = after_equals.find(',');
        

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
                LOG_ERROR(Input, "todo: {}", line);
                continue;
            }
            LOG_ERROR(Input, "Invalid format at line: {}, data: \"{}\", skipping line.", lineCount, line);
            continue;
        }
        if (before_equals == "mouse_movement_params") {
            LOG_ERROR(Input, "todo: {}", line);
            continue;
        }

        // normal cases
        InputBinding binding = getBindingFromString(after_equals);
        BindingConnection connection(0, nullptr);
        auto button_it = string_to_cbutton_map.find(before_equals);
        auto axis_it = string_to_axis_map.find(before_equals);

        if(binding.isEmpty()) {
            LOG_ERROR(Input, "Invalid format at line: {}, data: \"{}\", skipping line.", lineCount, line);
            continue;
        }
        if (button_it != string_to_cbutton_map.end()) {
            connection = BindingConnection(binding, getOutputPointer(ControllerOutput(button_it->second)));
            connections.insert(connections.end(), connection);

        } else if (axis_it != string_to_axis_map.end()) {
            connection = BindingConnection(binding, getOutputPointer(ControllerOutput(0, axis_it->second.axis)), axis_it->second.value);
            connections.insert(connections.end(), connection);
        } else {
            LOG_ERROR(Input, "Invalid format at line: {}, data: \"{}\", skipping line.", lineCount, line);
            continue;
        }
        //LOG_INFO(Input, "Succesfully parsed line {}", lineCount);
    }
    file.close();
    LOG_INFO(Input, "Done parsing the input config!");
}

u32 getMouseWheelEvent(const SDL_Event& event) {
    if (event.type != SDL_EVENT_MOUSE_WHEEL && event.type != SDL_EVENT_MOUSE_WHEEL_OFF) {
        LOG_ERROR(Input, "Something went wrong with wheel input parsing!");
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

u32 InputBinding::getInputIDFromEvent(const SDL_Event& e) {
    switch(e.type) {
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:
        return e.key.key;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
        return (u32)e.button.button;
    case SDL_EVENT_MOUSE_WHEEL:
    case SDL_EVENT_MOUSE_WHEEL_OFF:
        return getMouseWheelEvent(e);
    default:
        return (u32)-1;
    }
}

GameController* ControllerOutput::controller = nullptr;
void ControllerOutput::setControllerOutputController(GameController* c) {
    ControllerOutput::controller = c;
}

void ControllerOutput::update(bool pressed, int a_value) {
    float touchpad_x = 0;
    if(button != 0){
        switch (button) {
        case OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_TOUCH_PAD:
            touchpad_x = Config::getBackButtonBehavior() == "left"    ? 0.25f
                        : Config::getBackButtonBehavior() == "right" ? 0.75f
                                                                    : 0.5f;
            controller->SetTouchpadState(0, true, touchpad_x, 0.5f);
            controller->CheckButton(0, button, pressed);
            break;
        case LEFTJOYSTICK_HALFMODE:
            leftjoystick_halfmode ^= pressed; // toggle if pressed, don't change otherwise
            break;
        case RIGHTJOYSTICK_HALFMODE:
            rightjoystick_halfmode ^= pressed;
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
            break;
        case Axis::RightX:
        case Axis::RightY:
            multiplier = rightjoystick_halfmode ? 0.5 : 1.0;
            break;
        case Axis::TriggerLeft:
        case Axis::TriggerRight:
            // todo: verify this works (This probably works from testing, 
            // but needs extra info (multiple input to the same trigger?))
            axis_value = SDL_clamp((pressed ? a_value : 0) * multiplier, 0, 127);
            controller->Axis(0, axis, GetAxis(0, 0x80, axis_value));
            return;
        default:
            break;
        }
        axis_value = SDL_clamp((pressed ? a_value : 0) * multiplier, -127, 127);
        int ax = GetAxis(-0x80, 0x80, axis_value);
        controller->Axis(0, axis, ax);
    } else {
        LOG_ERROR(Input, "Controller output with no values detected!");
    }
}
void ControllerOutput::addUpdate(bool pressed, int a_value) {
    
    float touchpad_x = 0;
    if(button != 0){
        if(!pressed) {
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
            leftjoystick_halfmode = pressed;
            break;
        case RIGHTJOYSTICK_HALFMODE:
            rightjoystick_halfmode = pressed;
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
            break;
        case Axis::RightX:
        case Axis::RightY:
            multiplier = rightjoystick_halfmode ? 0.5 : 1.0;
            break;
        case Axis::TriggerLeft:
        case Axis::TriggerRight:
            // todo: verify this works
            axis_value = SDL_clamp((pressed ? a_value : 0) * multiplier + axis_value, 0, 127);
            controller->Axis(0, axis, GetAxis(0, 0x80, axis_value));
            return;
        default:
            break;
        }
        axis_value = SDL_clamp((pressed ? a_value : 0) * multiplier + axis_value, -127, 127);
        controller->Axis(0, axis, GetAxis(-0x80, 0x80, axis_value));
        //LOG_INFO(Input, "Axis value delta: {} final value: {}", (pressed ? a_value : 0), axis_value);
    } else {
        LOG_ERROR(Input, "Controller output with no values detected!");
    }
}

void updatePressedKeys(u32 value, bool is_pressed) {
    if (is_pressed) {
        // Find the correct position for insertion to maintain order
        auto it = std::lower_bound(pressed_keys.begin(), pressed_keys.end(), value);
        
        // Insert only if 'value' is not already in the list
        if (it == pressed_keys.end() || *it != value) {
            pressed_keys.insert(it, value);
        }
    } else {
        // Remove 'value' from the list if it's not pressed
        pressed_keys.remove(value);
    }
}

// Check if a given binding's all keys are currently active.
bool isInputActive(const InputBinding& i) {
    /* how to check if a binding is currently held down:
        iterate until connection.InputBinding.key3 is found or we reach the end
            iterate from that point until connection.InputBinding.key2 is found or we reach the end
                iterate from that point until connection.InputBinding.key1 is found or we reach the end
        if we ever reach the end, return false
        if the next key to find would be 0, return true
        if all three are found return true
    */
    auto it = pressed_keys.begin();

    if (i.key1 != 0) {
        it = std::find(it, pressed_keys.end(), i.key1);
        if (it == pressed_keys.end()) return false;
        ++it;  // Move to the next element for subsequent checks
    }

    if (i.key2 != 0) {
        it = std::find(it, pressed_keys.end(), i.key2);
        if (it == pressed_keys.end()) return false;
        ++it;
    }

    if (i.key3 != 0) {
        it = std::find(it, pressed_keys.end(), i.key3);
        if (it == pressed_keys.end()) return false;
    }

    // All required keys were found in order
    LOG_INFO(Input, "A valid held input is found!");
    return true;
}

void activateOutputsFromInputs() {
    // reset everything
    for(auto it = connections.begin(); it != connections.end(); it++) {
        if (it->output) {
            it->output->update(false, 0);
        } else {
            // LOG_ERROR(Input, "Null output in BindingConnection at position {}\n    data: {}: {}", 
            //         std::distance(connections.begin(), it), 
            //         it->binding.toString(), it->output->toString());
            LOG_ERROR(Input, "Null output in BindingConnection at position {}", std::distance(connections.begin(), it));
        }
    }
    // iterates over the connections, and updates them depending on whether the corresponding input trio is found
    for(auto it = connections.begin(); it != connections.end(); it++) {
        if (it->output) {
            it->output->addUpdate(isInputActive(it->binding), it->axis_value);
        } else {
            //LOG_ERROR(Input, "Null output in BindingConnection at position {}", std::distance(connections.begin(), it));
        }
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
        controller->Axis(0, axis_x, GetAxis(-0x80, 0x80, a_x));
        controller->Axis(0, axis_y, GetAxis(-0x80, 0x80, a_y));
    } else {
        controller->Axis(0, axis_x, GetAxis(-0x80, 0x80, 0));
        controller->Axis(0, axis_y, GetAxis(-0x80, 0x80, 0));
    }
}

Uint32 mousePolling(void* param, Uint32 id, Uint32 interval) {
    auto* data = (GameController*)param;
    updateMouse(data);
    return interval;
}

}