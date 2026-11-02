// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include "common/logging/log.h"
#include "core/emulator_settings.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/network/net_ctl_codes.h"
#include "core/libraries/network/net_ctl_obj.h"
#include "core/tls.h"

namespace Libraries::NetCtl {

NetCtlInternal::NetCtlInternal() = default;

NetCtlInternal::~NetCtlInternal() = default;

s32 NetCtlInternal::RegisterCallback(OrbisNetCtlCallback func, void* arg) {
    std::scoped_lock lock{m_mutex};

    // Find the next available slot
    const auto it = std::ranges::find(callbacks, nullptr, &NetCtlCallback::func);
    if (it == callbacks.end()) {
        return ORBIS_NET_CTL_ERROR_CALLBACK_MAX;
    }

    const int next_id = std::distance(callbacks.begin(), it);
    callbacks[next_id].func = func;
    callbacks[next_id].arg = arg;
    return next_id;
}

s32 NetCtlInternal::RegisterNpToolkitCallback(OrbisNetCtlCallbackForNpToolkit func, void* arg) {
    std::scoped_lock lock{m_mutex};

    // Find the next available slot
    const auto it = std::ranges::find(nptool_callbacks, nullptr, &NetCtlCallbackForNpToolkit::func);
    if (it == nptool_callbacks.end()) {
        return ORBIS_NET_CTL_ERROR_CALLBACK_MAX;
    }

    const int next_id = std::distance(nptool_callbacks.begin(), it);
    nptool_callbacks[next_id].func = func;
    nptool_callbacks[next_id].arg = arg;
    return next_id;
}

void NetCtlInternal::CheckCallback() {
    std::scoped_lock lock{m_mutex};
    const auto event = EmulatorSettings.IsConnectedToNetwork()
                           ? ORBIS_NET_CTL_EVENT_TYPE_IPOBTAINED
                           : ORBIS_NET_CTL_EVENT_TYPE_DISCONNECTED;
    for (const auto [func, arg] : callbacks) {
        if (func != nullptr) {
            func(event, arg);
        }
    }
}

void NetCtlInternal::CheckNpToolkitCallback() {
    std::scoped_lock lock{m_mutex};
    const auto event = EmulatorSettings.IsConnectedToNetwork()
                           ? ORBIS_NET_CTL_EVENT_TYPE_IPOBTAINED
                           : ORBIS_NET_CTL_EVENT_TYPE_DISCONNECTED;
    for (const auto [func, arg] : nptool_callbacks) {
        if (func != nullptr) {
            func(event, arg);
        }
    }
}

s32 NetCtlInternal::RegisterApCallback(OrbisNetCtlCallback func, void* arg) {
    std::scoped_lock lock{m_mutex};

    const auto it = std::ranges::find(ap_callbacks, nullptr, &NetCtlCallback::func);
    if (it == ap_callbacks.end()) {
        return ORBIS_NET_CTL_ERROR_CALLBACK_MAX;
    }

    const int next_id = std::distance(ap_callbacks.begin(), it);
    ap_callbacks[next_id].func = func;
    ap_callbacks[next_id].arg = arg;
    return next_id;
}

s32 NetCtlInternal::UnregisterApCallback(s32 cid) {
    std::scoped_lock lock{m_mutex};
    if (cid < 0 || cid >= static_cast<s32>(ap_callbacks.size())) {
        return ORBIS_NET_CTL_ERROR_INVALID_ID;
    }
    if (ap_callbacks[cid].func == nullptr) {
        return ORBIS_NET_CTL_ERROR_ID_NOT_FOUND;
    }
    ap_callbacks[cid] = {};
    return ORBIS_OK;
}

void NetCtlInternal::CheckApCallback() {
    std::vector<int> events;
    std::array<NetCtlCallback, 8> targets;
    {
        std::scoped_lock lock{m_mutex};
        events.swap(ap_events);
        targets = ap_callbacks;
    }
    // Callbacks query the AP state, so they run without the lock held.
    for (const int event : events) {
        for (const auto [func, arg] : targets) {
            if (func != nullptr) {
                func(event, arg);
            }
        }
    }
}

void NetCtlInternal::ClearApEvent(int event) {
    std::scoped_lock lock{m_mutex};
    std::erase(ap_events, event);
}

void NetCtlInternal::StartAp(std::string ssid, std::string wpa_key) {
    std::scoped_lock lock{m_mutex};
    ap_started = true;
    ap_credentials = {std::move(ssid), std::move(wpa_key)};
}

void NetCtlInternal::SetApReady(std::string ip_address) {
    std::scoped_lock lock{m_mutex};
    if (ap_started) {
        ap_ip_address = std::move(ip_address);
        ap_events.push_back(ORBIS_NET_CTL_AP_EVENT_IPOBTAINED);
    }
}

void NetCtlInternal::StopAp() {
    std::scoped_lock lock{m_mutex};
    ap_started = false;
    ap_events.clear();
    ap_ip_address.clear();
}

s32 NetCtlInternal::GetApState() {
    std::scoped_lock lock{m_mutex};
    return ap_started ? ORBIS_NET_CTL_AP_STATE_STARTED : ORBIS_NET_CTL_AP_STATE_STOPPED;
}

NetCtlApCredentials NetCtlInternal::GetApCredentials() {
    std::scoped_lock lock{m_mutex};
    return ap_credentials;
}

std::string NetCtlInternal::GetApAddress() {
    std::scoped_lock lock{m_mutex};
    return ap_ip_address;
}

} // namespace Libraries::NetCtl
