// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>
#include <ctime>
#include "src/qt_gui/discord_rpc_handler.h"

namespace DiscordRPCHandler {
	
	void RPC::init() {
		DiscordEventHandlers handlers{};
        std::string discordAppId = "1290207945476280360";
        Discord_Initialize(discordAppId.c_str(), &handlers, 1, nullptr);
        startTimestamp = time(nullptr);
        enabled = true;
	}

	void RPC::setStatusIdling() {
        DiscordRichPresence rpc{};
        rpc.largeImageKey = "https://github.com/shadps4-emu/shadPS4/raw/main/.github/shadps4.png";
        rpc.largeImageText = "ShadPS4 is a PS4 emulator";
        rpc.startTimestamp = startTimestamp;
        rpc.details = "Idle";

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

        Discord_UpdatePresence(&rpc);
    }

} // namespace DiscordRPCHandler
