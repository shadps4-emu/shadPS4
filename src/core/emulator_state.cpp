// SPDX-FileCopyrightText: Copyright 2025-2026 shadLauncher4 Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "emulator_state.h"

EmulatorState::EmulatorState() = default;

EmulatorState::~EmulatorState() = default;

bool EmulatorState::IsGameRunning() const {
    return m_running;
}
void EmulatorState::SetGameRunning(bool running) {
    m_running = running;
}

bool EmulatorState::IsAutoPatchesLoadEnabled() const {
    return m_load_patches_auto;
}
void EmulatorState::SetAutoPatchesLoadEnabled(bool enable) {
    m_load_patches_auto = enable;
}

bool EmulatorState::IsGameSpecifigConfigUsed() const {
    return m_game_specific_config_used;
}

void EmulatorState::SetGameSpecifigConfigUsed(bool used) {
    m_game_specific_config_used = used;
}
