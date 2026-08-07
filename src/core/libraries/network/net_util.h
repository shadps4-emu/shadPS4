// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <mutex>
#include "common/types.h"

namespace Libraries::Net {
struct OrbisNetInAddr;
}

namespace NetUtil {

class NetUtilInternal {
public:
    explicit NetUtilInternal() = default;
    ~NetUtilInternal() = default;

private:
    std::array<u8, 6> ether_address{};
    std::string default_gateway{};
    std::string netmask{};
    std::string ip{};
    u32 external_ip{0};
    u32 nat_type{0};
    std::mutex m_mutex;

public:
    const std::array<u8, 6>& GetEthernetAddr() const;
    const std::string& GetDefaultGateway() const;
    const std::string& GetNetmask() const;
    const std::string& GetIp() const;
    u32 GetExternalIp() const;
    void SetExternalIp(u32 addr);
    u32 GetNatType() const;
    bool RetrieveEthernetAddr();
    bool RetrieveDefaultGateway();
    bool RetrieveNetmask();
    bool RetrieveIp();
    int ResolveHostname(const char* hostname, Libraries::Net::OrbisNetInAddr* addr);
};
} // namespace NetUtil