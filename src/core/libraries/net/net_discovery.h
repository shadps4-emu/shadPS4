// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include <stop_token>
#include <string>
#include <thread>

#include "core/net/host_net.h"

namespace Libraries::Net {

// Answers PS4 Device Discovery Protocol searches on UDP 987 the way the console's system software
// does, so companion apps (PlayLink, second screen) can find the running title.
class DiscoveryResponder {
public:
    static DiscoveryResponder& Instance();

    void Start();

private:
    DiscoveryResponder() = default;
    ~DiscoveryResponder();
    DiscoveryResponder(const DiscoveryResponder&) = delete;
    DiscoveryResponder& operator=(const DiscoveryResponder&) = delete;

    void Run(std::stop_token stop);
    std::string BuildResponse() const;

    std::atomic<bool> m_started{false};
    Core::Net::Host::WakeHandle m_wake;
    std::jthread m_thread;
};

} // namespace Libraries::Net
