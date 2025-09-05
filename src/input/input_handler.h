// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <map>
#include <string>
#include <unordered_set>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_timer.h"

#include "common/logging/log.h"
#include "common/types.h"
#include "core/libraries/pad/pad.h"
#include "fmt/format.h"
#include "input/controller.h"

// +1 and +2 is taken
#define SDL_MOUSE_WHEEL_UP SDL_EVENT_MOUSE_WHEEL + 3
#define SDL_MOUSE_WHEEL_DOWN SDL_EVENT_MOUSE_WHEEL + 4
#define SDL_MOUSE_WHEEL_LEFT SDL_EVENT_MOUSE_WHEEL + 5
#define SDL_MOUSE_WHEEL_RIGHT SDL_EVENT_MOUSE_WHEEL + 7

#define SDL_GAMEPAD_BUTTON_TOUCHPAD_LEFT SDL_GAMEPAD_BUTTON_COUNT + 1
#define SDL_GAMEPAD_BUTTON_TOUCHPAD_CENTER SDL_GAMEPAD_BUTTON_COUNT + 2
#define SDL_GAMEPAD_BUTTON_TOUCHPAD_RIGHT SDL_GAMEPAD_BUTTON_COUNT + 3

#define SDL_EVENT_TOGGLE_FULLSCREEN SDL_EVENT_USER + 1
#define SDL_EVENT_TOGGLE_PAUSE SDL_EVENT_USER + 2
#define SDL_EVENT_CHANGE_CONTROLLER SDL_EVENT_USER + 3
#define SDL_EVENT_TOGGLE_SIMPLE_FPS SDL_EVENT_USER + 4
#define SDL_EVENT_RELOAD_INPUTS SDL_EVENT_USER + 5
#define SDL_EVENT_MOUSE_TO_JOYSTICK SDL_EVENT_USER + 6
#define SDL_EVENT_MOUSE_TO_GYRO SDL_EVENT_USER + 7
#define SDL_EVENT_RDOC_CAPTURE SDL_EVENT_USER + 8
#define SDL_EVENT_QUIT_DIALOG SDL_EVENT_USER + 9
#define SDL_EVENT_MOUSE_WHEEL_OFF SDL_EVENT_USER + 10

#define LEFTJOYSTICK_HALFMODE 0x00010000
#define RIGHTJOYSTICK_HALFMODE 0x00020000
#define BACK_BUTTON 0x00040000

#define KEY_TOGGLE 0x00200000
#define MOUSE_GYRO_ROLL_MODE 0x00400000

#define HOTKEY_FULLSCREEN 0xf0000001
#define HOTKEY_PAUSE 0xf0000002
#define HOTKEY_SIMPLE_FPS 0xf0000003
#define HOTKEY_QUIT 0xf0000004
#define HOTKEY_RELOAD_INPUTS 0xf0000005
#define HOTKEY_TOGGLE_MOUSE_TO_JOYSTICK 0xf0000006
#define HOTKEY_TOGGLE_MOUSE_TO_GYRO 0xf0000007
#define HOTKEY_RENDERDOC 0xf0000008

#define SDL_UNMAPPED UINT32_MAX - 1

namespace Input {
using Input::Axis;
using Libraries::Pad::OrbisPadButtonDataOffset;

struct AxisMapping {
    u32 axis;
    s16 value;
    AxisMapping(SDL_GamepadAxis a, s16 v) : axis(a), value(v) {}
};

enum class InputType { Axis, KeyboardMouse, Controller, Count };
const std::array<std::string, 4> input_type_names = {"Axis", "KBM", "Controller", "Unknown"};

class InputID {
public:
    InputType type;
    u32 sdl_id;
    InputID(InputType d = InputType::Count, u32 i = SDL_UNMAPPED) : type(d), sdl_id(i) {}
    bool operator==(const InputID& o) const {
        return type == o.type && sdl_id == o.sdl_id;
    }
    bool operator!=(const InputID& o) const {
        return type != o.type || sdl_id != o.sdl_id;
    }
    bool operator<=(const InputID& o) const {
        return type <= o.type && sdl_id <= o.sdl_id;
    }
    bool IsValid() const {
        return *this != InputID();
    }
    std::string ToString() {
        return fmt::format("({}: {:x})", input_type_names[static_cast<u8>(type)], sdl_id);
    }
};

class InputEvent {
public:
    InputID input;
    bool active;
    s8 axis_value;

