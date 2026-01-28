// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
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
#include "common/singleton.h"
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
std::pair<int, int> leftjoystick_deadzone, rightjoystick_deadzone, lefttrigger_deadzone,
    righttrigger_deadzone;

std::list<std::pair<InputEvent, bool>> pressed_keys;
std::list<InputID> toggled_keys;
static std::vector<BindingConnection> connections;

std::array<ControllerAllOutputs, 4> output_arrays = {
    ControllerAllOutputs(0),
    ControllerAllOutputs(1),
    ControllerAllOutputs(2),
    ControllerAllOutputs(3),
};

void ControllerOutput::LinkJoystickAxes() {
    // for (int i = 17; i < 23; i += 2) {
    //     delete output_array[i].new_param;
    //     output_array[i].new_param = output_array[i + 1].new_param;
    // }
}

static OrbisPadButtonDataOffset SDLGamepadToOrbisButton(u8 button) {
    using OPBDO = OrbisPadButtonDataOffset;

    switch (button) {
    case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
        return OPBDO::Down;
    case SDL_GAMEPAD_BUTTON_DPAD_UP:
        return OPBDO::Up;
    case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
        return OPBDO::Left;
    case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
        return OPBDO::Right;
    case SDL_GAMEPAD_BUTTON_SOUTH:
        return OPBDO::Cross;
    case SDL_GAMEPAD_BUTTON_NORTH:
        return OPBDO::Triangle;
    case SDL_GAMEPAD_BUTTON_WEST:
        return OPBDO::Square;
    case SDL_GAMEPAD_BUTTON_EAST:
        return OPBDO::Circle;
    case SDL_GAMEPAD_BUTTON_START:
        return OPBDO::Options;
    case SDL_GAMEPAD_BUTTON_TOUCHPAD:
        return OPBDO::TouchPad;
    case SDL_GAMEPAD_BUTTON_TOUCHPAD_LEFT:
        return OPBDO::TouchPad;
    case SDL_GAMEPAD_BUTTON_TOUCHPAD_CENTER:
        return OPBDO::TouchPad;
    case SDL_GAMEPAD_BUTTON_TOUCHPAD_RIGHT:
        return OPBDO::TouchPad;
    case SDL_GAMEPAD_BUTTON_BACK:
        return OPBDO::TouchPad;
    case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:
        return OPBDO::L1;
    case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER:
        return OPBDO::R1;
    case SDL_GAMEPAD_BUTTON_LEFT_STICK:
        return OPBDO::L3;
    case SDL_GAMEPAD_BUTTON_RIGHT_STICK:
        return OPBDO::R3;
    default:
        return OPBDO::None;
    }
}

Axis GetAxisFromSDLAxis(u8 sdl_axis) {
    switch (sdl_axis) {
    case SDL_GAMEPAD_AXIS_LEFTX:
        return Axis::LeftX;
    case SDL_GAMEPAD_AXIS_LEFTY:
        return Axis::LeftY;
    case SDL_GAMEPAD_AXIS_RIGHTX:
        return Axis::RightX;
    case SDL_GAMEPAD_AXIS_RIGHTY:
        return Axis::RightY;
    case SDL_GAMEPAD_AXIS_LEFT_TRIGGER:
        return Axis::TriggerLeft;
    case SDL_GAMEPAD_AXIS_RIGHT_TRIGGER:
        return Axis::TriggerRight;
    default:
        return Axis::AxisMax;
    }
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
            input = InputID(InputType::Axis, string_to_axis_map.at(t).axis);
        } else if (string_to_cbutton_map.find(t) != string_to_cbutton_map.end()) {
            input = InputID(InputType::Controller, string_to_cbutton_map.at(t));
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
    LOG_DEBUG(Input, "Parsed line: {}", InputBinding(keys[0], keys[1], keys[2]).ToString());
    return InputBinding(keys[0], keys[1], keys[2]);
}

std::optional<int> parseInt(const std::string& s) {
    try {
        return std::stoi(s);
    } catch (...) {
        return std::nullopt;
    }
};

