// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
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

GameController::GameController() : m_states_queue(64) {}

void GameController::ReadState(State* state, bool* isConnected, int* connectedCount) {
    *isConnected = m_connected;
    *connectedCount = m_connected_count;
    *state = m_state;
}

int GameController::ReadStates(State* states, int states_num, bool* isConnected,
                               int* connectedCount) {
    *isConnected = m_connected;
    *connectedCount = m_connected_count;

    int ret_num = 0;
    if (m_connected) {
        std::lock_guard lg(m_states_queue_mutex);
        for (int i = 0; i < states_num; i++) {
            auto o_state = m_states_queue.Pop();
            if (!o_state) {
                break;
            }
            states[ret_num++] = *o_state;
        }
    }
    return ret_num;
}

void GameController::Button(int id, OrbisPadButtonDataOffset button, bool is_pressed) {
    m_state.OnButton(button, is_pressed);
    PushState();
}

void GameController::Axis(int id, Input::Axis axis, int value) {
    m_state.OnAxis(axis, value);
    PushState();
}

void GameController::Gyro(int id, const float gyro[3]) {
    m_state.OnGyro(gyro);
    PushState();
}

void GameController::Acceleration(int id, const float acceleration[3]) {
    m_state.OnAccel(acceleration);
    PushState();
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
    if (!m_engine) {
        return;
    }
    m_engine->SetLightBarRGB(r, g, b);
}

void GameController::SetVibration(u8 smallMotor, u8 largeMotor) {
    if (!m_engine) {
        return;
    }
    m_engine->SetVibration(smallMotor, largeMotor);
}

void GameController::SetTouchpadState(int touchIndex, bool touchDown, float x, float y) {
    if (touchIndex < 2) {
        m_state.OnTouchpad(touchIndex, touchDown, x, y);
        PushState();
    }
}

u8 GameController::GetTouchCount() {
    return m_touch_count;
}

void GameController::SetTouchCount(u8 touchCount) {
    m_touch_count = touchCount;
}

u8 GameController::GetSecondaryTouchCount() {
    return m_secondary_touch_count;
}

void GameController::SetSecondaryTouchCount(u8 touchCount) {
    m_secondary_touch_count = touchCount;
    if (touchCount == 0) {
        m_was_secondary_reset = true;
    }
}

u8 GameController::GetPreviousTouchNum() {
    return m_previous_touchnum;
}

void GameController::SetPreviousTouchNum(u8 touchNum) {
    m_previous_touchnum = touchNum;
}

bool GameController::WasSecondaryTouchReset() {
    return m_was_secondary_reset;
}

void GameController::UnsetSecondaryTouchResetBool() {
    m_was_secondary_reset = false;
}

void GameController::SetLastOrientation(Libraries::Pad::OrbisFQuaternion& orientation) {
    m_orientation = orientation;
}

Libraries::Pad::OrbisFQuaternion GameController::GetLastOrientation() {
    return m_orientation;
}

std::chrono::steady_clock::time_point GameController::GetLastUpdate() {
    return m_last_update;
}

void GameController::SetLastUpdate(std::chrono::steady_clock::time_point lastUpdate) {
    m_last_update = lastUpdate;
}

void GameController::SetEngine(std::unique_ptr<Engine> engine) {
    m_engine = std::move(engine);
    if (m_engine) {
        m_engine->Init();
    }
}

Engine* GameController::GetEngine() {
    return m_engine.get();
}

void GameController::PushState() {
    std::lock_guard lg(m_states_queue_mutex);
    m_state.time = Libraries::Kernel::sceKernelGetProcessTime();
    m_states_queue.Push(m_state);
}

u32 GameController::Poll() {
    if (m_connected) {
        PushState();
    }
    return 33;
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
