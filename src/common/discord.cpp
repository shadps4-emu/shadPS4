// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>
#include <ctime>
#include "common/discord.h"

namespace Discord {

void RPC::init() {
    DiscordEventHandlers handlers{};
    Discord_Initialize("1139939140494971051", &handlers, 1, nullptr);

    startTimestamp = time(nullptr);
    enabled = true;
}

void RPC::update(Discord::RPCStatus status, const std::string& game) {
    DiscordRichPresence rpc{};

    if (status == Discord::RPCStatus::Playing) {
        rpc.details = "Playing a game";
        rpc.state = game.c_str();
    } else {
        rpc.details = "Idle";
    }

    rpc.largeImageKey = "shadps4";
    rpc.largeImageText = "ShadPS4 is a PS4 emulator";
    rpc.startTimestamp = startTimestamp;

    Discord_UpdatePresence(&rpc);
}

void RPC::stop() {
    if (enabled) {
        enabled = false;
        Discord_ClearPresence();
        Discord_Shutdown();
    }
}

} // namespace Discord
