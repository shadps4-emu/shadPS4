// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
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

// Intercepts game DNS traffic and answers it locally, so hostnames resolve to an
// override IP without ever reaching a real DNS server.
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

    // Inspect an outgoing DNS query
    s32 AnalyzeQuery(u64 sock, const u8* buf, u32 len);

    // Look up an override IP (network-order u32) for a hostname using the same
    // swap table as raw DNS interception.
    std::optional<u32> Lookup(const std::string& hostname);

    // Network-order IPv4 (u32) that forged answers appear to come from.
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
