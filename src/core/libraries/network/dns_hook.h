// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include <queue>
#include "common/types.h"

namespace Libraries::Net {

class DnsHook {
public:
    static DnsHook& Instance();

    // Mark/unmark a native socket as one carrying DNS traffic (dest port 53).
    void AddSpy(u64 sock);
    void RemoveSpy(u64 sock);

    bool IsSpy(u64 sock);
    bool HasQueued(u64 sock);

    // Pop a forged DNS response previously queued for this socket.
    std::vector<u8> PopPacket(u64 sock);

    // Inspect an outgoing DNS query. Returns -1 when not intercepted.
    s32 AnalyzeQuery(u64 sock, const u8* buf, u32 len);

    // Network-order IPv4
    u32 GetDnsServerAddr() const {
        return dns_server_addr;
    }

private:
    DnsHook();

    void LoadSwapList();
    // Returns network-order IPv4 for a hostname if it matches a redirect entry.
    std::optional<u32> GetRedir(const std::string& hostname);

    std::mutex mutex;
    std::unordered_map<u64, std::queue<std::vector<u8>>> spylist;
    std::vector<std::pair<std::string, u32>> redirs; // {pattern, network-order ip}
    u32 dns_server_addr = 0;                         // 127.0.0.1 by default
    bool loaded = false;
};

} // namespace Libraries::Net