void ParseInputConfig(const std::string game_id = "") {
    std::string game_id_or_default = Config::GetUseUnifiedInputConfig() ? "default" : game_id;
    const auto config_file = Config::GetFoolproofInputConfigFile(game_id_or_default);
    const auto global_config_file = Config::GetFoolproofInputConfigFile("global");

    // we reset these here so in case the user fucks up or doesn't include some of these,
    // we can fall back to default
    connections.clear();
    float mouse_deadzone_offset = 0.5;
    float mouse_speed = 1;
    float mouse_speed_offset = 0.125;

    leftjoystick_deadzone = {1, 127};
    rightjoystick_deadzone = {1, 127};
    lefttrigger_deadzone = {1, 127};
    righttrigger_deadzone = {1, 127};

    Config::SetOverrideControllerColor(false);
    Config::SetControllerCustomColor(0, 0, 255);

    int lineCount = 0;

    std::ifstream config_stream(config_file);
    std::ifstream global_config_stream(global_config_file);
    std::string line = "";
    auto ProcessLine = [&]() -> void {
        lineCount++;

        // Strip the ; and whitespace
        line.erase(std::remove_if(line.begin(), line.end(),
                                  [](unsigned char c) { return std::isspace(c); }),
                   line.end());
        if (line.empty()) {
            return;
        }

        // Truncate lines starting at #
        std::size_t comment_pos = line.find('#');
        if (comment_pos != std::string::npos) {
            line = line.substr(0, comment_pos);
        }
        if (line.empty()) {
            return;
        }

        // Split the line by '='
        std::size_t equal_pos = line.find('=');
        if (equal_pos == std::string::npos) {
            LOG_WARNING(Input, "Invalid format at line: {}, data: \"{}\", skipping line.",
                        lineCount, line);
            return;
        }

        std::string output_string = line.substr(0, equal_pos);
        std::string input_string = line.substr(equal_pos + 1);
        s8 input_gamepad_id = -1, output_gamepad_id = -1; // -1 means it's not specified

        // todo: here the inputs and outputs are formatted and split, we need to extract the
        // controller ID now

        // input gamepad id is only for controllers, it's discarded otherwise
        std::size_t input_colon_pos = input_string.find(':');
        if (input_colon_pos != std::string::npos) {
            auto temp = parseInt(input_string.substr(input_colon_pos + 1));
            if (!temp) {
                LOG_WARNING(Input, "Invalid gamepad ID value at line {}: \"{}\"", lineCount, line);
            } else {
                input_gamepad_id = *temp;
            }
            input_string = input_string.substr(0, input_colon_pos);
        }

        // if not provided, assume it's for all gamepads, if the input is a controller and that also
        // doesn't have an ID, and for the first otherwise
        std::size_t output_colon_pos = output_string.find(':');
        if (output_colon_pos != std::string::npos) {
            auto temp = parseInt(output_string.substr(output_colon_pos + 1));
            if (!temp) {
                LOG_WARNING(Input, "Invalid gamepad ID value at line {}: \"{}\"", lineCount, line);
            } else {
                output_gamepad_id = *temp;
            }
            output_string = output_string.substr(0, output_colon_pos);
        }

        std::size_t comma_pos = input_string.find(',');

        // todo: make remapping work for special bindings for gamepads that are not the first
        if (output_string == "mouse_to_joystick") {
            if (input_string == "left") {
                SetMouseToJoystick(1);
            } else if (input_string == "right") {
                SetMouseToJoystick(2);
            } else {
                LOG_WARNING(Input, "Invalid argument for mouse-to-joystick binding");
                SetMouseToJoystick(0);
            }
            return;
        } else if (output_string == "key_toggle") {
            if (comma_pos != std::string::npos) {
                // handle key-to-key toggling (separate list?)
                InputBinding toggle_keys = GetBindingFromString(input_string);
                if (toggle_keys.KeyCount() != 2) {
                    LOG_WARNING(Input,
                                "Syntax error: Please provide exactly 2 keys: "
                                "first is the toggler, the second is the key to toggle: {}",
                                line);
                    return;
                }
                ControllerOutput* toggle_out =
                    &*std::ranges::find(output_arrays[0].data, ControllerOutput(KEY_TOGGLE));
                BindingConnection toggle_connection = BindingConnection(
                    InputBinding(toggle_keys.keys[0]), toggle_out, 0, toggle_keys.keys[1]);
                connections.insert(connections.end(), toggle_connection);
                return;
            }
            LOG_WARNING(Input, "Invalid format at line: {}, data: \"{}\", skipping line.",
                        lineCount, line);
            return;
        } else if (output_string == "mouse_movement_params") {
            std::stringstream ss(input_string);
            char comma; // To hold the comma separators between the floats
            ss >> mouse_deadzone_offset >> comma >> mouse_speed >> comma >> mouse_speed_offset;

            // Check for invalid input (in case there's an unexpected format)
            if (ss.fail()) {
                LOG_WARNING(Input, "Failed to parse mouse movement parameters from line: {}", line);
                return;
            }
            SetMouseParams(mouse_deadzone_offset, mouse_speed, mouse_speed_offset);
            return;
        } else if (output_string == "analog_deadzone") {
            std::stringstream ss(input_string);
            std::string device, inner_deadzone_str, outer_deadzone_str;

            if (!std::getline(ss, device, ',') || !std::getline(ss, inner_deadzone_str, ',') ||
                !std::getline(ss, outer_deadzone_str)) {
                LOG_WARNING(Input, "Malformed deadzone config at line {}: \"{}\"", lineCount, line);
                return;
            }

            auto inner_deadzone = parseInt(inner_deadzone_str);
            auto outer_deadzone = parseInt(outer_deadzone_str);

            if (!inner_deadzone || !outer_deadzone) {
                LOG_WARNING(Input, "Invalid deadzone values at line {}: \"{}\"", lineCount, line);
                return;
            }

            std::pair<int, int> deadzone = {*inner_deadzone, *outer_deadzone};

            static std::unordered_map<std::string, std::pair<int, int>&> deadzone_map = {
                {"leftjoystick", leftjoystick_deadzone},
                {"rightjoystick", rightjoystick_deadzone},
                {"l2", lefttrigger_deadzone},
                {"r2", righttrigger_deadzone},
            };

            if (auto it = deadzone_map.find(device); it != deadzone_map.end()) {
                it->second = deadzone;
                LOG_DEBUG(Input, "Parsed deadzone: {} {} {}", device, inner_deadzone_str,
                          outer_deadzone_str);
            } else {
                LOG_WARNING(Input, "Invalid axis name at line {}: \"{}\", skipping line.",
                            lineCount, line);
            }
            return;
        } else if (output_string == "override_controller_color") {
            std::stringstream ss(input_string);
            std::string enable, r_s, g_s, b_s;
            std::optional<int> r, g, b;
            if (!std::getline(ss, enable, ',') || !std::getline(ss, r_s, ',') ||
                !std::getline(ss, g_s, ',') || !std::getline(ss, b_s)) {
                LOG_WARNING(Input, "Malformed controller color config at line {}: \"{}\"",
                            lineCount, line);
                return;
            }
            r = parseInt(r_s);
            g = parseInt(g_s);
            b = parseInt(b_s);
            if (!r || !g || !b) {
                LOG_WARNING(Input, "Invalid RGB values at line {}: \"{}\", skipping line.",
                            lineCount, line);
                return;
            }
            Config::SetOverrideControllerColor(enable == "true");
            Config::SetControllerCustomColor(*r, *g, *b);
            LOG_DEBUG(Input, "Parsed color settings: {} {} {} {}",
                      enable == "true" ? "override" : "no override", *r, *b, *g);
            return;
        }

        // normal cases
        InputBinding binding = GetBindingFromString(input_string);
        BindingConnection connection(InputID(), nullptr);
        auto button_it = string_to_cbutton_map.find(output_string);
        auto axis_it = string_to_axis_map.find(output_string);
        if (binding.IsEmpty()) {
            LOG_WARNING(Input, "Invalid format at line: {}, data: \"{}\", skipping line.",
                        lineCount, line);
            return;
        }
        if (button_it != string_to_cbutton_map.end()) {
            // todo add new shit here
            connection = BindingConnection(
                binding,
                &*std::ranges::find(output_arrays[std::clamp(output_gamepad_id - 1, 0, 3)].data,
                                    ControllerOutput(button_it->second)));

        } else if (axis_it != string_to_axis_map.end()) {
            // todo add new shit here
            int value_to_set = binding.keys[2].type == InputType::Axis ? 0 : axis_it->second.value;
            connection = BindingConnection(
                binding,
                &*std::ranges::find(output_arrays[std::clamp(output_gamepad_id - 1, 0, 3)].data,
                                    ControllerOutput(SDL_GAMEPAD_BUTTON_INVALID,
                                                     axis_it->second.axis,
                                                     axis_it->second.value >= 0)),
                value_to_set);
        } else {
            LOG_WARNING(Input, "Invalid format at line: {}, data: \"{}\", skipping line.",
                        lineCount, line);
            return;
        }
        // todo make the following: if the input binding contains a controller input, and gamepad ID
        // isn't specified for either inputs or output (both are -1), then multiply the binding and
        // add it to all 4 controllers
        if (connection.HasGamepadInput() && input_gamepad_id == -1 && output_gamepad_id == -1) {
            for (int i = 0; i < 4; i++) {
                BindingConnection copy = connection.CopyWithChangedGamepadId(i + 1);
                copy.output = &*std::ranges::find(output_arrays[i].data, *connection.output);
                connections.push_back(copy);
            }
        } else {
            connections.push_back(connection);
        }
        LOG_DEBUG(Input, "Succesfully parsed line {}", lineCount);
    };
    while (std::getline(global_config_stream, line)) {
        ProcessLine();
    }
    lineCount = 0;
    while (std::getline(config_stream, line)) {
        ProcessLine();
    }
    config_stream.close();
    std::sort(connections.begin(), connections.end());
    for (auto& c : connections) {
        // todo add new shit here
        LOG_DEBUG(Input, "Binding: {} : {}", c.output->ToString(), c.binding.ToString());
    }
    LOG_DEBUG(Input, "Done parsing the input config!");
}

