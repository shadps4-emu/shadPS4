// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <vector>
#include <CLI/CLI.hpp>
#include <SDL3/SDL_messagebox.h>

#include <core/emulator_state.h>
#include "common/config.h"
#include "common/key_manager.h"
#include "common/logging/backend.h"
#include "common/memory_patcher.h"
#include "common/path_util.h"
#include "core/debugger.h"
#include "core/file_sys/fs.h"
#include "core/ipc/ipc.h"
#include "emulator.h"

#ifdef _WIN32
#include <windows.h>
#endif

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    IPC::Instance().Init();

    auto emu_state = std::make_shared<EmulatorState>();
    EmulatorState::SetInstance(emu_state);

    const auto user_dir = Common::FS::GetUserPath(Common::FS::PathType::UserDir);
    Config::load(user_dir / "config.toml");

    // ---- Trophy key migration ----
    auto key_manager = KeyManager::GetInstance();
    key_manager->LoadFromFile();
    if (key_manager->GetAllKeys().TrophyKeySet.ReleaseTrophyKey.empty() &&
        !Config::getTrophyKey().empty()) {
        auto keys = key_manager->GetAllKeys();
        if (keys.TrophyKeySet.ReleaseTrophyKey.empty() && !Config::getTrophyKey().empty()) {
            keys.TrophyKeySet.ReleaseTrophyKey =
                KeyManager::HexStringToBytes(Config::getTrophyKey());
            key_manager->SetAllKeys(keys);
            key_manager->SaveToFile();
        }
    }

    CLI::App app{"shadPS4 Emulator CLI"};

    // ---- CLI state ----
    std::optional<std::string> gamePath;
    std::vector<std::string> gameArgs;
    std::optional<std::filesystem::path> overrideRoot;
    std::optional<int> waitPid;
    bool waitForDebugger = false;

    std::optional<std::string> fullscreenStr;
    bool ignoreGamePatch = false;
    bool showFps = false;
    bool configClean = false;
    bool configGlobal = false;
    bool logAppend = false;

    std::optional<std::filesystem::path> addGameFolder;
    std::optional<std::filesystem::path> setAddonFolder;
    std::optional<std::string> patchFile;

        // const char* const ebootPath = "M:/PS4/dumpedgames/CUSA00207/eboot.bin"; // bloodborne
    // const char* const ebootPath = "D:/ps4/shadps4games/CUSA07010/eboot.bin"; // sonic mania
    //  const char* const ebootPath = "C:/ps4tests/CUSA03318/eboot.bin";//carmageddon
    // const char* const ebootPath = "C:/ps4tests/CUSA04518/eboot.bin"; // project diva x
    // const char* const ebootPath = "D:/ps4sdk/rain.elf";
    // const char* const ebootPath = "C:/ps4tests/CUSA05344/eboot.bin";//here we lie
    // const char* const ebootPath = "C:/ps4tests/CUSA01499/eboot.bin"; // mirror's edge catalyst
    // const char* const ebootPath = "D:/ps4/shadps4games/CUSA30582/eboot.bin";//SpongeBob
    // SquarePants: The Cosmic Shake
    //const char* const ebootPath = "D:/ps4/shadps4games/CUSA36843/eboot.bin"; // red dead
    // const char* const ebootPath = "C:/ps4tests/CUSA00375/eboot.bin"; // the evil within
    // const char* const ebootPath = "C:/ps4tests/CUSA51536/eboot.bin";//formula legends
    // const char* const ebootPath = "D:/ps4/shadps4games/CUSA11616/eboot.bin"; // Reverie
    // const char* const ebootPath = "C:/ps4tests/CUSA00093/eboot.bin";//driveclub
    // const char* const ebootPath = "D:/ps4games/playable/CUSA05635/eboot.bin";//puyo tetris
    //const char* const ebootPath = "M:/PS4/dumpedgames/CUSA07410/eboot.bin";//gow
    //const char* const ebootPath = "D:/ps4/shadps4games/CUSA31498/eboot.bin";//street fighter 6
    const char* const ebootPath = "M:/PS4/dumpedgames/CUSA20099/eboot.bin";//gi joe

    auto* emulator = Common::Singleton<Core::Emulator>::Instance();
    emulator->executableName = argv[0];
    emulator->waitForDebuggerBeforeRun = waitForDebugger;
    emulator->Run(ebootPath, gameArgs, overrideRoot);

    return 0;
}
