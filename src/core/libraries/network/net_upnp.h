// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include "common/types.h"

#include <miniupnpc.h>
#include <upnpcommands.h>

namespace Libraries::Net {

class UPnPClient {
public:
    static UPnPClient& Instance();

    // Kick off async IGD discovery. Safe to call multiple times — only the first call has effect.
    void Start();

    // Block until discovery completes or timeout_ms elapses. Returns true if IGD was found.
    bool WaitReady(int timeout_ms);

    bool IsAvailable() const;

    // External IP in network byte order as reported by the IGD. 0 if not yet known.
    u32 GetExternalIp() const;

    // External IP as a dotted-decimal string. Empty if not yet known.
    std::string GetExternalIpString() const;

    // External port the IGD mapped for us. Matches the internal port unless the router remapped it.
    u16 GetExternalPort() const;

    void AddMapping(u16 port);
    void RemoveMapping(u16 port);

private:
    UPnPClient() = default;
    ~UPnPClient();
    UPnPClient(const UPnPClient&) = delete;
    UPnPClient& operator=(const UPnPClient&) = delete;

    void DiscoverThread();

    std::atomic<bool> m_started{false};
    std::atomic<bool> m_done{false};
    std::atomic<bool> m_available{false};
    std::atomic<u32> m_external_ip_nbo{0};
    std::atomic<u16> m_external_port{0};
    char m_external_ip_str[16]{};

    char m_lan_addr[64]{};
    UPNPUrls m_urls{};
    IGDdatas m_data{};

    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::thread m_thread;
};

} // namespace Libraries::Net