    InputEvent(InputID i = InputID(), bool a = false, s8 v = 0)
        : input(i), active(a), axis_value(v) {}
    InputEvent(InputType d, u32 i, bool a = false, s8 v = 0)
        : input(d, i), active(a), axis_value(v) {}
};

// i strongly suggest you collapse these maps
const std::map<std::string, u32> string_to_cbutton_map = {
    {"triangle", SDL_GAMEPAD_BUTTON_NORTH},
    {"circle", SDL_GAMEPAD_BUTTON_EAST},
    {"cross", SDL_GAMEPAD_BUTTON_SOUTH},
    {"square", SDL_GAMEPAD_BUTTON_WEST},
    {"l1", SDL_GAMEPAD_BUTTON_LEFT_SHOULDER},
    {"r1", SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER},
    {"l3", SDL_GAMEPAD_BUTTON_LEFT_STICK},
    {"r3", SDL_GAMEPAD_BUTTON_RIGHT_STICK},
    {"pad_up", SDL_GAMEPAD_BUTTON_DPAD_UP},
    {"pad_down", SDL_GAMEPAD_BUTTON_DPAD_DOWN},
    {"pad_left", SDL_GAMEPAD_BUTTON_DPAD_LEFT},
    {"pad_right", SDL_GAMEPAD_BUTTON_DPAD_RIGHT},
    {"options", SDL_GAMEPAD_BUTTON_START},

    // these are outputs only (touchpad can only be bound to itself)
    {"touchpad_left", SDL_GAMEPAD_BUTTON_TOUCHPAD_LEFT},
    {"touchpad_center", SDL_GAMEPAD_BUTTON_TOUCHPAD_CENTER},
    {"touchpad_right", SDL_GAMEPAD_BUTTON_TOUCHPAD_RIGHT},
    {"leftjoystick_halfmode", LEFTJOYSTICK_HALFMODE},
    {"rightjoystick_halfmode", RIGHTJOYSTICK_HALFMODE},

    // this is only for input
    {"back", SDL_GAMEPAD_BUTTON_BACK},
    {"share", SDL_GAMEPAD_BUTTON_BACK},
    {"lpaddle_high", SDL_GAMEPAD_BUTTON_LEFT_PADDLE1},
    {"lpaddle_low", SDL_GAMEPAD_BUTTON_LEFT_PADDLE2},
    {"rpaddle_high", SDL_GAMEPAD_BUTTON_RIGHT_PADDLE1},
    {"rpaddle_low", SDL_GAMEPAD_BUTTON_RIGHT_PADDLE2},
    {"mouse_gyro_roll_mode", MOUSE_GYRO_ROLL_MODE},
    {"hotkey_pause", HOTKEY_PAUSE},
    {"hotkey_fullscreen", HOTKEY_FULLSCREEN},
    {"hotkey_show_fps", HOTKEY_SIMPLE_FPS},
    {"hotkey_quit", HOTKEY_QUIT},
    {"hotkey_reload_inputs", HOTKEY_RELOAD_INPUTS},
    {"hotkey_toggle_mouse_to_joystick", HOTKEY_TOGGLE_MOUSE_TO_JOYSTICK},
    {"hotkey_toggle_mouse_to_gyro", HOTKEY_TOGGLE_MOUSE_TO_GYRO},
    {"hotkey_renderdoc_capture", HOTKEY_RENDERDOC},
};

const std::map<std::string, AxisMapping> string_to_axis_map = {
    {"axis_left_x_plus", {SDL_GAMEPAD_AXIS_LEFTX, 127}},
    {"axis_left_x_minus", {SDL_GAMEPAD_AXIS_LEFTX, -127}},
    {"axis_left_y_plus", {SDL_GAMEPAD_AXIS_LEFTY, 127}},
    {"axis_left_y_minus", {SDL_GAMEPAD_AXIS_LEFTY, -127}},
    {"axis_right_x_plus", {SDL_GAMEPAD_AXIS_RIGHTX, 127}},
    {"axis_right_x_minus", {SDL_GAMEPAD_AXIS_RIGHTX, -127}},
    {"axis_right_y_plus", {SDL_GAMEPAD_AXIS_RIGHTY, 127}},
    {"axis_right_y_minus", {SDL_GAMEPAD_AXIS_RIGHTY, -127}},

    {"l2", {SDL_GAMEPAD_AXIS_LEFT_TRIGGER, 127}},
    {"r2", {SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, 127}},

    // should only use these to bind analog inputs to analog outputs
    {"axis_left_x", {SDL_GAMEPAD_AXIS_LEFTX, 127}},
    {"axis_left_y", {SDL_GAMEPAD_AXIS_LEFTY, 127}},
    {"axis_right_x", {SDL_GAMEPAD_AXIS_RIGHTX, 127}},
    {"axis_right_y", {SDL_GAMEPAD_AXIS_RIGHTY, 127}},
};
const std::map<std::string, u32> string_to_keyboard_key_map = {
    // alphanumeric
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

    // F keys
    {"f1", SDLK_F1},
    {"f2", SDLK_F2},
    {"f3", SDLK_F3},
    {"f4", SDLK_F4},
    {"f5", SDLK_F5},
    {"f6", SDLK_F6},
    {"f7", SDLK_F7},
    {"f8", SDLK_F8},
    {"f9", SDLK_F9},
    {"f10", SDLK_F10},
    {"f11", SDLK_F11},
    {"f12", SDLK_F12},

    // symbols
    {"grave", SDLK_GRAVE},
    {"tilde", SDLK_TILDE},
    {"exclamation", SDLK_EXCLAIM},
    {"at", SDLK_AT},
    {"hash", SDLK_HASH},
    {"dollar", SDLK_DOLLAR},
    {"percent", SDLK_PERCENT},
    {"caret", SDLK_CARET},
    {"ampersand", SDLK_AMPERSAND},
    {"asterisk", SDLK_ASTERISK},
    {"lparen", SDLK_LEFTPAREN},
    {"rparen", SDLK_RIGHTPAREN},
    {"minus", SDLK_MINUS},
    {"underscore", SDLK_UNDERSCORE},
    {"equals", SDLK_EQUALS},
    {"plus", SDLK_PLUS},
    {"lbracket", SDLK_LEFTBRACKET},
    {"rbracket", SDLK_RIGHTBRACKET},
    {"lbrace", SDLK_LEFTBRACE},
    {"rbrace", SDLK_RIGHTBRACE},
    {"backslash", SDLK_BACKSLASH},
    {"pipe", SDLK_PIPE},
    {"semicolon", SDLK_SEMICOLON},
    {"colon", SDLK_COLON},
    {"apostrophe", SDLK_APOSTROPHE},
    {"quote", SDLK_DBLAPOSTROPHE},
    {"comma", SDLK_COMMA},
    {"less", SDLK_LESS},
    {"period", SDLK_PERIOD},
    {"greater", SDLK_GREATER},
    {"slash", SDLK_SLASH},
    {"question", SDLK_QUESTION},

    // special keys
    {"escape", SDLK_ESCAPE},
    {"printscreen", SDLK_PRINTSCREEN},
    {"scrolllock", SDLK_SCROLLLOCK},
    {"pausebreak", SDLK_PAUSE},
    {"backspace", SDLK_BACKSPACE},
    {"delete", SDLK_DELETE},
    {"insert", SDLK_INSERT},
    {"home", SDLK_HOME},
    {"end", SDLK_END},
    {"pgup", SDLK_PAGEUP},
    {"pgdown", SDLK_PAGEDOWN},
    {"tab", SDLK_TAB},
    {"capslock", SDLK_CAPSLOCK},
    {"enter", SDLK_RETURN},
    {"lshift", SDLK_LSHIFT},
    {"rshift", SDLK_RSHIFT},
    {"lctrl", SDLK_LCTRL},
    {"rctrl", SDLK_RCTRL},
    {"lalt", SDLK_LALT},
    {"ralt", SDLK_RALT},
    {"lmeta", SDLK_LGUI},
    {"rmeta", SDLK_RGUI},
    {"lwin", SDLK_LGUI},
    {"rwin", SDLK_RGUI},
    {"space", SDLK_SPACE},
    {"up", SDLK_UP},
    {"down", SDLK_DOWN},
    {"left", SDLK_LEFT},
    {"right", SDLK_RIGHT},

    // keypad
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
    {"kpperiod", SDLK_KP_PERIOD},
    {"kpcomma", SDLK_KP_COMMA},
    {"kpslash", SDLK_KP_DIVIDE},
    {"kpasterisk", SDLK_KP_MULTIPLY},
    {"kpminus", SDLK_KP_MINUS},
    {"kpplus", SDLK_KP_PLUS},
    {"kpequals", SDLK_KP_EQUALS},
    {"kpenter", SDLK_KP_ENTER},

    // mouse
    {"leftbutton", SDL_BUTTON_LEFT},
    {"rightbutton", SDL_BUTTON_RIGHT},
    {"middlebutton", SDL_BUTTON_MIDDLE},
    {"sidebuttonback", SDL_BUTTON_X1},
    {"sidebuttonforward", SDL_BUTTON_X2},
    {"mousewheelup", SDL_MOUSE_WHEEL_UP},
    {"mousewheeldown", SDL_MOUSE_WHEEL_DOWN},
    {"mousewheelleft", SDL_MOUSE_WHEEL_LEFT},
    {"mousewheelright", SDL_MOUSE_WHEEL_RIGHT},

    // no binding
    {"unmapped", SDL_UNMAPPED},
};

void ParseInputConfig(const std::string game_id);

class InputBinding {
public:
    InputID keys[3];
    InputBinding(InputID k1 = InputID(), InputID k2 = InputID(), InputID k3 = InputID()) {
        // we format the keys so comparing them will be very fast, because we will only have to
        // compare 3 sorted elements, where the only possible duplicate item is 0

        // duplicate entries get changed to one original, one null
        if (k1 == k2 && k1 != InputID()) {
            k2 = InputID();
        }
        if (k1 == k3 && k1 != InputID()) {
            k3 = InputID();
        }
        if (k3 == k2 && k2 != InputID()) {
            k2 = InputID();
        }
        // this sorts them
        if (k1 <= k2 && k1 <= k3) {
            keys[0] = k1;
            if (k2 <= k3) {
                keys[1] = k2;
                keys[2] = k3;
            } else {
                keys[1] = k3;
                keys[2] = k2;
            }
        } else if (k2 <= k1 && k2 <= k3) {
            keys[0] = k2;
            if (k1 <= k3) {
                keys[1] = k1;
                keys[2] = k3;
            } else {
                keys[1] = k3;
                keys[2] = k1;
            }
        } else {
            keys[0] = k3;
            if (k1 <= k2) {
                keys[1] = k1;
                keys[2] = k2;
            } else {
                keys[1] = k2;
                keys[3] = k1;
            }
        }
    }
    // copy ctor
    InputBinding(const InputBinding& o) {
        keys[0] = o.keys[0];
        keys[1] = o.keys[1];
        keys[2] = o.keys[2];
    }

