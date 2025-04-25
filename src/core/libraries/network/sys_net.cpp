// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "sys_net.h"

namespace Libraries::Net {

int sys_connect(OrbisNetId s, const OrbisNetSockaddr* addr, u32 addrlen) {
    return -1;
}
int sys_bind(OrbisNetId s, const OrbisNetSockaddr* addr, u32 addrlen) {
    return -1;
}
int sys_accept(OrbisNetId s, const OrbisNetSockaddr* addr, u32* paddrlen) {
    return -1;
}
int sys_getpeername(OrbisNetId s, const OrbisNetSockaddr* addr, u32* paddrlen) {
    return -1;
}
int sys_getsockname(OrbisNetId s, const OrbisNetSockaddr* addr, u32* paddrlen) {
    return -1;
}
int sys_getsockopt(OrbisNetId s, int level, int optname, void* optval, u32* optlen) {
    return -1;
}
int sys_listen(OrbisNetId s, int backlog) {
    return -1;
}
int sys_setsockopt(OrbisNetId s, int level, int optname, const void* optval, u32 optlen) {
    return -1;
}
int sys_shutdown(OrbisNetId s, int how) {
    return -1;
}
int sys_socketex(const char* name, int family, int type, int protocol) {
    return -1;
}
int sys_netabort(OrbisNetId s, int flags) {
    return -1;
}
int sys_socketclose(OrbisNetId s) {
    return -1;
}
int sys_sendto(OrbisNetId s, const void* buf, u64 len, int flags, const OrbisNetSockaddr* addr,
               u32 addrlen) {
    return -1;
}
int sys_sendmsg(OrbisNetId s, const OrbisNetMsghdr* msg, int flags) {
    return -1;
}
} // namespace Libraries::Net