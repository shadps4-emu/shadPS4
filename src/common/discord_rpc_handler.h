// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstdint>
#include <string>
#include <discord_rpc.h>

namespace DiscordRPCHandler {

enum class RPCStatus {
    Idling,
    Playing,
};

class RPC {
    std::uint64_t startTimestamp;
    bool rpcEnabled = false;
    RPCStatus status;

public:
    void init();
    void setStatusIdling();
    void setStatusPlaying(const std::string& game_name, const std::string& game_id);
    void shutdown();
    bool getRPCEnabled();
};

} // namespace DiscordRPCHandler
