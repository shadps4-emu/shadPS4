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

    auto linker = Common::Singleton<Core::Linker>::Instance();
    Core::Libraries::InitHLELibs(&linker->getHLESymbols());
    linker->LoadModule(path);
    std::jthread mainthread([linker](std::stop_token stop_token, void*) { linker->Execute(); }, nullptr);
    Discord::RPC discordRPC;
    discordRPC.init();
    discordRPC.update(Discord::RPCStatus::Idling, "");
    Emu::emuRun();

    discordRPC.stop();
    return 0;
}
