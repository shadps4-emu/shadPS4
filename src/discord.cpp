#include "discord.h"

#include <cstring>
#include <ctime>

void Discord::RPC::init() {
	DiscordEventHandlers handlers{};
	Discord_Initialize("1138176975865909360", &handlers, 1, nullptr);

	startTimestamp = time(nullptr);
	enabled = true;
}

void Discord::RPC::update(Discord::RPCStatus status, const std::string& game) {
	DiscordRichPresence rpc{};

	if (status == Discord::RPCStatus::Playing) {
		rpc.details = "Playing a game";
		rpc.state = game.c_str();
	} else {
		rpc.details = "Idle";
	}

	rpc.largeImageKey = "pand";
	rpc.largeImageText = "ShadPS4 is a PS4 emulator";
	rpc.startTimestamp = startTimestamp;

	Discord_UpdatePresence(&rpc);
}

void Discord::RPC::stop() {
	if (enabled) {
		enabled = false;
		Discord_ClearPresence();
		Discord_Shutdown();
	}
}
