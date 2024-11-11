// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/kernel/time_management.h"
#include "input/mouse.h"

namespace Input {

GameMouse::GameMouse() {
    m_states_num = 0;
    m_last_state = MouseState();
}

int GameMouse::ReadStates(MouseState* states, int states_num) {
    if (states_num == 0) {
        return 0;
    }
    std::scoped_lock lock{m_mutex};

    const u32 count = std::min(m_states_num, u32(states_num));

    if (count == 0) {
        states[0] = m_last_state;
        return 1;
    }

    u32 begin = (m_index - m_states_num + 1) % MAX_MOUSE_STATES;
    for (u32 i = 0; i < count; i++) {
        u32 idx = (begin + i) % MAX_MOUSE_STATES;
        states[i] = m_states[idx];
    }

    m_states_num -= count;
    return static_cast<int>(count);
}

void GameMouse::AddState(const MouseState& state) {
    std::scoped_lock lock{m_mutex};

    m_index = (m_index + 1) % MAX_MOUSE_STATES;
    if (m_states_num < MAX_MOUSE_STATES) {
        ++m_states_num;
    }
    m_states[m_index] = state;
    m_last_state = MouseState{
        .button_state = state.button_state,
    };
}

void GameMouse::CheckButton(u32 button, bool isPressed) {
    if (!m_connected) {
        return;
    }
    MouseState state = m_last_state;
    state.time = Libraries::Kernel::sceKernelGetProcessTime();
    if (isPressed) {
        state.button_state |= button;
    } else {
        state.button_state &= ~button;
    }

    AddState(state);
}

void GameMouse::CheckMove(int x, int y) {
    if (!m_connected) {
        return;
    }
    MouseState state = m_last_state;
    state.time = Libraries::Kernel::sceKernelGetProcessTime();
    state.x_axis = x;
    state.y_axis = y;

    AddState(state);
}

void GameMouse::CheckWheel(int x, int y) {
    if (!m_connected) {
        return;
    }
    MouseState state = m_last_state;
    state.time = Libraries::Kernel::sceKernelGetProcessTime();
    state.wheel = y;
    state.tilt = x;

    AddState(state);
}

}; // namespace Input
