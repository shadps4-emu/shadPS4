// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "net.h"

namespace Libraries::Net {

int PS4_SYSV_ABI sys_connect(OrbisNetId s, const OrbisNetSockaddr* addr, u32 addrlen);
int PS4_SYSV_ABI sys_bind(OrbisNetId s, const OrbisNetSockaddr* addr, u32 addrlen);
int PS4_SYSV_ABI sys_accept(OrbisNetId s, OrbisNetSockaddr* addr, u32* paddrlen);
int PS4_SYSV_ABI sys_getpeername(OrbisNetId s, OrbisNetSockaddr* addr, u32* paddrlen);
int PS4_SYSV_ABI sys_getsockname(OrbisNetId s, OrbisNetSockaddr* addr, u32* paddrlen);
int PS4_SYSV_ABI sys_getsockopt(OrbisNetId s, int level, int optname, void* optval, u32* optlen);
int PS4_SYSV_ABI sys_listen(OrbisNetId s, int backlog);
int PS4_SYSV_ABI sys_setsockopt(OrbisNetId s, int level, int optname, const void* optval,
                                u32 optlen);
int PS4_SYSV_ABI sys_shutdown(OrbisNetId s, int how);
int PS4_SYSV_ABI sys_socketex(const char* name, int family, int type, int protocol);
int PS4_SYSV_ABI sys_socket(int family, int type, int protocol);
int PS4_SYSV_ABI sys_socketpair(int family, int type, int protocol, int sv[2]);
int PS4_SYSV_ABI sys_netabort(OrbisNetId s, int flags);
int PS4_SYSV_ABI sys_socketclose(OrbisNetId s);
int PS4_SYSV_ABI sys_send(OrbisNetId s, const void* buf, u64 len, int flags);
int PS4_SYSV_ABI sys_sendto(OrbisNetId s, const void* buf, u64 len, int flags,
                            const OrbisNetSockaddr* addr, u32 addrlen);
int PS4_SYSV_ABI sys_sendmsg(OrbisNetId s, const OrbisNetMsghdr* msg, int flags);
s64 PS4_SYSV_ABI sys_recv(OrbisNetId s, void* buf, u64 len, int flags);
s64 PS4_SYSV_ABI sys_recvfrom(OrbisNetId s, void* buf, u64 len, int flags, OrbisNetSockaddr* addr,
                              u32* paddrlen);
s64 PS4_SYSV_ABI sys_recvmsg(OrbisNetId s, OrbisNetMsghdr* msg, int flags);
int PS4_SYSV_ABI sys_htons(u16 v);
int PS4_SYSV_ABI sys_htonl(u32 v);

} // namespace Libraries::Net
