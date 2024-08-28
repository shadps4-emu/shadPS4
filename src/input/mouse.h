// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include <mutex>
#include "common/types.h"

namespace Input {

struct MouseState {
    u32 buttonsState = 0;
    u64 time = 0;
};

constexpr u32 MAX_MOUSE_STATES = 64;

class GameMouse {
public:
    GameMouse();
    virtual ~GameMouse() = default;

    int ReadStates(MouseState* states, int states_num, bool* isConnected);
    MouseState GetLastState() const;
    void CheckButton(int id, u32 button, bool isPressed);
    void AddState(const MouseState& state);

private:
    struct StateInternal {
        bool obtained = false;
    };

    std::mutex m_mutex;
    bool m_connected = true;
    MouseState m_last_state;
    int m_connected_count = 0;
    u32 m_states_num = 0;
    u32 m_first_state = 0;
    std::array<MouseState, MAX_MOUSE_STATES> m_states;
    std::array<StateInternal, MAX_MOUSE_STATES> m_private;

};

} // namespace Input