#include <SDL3/SDL.h>
#include <cstdio>
#include <fmt/core.h>
#include "types.h"
#include "Util/log.h"
#include <Core/PS4/HLE/Graphics/video_out.h>
#include <Util/config.h>
#include <Zydis/Zydis.h>
#include <emulator.h>
#include <cinttypes>
#include <thread>
#include "Core/PS4/HLE/Libs.h"
#include "Core/PS4/Linker.h"
#include "Emulator/Util\singleton.h"
#include "discord.h"
#include "emuTimer.h"

// Main code
int main(int argc, char* argv[]) {
    if (argc == 1) {
        fmt::print("Usage: {} <elf or eboot.bin path>\n", argv[0]);
        return -1;
    }
    Config::load("config.toml");
    logging::init(true);  // init logging
    auto width = Config::getScreenWidth();
    auto height = Config::getScreenHeight();
    Emu::emuInit(width, height);
    HLE::Libs::Graphics::VideoOut::videoOutInit(width, height);
    Emulator::emuTimer::start();

    const char* const path = argv[1];  // argument 1 is the path of self file to boot

    auto linker = singleton<Linker>::instance();
    HLE::Libs::Init_HLE_Libs(&linker->getHLESymbols());
    linker->LoadModule(path);  // Load main executable
    std::jthread mainthread(
        [](std::stop_token stop_token, void*) {
            auto* linker = singleton<Linker>::instance();
            linker->Execute();
        },
        nullptr);
    Discord::RPC discordRPC;
    discordRPC.init();
    discordRPC.update(Discord::RPCStatus::Idling, "");
    Emu::emuRun();

    discordRPC.stop();
    return 0;
}
