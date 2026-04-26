// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <vector>
#include <CLI/CLI.hpp>
#include <SDL3/SDL_messagebox.h>

#include <core/emulator_settings.h>
#include <core/emulator_state.h>
#include "common/config.h"
#include "common/key_manager.h"
#include "common/logging/log.h"
#include "common/memory_patcher.h"
#include "common/path_util.h"
#include "core/debugger.h"
#include "core/file_sys/fs.h"
#include "core/ipc/ipc.h"
#include "emulator.h"
#include "imgui/big_picture.h"

#ifdef _WIN32
#include <windows.h>
#endif
#include <core/user_settings.h>

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

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
    bool bigPicture = false;

    std::optional<std::filesystem::path> addGameFolder;
    std::optional<std::filesystem::path> setAddonFolder;
    std::optional<std::string> patchFile;

    // ---- Options ----
    app.add_option("-g,--game", gamePath, "Game path or ID");
    app.add_option("-p,--patch", patchFile, "Patch file to apply");
    app.add_flag("-i,--ignore-game-patch", ignoreGamePatch,
                 "Disable automatic loading of game patches");

    app.add_flag("-b,--big-picture", bigPicture, "Start in Big Picture Mode");

    // FULLSCREEN: behavior-identical
    app.add_option("-f,--fullscreen", fullscreenStr, "Fullscreen mode (true|false)");

    app.add_option("--override-root", overrideRoot)->check(CLI::ExistingDirectory);

    app.add_flag("--wait-for-debugger", waitForDebugger);
    app.add_option("--wait-for-pid", waitPid);

    app.add_flag("--show-fps", showFps);
    app.add_flag("--config-clean", configClean);
    app.add_flag("--config-global", configGlobal);
    app.add_flag("--log-append", Common::Log::g_should_append);

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

    if (waitPid)
        Core::Debugger::WaitForPid(*waitPid);

    // Start default log
    Common::Log::Setup("shad_log.txt");

    IPC::Instance().Init();

    auto emu_state = std::make_shared<EmulatorState>();
    EmulatorState::SetInstance(emu_state);
    UserSettings.Load();

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

    // Load configurations
    std::shared_ptr<EmulatorSettingsImpl> emu_settings = std::make_shared<EmulatorSettingsImpl>();
    EmulatorSettingsImpl::SetInstance(emu_settings);
    emu_settings->Load();

    Common::Log::Shutdown();
    // Start configured log
    Common::Log::g_should_append |= EmulatorSettings.IsLogAppend();
    Common::Log::Setup("shad_log.txt");

    if (bigPicture) {
        BigPictureMode::Launch(argv[0]);
        return 0;
    }

    // ---- Utility commands ----
    if (addGameFolder) {
        EmulatorSettings.AddGameInstallDir(*addGameFolder);
        EmulatorSettings.Save();
        std::cout << "Game folder successfully saved.\n";
        return 0;
    }

    if (setAddonFolder) {
        EmulatorSettings.SetAddonInstallDir(*setAddonFolder);
        EmulatorSettings.Save();
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
    if (!gameArgs.empty()) {
        if (gameArgs.front() == "--") {
            gameArgs.erase(gameArgs.begin());
        } else {
            std::cerr << "Error: unhandled flags\n";
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
            EmulatorSettings.SetFullScreen(true);
        } else if (*fullscreenStr == "false") {
            EmulatorSettings.SetFullScreen(false);
        } else {
            std::cerr << "Error: Invalid argument for --fullscreen (use true|false)\n";
            return 1;
        }
    }

    if (showFps)
        EmulatorSettings.SetShowFpsCounter(true);

    if (configClean)
        EmulatorSettings.SetConfigMode(ConfigMode::Clean);

    if (configGlobal)
        EmulatorSettings.SetConfigMode(ConfigMode::Global);

    // ---- Resolve game path or ID ----
    std::filesystem::path ebootPath(*gamePath);
    if (!std::filesystem::exists(ebootPath)) {
        bool found = false;
        constexpr int maxDepth = 5;
        for (const auto& installDir : EmulatorSettings.GetGameInstallDirs()) {
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

    auto* emulator = Common::Singleton<Core::Emulator>::Instance();
    emulator->executableName = argv[0];
    emulator->waitForDebuggerBeforeRun = waitForDebugger;
    emulator->Run(ebootPath, gameArgs, overrideRoot);

    return 0;
}
