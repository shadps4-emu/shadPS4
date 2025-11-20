// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <filesystem>
#include <optional>
#include <thread>

#include "common/singleton.h"
#include "core/linker.h"
#include "input/controller.h"
#include "sdl_window.h"

namespace Core {

using HLEInitDef = void (*)(Core::Loader::SymbolsResolver* sym);

struct SysModules {
    std::string_view module_name;
    HLEInitDef callback;
};

class Emulator {
public:
    Emulator();
    ~Emulator();

    void Run(std::filesystem::path file, std::vector<std::string> args = {},
             std::optional<std::filesystem::path> game_folder = {});
    void UpdatePlayTime(const std::string& serial);

    /**
     * This will kill the current process and launch a new process with the same configuration
     * (using CLI args) but replacing the eboot image and guest arguments
     */
    void Restart(std::filesystem::path eboot_path, const std::vector<std::string>& guest_args = {});

    const char* executableName;
    bool waitForDebuggerBeforeRun{false};

private:
    void LoadSystemModules(const std::string& game_serial);

    Core::MemoryManager* memory;
    Input::GameController* controller;
    Core::Linker* linker;
    std::unique_ptr<Frontend::WindowSDL> window;
    std::chrono::steady_clock::time_point start_time;
};

} // namespace Core
