// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <mutex>
#include <SDL3/SDL_gamepad.h>
#include "SDL3/SDL_joystick.h"
#include "common/assert.h"
#include "common/types.h"
#include "core/libraries/pad/pad.h"

struct SDL_Gamepad;

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

struct State {
    Libraries::Pad::OrbisPadButtonDataOffset buttonsState{};
    u64 time = 0;
    int axes[static_cast<int>(Axis::AxisMax)] = {128, 128, 128, 128, 0, 0};
    TouchpadEntry touchpad[2] = {{false, 0, 0}, {false, 0, 0}};
    Libraries::Pad::OrbisFVector3 acceleration = {0.0f, 0.0f, 0.0f};
    Libraries::Pad::OrbisFVector3 angularVelocity = {0.0f, 0.0f, 0.0f};
    Libraries::Pad::OrbisFQuaternion orientation = {0.0f, 0.0f, 0.0f, 1.0f};
};

inline int GetAxis(int min, int max, int value) {
    int v = (255 * (value - min)) / (max - min);
    return (v < 0 ? 0 : (v > 255 ? 255 : v));
}

constexpr u32 MAX_STATES = 32;

class GameController {
    friend class GameControllers;

public:
    GameController();
    virtual ~GameController() = default;

    void ReadState(State* state, bool* isConnected, int* connectedCount);
    int ReadStates(State* states, int states_num, bool* isConnected, int* connectedCount);
    State GetLastState() const;
    void CheckButton(int id, Libraries::Pad::OrbisPadButtonDataOffset button, bool isPressed);
    void AddState(const State& state);
    void Axis(int id, Input::Axis axis, int value);
    void Gyro(int id, const float gyro[3]);
    void Acceleration(int id, const float acceleration[3]);
    void SetLightBarRGB(u8 r, u8 g, u8 b);
    bool SetVibration(u8 smallMotor, u8 largeMotor);
    void SetTouchpadState(int touchIndex, bool touchDown, float x, float y);
    u32 Poll();

    u8 GetTouchCount();
    void SetTouchCount(u8 touchCount);
    u8 GetSecondaryTouchCount();
    void SetSecondaryTouchCount(u8 touchCount);
    u8 GetPreviousTouchNum();
    void SetPreviousTouchNum(u8 touchNum);
    bool WasSecondaryTouchReset();
    void UnsetSecondaryTouchResetBool();

    void SetLastOrientation(Libraries::Pad::OrbisFQuaternion& orientation);
    Libraries::Pad::OrbisFQuaternion GetLastOrientation();
    std::chrono::steady_clock::time_point GetLastUpdate();
    void SetLastUpdate(std::chrono::steady_clock::time_point lastUpdate);
    static void CalculateOrientation(Libraries::Pad::OrbisFVector3& acceleration,
                                     Libraries::Pad::OrbisFVector3& angularVelocity,
                                     float deltaTime,
                                     Libraries::Pad::OrbisFQuaternion& lastOrientation,
                                     Libraries::Pad::OrbisFQuaternion& orientation);

    float gyro_poll_rate;
    float accel_poll_rate;
    u32 user_id = -1; // ORBIS_USER_SERVICE_USER_ID_INVALID
    SDL_Gamepad* m_sdl_gamepad = nullptr;
    static constexpr int max_smoothing_ticks = 2;
    int axis_smoothing_ticks[static_cast<int>(Input::Axis::AxisMax)]{0};
    int axis_smoothing_values[static_cast<int>(Input::Axis::AxisMax)]{0};

private:
    struct StateInternal {
        bool obtained = false;
    };

    std::mutex m_mutex;
    bool m_connected = true;
    State m_last_state;
    int m_connected_count = 0;
    u32 m_states_num = 0;
    u32 m_first_state = 0;
    u8 m_touch_count = 0;
    u8 m_secondary_touch_count = 0;
    u8 m_previous_touch_count = 0;
    u8 m_previous_touchnum = 0;
    bool m_was_secondary_reset = false;
    std::array<State, MAX_STATES> m_states;
    std::array<StateInternal, MAX_STATES> m_private;
    std::chrono::steady_clock::time_point m_last_update = {};
    Libraries::Pad::OrbisFQuaternion m_orientation = {0.0f, 0.0f, 0.0f, 1.0f};

    u8 player_index = -1;
};

class GameControllers {
    std::array<GameController*, 4> controllers;

public:
    GameControllers()
        : controllers({new GameController(), new GameController(), new GameController(),
                       new GameController()}) {};
    virtual ~GameControllers() = default;
    GameController* operator[](const size_t& i) const {
        if (i > 3) {
            UNREACHABLE_MSG("Index {} is out of bounds for GameControllers!", i);
        }
        return controllers[i];
    }
    static void TryOpenSDLControllers(GameControllers& controllers);
    static u8 GetGamepadIndexFromJoystickId(SDL_JoystickID id);
};

} // namespace Input

namespace GamepadSelect {

std::optional<u8> GetControllerIndexFromUserID(s32 user_id);
int GetIndexfromGUID(SDL_JoystickID* gamepadIDs, int gamepadCount, std::string GUID);
std::string GetGUIDString(SDL_JoystickID* gamepadIDs, int index);
std::string GetSelectedGamepad();
void SetSelectedGamepad(std::string GUID);

} // namespace GamepadSelect
