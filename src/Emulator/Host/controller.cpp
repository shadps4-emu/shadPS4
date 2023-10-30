#include "controller.h"
#include <Core/hle/libraries/libkernel/time_management.h>

namespace Emulator::Host::Controller {
GameController::GameController() { m_states_num = 0;
    m_last_state = State();
}
void GameController::readState(State* state, bool* isConnected, int* connectedCount) {
    std::scoped_lock lock{m_mutex};

    *isConnected = m_connected;
    *connectedCount = m_connected_count;
    *state = getLastState();
}

State GameController::getLastState() const {
    if (m_states_num == 0) {
        return m_last_state;
    }

    auto last = (m_first_state + m_states_num - 1) % MAX_STATES;

    return m_states[last];
}

void GameController::addState(const State& state) {
    if (m_states_num >= MAX_STATES) {
        m_states_num = MAX_STATES - 1;
        m_first_state = (m_first_state + 1) % MAX_STATES;
    }

    auto index = (m_first_state + m_states_num) % MAX_STATES;

    m_states[index] = state;
    m_last_state = state;

    m_states_num++;
}

void GameController::checKButton(int id, u32 button, bool isPressed) {
    std::scoped_lock lock{m_mutex};
    auto state = getLastState();
    state.time = Core::Libraries::sceKernelGetProcessTime();
    if (isPressed) {
        state.buttonsState |= button;
    } else {
        state.buttonsState &= ~button;
    }

    addState(state);
}

}  // namespace Emulator::Host::Controller