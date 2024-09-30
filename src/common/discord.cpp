// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>
#include <ctime>
#include "common/discord.h"
#include "logging/log.h"
#include "common/debug.h"
namespace Discord {

void RPC::init() {
    DiscordEventHandlers handlers{};
    std::string discordAppId = "1290207945476280360";
    Discord_Initialize(discordAppId.c_str(), &handlers, 1, nullptr);
    LOG_INFO(Loader, "Discord RPC Initalized with App Id: {}", discordAppId);
    startTimestamp = time(nullptr);
    enabled = true;
}

void RPC::updatePlaying(const std::string& game, const std::string& game_id) {
    DiscordRichPresence rpc{};

    rpc.details = "Playing a game";
    rpc.state = game.c_str();
    std::string largeImageUrl =
        "https://store.playstation.com/store/api/chihiro/00_09_000/titlecontainer/US/en/999/" +
        game_id + "_00/image";
    rpc.largeImageKey = largeImageUrl.c_str();
    rpc.largeImageText = game.c_str();
    rpc.startTimestamp = startTimestamp;
    LOG_INFO(Loader, "Found game. Name: {}, Id: {}", game.c_str(), game_id.c_str());
   

    Discord_UpdatePresence(&rpc);
}

void RPC::updateIdle() {

    DiscordRichPresence rpc{};
    rpc.largeImageKey = "shadps4";
    rpc.largeImageText = "ShadPS4 is a PS4 emulator";
    rpc.startTimestamp = startTimestamp;
    rpc.details = "Idle";

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
