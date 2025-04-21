// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "net.h"

namespace Libraries::Net {

int sys_connect(OrbisNetId s, const OrbisNetSockaddr* addr, u32 addrlen);
int sys_bind(OrbisNetId s, const OrbisNetSockaddr* addr, u32 addrlen);
int sys_accept(OrbisNetId s, const OrbisNetSockaddr* addr, u32* paddrlen);
int sys_getpeername(OrbisNetId s, const OrbisNetSockaddr* addr, u32* paddrlen);
int sys_getsockname(OrbisNetId s, const OrbisNetSockaddr* addr, u32* paddrlen);

} // namespace Libraries::Net