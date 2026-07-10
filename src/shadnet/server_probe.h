// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include "common/types.h"

namespace ShadNet {

// Default timeout for the startup reachability probe.
static constexpr u32 SHAD_PROBE_TIMEOUT_MS = 3000;

enum class ProbeResult {
    Ok,              // reachable and protocol version matches SHAD_PROTOCOL_VERSION
    Unreachable,     // DNS failure, connect failure, or timeout
    VersionMismatch, // reachable, but ServerInfo protocol version differs from ours
    ProtocolError,   // reachable, but the ServerInfo handshake was malformed/absent
};

struct ProbeInfo {
    ProbeResult result = ProbeResult::Unreachable;
    // Protocol version reported by the server's ServerInfo packet.
    // 0 when unknown (unreachable / malformed / server sent no version field).
    u32 server_version = 0;
};

ProbeInfo ProbeServer(const std::string& host, u16 port, u32 timeout_ms = SHAD_PROBE_TIMEOUT_MS);

} // namespace ShadNet
