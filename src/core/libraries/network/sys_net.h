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
int sys_getsockopt(OrbisNetId s, int level, int optname, void* optval, u32* optlen);
int sys_listen(OrbisNetId s, int backlog);
int sys_setsockopt(OrbisNetId s, int level, int optname, const void* optval, u32 optlen);
int sys_shutdown(OrbisNetId s, int how);
int sys_socketex(const char* name, int family, int type, int protocol);
int sys_netabort(OrbisNetId s, int flags);
int sys_socketclose(OrbisNetId s);
int sys_sendto(OrbisNetId s, const void* buf, u64 len, int flags, const OrbisNetSockaddr* addr,
               u32 addrlen);
int sys_sendmsg(OrbisNetId s, const OrbisNetMsghdr* msg, int flags);

} // namespace Libraries::Net