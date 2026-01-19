// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

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
    std::shared_ptr<EmulatorState> m_emu_state = std::make_shared<EmulatorState>();
    EmulatorState::SetInstance(m_emu_state);
    // Load configurations
    const auto user_dir = Common::FS::GetUserPath(Common::FS::PathType::UserDir);
    Config::load(user_dir / "config.toml");
    // temp copy the trophy key from old config to key manager if exists
    auto key_manager = KeyManager::GetInstance();
    if (key_manager->GetAllKeys().TrophyKeySet.ReleaseTrophyKey.empty()) {
        if (!Config::getTrophyKey().empty()) {

            key_manager->SetAllKeys(
                {.TrophyKeySet = {.ReleaseTrophyKey =
                                      KeyManager::HexStringToBytes(Config::getTrophyKey())}});
            key_manager->SaveToFile();
        }
    }
    bool has_game_argument = false;
    std::string game_path;
    std::vector<std::string> game_args{};
    std::optional<std::filesystem::path> game_folder;

    bool waitForDebugger = false;
    const char* const eboot_path = "C:/ps4/games/CUSA11183/eboot.bin";

    // Run the emulator with the resolved eboot path
    Core::Emulator* emulator = Common::Singleton<Core::Emulator>::Instance();
    emulator->executableName = argv[0];
    emulator->waitForDebuggerBeforeRun = waitForDebugger;
    emulator->Run(eboot_path, game_args, game_folder);
}