// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include "common/config.h"
#include "common/logging/log.h"
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
    const auto event = Config::getIsConnectedToNetwork() ? ORBIS_NET_CTL_EVENT_TYPE_IPOBTAINED
                                                         : ORBIS_NET_CTL_EVENT_TYPE_DISCONNECTED;
    for (const auto [func, arg] : callbacks) {
        if (func != nullptr) {
            Core::ExecuteGuest(func, event, arg);
        }
    }
}

void NetCtlInternal::CheckNpToolkitCallback() {
    std::scoped_lock lock{m_mutex};
    const auto event = Config::getIsConnectedToNetwork() ? ORBIS_NET_CTL_EVENT_TYPE_IPOBTAINED
                                                         : ORBIS_NET_CTL_EVENT_TYPE_DISCONNECTED;
    for (const auto [func, arg] : nptool_callbacks) {
        if (func != nullptr) {
            Core::ExecuteGuest(func, event, arg);
        }
    }
}

} // namespace Libraries::NetCtl
