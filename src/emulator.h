// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <filesystem>
#include <thread>
#include "common/elf_info.h"

#include "common/singleton.h"
#include "core/linker.h"
#include "input/controller.h"
#include "sdl_window.h"
#include <QString>

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

    void Run(const std::filesystem::path& file, const std::vector<std::string> args = {});
    void UpdatePlayTime(const std::string& serial);
    static Emulator& GetInstance();
    void StopEmulation(); 
    bool is_running = false;
    void Restart();  
    void Shutdown();


private:
    void LoadSystemModules(const std::string& game_serial);
    Common::ElfInfo game_info;
    QString lastEbootPath;
    void saveLastEbootPath(const QString& path);
    QString getLastEbootPath();
    bool isRunning = false;
    Core::MemoryManager* memory;
    Input::GameController* controller;
    Core::Linker* linker;
    std::unique_ptr<Frontend::WindowSDL> window;
    std::chrono::steady_clock::time_point start_time;
};

} // namespace Core
