// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <mutex>
#include "common/types.h"

namespace Emulator::Host::Controller {
struct State {
    u32 buttonsState = 0;
    u64 time = 0;
};

constexpr u32 MAX_STATES = 64;

class GameController {
public:
    GameController();
    virtual ~GameController() = default;

    void readState(State* state, bool* isConnected, int* connectedCount);
    State getLastState() const;
    void checKButton(int id, u32 button, bool isPressed);
    void addState(const State& state);

private:
    std::mutex m_mutex;
    bool m_connected = false;
    State m_last_state;
    int m_connected_count = 0;
    u32 m_states_num = 0;
    u32 m_first_state = 0;
    State m_states[MAX_STATES];
};

} // namespace Emulator::Host::Controller
