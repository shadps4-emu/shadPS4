// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <SDL3/SDL.h>
#include <Zydis/Zydis.h>
#include <fmt/core.h>

#include <cinttypes>
#include <cstdio>
#include <thread>

#include <core/hle/libraries/libkernel/thread_management.h>
#include "common/config.h"
#include "common/discord.h"
#include "common/logging/backend.h"
#include "common/path_util.h"
#include "common/singleton.h"
#include "common/types.h"
#include "core/PS4/HLE/Graphics/video_out.h"
#include "core/file_sys/fs.h"
#include "core/hle/libraries/libs.h"
#include "core/linker.h"
#include "core/tls.h"
#include "emulator.h"

int main(int argc, char* argv[]) {
    if (argc == 1) {
        fmt::print("Usage: {} <elf or eboot.bin path>\n", argv[0]);
        return -1;
    }
    const auto config_dir = Common::FS::GetUserPath(Common::FS::PathType::UserDir);
    Config::load(config_dir / "config.toml");
    Common::Log::Initialize();
    Common::Log::Start();
    Core::Libraries::LibKernel::init_pthreads();
    auto width = Config::getScreenWidth();
    auto height = Config::getScreenHeight();
    Emu::emuInit(width, height);
    HLE::Libs::Graphics::VideoOut::videoOutInit(width, height);

    // Argument 1 is the path of self file to boot
    const char* const path = argv[1];

    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();
    std::filesystem::path p = std::string(path);
    mnt->Mount(p.parent_path(), "/app0");

    auto linker = Common::Singleton<Core::Linker>::Instance();
    OldLibraries::InitHLELibs(&linker->getHLESymbols());
    Core::InstallTlsHandler();
    linker->LoadModule(path);
    std::jthread mainthread([linker](std::stop_token stop_token, void*) { linker->Execute(); },
                            nullptr);
    Discord::RPC discordRPC;
    discordRPC.init();
    discordRPC.update(Discord::RPCStatus::Idling, "");
    Emu::emuRun();

    discordRPC.stop();
    return 0;
}
