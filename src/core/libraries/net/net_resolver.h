// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

// Resolver ids share the net id space, so they count toward the 128 socket limit and can be
// closed with close(). A finished async lookup shows up in epoll as ORBIS_NET_EPOLLDESCID.
//
// Lookups use host getaddrinfo (IPv4 only). It can't be cancelled or timed out, so async
// lookups get their own thread and an abort just discards the result.
// TODO: guest timeout and retry counts are ignored.

#pragma once

#include <functional>
#include <string>

#include "common/types.h"
#include "core/libraries/net/net_types.h"

namespace Libraries::Net {

// Returns the id, or -errno (EMFILE when out of ids).
s32 CreateResolver(std::string name);
// A lookup still in flight finishes, but its result is dropped.
int DestroyResolver(s32 id);
// A running async lookup ends with EINTR. If none is running, the *_PRESERVATION flags make
// the next lookup of that kind fail with EINTR instead.
int AbortResolver(s32 id, u32 flags);

// StartNtoa fills addr, MultipleRecords fills info (up to 10 records).
struct ResolverOutput {
    OrbisNetInAddr* addr = nullptr;
    OrbisNetResolverInfo* info = nullptr;
};

// Sync returns 0 or an ORBIS_NET_ERROR_RESOLVER_* code. Async returns 0, or RESOLVER_EBUSY if
// a lookup is already running, and the result comes through ResolverGetError and epoll.
// disable_ipaddress: ORBIS_NET_RESOLVER_START_NTOA_DISABLE_IPADDRESS, numeric addresses go
// to DNS instead of resolving to themselves.
s32 ResolverStartNtoa(s32 id, const char* hostname, ResolverOutput output, bool async,
                      bool disable_ipaddress);

// len includes the terminator. ENOSPC if the name doesn't fit.
s32 ResolverStartAton(s32 id, const OrbisNetInAddr* addr, char* hostname, s32 len, bool async);

// status is the last lookup's result.
int ResolverGetError(s32 id, s32* status);

// While offline every lookup fails with ENODNS.
void SetResolverOnlineCheck(std::function<bool()> is_online);

} // namespace Libraries::Net
