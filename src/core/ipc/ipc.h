//  SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
//  SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/singleton.h"

#include <semaphore>
#include <thread>

class IPC {
    bool enabled{false};
    std::jthread input_thread{};

    std::binary_semaphore run_semaphore{0};
    std::binary_semaphore start_semaphore{0};

public:
    static IPC& Instance() {
        return *Common::Singleton<IPC>::Instance();
    }

    void Init();

    operator bool() const {
        return enabled;
    }

    [[nodiscard]] bool IsEnabled() const {
        return enabled;
    }

    void WaitForStart() {
        start_semaphore.acquire();
    }

private:
    [[noreturn]] void InputLoop();
};
