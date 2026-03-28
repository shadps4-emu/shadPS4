// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <vector>
#include <CLI/CLI.hpp>
#include <SDL3/SDL_messagebox.h>

#include "common/config.h"
#include "common/key_manager.h"
#include "common/logging/log.h"
#include "common/memory_patcher.h"
#include "common/path_util.h"
#include "core/debugger.h"
#include "core/emulator_settings.h"
#include "core/emulator_state.h"
#include "core/file_sys/fs.h"
#include "core/ipc/ipc.h"
//#include "core/libraries/libs.h"
#include "core/user_settings.h"
#include "emulator.h"

namespace Libraries {

struct HleLayer;

}

class ShadPs4App {
public:
    static std::shared_ptr<ShadPs4App> instance;

    static auto GetInstance() {
        if (instance == nullptr) {
            throw std::runtime_error("ShadPs4App instance has been destroyed");
        }
        return instance;
    }

    std::optional<int> parse(int argc, char* argv[]) {
        CLI::App app{"shadPS4 Emulator CLI"};

        // ---- CLI state ----
        std::optional<int> waitPid;

        std::optional<std::string> fullscreenStr;
        bool ignoreGamePatch = false;
        bool showFps = false;
        bool configClean = false;
        bool configGlobal = false;
        std::optional<bool> logAppend;
        // Common::Log::g_should_append = EmulatorSettings.IsLogAppend();

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
            SDL_ShowSimpleMessageBox(
                SDL_MESSAGEBOX_INFORMATION, "shadPS4",
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

        executableName = argv[0];

        if (waitPid)
            Core::Debugger::WaitForPid(*waitPid);

        // ---- Apply flags ----
        {
            if (patchFile)
                MemoryPatcher::patch_file = *patchFile;

            if (ignoreGamePatch)
                Core::FileSys::MntPoints::ignore_game_patches = true;
        }

        // ---- update settings from CLI ----
        {
            if (fullscreenStr) {
                if (*fullscreenStr == "true") {
                    m_emulator_settings.SetFullScreen(true);
                } else if (*fullscreenStr == "false") {
                    m_emulator_settings.SetFullScreen(false);
                } else {
                    std::cerr << "Error: Invalid argument for --fullscreen (use true|false)\n";
                    return 1;
                }
            }

            if (showFps)
                m_emulator_settings.SetShowFpsCounter(true);

            if (configClean)
                m_emulator_settings.SetConfigMode(ConfigMode::Clean);

            if (configGlobal)
                m_emulator_settings.SetConfigMode(ConfigMode::Global);
        }

        // ---- Utility commands ----
        {
            if (addGameFolder) {
                m_emulator_settings.AddGameInstallDir(*addGameFolder);
                m_emulator_settings.Save();
                std::cout << "Game folder successfully saved.\n";
                return 0;
            }

            if (setAddonFolder) {
                m_emulator_settings.SetAddonInstallDir(*setAddonFolder);
                m_emulator_settings.Save();
                std::cout << "Addon folder successfully saved.\n";
                return 0;
            }
        }

        // ---- fix arguments ----
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

        return {};
    }

    void init() {
        m_ipc.Init(m_emulator_state);

        // Load configurations
        m_user_settings.Load();

        Config::load(Common::FS::GetUserPath(Common::FS::PathType::UserDir) / "config.toml");

        // ---- Trophy key migration ----
        m_key_manager.LoadFromFile();
        if (m_key_manager.GetAllKeys().TrophyKeySet.ReleaseTrophyKey.empty() &&
            !Config::getTrophyKey().empty()) {
            auto keys = m_key_manager.GetAllKeys();
            if (keys.TrophyKeySet.ReleaseTrophyKey.empty() && !Config::getTrophyKey().empty()) {
                keys.TrophyKeySet.ReleaseTrophyKey =
                    KeyManager::HexStringToBytes(Config::getTrophyKey());
                m_key_manager.SetAllKeys(keys);
                m_key_manager.SaveToFile();
            }
        }

        m_emulator_settings.Load();
    }

    int run() {
        // ---- Resolve game path or ID ----
        std::filesystem::path ebootPath(*gamePath);

        if (!std::filesystem::exists(ebootPath)) {
            bool found = false;
            constexpr int maxDepth = 5;
            for (const auto& installDir : m_emulator_settings.GetGameInstallDirs()) {
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
        emulator->executableName = executableName;
        emulator->waitForDebuggerBeforeRun = waitForDebugger;
        emulator->Run(ebootPath, gameArgs, overrideRoot);

        return 0;
    }

    const char* executableName;
    bool waitForDebugger = false;

    std::optional<std::string> gamePath;
    std::vector<std::string> gameArgs;
    std::optional<std::filesystem::path> overrideRoot;

    EmulatorState m_emulator_state;
    IPC m_ipc;

    UserSettingsImpl m_user_settings;
    KeyManager m_key_manager;
    EmulatorSettingsImpl m_emulator_settings;

    std::unique_ptr<Libraries::HleLayer> m_hle_layer;
};
