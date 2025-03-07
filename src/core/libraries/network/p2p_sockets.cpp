// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <common/assert.h>
#include "net.h"
#include "net_error.h"
#include "sockets.h"

namespace Libraries::Net {
int P2PSocket::SetSocketOptions(int level, int optname, const void* optval, unsigned int optlen) {
    return 0;
}

int P2PSocket::Bind(const OrbisNetSockaddr* addr, unsigned int addrlen) {
    return 0;
}

int P2PSocket::Listen(int backlog) {
    return 0;
}

int P2PSocket::SendPacket(const void* msg, u32 len, int flags, const OrbisNetSockaddr* to,
                          u32 tolen) {
    return -1; // fake value makes peggle2 work
}

int P2PSocket::ReceivePacket(void* buf, u32 len, int flags, OrbisNetSockaddr* from, u32* fromlen) {
    return -1; // fake it
}

SocketPtr P2PSocket::Accept(OrbisNetSockaddr* addr, u32* addrlen) {
    return nullptr;
}

} // namespace Libraries::Net