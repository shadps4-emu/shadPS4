// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "common/key_manager.h"
#include "core/emulator_settings.h"
#include "core/emulator_state.h"
#include "core/ipc/ipc.h"
#include "core/user_settings.h"
#include "emulator.h"
#include "core/devtools//layer.h"
#include "common/discord_rpc_handler.h"
#include "core/debug_state.h"

class ShadPs4App {
public:
    static std::shared_ptr<ShadPs4App> g_instance;

    static auto GetInstance() {
        if (auto instance = g_instance) {
            return instance;
        }
        throw std::runtime_error("ShadPs4App instance has been destroyed");
    }

    std::optional<int> parse(int argc, char* argv[]);

    void init();

    int run();

    const char* executableName;
    bool waitForDebugger = false;
    bool bigPicture = false;

    std::optional<std::string> gamePath;
    std::vector<std::string> gameArgs;
    std::optional<std::filesystem::path> overrideRoot;

    EmulatorState m_emulator_state;
    IPC m_ipc;

    UserSettingsImpl m_user_settings;
    KeyManager m_key_manager;
    EmulatorSettingsImpl m_emulator_settings;

    Core::Emulator m_emulator;
    Core::Devtools::Layer m_devtools_layer;
    DebugStateType::DebugStateImpl DebugState;
    DiscordRPCHandler::RPC m_discord;
};
