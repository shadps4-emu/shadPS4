// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstdint>
#include <string>
#include <discord_rpc.h>

namespace Discord {

enum class RPCStatus {
    Idling,
    Playing,
};

class RPC {
    std::uint64_t startTimestamp;
    bool enabled = false;

public:
    void init();
    void update(RPCStatus status, const std::string& title);
    void stop();
};

} // namespace Discord
