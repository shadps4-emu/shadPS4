#include "emulator.h"
// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <common/logging/log.h>
#include <core/file_format/psf.h>
#include <core/file_format/splash.h>
#include <core/libraries/libc/libc.h>
#include <core/libraries/videoout/video_out.h>
#include <fmt/core.h>
#include "common/config.h"
#include "common/logging/backend.h"
#include "common/path_util.h"
#include "common/singleton.h"
#include "core/file_sys/fs.h"
#include "core/libraries/kernel/thread_management.h"
#include "core/libraries/libs.h"
#include "core/linker.h"
#include "core/memory.h"
#include "emulator.h"

Frontend::WindowSDL* g_window = nullptr;

namespace Core {

static constexpr s32 WindowWidth = 1280;
static constexpr s32 WindowHeight = 720;

Emulator::Emulator() : window{WindowWidth, WindowHeight, controller} {
    g_window = &window;

    // Initialize memory system as early as possible to reserve mappings.
    [[maybe_unused]] const auto* memory = Core::Memory::Instance();

    // Read configuration file.
    const auto config_dir = Common::FS::GetUserPath(Common::FS::PathType::UserDir);
    Config::load(config_dir / "config.toml");

    // Start logger.
    Common::Log::Initialize();
    Common::Log::Start();

    // Start discord integration
    discord_rpc.init();
    discord_rpc.update(Discord::RPCStatus::Idling, "");

    // Initialize kernel and library facilities.
    Libraries::Kernel::init_pthreads();
    Libraries::InitHLELibs(&linker->GetHLESymbols());
}

Emulator::~Emulator() {
    const auto config_dir = Common::FS::GetUserPath(Common::FS::PathType::UserDir);
    Config::save(config_dir / "config.toml");
    discord_rpc.stop();
}

void Emulator::Run(const std::filesystem::path& file) {
    // Applications expect to be run from /app0 so mount the file's parent path as app0.
    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();
    mnt->Mount(file.parent_path(), "/app0");

    // Loading param.sfo file if exists
    std::filesystem::path sce_sys_folder = file.parent_path() / "sce_sys";
    if (std::filesystem::is_directory(sce_sys_folder)) {
        for (const auto& entry : std::filesystem::directory_iterator(sce_sys_folder)) {
            if (entry.path().filename() == "param.sfo") {
                auto* param_sfo = Common::Singleton<PSF>::Instance();
                param_sfo->open(sce_sys_folder.string() + "/param.sfo");
                std::string id(param_sfo->GetString("CONTENT_ID"), 7, 9);
                std::string title(param_sfo->GetString("TITLE"));
                LOG_INFO(Loader, "Game id: {} Title: {}", id, title);
                u32 fw_version = param_sfo->GetInteger("SYSTEM_VER");
                std::string app_version = param_sfo->GetString("APP_VER");
                LOG_INFO(Loader, "Fw: {:#x} App Version: {}", fw_version, app_version);
            } else if (entry.path().filename() == "pic0.png" ||
                       entry.path().filename() == "pic1.png") {
                auto* splash = Common::Singleton<Splash>::Instance();
                if (splash->IsLoaded()) {
                    continue;
                }
                if (!splash->Open(entry.path().string())) {
                    LOG_ERROR(Loader, "Game splash: unable to open file");
                }
            }
        }
    }

    // Load the module with the linker
    linker->LoadModule(file);

    // check if we have system modules to load
    LoadSystemModules(file);

    // Check if there is a libc.prx in sce_module folder
    bool found = false;
    if (Config::isLleLibc()) {
        std::filesystem::path sce_module_folder = file.parent_path() / "sce_module";
        if (std::filesystem::is_directory(sce_module_folder)) {
            for (const auto& entry : std::filesystem::directory_iterator(sce_module_folder)) {
                if (entry.path().filename() == "libc.prx" ||
                    entry.path().filename() == "libSceFios2.prx") {
                    found = true;
                    LOG_INFO(Loader, "Loading {}", entry.path().string().c_str());
                    linker->LoadModule(entry.path());
                }
            }
        }
    }
    if (!found) {
        Libraries::LibC::libcSymbolsRegister(&linker->GetHLESymbols());
    }

    // start execution
    std::jthread mainthread =
        std::jthread([this](std::stop_token stop_token) { linker->Execute(); });

    // Begin main window loop until the application exits
    static constexpr std::chrono::microseconds FlipPeriod{100000};

    while (window.isOpen()) {
        window.waitEvent();
        Libraries::VideoOut::Flip(FlipPeriod);
        Libraries::VideoOut::Vblank();
    }

    std::exit(0);
}

void Emulator::LoadSystemModules(const std::filesystem::path& file) {
    const auto& sys_module_path = Common::FS::GetUserPath(Common::FS::PathType::SysModuleDir);
    for (const auto& entry : std::filesystem::directory_iterator(sys_module_path)) {
        if (entry.path().filename() == "libSceNgs2.sprx") {
            LOG_INFO(Loader, "Loading {}", entry.path().string().c_str());
            linker->LoadModule(entry.path());
        }
    }
}

} // namespace Core