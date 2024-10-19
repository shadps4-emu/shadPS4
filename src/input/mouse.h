// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include <mutex>
#include "common/types.h"

namespace Input {

struct MouseState {
    u64 time = 0;
    u32 button_state = 0;
    s32 x_axis = 0;
    s32 y_axis = 0;
    s32 wheel = 0;
    s32 tilt = 0;
};

constexpr u32 MAX_MOUSE_STATES = 64;

class GameMouse {
public:
    GameMouse();
    virtual ~GameMouse() = default;

    int ReadStates(MouseState* states, int states_num);

    void CheckButton(u32 button, bool isPressed);
    void CheckMove(int x, int y);
    void CheckWheel(int x, int y);

    bool m_connected = false;
    float speed = 1.0f;

private:
    void AddState(const MouseState& state);

    std::mutex m_mutex;
    MouseState m_last_state;
    u32 m_states_num = 0;
    u32 m_index = 0;
    std::array<MouseState, MAX_MOUSE_STATES> m_states;
};

} // namespace Input