// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "controller.h"

#include "common/assert.h"
#include "core/libraries/kernel/time_management.h"
#include "core/libraries/pad/pad.h"

#include <SDL3/SDL.h>

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

    auto last = (m_first_state + m_states_num - 1) % MAX_STATES;

    return m_states[last];
}

void GameController::AddState(const State& state) {
    if (m_states_num >= MAX_STATES) {
        m_states_num = MAX_STATES - 1;
        m_first_state = (m_first_state + 1) % MAX_STATES;
    }

    auto index = (m_first_state + m_states_num) % MAX_STATES;

    m_states[index] = state;
    m_last_state = state;
    m_private[index].obtained = false;
    m_states_num++;
}

void GameController::CheckButton(int id, u32 button, bool isPressed) {
    std::scoped_lock lock{m_mutex};
    auto state = GetLastState();
    state.time = Libraries::Kernel::sceKernelGetProcessTime();
    if (isPressed) {
        state.buttonsState |= button;
    } else {
        state.buttonsState &= ~button;
    }

    AddState(state);
}

void GameController::Axis(int id, Input::Axis axis, int value) {
    std::scoped_lock lock{m_mutex};
    auto state = GetLastState();

    state.time = Libraries::Kernel::sceKernelGetProcessTime();

    int axis_id = static_cast<int>(axis);

    state.axes[axis_id] = value;

    if (axis == Input::Axis::TriggerLeft) {
        if (value > 0) {
            state.buttonsState |= Libraries::Pad::OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_L2;
        } else {
            state.buttonsState &= ~Libraries::Pad::OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_L2;
        }
    }

    if (axis == Input::Axis::TriggerRight) {
        if (value > 0) {
            state.buttonsState |= Libraries::Pad::OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_R2;
        } else {
            state.buttonsState &= ~Libraries::Pad::OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_R2;
        }
    }

    AddState(state);
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

void GameController::TryOpenSDLController() {
    if (m_sdl_gamepad == nullptr || !SDL_GamepadConnected(m_sdl_gamepad)) {
        int gamepad_count;
        SDL_JoystickID* gamepads = SDL_GetGamepads(&gamepad_count);
        m_sdl_gamepad = gamepad_count > 0 ? SDL_OpenGamepad(gamepads[0]) : nullptr;
        SDL_free(gamepads);

        SetLightBarRGB(0, 0, 255);
    }
}

u32 GameController::Poll() {
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

} // namespace Input
