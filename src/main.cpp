// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <thread>
#include <SDL3/SDL.h>
#include <Zydis/Zydis.h>
#include <fmt/core.h>

#include "common/config.h"
#include "common/discord.h"
#include "common/logging/backend.h"
#include "common/logging/log.h"
#include "common/path_util.h"
#include "common/singleton.h"
#include "core/file_sys/fs.h"
#include "core/libraries/kernel/thread_management.h"
#include "core/libraries/libc/libc.h"
#include "core/libraries/libs.h"
#include "core/libraries/videoout/video_out.h"
#include "core/linker.h"
#include "core/tls.h"
#include "input/controller.h"
#include "sdl_window.h"

Frontend::WindowSDL* g_window;

int main(int argc, char* argv[]) {
    if (argc == 1) {
        fmt::print("Usage: {} <elf or eboot.bin path>\n", argv[0]);
        return -1;
    }
    const auto config_dir = Common::FS::GetUserPath(Common::FS::PathType::UserDir);
    Config::load(config_dir / "config.toml");
    Common::Log::Initialize();
    Common::Log::Start();
    Libraries::Kernel::init_pthreads();
    s32 width = Config::getScreenWidth();
    s32 height = Config::getScreenHeight();

    auto* controller = Common::Singleton<Input::GameController>::Instance();
    Frontend::WindowSDL window{width, height, controller};
    g_window = &window;

    // Argument 1 is the path of self file to boot
    const char* const path = argv[1];

    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();
    std::filesystem::path p = std::string(path);
    mnt->Mount(p.parent_path(), "/app0");

    auto linker = Common::Singleton<Core::Linker>::Instance();
    Libraries::InitHLELibs(&linker->getHLESymbols());
    linker->LoadModule(path);

    // Check if there is a libc.prx in sce_module folder
    bool found = false;
    if (Config::isLleLibc()) {
        std::filesystem::path sce_module_folder = p.parent_path() / "sce_module";
        if (std::filesystem::is_directory(sce_module_folder)) {
            for (const auto& entry : std::filesystem::directory_iterator(sce_module_folder)) {
                if (entry.path().filename() == "libc.prx" ||
                    entry.path().filename() == "libSceFios2.prx") {
                    found = true;
                    LOG_INFO(Loader, "Loading {}", entry.path().string().c_str());
                    linker->LoadModule(entry.path().string().c_str());
                }
            }
        }
    }
    if (!found) {
        Libraries::LibC::libcSymbolsRegister(&linker->getHLESymbols());
    }
    std::thread mainthread([linker]() { linker->Execute(); });
    Discord::RPC discordRPC;
    discordRPC.init();
    discordRPC.update(Discord::RPCStatus::Idling, "");

    static constexpr std::chrono::microseconds FlipPeriod{100000};

    while (window.isOpen()) {
        window.waitEvent();
        Libraries::VideoOut::Flip(FlipPeriod);
        Libraries::VideoOut::Vblank();
    }

    discordRPC.stop();
    return 0;
}
