// SPDX-FileCopyrightText: Copyright 2025-2026 shadLauncher4 Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "emulator_state.h"

std::shared_ptr<EmulatorState> EmulatorState::s_instance = nullptr;
std::mutex EmulatorState::s_mutex;

EmulatorState::EmulatorState() {}

EmulatorState::~EmulatorState() {}

std::shared_ptr<EmulatorState> EmulatorState::GetInstance() {
    std::lock_guard<std::mutex> lock(s_mutex);
    if (!s_instance)
        s_instance = std::make_shared<EmulatorState>();
    return s_instance;
}

void EmulatorState::SetInstance(std::shared_ptr<EmulatorState> instance) {
    std::lock_guard<std::mutex> lock(s_mutex);
    s_instance = instance;
}

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
