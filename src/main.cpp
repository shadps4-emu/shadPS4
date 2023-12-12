#include <SDL3/SDL.h>
#include <Zydis/Zydis.h>
#include <fmt/core.h>

#include <cinttypes>
#include <cstdio>
#include <thread>

#include "Util/config.h"
#include "common/discord.h"
#include "common/log.h"
#include "common/singleton.h"
#include "common/types.h"
#include "core/PS4/HLE/Graphics/video_out.h"
#include "core/hle/libraries/libs.h"
#include "core/linker.h"
#include "emuTimer.h"
#include "emulator.h"
#include <core/hle/libraries/libkernel/thread_management.h>
#include "core/file_sys/fs.h"

int main(int argc, char* argv[]) {
    if (argc == 1) {
        fmt::print("Usage: {} <elf or eboot.bin path>\n", argv[0]);
        return -1;
    }
    Config::load("config.toml");
    Common::Log::Init(true);
    Core::Libraries::LibKernel::init_pthreads();
    auto width = Config::getScreenWidth();
    auto height = Config::getScreenHeight();
    Emu::emuInit(width, height);
    HLE::Libs::Graphics::VideoOut::videoOutInit(width, height);
    Emulator::emuTimer::start();

    // Argument 1 is the path of self file to boot
    const char* const path = argv[1];

    auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();
    std::filesystem::path p = std::string(path);
    mnt->mount(p.parent_path().string(), "/app0");

    auto linker = Common::Singleton<Core::Linker>::Instance();
    Core::Libraries::InitHLELibs(&linker->getHLESymbols());
    linker->LoadModule(path);
    //check if there is a libc.prx in sce_module folder
    if (Config::isLleLibc()) {
        std::filesystem::path sce_module_folder = std::string(p.parent_path().string() + "\\sce_module");
        if (std::filesystem::exists(sce_module_folder)) {
            for (const auto& entry : std::filesystem::directory_iterator(sce_module_folder)) {
                printf("%s\n", entry.path().string().c_str());
            }
        }
    }
    std::jthread mainthread([linker](std::stop_token stop_token, void*) { linker->Execute(); }, nullptr);
    Discord::RPC discordRPC;
    discordRPC.init();
    discordRPC.update(Discord::RPCStatus::Idling, "");
    Emu::emuRun();

    discordRPC.stop();
    return 0;
}
