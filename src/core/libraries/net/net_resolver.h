// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <functional>
#include <string>

#include "common/types.h"
#include "core/libraries/net/net_types.h"

namespace Libraries::Net {

s32 CreateResolver(std::string name);
int DestroyResolver(s32 id);
int AbortResolver(s32 id, u32 flags);
struct ResolverOutput {
    OrbisNetInAddr* addr = nullptr;
    OrbisNetResolverInfo* info = nullptr;
};
s32 ResolverStartNtoa(s32 id, const char* hostname, ResolverOutput output, bool async,
                      bool disable_ipaddress);
s32 ResolverStartAton(s32 id, const OrbisNetInAddr* addr, char* hostname, s32 len, bool async);
int ResolverGetError(s32 id, s32* status);
void SetResolverOnlineCheck(std::function<bool()> is_online);

} // namespace Libraries::Net
