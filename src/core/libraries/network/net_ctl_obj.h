// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <mutex>
#include <string>
#include <vector>
#include "common/types.h"

namespace Libraries::NetCtl {

using OrbisNetCtlCallback = PS4_SYSV_ABI void (*)(int eventType, void* arg);
using OrbisNetCtlCallbackForNpToolkit = PS4_SYSV_ABI void (*)(int eventType, void* arg);

struct NetCtlCallback {
    OrbisNetCtlCallback func;
    void* arg;
};

struct NetCtlCallbackForNpToolkit {
    OrbisNetCtlCallbackForNpToolkit func;
    void* arg;
};

struct NetCtlApCredentials {
    std::string ssid;
    std::string wpa_key;
};

class NetCtlInternal {
public:
    explicit NetCtlInternal();
    ~NetCtlInternal();

    s32 RegisterCallback(OrbisNetCtlCallback func, void* arg);
    s32 RegisterNpToolkitCallback(OrbisNetCtlCallbackForNpToolkit func, void* arg);
    void CheckCallback();
    void CheckNpToolkitCallback();

    s32 RegisterApCallback(OrbisNetCtlCallback func, void* arg);
    s32 UnregisterApCallback(s32 cid);
    void CheckApCallback();
    void ClearApEvent(int event);
    void StartAp(std::string ssid, std::string wpa_key);
    void SetApReady(std::string ip_address);
    void StopAp();
    s32 GetApState();
    NetCtlApCredentials GetApCredentials();
    std::string GetApAddress();

public:
    std::array<NetCtlCallbackForNpToolkit, 8> nptool_callbacks{};
    std::array<NetCtlCallback, 8> callbacks{};
    std::mutex m_mutex;

private:
    std::array<NetCtlCallback, 8> ap_callbacks{};
    std::vector<int> ap_events;
    bool ap_started = false;
    NetCtlApCredentials ap_credentials;
    std::string ap_ip_address;
};
} // namespace Libraries::NetCtl
