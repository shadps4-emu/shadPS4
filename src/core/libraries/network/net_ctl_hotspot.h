// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <optional>
#include <string>

namespace Libraries::NetCtl {

struct HostHotspot {
    std::string ssid;
    std::string passphrase;
    // The host's own address on the hotspot network, empty while the hotspot is off.
    std::string ip_address;
};

// The host's own Wi-Fi hotspot, which stands in for the access point a PS4 would create.
std::optional<HostHotspot> QueryHostHotspot();

} // namespace Libraries::NetCtl
