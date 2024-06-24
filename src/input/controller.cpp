// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/kernel/time_management.h"
#include "core/libraries/pad/pad.h"
#include "input/controller.h"
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

void GameController::InitGamePad() {
    int count;
    SDL_JoystickID* joysticks = SDL_GetJoysticks(&count);
    SDL_JoystickID instance_id = joysticks[0];
    gamepad = SDL_OpenGamepad(instance_id);
}

u32 GameController::GetGamepadButtons() {
    u32 buttons = 0;
    if (gamepad) {
        buttons |= SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_LEFT_STICK) == SDL_PRESSED
                       ? (Libraries::Pad::OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_L3)
                       : 0;
        buttons |= SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_RIGHT_STICK) == SDL_PRESSED
                       ? (Libraries::Pad::OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_R3)
                       : 0;
        buttons |= SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_START) == SDL_PRESSED
                       ? (Libraries::Pad::OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_OPTIONS)
                       : 0;
        buttons |= SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_UP) == SDL_PRESSED
                       ? (Libraries::Pad::OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_UP)
                       : 0;
        buttons |= SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_RIGHT) == SDL_PRESSED
                       ? (Libraries::Pad::OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_RIGHT)
                       : 0;
        buttons |= SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_DOWN) == SDL_PRESSED
                       ? (Libraries::Pad::OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_DOWN)
                       : 0;
        buttons |= SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_LEFT) == SDL_PRESSED
                       ? (Libraries::Pad::OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_LEFT)
                       : 0;
        buttons |= SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER) == SDL_PRESSED
                       ? (Libraries::Pad::OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_L1)
                       : 0;
        buttons |= SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER)
                       ? (Libraries::Pad::OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_L2)
                       : 0;
        buttons |= SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER) == SDL_PRESSED
                       ? (Libraries::Pad::OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_R1)
                       : 0;
        buttons |= SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER)
                       ? (Libraries::Pad::OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_R2)
                       : 0;
        buttons |= SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_NORTH) == SDL_PRESSED
                       ? (Libraries::Pad::OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_TRIANGLE)
                       : 0;
        buttons |= SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_EAST) == SDL_PRESSED
                       ? (Libraries::Pad::OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_CIRCLE)
                       : 0;
        buttons |= SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_SOUTH) == SDL_PRESSED
                       ? (Libraries::Pad::OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_CROSS)
                       : 0;
        buttons |= SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_WEST) == SDL_PRESSED
                       ? (Libraries::Pad::OrbisPadButtonDataOffset::ORBIS_PAD_BUTTON_SQUARE)
                       : 0;
    }
    return buttons;
}

State GameController::GetGamePadAxis() {
    u8 lx = static_cast<u8>((SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTX) + 32768) / 257);
    u8 ly = static_cast<u8>((SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTY) + 32768) / 257);
    u8 rx = static_cast<u8>((SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHTX) + 32768) / 257);
    u8 ry = static_cast<u8>((SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHTY) + 32768) / 257);
    u8 lt = static_cast<u8>(SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER));
    u8 rt = static_cast<u8>(SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER));
    return {0, 0, {lx, ly, rx, ry, lt, rt}};
}

int GameController::GetRumble(u16 smallFreq, u16 bigFreq) {
    return SDL_RumbleGamepad(gamepad, smallFreq, bigFreq, -1);
}
} // namespace Input
