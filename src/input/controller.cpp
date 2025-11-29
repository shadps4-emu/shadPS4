// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <SDL3/SDL.h>
#include "common/config.h"
#include "common/logging/log.h"
#include "core/libraries/kernel/time.h"
#include "core/libraries/pad/pad.h"
#include "input/controller.h"

static std::string SelectedGamepad = "";

namespace Input {

using Libraries::Pad::OrbisPadButtonDataOffset;

void State::OnButton(OrbisPadButtonDataOffset button, bool isPressed) {
    if (isPressed) {
        buttonsState |= button;
    } else {
        buttonsState &= ~button;
    }
}

void State::OnAxis(Axis axis, int value) {
    const auto toggle = [&](const auto button) {
        if (value > 0) {
            buttonsState |= button;
        } else {
            buttonsState &= ~button;
        }
    };
    switch (axis) {
    case Axis::TriggerLeft:
        toggle(OrbisPadButtonDataOffset::L2);
        break;
    case Axis::TriggerRight:
        toggle(OrbisPadButtonDataOffset::R2);
        break;
    default:
        break;
    }
    axes[static_cast<int>(axis)] = value;
}

void State::OnTouchpad(int touchIndex, bool isDown, float x, float y) {
    touchpad[touchIndex].state = isDown;
    touchpad[touchIndex].x = static_cast<u16>(x * 1920);
    touchpad[touchIndex].y = static_cast<u16>(y * 941);
}

void State::OnGyro(const float gyro[3]) {
    angularVelocity.x = gyro[0];
    angularVelocity.y = gyro[1];
    angularVelocity.z = gyro[2];
}

void State::OnAccel(const float accel[3]) {
    acceleration.x = accel[0];
    acceleration.y = accel[1];
    acceleration.z = accel[2];
}

GameController::GameController() {
    m_states_num = 0;
    m_last_state = State();
}

void GameController::ReadState(State* state, bool* isConnected, int* connectedCount) {
    std::scoped_lock lock{m_mutex};

    *isConnected = m_connected;
    *connectedCount = m_connected_count;
    *state = GetLastState();
}

int GameController::ReadStates(State* states, int states_num, bool* isConnected,
                               int* connectedCount) {
    std::scoped_lock lock{m_mutex};

    *isConnected = m_connected;
    *connectedCount = m_connected_count;

    int ret_num = 0;

    if (m_connected) {
        if (m_states_num == 0) {
            ret_num = 1;
            states[0] = m_last_state;
        } else {
            for (uint32_t i = 0; i < m_states_num; i++) {
                if (ret_num >= states_num) {
                    break;
                }
                auto index = (m_first_state + i) % MAX_STATES;
                if (!m_private[index].obtained) {
                    m_private[index].obtained = true;

                    states[ret_num++] = m_states[index];
                }
            }
        }
    }

    return ret_num;
}

State GameController::GetLastState() const {
    if (m_states_num == 0) {
        return m_last_state;
    }
    const u32 last = (m_first_state + m_states_num - 1) % MAX_STATES;
    return m_states[last];
}

void GameController::AddState(const State& state) {
    if (m_states_num >= MAX_STATES) {
        m_states_num = MAX_STATES - 1;
        m_first_state = (m_first_state + 1) % MAX_STATES;
    }

    const u32 index = (m_first_state + m_states_num) % MAX_STATES;
    m_states[index] = state;
    m_last_state = state;
    m_private[index].obtained = false;
    m_states_num++;
}

void GameController::CheckButton(int id, OrbisPadButtonDataOffset button, bool is_pressed) {
    std::scoped_lock lock{m_mutex};
    auto state = GetLastState();

    state.time = Libraries::Kernel::sceKernelGetProcessTime();
    state.OnButton(button, is_pressed);

    AddState(state);
}

void GameController::Axis(int id, Input::Axis axis, int value) {
    std::scoped_lock lock{m_mutex};
    auto state = GetLastState();

    state.time = Libraries::Kernel::sceKernelGetProcessTime();
    state.OnAxis(axis, value);

    AddState(state);
}

void GameController::Gyro(int id, const float gyro[3]) {
    std::scoped_lock lock{m_mutex};
    auto state = GetLastState();
    state.time = Libraries::Kernel::sceKernelGetProcessTime();

    // Update the angular velocity (gyro data)
    state.OnGyro(gyro);

    AddState(state);
}

void GameController::Acceleration(int id, const float acceleration[3]) {
    std::scoped_lock lock{m_mutex};
    auto state = GetLastState();
    state.time = Libraries::Kernel::sceKernelGetProcessTime();

    // Update the acceleration values
    state.OnAccel(acceleration);

    AddState(state);
}

void GameController::CalculateOrientation(Libraries::Pad::OrbisFVector3& acceleration,
                                          Libraries::Pad::OrbisFVector3& angularVelocity,
                                          float deltaTime,
                                          Libraries::Pad::OrbisFQuaternion& lastOrientation,
                                          Libraries::Pad::OrbisFQuaternion& orientation) {
    Libraries::Pad::OrbisFQuaternion q = lastOrientation;
    Libraries::Pad::OrbisFQuaternion ω = {angularVelocity.x, angularVelocity.y, angularVelocity.z,
                                          0.0f};

    Libraries::Pad::OrbisFQuaternion qω = {q.w * ω.x + q.x * ω.w + q.y * ω.z - q.z * ω.y,
                                           q.w * ω.y + q.y * ω.w + q.z * ω.x - q.x * ω.z,
                                           q.w * ω.z + q.z * ω.w + q.x * ω.y - q.y * ω.x,
                                           q.w * ω.w - q.x * ω.x - q.y * ω.y - q.z * ω.z};

    Libraries::Pad::OrbisFQuaternion qDot = {0.5f * qω.x, 0.5f * qω.y, 0.5f * qω.z, 0.5f * qω.w};

    q.x += qDot.x * deltaTime;
    q.y += qDot.y * deltaTime;
    q.z += qDot.z * deltaTime;
    q.w += qDot.w * deltaTime;

    float norm = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
    q.x /= norm;
    q.y /= norm;
    q.z /= norm;
    q.w /= norm;

    orientation.x = q.x;
    orientation.y = q.y;
    orientation.z = q.z;
    orientation.w = q.w;
    LOG_DEBUG(Lib_Pad, "Calculated orientation: {:.2f} {:.2f} {:.2f} {:.2f}", orientation.x,
              orientation.y, orientation.z, orientation.w);
}

void GameController::SetLightBarRGB(u8 r, u8 g, u8 b) {
    std::scoped_lock _{m_mutex};
    if (m_sdl_gamepad) {
        SDL_SetGamepadLED(m_sdl_gamepad, r, g, b);
    }
}

void GameController::SetVibration(u8 smallMotor, u8 largeMotor) {
    std::scoped_lock _{m_mutex};
    if (m_sdl_gamepad) {
        const auto low_freq = (smallMotor / 255.0f) * 0xFFFF;
        const auto high_freq = (largeMotor / 255.0f) * 0xFFFF;
        SDL_RumbleGamepad(m_sdl_gamepad, low_freq, high_freq, -1);
    }
}

void GameController::SetTouchpadState(int touchIndex, bool touchDown, float x, float y) {
    if (touchIndex < 2) {
        std::scoped_lock lock{m_mutex};
        auto state = GetLastState();

        state.time = Libraries::Kernel::sceKernelGetProcessTime();
        state.OnTouchpad(touchIndex, touchDown, x, y);

        AddState(state);
    }
}

u8 GameController::GetTouchCount() {
    std::scoped_lock lock{m_mutex};
    return m_touch_count;
}

void GameController::SetTouchCount(u8 touchCount) {
    std::scoped_lock lock{m_mutex};
    m_touch_count = touchCount;
}

u8 GameController::GetSecondaryTouchCount() {
    std::scoped_lock lock{m_mutex};
    return m_secondary_touch_count;
}

void GameController::SetSecondaryTouchCount(u8 touchCount) {
    std::scoped_lock lock{m_mutex};
    m_secondary_touch_count = touchCount;
    if (touchCount == 0) {
        m_was_secondary_reset = true;
    }
}

u8 GameController::GetPreviousTouchNum() {
    std::scoped_lock lock{m_mutex};
    return m_previous_touchnum;
}

void GameController::SetPreviousTouchNum(u8 touchNum) {
    std::scoped_lock lock{m_mutex};
    m_previous_touchnum = touchNum;
}

bool GameController::WasSecondaryTouchReset() {
    std::scoped_lock lock{m_mutex};
    return m_was_secondary_reset;
}

void GameController::UnsetSecondaryTouchResetBool() {
    std::scoped_lock lock{m_mutex};
    m_was_secondary_reset = false;
}

void GameController::SetLastOrientation(Libraries::Pad::OrbisFQuaternion& orientation) {
    std::scoped_lock lock{m_mutex};
    m_orientation = orientation;
}

Libraries::Pad::OrbisFQuaternion GameController::GetLastOrientation() {
    std::scoped_lock lock{m_mutex};
    return m_orientation;
}

std::chrono::steady_clock::time_point GameController::GetLastUpdate() {
    std::scoped_lock lock{m_mutex};
    return m_last_update;
}

void GameController::SetLastUpdate(std::chrono::steady_clock::time_point lastUpdate) {
    std::scoped_lock lock{m_mutex};
    m_last_update = lastUpdate;
}

void GameController::TryOpenSDLController() {
    if (m_sdl_gamepad) {
        SDL_CloseGamepad(m_sdl_gamepad);
        m_sdl_gamepad = nullptr;
    }

    int gamepad_count;
    SDL_JoystickID* gamepads = SDL_GetGamepads(&gamepad_count);
    if (!gamepads) {
        LOG_ERROR(Input, "Cannot get gamepad list: {}", SDL_GetError());
        return;
    }
    if (gamepad_count == 0) {
        LOG_INFO(Input, "No gamepad found!");
        SDL_free(gamepads);
        return;
    }

    int selectedIndex = GamepadSelect::GetIndexfromGUID(gamepads, gamepad_count,
                                                        GamepadSelect::GetSelectedGamepad());
    int defaultIndex =
        GamepadSelect::GetIndexfromGUID(gamepads, gamepad_count, Config::getDefaultControllerID());

    // If user selects a gamepad in the GUI, use that, otherwise try the default
    if (!m_sdl_gamepad) {
        if (selectedIndex != -1) {
            m_sdl_gamepad = SDL_OpenGamepad(gamepads[selectedIndex]);
            LOG_INFO(Input, "Opening gamepad selected in GUI.");
        } else if (defaultIndex != -1) {
            m_sdl_gamepad = SDL_OpenGamepad(gamepads[defaultIndex]);
            LOG_INFO(Input, "Opening default gamepad.");
        } else {
            m_sdl_gamepad = SDL_OpenGamepad(gamepads[0]);
            LOG_INFO(Input, "Got {} gamepads. Opening the first one.", gamepad_count);
        }
    }

    if (!m_sdl_gamepad) {
        if (!m_sdl_gamepad) {
            LOG_ERROR(Input, "Failed to open gamepad: {}", SDL_GetError());
            SDL_free(gamepads);
            return;
        }
    }

    SDL_Joystick* joystick = SDL_GetGamepadJoystick(m_sdl_gamepad);
    Uint16 vendor = SDL_GetJoystickVendor(joystick);
    Uint16 product = SDL_GetJoystickProduct(joystick);

    bool isDualSense = (vendor == 0x054C && product == 0x0CE6);

    LOG_INFO(Input, "Gamepad Vendor: {:04X}, Product: {:04X}", vendor, product);
    if (isDualSense) {
        LOG_INFO(Input, "Detected DualSense Controller");
    }

    if (Config::getIsMotionControlsEnabled()) {
        if (SDL_SetGamepadSensorEnabled(m_sdl_gamepad, SDL_SENSOR_GYRO, true)) {
            m_gyro_poll_rate = SDL_GetGamepadSensorDataRate(m_sdl_gamepad, SDL_SENSOR_GYRO);
            LOG_INFO(Input, "Gyro initialized, poll rate: {}", m_gyro_poll_rate);
        } else {
            LOG_ERROR(Input, "Failed to initialize gyro controls for gamepad, error: {}",
                      SDL_GetError());
            SDL_SetGamepadSensorEnabled(m_sdl_gamepad, SDL_SENSOR_GYRO, false);
        }
        if (SDL_SetGamepadSensorEnabled(m_sdl_gamepad, SDL_SENSOR_ACCEL, true)) {
            m_accel_poll_rate = SDL_GetGamepadSensorDataRate(m_sdl_gamepad, SDL_SENSOR_ACCEL);
            LOG_INFO(Input, "Accel initialized, poll rate: {}", m_accel_poll_rate);
        } else {
            LOG_ERROR(Input, "Failed to initialize accel controls for gamepad, error: {}",
                      SDL_GetError());
            SDL_SetGamepadSensorEnabled(m_sdl_gamepad, SDL_SENSOR_ACCEL, false);
        }
    }

    SDL_free(gamepads);

    int* rgb = Config::GetControllerCustomColor();

    if (isDualSense) {
        if (SDL_SetJoystickLED(joystick, rgb[0], rgb[1], rgb[2]) == 0) {
            LOG_INFO(Input, "Set DualSense LED to R:{} G:{} B:{}", rgb[0], rgb[1], rgb[2]);
        } else {
            LOG_ERROR(Input, "Failed to set DualSense LED: {}", SDL_GetError());
        }
    } else {
        SetLightBarRGB(rgb[0], rgb[1], rgb[2]);
    }
}

u32 GameController::Poll() {
    if (m_connected) {
        std::scoped_lock lock{m_mutex};
        auto time = Libraries::Kernel::sceKernelGetProcessTime();
        if (m_states_num == 0) {
            auto diff = (time - m_last_state.time) / 1000;
            if (diff >= 100) {
                AddState(GetLastState());
            }
        } else {
            auto index = (m_first_state - 1 + m_states_num) % MAX_STATES;
            auto diff = (time - m_states[index].time) / 1000;
            if (m_private[index].obtained && diff >= 100) {
                AddState(GetLastState());
            }
        }
    }
    return 100;
}

} // namespace Input

