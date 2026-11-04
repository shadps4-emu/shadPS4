// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>
#include <string_view>
#include <fmt/format.h>

#include "common/elf_info.h"
#include "common/logging/log.h"
#include "common/singleton.h"
#include "core/libraries/kernel/kernel.h"
#include "core/libraries/net/net_discovery.h"
#include "core/libraries/net/net_util.h"

namespace Libraries::Net {

namespace Host = Core::Net::Host;

namespace {

constexpr u16 DiscoveryPort = 987;
constexpr std::string_view SearchRequest = "SRCH ";

} // namespace

DiscoveryResponder& DiscoveryResponder::Instance() {
    static DiscoveryResponder instance;
    return instance;
}

DiscoveryResponder::~DiscoveryResponder() {
    if (m_thread.joinable()) {
        m_thread.request_stop();
        m_wake.Signal();
    }
}

void DiscoveryResponder::Start() {
    bool expected = false;
    if (!m_started.compare_exchange_strong(expected, true)) {
        return;
    }
    if (!m_wake.Valid()) {
        LOG_ERROR(Lib_Net, "DDP: could not create the wake handle, discovery disabled");
        return;
    }
    m_thread = std::jthread([this](std::stop_token stop) { Run(stop); });
}

std::string DiscoveryResponder::BuildResponse() const {
    auto* netinfo = Common::Singleton<NetUtil::NetUtilInternal>::Instance();
    netinfo->RetrieveEthernetAddr();
    const auto& mac = netinfo->GetEthernetAddr();
    const auto& elf = Common::ElfInfo::Instance();
    return fmt::format("HTTP/1.1 200 Ok\n"
                       "host-id:{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}\n"
                       "host-type:PS4\n"
                       "host-name:shadPS4\n"
                       "host-request-port:997\n"
                       "device-discovery-protocol-version:00020020\n"
                       "system-version:{:08X}\n"
                       "running-app-name:{}\n"
                       "running-app-titleid:{}\n",
                       mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], CURRENT_FIRMWARE_VERSION,
                       elf.Title(), elf.GameSerial());
}

void DiscoveryResponder::Run(std::stop_token stop) {
    Host::Error error;
    const Host::NativeSocket s = Host::CreateSocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, &error);
    if (s == Host::InvalidSocket) {
        LOG_WARNING(Lib_Net, "DDP: could not create a socket (error {})", static_cast<s32>(error));
        return;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(DiscoveryPort);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    error = Host::Bind(s, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));
    if (error != Host::Error::Ok) {
        LOG_WARNING(Lib_Net, "DDP: could not bind UDP port {} (error {}), discovery disabled",
                    DiscoveryPort, static_cast<s32>(error));
        Host::CloseSocket(s);
        return;
    }

    const std::string response = BuildResponse();
    LOG_INFO(Lib_Net, "DDP: answering device discovery on UDP port {}", DiscoveryPort);

    std::array<char, 2048> buf;
    while (!stop.stop_requested()) {
        const auto wait = Host::WaitOne(s, Host::Readable, m_wake, std::nullopt);
        if (wait == Host::WaitResult::Woken) {
            break;
        }
        if (wait == Host::WaitResult::Failed) {
            LOG_WARNING(Lib_Net, "DDP: wait failed (error {}), discovery stopped",
                        static_cast<s32>(Host::LastError()));
            break;
        }

        sockaddr_in from{};
        socklen_t from_len = sizeof(from);
        const auto r = Host::RecvFrom(s, buf.data(), buf.size(), 0,
                                      reinterpret_cast<sockaddr*>(&from), &from_len);
        if (r.error != Host::Error::Ok) {
            continue;
        }
        if (!std::string_view(buf.data(), static_cast<size_t>(r.value))
                 .starts_with(SearchRequest)) {
            continue;
        }
        Host::SendTo(s, response.data(), response.size(), 0, reinterpret_cast<sockaddr*>(&from),
                     from_len);
    }
    Host::CloseSocket(s);
}

} // namespace Libraries::Net
