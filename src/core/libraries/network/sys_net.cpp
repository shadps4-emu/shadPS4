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
int sys_geetpeername(OrbisNetId s, const OrbisNetSockaddr* addr, u32* paddrlen) {
    return -1;
}
int sys_geetsockname(OrbisNetId s, const OrbisNetSockaddr* addr, u32* paddrlen) {
    return -1;
}
} // namespace Libraries::Net