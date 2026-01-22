// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <CLI/CLI.hpp>
#include <SDL3/SDL_messagebox.h>
#include "functional"
#include "iostream"
#include "string"
#include "system_error"
#include "unordered_map"

#include <core/emulator_state.h>
#include <fmt/core.h>
#include "common/config.h"
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
#include <common/key_manager.h>

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    IPC::Instance().Init();

    // Init emulator state
    auto emu_state = std::make_shared<EmulatorState>();
    EmulatorState::SetInstance(emu_state);

    // Load configuration
    const auto user_dir = Common::FS::GetUserPath(Common::FS::PathType::UserDir);
    Config::load(user_dir / "config.toml");

    // Migrate trophy key if needed
    auto key_manager = KeyManager::GetInstance();
    if (key_manager->GetAllKeys().TrophyKeySet.ReleaseTrophyKey.empty()) {
        if (!Config::getTrophyKey().empty()) {
            key_manager->SetAllKeys(
                {.TrophyKeySet = {.ReleaseTrophyKey =
                                      KeyManager::HexStringToBytes(Config::getTrophyKey())}});
            key_manager->SaveToFile();
        }
    }

    CLI::App app{"shadPS4 Emulator CLI"};

    // ---- CLI variables ----
    std::optional<std::string> gamePath;
    std::vector<std::string> gameArgs;

    std::optional<std::filesystem::path> overrideRoot;
    std::optional<int> waitPid;

    bool waitForDebugger = false;
    bool ignoreGamePatch = false;
    bool fullscreen = false;
    bool showFps = false;
    bool configClean = false;
    bool configGlobal = false;
    bool logAppend = false;

    std::optional<std::filesystem::path> addGameFolder;
    std::optional<std::filesystem::path> setAddonFolder;
    std::optional<std::string> patchFile;

    // ---- Options ----
    // Optional alias for explicit -g/--game
    app.add_option("-g,--game", gamePath, "Game path or ID");

    app.add_option("-p,--patch", patchFile, "Patch file to apply");

    app.add_flag("-i,--ignore-game-patch", ignoreGamePatch,
                 "Disable automatic loading of game patches");

    app.add_flag("-f,--fullscreen", fullscreen, "Start in fullscreen mode");

    app.add_option("--override-root", overrideRoot, "Override game root folder")
        ->check(CLI::ExistingDirectory);

    app.add_flag("--wait-for-debugger", waitForDebugger, "Wait for debugger to attach");

    app.add_option("--wait-for-pid", waitPid, "Wait for process with specified PID");

    app.add_flag("--show-fps", showFps, "Show FPS counter at startup");

    app.add_flag("--config-clean", configClean, "Ignore config files and use defaults");

    app.add_flag("--config-global", configGlobal, "Use base config only");

    app.add_flag("--log-append", logAppend, "Append log output instead of overwriting");

    app.add_option("--add-game-folder", addGameFolder, "Add a new game folder to the config")
        ->check(CLI::ExistingDirectory);

    app.add_option("--set-addon-folder", setAddonFolder, "Set addon folder in the config")
        ->check(CLI::ExistingDirectory);

    // ---- Positional arguments ----
    app.add_option("game", gamePath, "Game path or ID");
    app.add_option("game_args", gameArgs, "Arguments passed to the game executable")->expected(-1);

    // ---- Show SDL message if no args provided ----
    if (argc == 1) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "shadPS4",
                                 "This is a CLI application.\n"
                                 "Please use the QTLauncher for a GUI:\n"
                                 "https://github.com/shadps4-emu/shadps4-qtlauncher/releases",
                                 nullptr);

        std::cout << app.help();
        return 0;
    }

    // ---- Parse CLI ----
    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError& e) {
        return app.exit(e);
    }

    // ---- Handle config-only commands ----
    if (addGameFolder.has_value()) {
        Config::addGameInstallDir(*addGameFolder);
        Config::save(user_dir / "config.toml");
        std::cout << "Game folder successfully saved.\n";
        return 0;
    }

    if (setAddonFolder.has_value()) {
        Config::setAddonInstallDir(*setAddonFolder);
        Config::save(user_dir / "config.toml");
        std::cout << "Addon folder successfully saved.\n";
        return 0;
    }

    // ---- Ensure game path exists ----
    if (!gamePath.has_value()) {
        std::cerr << "Error: Please provide a game path or ID.\n";
        return 1;
    }

    // ---- Apply flags ----
    if (patchFile.has_value())
        MemoryPatcher::patch_file = *patchFile;

    Core::FileSys::MntPoints::ignore_game_patches = ignoreGamePatch;

    if (fullscreen)
        Config::setIsFullscreen(true);

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
            if (auto foundPath = Common::FS::FindGameByID(installDir, *gamePath, maxDepth);
                foundPath.has_value()) {
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

    // ---- Resolve game root ----
    std::optional<std::filesystem::path> gameRoot;
    if (overrideRoot.has_value()) {
        gameRoot = overrideRoot;
    } else {
        gameRoot = ebootPath.parent_path();
    }

    // ---- Wait for PID ----
    if (waitPid.has_value())
        Core::Debugger::WaitForPid(*waitPid);

    // ---- Run emulator ----
    auto* emulator = Common::Singleton<Core::Emulator>::Instance();
    emulator->executableName = argv[0];
    emulator->waitForDebuggerBeforeRun = waitForDebugger;
    emulator->Run(ebootPath, gameArgs, gameRoot);

    return 0;
}
