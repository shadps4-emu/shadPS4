// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <mutex>
#include <optional>
#include <utility>

#include <SDL3/SDL_gamepad.h>
#include "SDL3/SDL_joystick.h"
#include "common/assert.h"
#include "common/ring_buffer_queue.h"
#include "core/libraries/pad/pad.h"
#include "core/libraries/system/userservice.h"

namespace Input {

enum class Axis {
    LeftX = 0,
    LeftY = 1,
    RightX = 2,
    RightY = 3,
    TriggerLeft = 4,
    TriggerRight = 5,

    AxisMax
};

struct TouchpadEntry {
    u8 ID = 0;
    bool state{};
    u16 x{};
    u16 y{};
};

struct Colour {
    u8 r, g, b;
};
static constexpr Input::Colour g_user_colours[4]{
    {0, 0, 255},   // blue
    {255, 0, 0},   // red
    {0, 255, 0},   // green
    {255, 0, 255}, // pink
};

struct State {
private:
    template <typename T>
    using AxisArray = std::array<T, std::to_underlying(Axis::AxisMax)>;
    static constexpr AxisArray<s32> axis_defaults{128, 128, 128, 128, 0, 0};
    static constexpr u64 axis_smoothing_time{33000};
    AxisArray<bool> axis_smoothing_flags{true};
    AxisArray<u64> axis_smoothing_start_times{0};
    AxisArray<int> axis_smoothing_start_values{axis_defaults};
    AxisArray<int> axis_smoothing_end_values{axis_defaults};

public:
    void OnButton(Libraries::Pad::OrbisPadButtonDataOffset, bool);
    void OnAxis(Axis, int, u64 timestamp, bool smooth = true);
    void OnTouchpad(int touchIndex, bool isDown, float x, float y);
    void OnGyro(const float[3]);
    void OnAccel(const float[3]);
    void UpdateAxisSmoothing(u64 timestamp);

    Libraries::Pad::OrbisPadButtonDataOffset buttonsState{};
    u64 time = 0;
    AxisArray<s32> axes{axis_defaults};
    TouchpadEntry touchpad[2]{};
    Libraries::Pad::OrbisFVector3 acceleration = {0.0f, -9.81f, 0.0f};
    Libraries::Pad::OrbisFVector3 angularVelocity = {0.0f, 0.0f, 0.0f};
    Libraries::Pad::OrbisFQuaternion orientation = {0.0f, 0.0f, 0.0f, 1.0f};
    u64 touch_time_since_held_down{};
    bool connected{};
    u8 connected_count{};
};

inline int GetAxis(int min, int max, int value) {
    int v = (255 * (value - min)) / (max - min);
    return (v < 0 ? 0 : (v > 255 ? 255 : v));
}

class GameController {
    friend class GameControllers;

public:
    GameController();
    virtual ~GameController() = default;
    void ConnectController(SDL_Gamepad* pad);
    void DisconnectController();

    State ReadState();
    int ReadStates(State* states, int states_num);

    void Button(Libraries::Pad::OrbisPadButtonDataOffset button, bool isPressed);
    void Axis(Input::Axis axis, int value, bool smooth = true);
    void UpdateGyro(const float gyro[3]);
    void UpdateAcceleration(const float acceleration[3]);
    void PollState();
    void ResetOrientation();
    void SetLightBarRGB(u8 const r, u8 const g, u8 const b);
    void SetLightBarRGB(Colour const c);
    Colour GetLightBarRGB();
    void PollLightColour();
    bool SetVibration(u8 smallMotor, u8 largeMotor);
    void SetTouchpadState(int touchIndex, bool touchDown, float x, float y);

    float gyro_buf[3] = {0.0f, 0.0f, 0.0f}, accel_buf[3] = {0.0f, 9.81f, 0.0f};
    s32 user_id = Libraries::UserService::ORBIS_USER_SERVICE_USER_ID_INVALID;
    SDL_Gamepad* m_sdl_gamepad = nullptr;

private:
    // m_state_mutex must be held by the caller.
    void PushStateLocked(u64 timestamp = 0);
    void UpdateOrientationLocked(u64 timestamp);

    u8 m_next_touch_id{1};
    u64 m_touch_down_timestamp{};
    u64 m_last_orientation_update{};
    Colour colour;
    std::optional<Colour> override_colour{};

    State m_state;

    std::mutex m_state_mutex;
    RingBufferQueue<State> m_states_queue;
};

class GameControllers {
    std::array<GameController*, 5> controllers;

public:
    GameControllers()
        : controllers({new GameController(), new GameController(), new GameController(),
                       new GameController(), new GameController()}) {};
    virtual ~GameControllers() = default;
    GameController* operator[](const size_t& i) const {
        if (i > 4) {
            UNREACHABLE_MSG("Index {} is out of bounds for GameControllers!", i);
        }
        return controllers[i];
    }
    void TryOpenSDLControllers();
    u8 GetGamepadIndexFromJoystickId(SDL_JoystickID id);
    static std::optional<u8> GetControllerIndexFromUserID(s32 user_id);
    static std::optional<u8> GetControllerIndexFromControllerID(s32 controller_id);

    void SetControllerCustomColor(s32 i, u8 r, u8 g, u8 b) {
        // reset to ensure the next function always runs, even if there already was a preexisting
        // override colour before
        controllers[i]->override_colour = std::nullopt;
        controllers[i]->SetLightBarRGB(r, g, b);
        controllers[i]->override_colour = {r, g, b};
    }
    void ResetLightbarColors();
};

} // namespace Input
