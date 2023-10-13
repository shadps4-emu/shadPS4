#pragma once
#include <types.h>
#include "Lib/Threads.h"

namespace Emulator::Host::Controller {
struct State {
    u32 buttonsState;
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
    Lib::Mutex m_mutex;
    bool m_connected = false;
    State m_last_state;
    int m_connected_count = 0;
    u32 m_states_num = 0;
    u32 m_first_state = 0;
    State m_states[MAX_STATES];
};

}  // namespace Emulator::HLE::Libraries::Controller