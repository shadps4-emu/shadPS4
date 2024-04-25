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
#include "core/PS4/HLE/Graphics/video_out.h"
#include "core/file_sys/fs.h"
#include "core/libraries/kernel/thread_management.h"
#include "core/libraries/libc/libc.h"
#include "core/libraries/libs.h"
#include "core/linker.h"
#include "core/tls.h"
#include "emulator.h"

int main(int argc, char* argv[]) {
    /* if (argc == 1) {
        fmt::print("Usage: {} <elf or eboot.bin path>\n", argv[0]);
        return -1;
    }*/
    const auto config_dir = Common::FS::GetUserPath(Common::FS::PathType::UserDir);
    Config::load(config_dir / "config.toml");
    Common::Log::Initialize();
    Common::Log::Start();
    Libraries::Kernel::init_pthreads();
    auto width = Config::getScreenWidth();
    auto height = Config::getScreenHeight();
    Emu::emuInit(width, height);
    HLE::Libs::Graphics::VideoOut::videoOutInit(width, height);

    // Argument 1 is the path of self file to boot
    // const char* const path = argv[1];

    //const char* const path = "C://ps4//games//CUSA09415-Undertale//eboot.bin";
    // const char* const path = "C://ps4//games//CUSA30991//eboot.bin"; //teenage shredder
    // const char* const path = "C://ps4//games//CUSA01997//eboot.bin"; // teenage muntant
    // const char* const path = "C://ps4//games//CUSA07010-SonicMania//eboot.bin";
    const char* const path = "C://ps4//games//CUSA05694//eboot.bin";//momodora
    // const char* const path = "C:\\ps4\\games\\CUSA02394\\eboot.bin"; //we are doomed
    // const char* const path = "C:\\ps4\\games\\CUSA05362\\eboot.bin";//ronin

    // open orbis samples
    //  const char* const path = "C://ps4//games//OpenOrbisDlg//eboot.bin";
    // ps4 sdk samples
    // const char* const path = "C:\\ps4\\ps4sdksamples\\simplet-simple-fs.elf";

    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();
    std::filesystem::path p = std::string(path);
    mnt->Mount(p.parent_path(), "/app0");

    auto linker = Common::Singleton<Core::Linker>::Instance();
    Libraries::InitHLELibs(&linker->getHLESymbols());
    Core::InstallTlsHandler();
    linker->LoadModule(path);
    // check if there is a libc.prx in sce_module folder
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
    if (!found) // load HLE libc
    {
        Core::Libraries::LibC::libcSymbolsRegister(&linker->getHLESymbols());
    }
    std::jthread mainthread([linker](std::stop_token stop_token, void*) { linker->Execute(); },
                            nullptr);
    Discord::RPC discordRPC;
    discordRPC.init();
    discordRPC.update(Discord::RPCStatus::Idling, "");
    Emu::emuRun();

    discordRPC.stop();
    return 0;
}
