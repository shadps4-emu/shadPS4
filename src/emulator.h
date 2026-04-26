// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <filesystem>
#include <optional>
#include <thread>

#include "core/file_sys/fs.h"
#include "core/libraries/kernel/threads/thread_state.h"
#include "core/libraries/network/net_util.h"
#include "input/controller.h"
#include "sdl_window.h"

class PSF;

namespace Libraries {
struct HleLayer;
}

namespace Storage {
struct DataBase;
}

namespace Common {
class ElfInfo;
class DecoderImpl;
}

namespace Libraries::Kernel {
struct ThreadState;
}

namespace Platform {
struct IrqController;
}

namespace Core {
class Linker;
class MemoryManager;
class SignalDispatch;

namespace FileSys {
class MntPoints;
}

struct SysModules {
    std::string_view module_name;
    bool should_init;
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

    void LoadSystemModules(const std::string& game_serial);

    std::unique_ptr<Core::MemoryManager> memory;
    std::unique_ptr<Input::GameControllers> controllers;
    std::unique_ptr<Core::Linker> linker;
    std::unique_ptr<Frontend::WindowSDL> window;
    std::chrono::steady_clock::time_point start_time;
    std::jthread play_time_thread;
    std::unique_ptr<Libraries::HleLayer> m_hle_layer;

    std::unique_ptr<Core::FileSys::MntPoints> m_mnt_points;
    std::unique_ptr<Common::ElfInfo> m_elf_info;
    std::unique_ptr<Platform::IrqController> irq_controller;
    std::unique_ptr<NetUtil::NetUtilInternal> m_net_util_internal;
    std::unique_ptr<PSF> m_psf;
    std::unique_ptr<Core::FileSys::HandleTable> m_handle_table;
    std::unique_ptr<Libraries::Kernel::ThreadState> m_thread_state;
    std::unique_ptr<Common::DecoderImpl> m_decoder;
    std::unique_ptr<Core::SignalDispatch> m_signals;
    std::unique_ptr<Storage::DataBase> m_database;
};

} // namespace Core