BindingConnection BindingConnection::CopyWithChangedGamepadId(u8 gamepad) {
    BindingConnection copy = *this;
    for (auto& key : copy.binding.keys) {
        if (key.type == InputType::Controller || key.type == InputType::Axis) {
            key.gamepad_id = gamepad;
        }
    }
    return copy;
}

u32 GetMouseWheelEvent(const SDL_Event& event) {
    if (event.type != SDL_EVENT_MOUSE_WHEEL && event.type != SDL_EVENT_MOUSE_WHEEL_OFF) {
        LOG_WARNING(Input, "Something went wrong with wheel input parsing!");
        return SDL_UNMAPPED;
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
    return SDL_UNMAPPED;
}

InputEvent InputBinding::GetInputEventFromSDLEvent(const SDL_Event& e) {
    // todo add new shit here
    u8 gamepad = 1;
    switch (e.type) {
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:
        return InputEvent(InputType::KeyboardMouse, e.key.key, e.key.down, 0);
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
        return InputEvent(InputType::KeyboardMouse, static_cast<u32>(e.button.button),
                          e.button.down, 0);
    case SDL_EVENT_MOUSE_WHEEL:
    case SDL_EVENT_MOUSE_WHEEL_OFF:
        return InputEvent(InputType::KeyboardMouse, GetMouseWheelEvent(e),
                          e.type == SDL_EVENT_MOUSE_WHEEL, 0);
    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
    case SDL_EVENT_GAMEPAD_BUTTON_UP:
        gamepad = GameControllers::GetGamepadIndexFromJoystickId(e.gbutton.which) + 1;
        return InputEvent({InputType::Controller, (u32)e.gbutton.button, gamepad}, e.gbutton.down,
                          0);
    case SDL_EVENT_GAMEPAD_AXIS_MOTION:
        gamepad = GameControllers::GetGamepadIndexFromJoystickId(e.gaxis.which) + 1;
        return InputEvent({InputType::Axis, (u32)e.gaxis.axis, gamepad}, true, e.gaxis.value / 256);
    default:
        return InputEvent();
    }
}

GameControllers ControllerOutput::controllers =
    *Common::Singleton<Input::GameControllers>::Instance();

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
    *new_param = 0; // bruh
}
void ControllerOutput::AddUpdate(InputEvent event) {
    switch (button) {
    case KEY_TOGGLE:
        if (event.active) {
            ToggleKeyInList(event.input);
        }
        return;
    default:
        break;
    }
    if (button != SDL_GAMEPAD_BUTTON_INVALID) {
        if (event.input.type == InputType::Axis) {
            bool temp = event.axis_value * (positive_axis ? 1 : -1) > 0x40;
            new_button_state |= event.active && event.axis_value * (positive_axis ? 1 : -1) > 0x40;
            if (temp) {
                LOG_DEBUG(Input, "Toggled a button from an axis");
            }
        } else {
            new_button_state |= event.active;
        }

    } else if (axis != SDL_GAMEPAD_AXIS_INVALID) {
        *new_param = (event.active ? event.axis_value : 0) + *new_param;
    }
}
void ControllerOutput::FinalizeUpdate(u8 gamepad_index) {
    auto PushSDLEvent = [&](u32 event_type) {
        if (new_button_state) {
            SDL_Event e;
            SDL_memset(&e, 0, sizeof(e));
            e.type = event_type;
            SDL_PushEvent(&e);
        }
    };
    state_changed = old_button_state != new_button_state || old_param != *new_param;
    if (!state_changed) {
        return;
    }
    old_button_state = new_button_state;
    old_param = *new_param;
    if (button != SDL_GAMEPAD_BUTTON_INVALID) {
        auto controller = controllers[gamepad_index];
        switch (button) {
        case SDL_GAMEPAD_BUTTON_TOUCHPAD_LEFT:
            controller->SetTouchpadState(0, new_button_state, 0.25f, 0.5f);
            controller->CheckButton(0, SDLGamepadToOrbisButton(button), new_button_state);
            break;
        case SDL_GAMEPAD_BUTTON_TOUCHPAD_CENTER:
            controller->SetTouchpadState(0, new_button_state, 0.50f, 0.5f);
            controller->CheckButton(0, SDLGamepadToOrbisButton(button), new_button_state);
            break;
        case SDL_GAMEPAD_BUTTON_TOUCHPAD_RIGHT:
            controller->SetTouchpadState(0, new_button_state, 0.75f, 0.5f);
            controller->CheckButton(0, SDLGamepadToOrbisButton(button), new_button_state);
            break;
        case LEFTJOYSTICK_HALFMODE:
            leftjoystick_halfmode = new_button_state;
            break;
        case RIGHTJOYSTICK_HALFMODE:
            rightjoystick_halfmode = new_button_state;
            break;
        case HOTKEY_FULLSCREEN:
            PushSDLEvent(SDL_EVENT_TOGGLE_FULLSCREEN);
            break;
        case HOTKEY_PAUSE:
            PushSDLEvent(SDL_EVENT_TOGGLE_PAUSE);
            break;
        case HOTKEY_SIMPLE_FPS:
            PushSDLEvent(SDL_EVENT_TOGGLE_SIMPLE_FPS);
            break;
        case HOTKEY_RELOAD_INPUTS:
            PushSDLEvent(SDL_EVENT_RELOAD_INPUTS);
            break;
        case HOTKEY_TOGGLE_MOUSE_TO_JOYSTICK:
            PushSDLEvent(SDL_EVENT_MOUSE_TO_JOYSTICK);
            break;
        case HOTKEY_TOGGLE_MOUSE_TO_GYRO:
            PushSDLEvent(SDL_EVENT_MOUSE_TO_GYRO);
            break;
        case HOTKEY_TOGGLE_MOUSE_TO_TOUCHPAD:
            PushSDLEvent(SDL_EVENT_MOUSE_TO_TOUCHPAD);
            break;
        case HOTKEY_RENDERDOC:
            PushSDLEvent(SDL_EVENT_RDOC_CAPTURE);
            break;
        case HOTKEY_ADD_VIRTUAL_USER:
            PushSDLEvent(SDL_EVENT_ADD_VIRTUAL_USER);
            break;
        case HOTKEY_REMOVE_VIRTUAL_USER:
            PushSDLEvent(SDL_EVENT_REMOVE_VIRTUAL_USER);
            break;
        case HOTKEY_QUIT:
            PushSDLEvent(SDL_EVENT_QUIT_DIALOG);
            break;
        case KEY_TOGGLE:
            // noop
            break;
        case MOUSE_GYRO_ROLL_MODE:
            SetMouseGyroRollMode(new_button_state);
            break;
        default: // is a normal key (hopefully)
            controller->CheckButton(0, SDLGamepadToOrbisButton(button), new_button_state);
            break;
        }
    } else if (axis != SDL_GAMEPAD_AXIS_INVALID && positive_axis) {
        // avoid double-updating axes, but don't skip directional button bindings
        auto ApplyDeadzone = [](s16* value, std::pair<int, int> deadzone) {
            if (std::abs(*value) <= deadzone.first || deadzone.first == deadzone.second) {
                *value = 0;
            } else {
                *value = (*value >= 0 ? 1 : -1) *
                         std::clamp(static_cast<s32>((128.0 * (std::abs(*value) - deadzone.first)) /
                                                     (float)(deadzone.second - deadzone.first)),
                                    0, 128);
            }
        };
        float multiplier = 1.0;
        Axis c_axis = GetAxisFromSDLAxis(axis);
        switch (c_axis) {
        case Axis::LeftX:
        case Axis::LeftY:
            ApplyDeadzone(new_param, leftjoystick_deadzone);
            multiplier = leftjoystick_halfmode ? 0.5 : 1.0;
            break;
        case Axis::RightX:
        case Axis::RightY:
            ApplyDeadzone(new_param, rightjoystick_deadzone);
            multiplier = rightjoystick_halfmode ? 0.5 : 1.0;
            break;
        case Axis::TriggerLeft:
            ApplyDeadzone(new_param, lefttrigger_deadzone);
            controllers[gamepad_index]->Axis(0, c_axis, GetAxis(0x0, 0x7f, *new_param));
            controllers[gamepad_index]->CheckButton(0, OrbisPadButtonDataOffset::L2,
                                                    *new_param > 0x20);
            return;
        case Axis::TriggerRight:
            ApplyDeadzone(new_param, righttrigger_deadzone);
            controllers[gamepad_index]->Axis(0, c_axis, GetAxis(0x0, 0x7f, *new_param));
            controllers[gamepad_index]->CheckButton(0, OrbisPadButtonDataOffset::R2,
                                                    *new_param > 0x20);
            return;
        default:
            break;
        }
        controllers[gamepad_index]->Axis(0, c_axis, GetAxis(-0x80, 0x7f, *new_param * multiplier));
    }
}

