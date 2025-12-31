// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <unordered_set>
#include <SDL3/SDL.h>
#include <common/singleton.h>
#include "common/config.h"
#include "common/logging/log.h"
#include "controller.h"
#include "core/libraries/kernel/time.h"
#include "core/libraries/pad/pad.h"
#include "core/libraries/system/userservice.h"
#include "input/controller.h"

static std::string SelectedGamepad = "";

namespace Input {

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
    auto copy = m_states[last];
    return copy;
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

void GameController::CheckButton(int id, Libraries::Pad::OrbisPadButtonDataOffset button,
                                 bool is_pressed) {
    std::scoped_lock lock{m_mutex};
    auto state = GetLastState();
    state.time = Libraries::Kernel::sceKernelGetProcessTime();
    if (is_pressed) {
        state.buttonsState |= button;
    } else {
        state.buttonsState &= ~button;
    }

    AddState(state);
}

void GameController::Axis(int id, Input::Axis axis, int value) {
    using Libraries::Pad::OrbisPadButtonDataOffset;

    std::scoped_lock lock{m_mutex};
    auto state = GetLastState();

    state.time = Libraries::Kernel::sceKernelGetProcessTime();
    int axis_id = static_cast<int>(axis);
    if (std::abs(state.axes[axis_id] - value) > 120) {
        LOG_DEBUG(Input, "Keyboard axis change detected");
        axis_smoothing_ticks[axis_id] = GameController::max_smoothing_ticks;
        axis_smoothing_values[axis_id] = state.axes[axis_id];
    }
    state.axes[axis_id] = value;

    if (axis == Input::Axis::TriggerLeft) {
        if (value > 0) {
            state.buttonsState |= OrbisPadButtonDataOffset::L2;
        } else {
            state.buttonsState &= ~OrbisPadButtonDataOffset::L2;
        }
    }

    if (axis == Input::Axis::TriggerRight) {
        if (value > 0) {
            state.buttonsState |= OrbisPadButtonDataOffset::R2;
        } else {
            state.buttonsState &= ~OrbisPadButtonDataOffset::R2;
        }
    }

    AddState(state);
}

void GameController::Gyro(int id, const float gyro[3]) {
    std::scoped_lock lock{m_mutex};
    auto state = GetLastState();
    state.time = Libraries::Kernel::sceKernelGetProcessTime();

    // Update the angular velocity (gyro data)
    state.angularVelocity.x = gyro[0]; // X-axis
    state.angularVelocity.y = gyro[1]; // Y-axis
    state.angularVelocity.z = gyro[2]; // Z-axis

    AddState(state);
}
void GameController::Acceleration(int id, const float acceleration[3]) {
    std::scoped_lock lock{m_mutex};
    auto state = GetLastState();
    state.time = Libraries::Kernel::sceKernelGetProcessTime();

    // Update the acceleration values
    state.acceleration.x = acceleration[0]; // X-axis
    state.acceleration.y = acceleration[1]; // Y-axis
    state.acceleration.z = acceleration[2]; // Z-axis

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
    if (m_sdl_gamepad != nullptr) {
        SDL_SetGamepadLED(m_sdl_gamepad, r, g, b);
    }
}

bool GameController::SetVibration(u8 smallMotor, u8 largeMotor) {
    if (m_sdl_gamepad != nullptr) {
        return SDL_RumbleGamepad(m_sdl_gamepad, (smallMotor / 255.0f) * 0xFFFF,
                                 (largeMotor / 255.0f) * 0xFFFF, -1);
    }
    return true;
}

void GameController::SetTouchpadState(int touchIndex, bool touchDown, float x, float y) {
    if (touchIndex < 2) {
        std::scoped_lock lock{m_mutex};
        auto state = GetLastState();
        state.time = Libraries::Kernel::sceKernelGetProcessTime();

        state.touchpad[touchIndex].state = touchDown;
        state.touchpad[touchIndex].x = static_cast<u16>(x * 1920);
        state.touchpad[touchIndex].y = static_cast<u16>(y * 941);

        AddState(state);
    }
}

bool is_first_check = true;

void GameControllers::TryOpenSDLControllers(GameControllers& controllers) {
    using namespace Libraries::UserService;
    int controller_count;
    SDL_JoystickID* new_joysticks = SDL_GetGamepads(&controller_count);

    std::unordered_set<SDL_JoystickID> assigned_ids;
    std::array<bool, 4> slot_taken{false, false, false, false};

    for (int i = 0; i < 4; i++) {
        SDL_Gamepad* pad = controllers[i]->m_sdl_gamepad;
        if (pad) {
            SDL_JoystickID id = SDL_GetGamepadID(pad);
            bool still_connected = false;
            for (int j = 0; j < controller_count; j++) {
                if (new_joysticks[j] == id) {
                    still_connected = true;
                    assigned_ids.insert(id);
                    slot_taken[i] = true;
                    break;
                }
            }
            if (!still_connected) {
                AddUserServiceEvent({OrbisUserServiceEventType::Logout, i + 1});
                SDL_CloseGamepad(pad);
                controllers[i]->m_sdl_gamepad = nullptr;
                controllers[i]->user_id = -1;
                slot_taken[i] = false;
            } else {
                controllers[i]->player_index = i;
            }
        }
    }

    for (int j = 0; j < controller_count; j++) {
        SDL_JoystickID id = new_joysticks[j];
        if (assigned_ids.contains(id))
            continue;

        SDL_Gamepad* pad = SDL_OpenGamepad(id);
        if (!pad)
            continue;

        for (int i = 0; i < 4; i++) {
            if (!slot_taken[i]) {
                auto* c = controllers[i];
                c->m_sdl_gamepad = pad;
                LOG_INFO(Input, "Gamepad registered for slot {}! Handle: {}", i,
                         SDL_GetGamepadID(pad));
                c->user_id = i + 1;
                slot_taken[i] = true;
                c->player_index = i;
                AddUserServiceEvent({OrbisUserServiceEventType::Login, i + 1});

                if (SDL_SetGamepadSensorEnabled(c->m_sdl_gamepad, SDL_SENSOR_GYRO, true)) {
                    c->gyro_poll_rate =
                        SDL_GetGamepadSensorDataRate(c->m_sdl_gamepad, SDL_SENSOR_GYRO);
                    LOG_INFO(Input, "Gyro initialized, poll rate: {}", c->gyro_poll_rate);
                } else {
                    LOG_ERROR(Input, "Failed to initialize gyro controls for gamepad {}",
                              c->user_id);
                }
                if (SDL_SetGamepadSensorEnabled(c->m_sdl_gamepad, SDL_SENSOR_ACCEL, true)) {
                    c->accel_poll_rate =
                        SDL_GetGamepadSensorDataRate(c->m_sdl_gamepad, SDL_SENSOR_ACCEL);
                    LOG_INFO(Input, "Accel initialized, poll rate: {}", c->accel_poll_rate);
                } else {
                    LOG_ERROR(Input, "Failed to initialize accel controls for gamepad {}",
                              c->user_id);
                }
                break;
            }
        }
    }
    if (is_first_check) [[unlikely]] {
        is_first_check = false;
        if (controller_count == 0) {
            controllers[0]->user_id = 1;
            AddUserServiceEvent({OrbisUserServiceEventType::Login, 1});
        }
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

u32 GameController::Poll() {
    std::scoped_lock lock{m_mutex};
    if (m_connected) {
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

u8 GameControllers::GetGamepadIndexFromJoystickId(SDL_JoystickID id) {
    s32 index = SDL_GetGamepadPlayerIndex(SDL_GetGamepadFromID(id));
    LOG_TRACE(Input, "Gamepad index: {}", index);
    return index;
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

std::optional<u8> GetControllerIndexFromUserID(s32 user_id) {
    if (user_id < 1 || user_id > 4)
        return std::nullopt;
    return static_cast<u8>(user_id - 1);
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