    inline bool operator==(const InputBinding& o) {
        // InputID() signifies an unused slot
        return (keys[0] == o.keys[0] || keys[0] == InputID() || o.keys[0] == InputID()) &&
               (keys[1] == o.keys[1] || keys[1] == InputID() || o.keys[1] == InputID()) &&
               (keys[2] == o.keys[2] || keys[2] == InputID() || o.keys[2] == InputID());
        // it is already very fast,
        // but reverse order makes it check the actual keys first instead of possible 0-s,
        // potenially skipping the later expressions of the three-way AND
    }
    inline int KeyCount() const {
        return (keys[0].IsValid() ? 1 : 0) + (keys[1].IsValid() ? 1 : 0) +
               (keys[2].IsValid() ? 1 : 0);
    }
    // Sorts by the amount of non zero keys - left side is 'bigger' here
    bool operator<(const InputBinding& other) const {
        return KeyCount() > other.KeyCount();
    }
    inline bool IsEmpty() {
        return !(keys[0].IsValid() || keys[1].IsValid() || keys[2].IsValid());
    }
    std::string ToString() { // todo add device type
        switch (KeyCount()) {
        case 1:
            return fmt::format("({})", keys[0].ToString());
        case 2:
            return fmt::format("({}, {})", keys[0].ToString(), keys[1].ToString());
        case 3:
            return fmt::format("({}, {}, {})", keys[0].ToString(), keys[1].ToString(),
                               keys[2].ToString());
        default:
            return "Empty";
        }
    }

