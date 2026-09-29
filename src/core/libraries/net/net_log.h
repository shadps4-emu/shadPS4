// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

// Lib.Net log levels:
//   Info     init, P2P start/stop, pools, resolver lookups.
//   Debug    one line per socket, epoll or option call.
//   Trace    every send, recv and epoll wait. Compiled out in release builds, which log a
//            per-socket traffic summary at Debug on close instead.
//   Warning  errors that point at a game bug or missing emulation (EINVAL, EFAULT,
//            EOPNOTSUPP...) and unsupported requests.
// Normal failures (EWOULDBLOCK, EINPROGRESS, refused or reset connections) go to Trace/Debug.
// The same function failing with the same error is logged 16 times, then once every 1000.

#pragma once

#include <source_location>
#include <string>
#include <string_view>

#include "common/types.h"
#include "core/libraries/net/net_types.h"

namespace Libraries::Net {

// e.g. 35 -> "EWOULDBLOCK", "E<n>" if unknown.
std::string ErrnoName(int orbis_errno);
std::string ErrorCodeName(s32 code);

// Picks the level from the errno and applies the rate limit.
void LogFailure(int orbis_errno, const std::source_location& where);

// "1.2.3.4:3658", with " vport N" appended for P2P addresses.
std::string FormatSockaddr(const OrbisNetSockaddr* addr, u32 len);
std::string_view SocketTypeName(s32 type);
std::string OptionName(s32 level, s32 name);
std::string EpollEventsName(u32 events);

void CountTraffic(OrbisNetId s, bool sent, s64 bytes);
// Logs the traffic summary and drops it.
void LogSocketClosed(OrbisNetId s);

} // namespace Libraries::Net
