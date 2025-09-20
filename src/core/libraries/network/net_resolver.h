// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/network/net.h"

#include <mutex>
#include <vector>

namespace Libraries::Net {

struct Resolver {
public:
    Resolver(const char* name, int poolid, int flags) : name(name), poolid(poolid), flags(flags) {}

    int ResolveAsync(const char* hostname, OrbisNetInAddr* addr, int timeout, int retry, int flags);
    void Resolve();

private:
    struct AsyncResolution {
        const char* hostname;
        OrbisNetInAddr* addr;
        int timeout;
        int retry;
        int flags;
    };

    std::string name;
    int poolid;
    int flags;
    std::optional<AsyncResolution> async_resolution{};
    int resolution_error = ORBIS_OK;
    std::mutex m_mutex;
};

} // namespace Libraries::Net