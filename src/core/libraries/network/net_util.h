// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <mutex>
#include "common/types.h"

namespace NetUtil {

class NetUtilInternal {
public:
    explicit NetUtilInternal() = default;
    ~NetUtilInternal() = default;

private:
    std::array<u8, 6> ether_address{};
    std::string default_gateway{};
    std::string netmask{};
    std::mutex m_mutex;

public:
    const std::array<u8, 6>& GetEthernetAddr() const;
    const std::string& GetDefaultGateway() const;
    const std::string& GetNetmask() const;
    bool RetrieveEthernetAddr();
    bool RetrieveDefaultGateway();
    bool RetrieveNetmask();
};
} // namespace NetUtil