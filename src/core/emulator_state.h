// SPDX-FileCopyrightText: Copyright 2025-2026 shadLauncher4 Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include <mutex>

class EmulatorState {
public:
    EmulatorState();
    ~EmulatorState();

    static std::shared_ptr<EmulatorState> GetInstance();
    static void SetInstance(std::shared_ptr<EmulatorState> instance);

    bool IsGameRunning() const;
    void SetGameRunning(bool running);
    bool IsAutoPatchesLoadEnabled() const;
    void SetAutoPatchesLoadEnabled(bool enable);
    bool IsGameSpecifigConfigUsed() const;
    void SetGameSpecifigConfigUsed(bool used);

private:
    static std::shared_ptr<EmulatorState> s_instance;
    static std::mutex s_mutex;

    // state variables
    bool m_running = false;
    bool m_load_patches_auto = true;
    bool m_game_specific_config_used = false;
};
