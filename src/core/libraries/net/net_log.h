// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <source_location>
#include <string>
#include <string_view>

#include "common/types.h"
#include "core/libraries/net/net_types.h"

namespace Libraries::Net {

std::string ErrnoName(int orbis_errno);
std::string ErrorCodeName(s32 code);
void LogFailure(int orbis_errno, const std::source_location& where);
std::string FormatSockaddr(const OrbisNetSockaddr* addr, u32 len);
std::string_view SocketTypeName(s32 type);
std::string OptionName(s32 level, s32 name);
std::string EpollEventsName(u32 events);
void CountTraffic(OrbisNetId s, bool sent, s64 bytes);
void NoteP2PPeer(OrbisNetId s, bool sent, const OrbisNetSockaddr* addr, u32 len);
void LogSocketClosed(OrbisNetId s);

} // namespace Libraries::Net
