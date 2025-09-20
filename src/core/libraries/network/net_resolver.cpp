// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "common/singleton.h"
#include "common/types.h"
#include "core/libraries/error_codes.h"
#include "net_error.h"
#include "net_resolver.h"
#include "net_util.h"

namespace Libraries::Net {

int Resolver::ResolveAsync(const char* hostname, OrbisNetInAddr* addr, int timeout, int retry,
                           int flags) {
    std::scoped_lock lock{m_mutex};

    if (async_resolution) {
        *sceNetErrnoLoc() = ORBIS_NET_RESOLVER_EBUSY;
        return ORBIS_NET_ERROR_RESOLVER_EBUSY;
    }

    async_resolution = AsyncResolution{hostname, addr, timeout, retry, flags};

    return ORBIS_OK;
}

void Resolver::Resolve() {
    if (async_resolution) {
        auto* netinfo = Common::Singleton<NetUtil::NetUtilInternal>::Instance();
        auto ret = netinfo->ResolveHostname(async_resolution->hostname, async_resolution->addr);

        resolution_error = ret;
    } else {
        LOG_ERROR(Lib_Net, "async resolution has not been set-up");
    }
}

} // namespace Libraries::Net
