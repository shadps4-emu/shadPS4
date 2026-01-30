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

    // ---- Options ----
    app.add_option("-g,--game", gamePath, "Game path or ID");
    app.add_option("-p,--patch", patchFile, "Patch file to apply");
    app.add_flag("-i,--ignore-game-patch", ignoreGamePatch,
                 "Disable automatic loading of game patches");

    // FULLSCREEN: behavior-identical
    app.add_option("-f,--fullscreen", fullscreenStr, "Fullscreen mode (true|false)");

    app.add_option("--override-root", overrideRoot)->check(CLI::ExistingDirectory);

    app.add_flag("--wait-for-debugger", waitForDebugger);
    app.add_option("--wait-for-pid", waitPid);

    app.add_flag("--show-fps", showFps);
    app.add_flag("--config-clean", configClean);
    app.add_flag("--config-global", configGlobal);
    app.add_flag("--log-append", logAppend);

    app.add_option("--add-game-folder", addGameFolder)->check(CLI::ExistingDirectory);
    app.add_option("--set-addon-folder", setAddonFolder)->check(CLI::ExistingDirectory);

    // ---- Capture args after `--` verbatim ----
    app.allow_extras();
    app.parse_complete_callback([&]() {
        const auto& extras = app.remaining();
        if (!extras.empty()) {
            gameArgs = extras;
        }
    });

    // ---- No-args behavior ----
    if (argc == 1) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "shadPS4",
                                 "This is a CLI application. Please use the QTLauncher for a GUI:\n"
                                 "https://github.com/shadps4-emu/shadps4-qtlauncher/releases",
                                 nullptr);
        std::cout << app.help();
        return -1;
    }

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError& e) {
        return app.exit(e);
    }

    // ---- Utility commands ----
    if (addGameFolder) {
        Config::addGameInstallDir(*addGameFolder);
        Config::save(user_dir / "config.toml");
        std::cout << "Game folder successfully saved.\n";
        return 0;
    }

    if (setAddonFolder) {
        Config::setAddonInstallDir(*setAddonFolder);
        Config::save(user_dir / "config.toml");
        std::cout << "Addon folder successfully saved.\n";
        return 0;
    }

    if (!gamePath.has_value()) {
        if (!gameArgs.empty()) {
            gamePath = gameArgs.front();
            gameArgs.erase(gameArgs.begin());
        } else {
            std::cerr << "Error: Please provide a game path or ID.\n";
            return 1;
        }
    }

    // ---- Apply flags ----
    if (patchFile)
        MemoryPatcher::patch_file = *patchFile;

    if (ignoreGamePatch)
        Core::FileSys::MntPoints::ignore_game_patches = true;

    if (fullscreenStr) {
        if (*fullscreenStr == "true") {
            Config::setIsFullscreen(true);
        } else if (*fullscreenStr == "false") {
            Config::setIsFullscreen(false);
        } else {
            std::cerr << "Error: Invalid argument for --fullscreen (use true|false)\n";
            return 1;
        }
    }

    if (showFps)
        Config::setShowFpsCounter(true);

    if (configClean)
        Config::setConfigMode(Config::ConfigMode::Clean);

    if (configGlobal)
        Config::setConfigMode(Config::ConfigMode::Global);

    if (logAppend)
        Common::Log::SetAppend();

    // ---- Resolve game path or ID ----
    std::filesystem::path ebootPath(*gamePath);
    if (!std::filesystem::exists(ebootPath)) {
        bool found = false;
        constexpr int maxDepth = 5;
        for (const auto& installDir : Config::getGameInstallDirs()) {
            if (auto foundPath = Common::FS::FindGameByID(installDir, *gamePath, maxDepth)) {
                ebootPath = *foundPath;
                found = true;
                break;
            }
        }
        if (!found) {
            std::cerr << "Error: Game ID or file path not found: " << *gamePath << "\n";
            return 1;
        }
    }

    if (waitPid)
        Core::Debugger::WaitForPid(*waitPid);

    auto* emulator = Common::Singleton<Core::Emulator>::Instance();
    emulator->executableName = argv[0];
    emulator->waitForDebuggerBeforeRun = waitForDebugger;
    emulator->Run(ebootPath, gameArgs, overrideRoot);

    return 0;
}
