// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <condition_variable>
#include <mutex>

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

class NetCtlInternal {
public:
    NetCtlInternal();
    ~NetCtlInternal();
    s32 registerCallback(OrbisNetCtlCallback func, void* arg);
    s32 registerNpToolkitCallback(OrbisNetCtlCallbackForNpToolkit func, void* arg);
    void checkCallback();
    void checkNpToolkitCallback();

public:
    std::array<NetCtlCallback, 8> nptoolCallbacks;
    std::array<NetCtlCallbackForNpToolkit, 8> callbacks;
    std::mutex m_mutex;
};
} // namespace Libraries::NetCtl
