// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <common/assert.h>
#include "net.h"
#include "sockets.h"

namespace Libraries::Net {

static int ConvertLevels(int level) {
    switch (level) {
    case ORBIS_NET_SOL_SOCKET:
        return SOL_SOCKET;
    case ORBIS_NET_IPPROTO_IP:
        return IPPROTO_IP;
    case ORBIS_NET_IPPROTO_TCP:
        return IPPROTO_TCP;
    }
    return -1;
}

int PosixSocket::SetSocketOptions(int level, int optname, const void* optval, unsigned int optlen) {
    level = ConvertLevels(level);
    if (level == SOL_SOCKET) {
        switch (optname) {
        case ORBIS_NET_SO_REUSEADDR:
            return setsockopt(sock, level, SO_REUSEADDR, (const char*)optval, optlen);
        }
    }
    UNREACHABLE_MSG("Unknown level ={} optname ={}", level, optname);
    return 0;
}
} // namespace Libraries::Net