// Updates the list of pressed keys with the given input.
// Returns whether the list was updated or not.
bool UpdatePressedKeys(InputEvent event) {
    // Skip invalid inputs
    InputID input = event.input;
    if (input.sdl_id == SDL_UNMAPPED) {
        return false;
    }
    if (input.type == InputType::Axis) {
        // analog input, it gets added when it first sends an event,
        // and from there, it only changes the parameter
        auto it = std::lower_bound(pressed_keys.begin(), pressed_keys.end(), input,
                                   [](const std::pair<InputEvent, bool>& e, InputID i) {
                                       return std::tie(e.first.input.type, e.first.input.sdl_id) <
                                              std::tie(i.type, i.sdl_id);
                                   });
        if (it == pressed_keys.end() || it->first.input != input) {
            pressed_keys.insert(it, {event, false});
            LOG_DEBUG(Input, "Added axis {} to the input list", event.input.sdl_id);
        } else {
            // noise filter
            if (std::abs(it->first.axis_value - event.axis_value) <= 1) {
                return false;
            }
            it->first.axis_value = event.axis_value;
        }
        return true;
    } else if (event.active) {
        // Find the correct position for insertion to maintain order
        auto it = std::lower_bound(pressed_keys.begin(), pressed_keys.end(), input,
                                   [](const std::pair<InputEvent, bool>& e, InputID i) {
                                       return std::tie(e.first.input.type, e.first.input.sdl_id) <
                                              std::tie(i.type, i.sdl_id);
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
    LOG_DEBUG(Input, "No change was made!");
    return false;
}
// Check if the binding's all keys are currently active.
// It also extracts the analog inputs' parameters, and updates the input hierarchy flags.
InputEvent BindingConnection::ProcessBinding() {
    // the last key is always set (if the connection isn't empty),
    // and the analog inputs are always the last one due to how they are sorted,
    // so this signifies whether or not the input is analog
    InputEvent event = InputEvent(binding.keys[0]);
    if (pressed_keys.empty()) {
        return event;
    }
    if (event.input.type != InputType::Axis) {
        // for button inputs
        event.axis_value = axis_param;
    }
    // it's a bit scuffed, but if the output is a toggle, then we put the key here
    if (output->button == KEY_TOGGLE) {
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
            if (pressed_it->first.input == key && (pressed_it->second == false)) {
                key_found = true;
                if (output->positive_axis) {
                    flags_to_set.push_back(&pressed_it->second);
                }
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
    if (binding.keys[0].type != InputType::Axis) { // the axes spam inputs, making this unreadable
        LOG_DEBUG(Input, "Input found: {}", binding.ToString());
    }
    event.active = true;
    return event; // All keys are active
}

void ActivateOutputsFromInputs() {

    // todo find a better solution
    for (int i = 0; i < 4; i++) {

        // Reset values and flags
        for (auto& it : pressed_keys) {
            it.second = false;
        }
        for (auto& it : output_arrays[i].data) {
            it.ResetUpdate();
        }

        // Check for input blockers
        ApplyMouseInputBlockers();

        // Iterate over all inputs, and update their respecive outputs accordingly
        for (auto& it : connections) {
            // only update this when it's the correct pass
            if (it.output->gamepad_id == i) {
                it.output->AddUpdate(it.ProcessBinding());
            }
        }

        // Update all outputs
        for (auto& it : output_arrays[i].data) {
            it.FinalizeUpdate(i);
        }
    }
}

} // namespace Input
