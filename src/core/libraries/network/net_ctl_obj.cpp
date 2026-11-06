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
    callbacks[next_id].last_event = -1;
    return next_id;
}

s32 NetCtlInternal::UnregisterCallback(int cid) {
    std::scoped_lock lock{m_mutex};
    if (cid < 0 || cid >= static_cast<s32>(callbacks.size())) {
        return ORBIS_NET_CTL_ERROR_INVALID_ID;
    }
    if (callbacks[cid].func == nullptr) {
        return ORBIS_NET_CTL_ERROR_ID_NOT_FOUND;
    }
    callbacks[cid].func = nullptr;
    callbacks[cid].arg = nullptr;
    callbacks[cid].last_event = -1;
    return ORBIS_OK;
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
    nptool_callbacks[next_id].last_event = -1;
    return next_id;
}

s32 NetCtlInternal::UnregisterNpToolkitCallback(int cid) {
    std::scoped_lock lock{m_mutex};
    if (cid < 0 || cid >= static_cast<s32>(nptool_callbacks.size())) {
        return ORBIS_NET_CTL_ERROR_INVALID_ID;
    }
    if (nptool_callbacks[cid].func == nullptr) {
        return ORBIS_NET_CTL_ERROR_ID_NOT_FOUND;
    }
    nptool_callbacks[cid].func = nullptr;
    nptool_callbacks[cid].arg = nullptr;
    nptool_callbacks[cid].last_event = -1;
    return ORBIS_OK;
}

void NetCtlInternal::CheckCallback() {
    std::scoped_lock lock{m_mutex};
    const auto event = EmulatorSettings.IsConnectedToNetwork()
                           ? ORBIS_NET_CTL_EVENT_TYPE_IPOBTAINED
                           : ORBIS_NET_CTL_EVENT_TYPE_DISCONNECTED;
    for (auto& entry : callbacks) {
        if (entry.func != nullptr && entry.last_event != event) {
            entry.last_event = event;
            entry.func(event, entry.arg);
        }
    }
}

void NetCtlInternal::CheckNpToolkitCallback() {
    std::scoped_lock lock{m_mutex};
    const auto event = EmulatorSettings.IsConnectedToNetwork()
                           ? ORBIS_NET_CTL_EVENT_TYPE_IPOBTAINED
                           : ORBIS_NET_CTL_EVENT_TYPE_DISCONNECTED;
    for (auto& entry : nptool_callbacks) {
        if (entry.func != nullptr && entry.last_event != event) {
            entry.last_event = event;
            entry.func(event, entry.arg);
        }
    }
}

} // namespace Libraries::NetCtl
