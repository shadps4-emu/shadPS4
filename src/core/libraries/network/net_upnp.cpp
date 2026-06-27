// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <chrono>
#include <cstring>
#include <upnperrors.h>
#include "common/logging/log.h"
#include "net_upnp.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <net/if.h>
#endif

namespace Libraries::Net {

UPnPClient& UPnPClient::Instance() {
    static UPnPClient instance;
    return instance;
}

UPnPClient::~UPnPClient() {
    if (m_thread.joinable())
        m_thread.join();
    if (m_available.load())
        FreeUPNPUrls(&m_urls);
}

void UPnPClient::Start() {
    bool expected = false;
    if (!m_started.compare_exchange_strong(expected, true))
        return;
    LOG_INFO(Lib_Net, "UPNP: starting discovery thread");
    m_thread = std::thread(&UPnPClient::DiscoverThread, this);
}

bool UPnPClient::WaitReady(int timeout_ms) {
    std::unique_lock lock(m_mutex);
    m_cv.wait_for(lock, std::chrono::milliseconds(timeout_ms), [this] { return m_done.load(); });
    return m_available.load();
}

bool UPnPClient::IsAvailable() const {
    return m_available.load();
}

u32 UPnPClient::GetExternalIp() const {
    return m_external_ip_nbo.load();
}

std::string UPnPClient::GetExternalIpString() const {
    return m_available.load() ? std::string(m_external_ip_str) : std::string{};
}

u16 UPnPClient::GetExternalPort() const {
    return m_external_port.load();
}

void UPnPClient::DiscoverThread() {
    int error = 0;
    UPNPDev* devlist = upnpDiscover(2000, nullptr, nullptr, UPNP_LOCAL_PORT_ANY, 0, 2, &error);

    if (!devlist) {
        LOG_INFO(Lib_Net, "UPNP: no IGD found (error={})", error);
        m_done = true;
        m_cv.notify_all();
        return;
    }

    char wan_addr[40]{};
    const int ret = UPNP_GetValidIGD(devlist, &m_urls, &m_data, m_lan_addr, sizeof(m_lan_addr),
                                     wan_addr, sizeof(wan_addr));
    freeUPNPDevlist(devlist);

    if (ret != 1) {
        LOG_INFO(Lib_Net, "UPNP: no valid IGD (ret={})", ret);
        m_done = true;
        m_cv.notify_all();
        return;
    }

    char external_ip[40]{};
    if (UPNP_GetExternalIPAddress(m_urls.controlURL, m_data.first.servicetype, external_ip) !=
        UPNPCOMMAND_SUCCESS) {
        LOG_WARNING(Lib_Net, "UPNP: IGD found but failed to get external IP");
        FreeUPNPUrls(&m_urls);
        m_done = true;
        m_cv.notify_all();
        return;
    }

    u32 ip_nbo = 0;
    inet_pton(AF_INET, external_ip, &ip_nbo);
    m_external_ip_nbo.store(ip_nbo);
    std::strncpy(m_external_ip_str, external_ip, sizeof(m_external_ip_str) - 1);
    m_available = true;

    LOG_INFO(Lib_Net, "UPNP: IGD found - external IP={} lan={} (port mapping available)",
             external_ip, m_lan_addr);

    m_done = true;
    m_cv.notify_all();
}

void UPnPClient::AddMapping(u16 port) {
    if (!m_available.load())
        return;

    const std::string port_str = std::to_string(port);
    const int ret =
        UPNP_AddPortMapping(m_urls.controlURL, m_data.first.servicetype, port_str.c_str(),
                            port_str.c_str(), m_lan_addr, "shadPS4", "UDP", nullptr, "0");

    if (ret == UPNPCOMMAND_SUCCESS) {
        m_external_port.store(port);
        LOG_INFO(Lib_Net, "UPNP: mapped UDP port {}", port);
    } else {
        // Store the requested port as best-guess even on failure so callers still get a value.
        m_external_port.store(port);
        LOG_WARNING(Lib_Net, "UPNP: failed to map port {} ({})", port, strupnperror(ret));
    }
}

void UPnPClient::RemoveMapping(u16 port) {
    if (!m_available.load())
        return;

    const std::string port_str = std::to_string(port);
    UPNP_DeletePortMapping(m_urls.controlURL, m_data.first.servicetype, port_str.c_str(), "UDP",
                           nullptr);

    LOG_INFO(Lib_Net, "UPNP: removed UDP port mapping {}", port);
}

} // namespace Libraries::Net
