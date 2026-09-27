// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <chrono>
#include <deque>
#include <functional>
#include <map>
#include <optional>
#include <span>
#include <vector>

#include "core/net/host_net.h"

namespace Core::Net::P2P {

/// Addresses used in the checksum pseudo-header
struct PseudoHeader {
    int family = AF_INET;       // AF_INET or AF_INET6
    std::array<u8, 16> local{}; // IPv4 uses the first 4 bytes,network byte order
    std::array<u8, 16> remote{};
};

} // namespace Core::Net::P2P