    // returns an InputEvent based on the event type (keyboard, mouse buttons/wheel, or controller)
    static InputEvent GetInputEventFromSDLEvent(const SDL_Event& e);
};

class ControllerOutput {
    static GameController* controller;

public:
    static void SetControllerOutputController(GameController* c);
    static void LinkJoystickAxes();

    u32 button;
    u32 axis;
    // these are only used as s8,
    // but I added some padding to avoid overflow if it's activated by multiple inputs
    // axis_plus and axis_minus pairs share a common new_param, the other outputs have their own
    s16 old_param;
    s16* new_param;
    bool old_button_state, new_button_state, state_changed, positive_axis;

    ControllerOutput(const u32 b, u32 a = SDL_GAMEPAD_AXIS_INVALID, bool p = true) {
        button = b;
        axis = a;
        new_param = new s16(0);
        old_param = 0;
        positive_axis = p;
    }
    ControllerOutput(const ControllerOutput& o) : button(o.button), axis(o.axis) {
        new_param = new s16(*o.new_param);
    }
    ~ControllerOutput() {
        delete new_param;
    }
    inline bool operator==(const ControllerOutput& o) const { // fucking consts everywhere
        return button == o.button && axis == o.axis;
    }
    inline bool operator!=(const ControllerOutput& o) const {
        return button != o.button || axis != o.axis;
    }
    std::string ToString() const {
        return fmt::format("({}, {}, {})", (s32)button, (int)axis, old_param);
    }
    inline bool IsButton() const {
        return axis == SDL_GAMEPAD_AXIS_INVALID && button != SDL_GAMEPAD_BUTTON_INVALID;
    }
    inline bool IsAxis() const {
        return axis != SDL_GAMEPAD_AXIS_INVALID && button == SDL_GAMEPAD_BUTTON_INVALID;
    }

    void ResetUpdate();
    void AddUpdate(InputEvent event);
    void FinalizeUpdate();
};
class BindingConnection {
public:
    InputBinding binding;
    ControllerOutput* output;
    u32 axis_param;
    InputID toggle;

    BindingConnection(InputBinding b, ControllerOutput* out, u32 param = 0, InputID t = InputID()) {
        binding = b;
        axis_param = param;
        output = out;
        toggle = t;
    }
    bool operator<(const BindingConnection& other) const {
        // a button is a higher priority than an axis, as buttons can influence axes
        // (e.g. joystick_halfmode)
        if (output->IsButton() &&
            (other.output->IsAxis() && (other.output->axis != SDL_GAMEPAD_AXIS_LEFT_TRIGGER &&
                                        other.output->axis != SDL_GAMEPAD_AXIS_RIGHT_TRIGGER))) {
            return true;
        }
        if (binding < other.binding) {
            return true;
        }
        return false;
    }
    InputEvent ProcessBinding();
};

// Updates the list of pressed keys with the given input.
// Returns whether the list was updated or not.
bool UpdatePressedKeys(InputEvent event);

void ActivateOutputsFromInputs();

} // namespace Input
