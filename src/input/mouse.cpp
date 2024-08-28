// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/libraries/kernel/time_management.h"
#include "input/mouse.h"

namespace Input {

GameMouse::GameMouse() {
    m_states_num = 0;
    m_last_state = MouseState();
}

int GameMouse::ReadStates(MouseState* states, int states_num, bool* isConnected) {
    std::scoped_lock lock{m_mutex};

    *isConnected = m_connected;

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
                auto index = (m_first_state + i) % MAX_MOUSE_STATES;
                if (!m_private[index].obtained) {
                    m_private[index].obtained = true;

                    states[ret_num++] = m_states[index];
                }
            }
        }
    }

    return ret_num;
}

MouseState GameMouse::GetLastState() const {
    if (m_states_num == 0) {
        return m_last_state;
    }

    auto last = (m_first_state + m_states_num - 1) % MAX_MOUSE_STATES;

    return m_states[last];
}

void GameMouse::AddState(const MouseState& state) {
    if (m_states_num >= MAX_MOUSE_STATES) {
        m_states_num = MAX_MOUSE_STATES - 1;
        m_first_state = (m_first_state + 1) % MAX_MOUSE_STATES;
    }

    auto index = (m_first_state + m_states_num) % MAX_MOUSE_STATES;

    m_states[index] = state;
    m_last_state = state;
    m_private[index].obtained = false;
    m_states_num++;
}

void GameMouse::CheckButton(int id, u32 button, bool isPressed) {
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

}; // namespace Input
