// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>
#include <ctime>
#include "src/common/discord_rpc_handler.h"

namespace DiscordRPCHandler {

void RPC::init() {
    DiscordEventHandlers handlers{};

    Discord_Initialize("1139939140494971051", &handlers, 1, nullptr);
    startTimestamp = time(nullptr);
    rpcEnabled = true;
}

void RPC::setStatusIdling() {
    DiscordRichPresence rpc{};
    rpc.largeImageKey = "https://github.com/shadps4-emu/shadPS4/raw/main/.github/shadps4.png";
    rpc.largeImageText = "shadPS4 is a PS4 emulator";
    rpc.startTimestamp = startTimestamp;
    rpc.details = "Idle";

    status = RPCStatus::Idling;
    Discord_UpdatePresence(&rpc);
}

void RPC::setStatusPlaying(const std::string& game_name, const std::string& game_id) {
    DiscordRichPresence rpc{};

    rpc.details = "Playing";
    rpc.state = game_name.c_str();
    std::string largeImageUrl =
        "https://store.playstation.com/store/api/chihiro/00_09_000/titlecontainer/US/en/999/" +
        game_id + "_00/image";
    rpc.largeImageKey = largeImageUrl.c_str();
    rpc.largeImageText = game_name.c_str();
    rpc.startTimestamp = startTimestamp;

    status = RPCStatus::Playing;
    Discord_UpdatePresence(&rpc);
}

void RPC::shutdown() {
    if (rpcEnabled) {
        rpcEnabled = false;
        Discord_ClearPresence();
        Discord_Shutdown();
    }
}

bool RPC::getRPCEnabled() {
    return rpcEnabled;
}

} // namespace DiscordRPCHandler