namespace GamepadSelect {

int GetDefaultGamepad(SDL_JoystickID* gamepadIDs, int gamepadCount) {
    char GUIDbuf[33];
    if (Config::getDefaultControllerID() != "") {
        for (int i = 0; i < gamepadCount; i++) {
            SDL_GUIDToString(SDL_GetGamepadGUIDForID(gamepadIDs[i]), GUIDbuf, 33);
            std::string currentGUID = std::string(GUIDbuf);
            if (currentGUID == Config::getDefaultControllerID()) {
                return i;
            }
        }
    }
    return -1;
}

int GetIndexfromGUID(SDL_JoystickID* gamepadIDs, int gamepadCount, std::string GUID) {
    char GUIDbuf[33];
    for (int i = 0; i < gamepadCount; i++) {
        SDL_GUIDToString(SDL_GetGamepadGUIDForID(gamepadIDs[i]), GUIDbuf, 33);
        std::string currentGUID = std::string(GUIDbuf);
        if (currentGUID == GUID) {
            return i;
        }
    }
    return -1;
}

std::string GetGUIDString(SDL_JoystickID* gamepadIDs, int index) {
    char GUIDbuf[33];
    SDL_GUIDToString(SDL_GetGamepadGUIDForID(gamepadIDs[index]), GUIDbuf, 33);
    std::string GUID = std::string(GUIDbuf);
    return GUID;
}

std::string GetSelectedGamepad() {
    return SelectedGamepad;
}

void SetSelectedGamepad(std::string GUID) {
    SelectedGamepad = GUID;
}

} // namespace GamepadSelect
