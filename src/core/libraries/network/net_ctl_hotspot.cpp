// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "common/types.h"
#include "core/libraries/network/net_ctl_hotspot.h"

#if defined(_WIN32) && __has_include(<winrt/Windows.Networking.NetworkOperators.h>)
#define HAS_WINDOWS_HOTSPOT
#include <array>
#include <thread>
#include <windows.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Networking.Connectivity.h>
#include <winrt/Windows.Networking.NetworkOperators.h>
#include <winrt/Windows.Networking.h>
#endif

namespace Libraries::NetCtl {

#ifdef HAS_WINDOWS_HOTSPOT

namespace {

// Internet Connection Sharing gives the host this address on the hotspot network.
std::wstring SharingAddress() {
    std::array<wchar_t, 64> buf{};
    DWORD size = sizeof(buf);
    if (RegGetValueW(HKEY_LOCAL_MACHINE,
                     L"SYSTEM\\CurrentControlSet\\Services\\SharedAccess\\Parameters",
                     L"ScopeAddress", RRF_RT_REG_SZ, nullptr, buf.data(), &size) == ERROR_SUCCESS) {
        return buf.data();
    }
    return L"192.168.137.1";
}

std::string LocalAddress(const std::wstring& address) {
    using namespace winrt::Windows::Networking;
    for (const auto& host_name : Connectivity::NetworkInformation::GetHostNames()) {
        if (host_name.Type() == HostNameType::Ipv4 && host_name.CanonicalName() == address) {
            return winrt::to_string(host_name.CanonicalName());
        }
    }
    return {};
}

std::optional<HostHotspot> QueryMobileHotspot() {
    using namespace winrt::Windows::Networking;
    using NetworkOperators::NetworkOperatorTetheringManager;

    const auto profile = Connectivity::NetworkInformation::GetInternetConnectionProfile();
    if (!profile) {
        LOG_WARNING(Lib_NetCtl, "No internet connection profile to share as a hotspot");
        return std::nullopt;
    }
    const auto capability =
        NetworkOperatorTetheringManager::GetTetheringCapabilityFromConnectionProfile(profile);
    if (capability != NetworkOperators::TetheringCapability::Enabled) {
        LOG_WARNING(Lib_NetCtl, "Mobile hotspot is not available (capability {})",
                    static_cast<s32>(capability));
        return std::nullopt;
    }
    const auto manager = NetworkOperatorTetheringManager::CreateFromConnectionProfile(profile);
    const auto config = manager.GetCurrentAccessPointConfiguration();
    HostHotspot hotspot{
        .ssid = winrt::to_string(config.Ssid()),
        .passphrase = winrt::to_string(config.Passphrase()),
    };
    if (manager.TetheringOperationalState() == NetworkOperators::TetheringOperationalState::On) {
        hotspot.ip_address = LocalAddress(SharingAddress());
    }
    return hotspot;
}

} // namespace

std::optional<HostHotspot> QueryHostHotspot() {
    std::optional<HostHotspot> hotspot;
    // WinRT needs a COM apartment, which guest threads must not be put into.
    std::thread([&hotspot] {
        try {
            winrt::init_apartment(winrt::apartment_type::multi_threaded);
        } catch (const winrt::hresult_error& e) {
            LOG_WARNING(Lib_NetCtl, "Could not initialize WinRT: {}",
                        winrt::to_string(e.message()));
            return;
        }
        try {
            hotspot = QueryMobileHotspot();
        } catch (const winrt::hresult_error& e) {
            LOG_WARNING(Lib_NetCtl, "Could not query the mobile hotspot: {}",
                        winrt::to_string(e.message()));
        }
        winrt::uninit_apartment();
    }).join();
    return hotspot;
}

#else

std::optional<HostHotspot> QueryHostHotspot() {
    LOG_WARNING(Lib_NetCtl, "Host hotspots are not supported on this platform");
    return std::nullopt;
}

#endif

} // namespace Libraries::NetCtl
