// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <common/assert.h>
#include "net.h"
#include "net_error.h"
#include "sockets.h"

namespace Libraries::Net {

int PosixSocket::Close() {
    return 0;
}
int PosixSocket::SetSocketOptions(int level, int optname, const void* optval, u32 optlen) {
    return 0;
}
int PosixSocket::GetSocketOptions(int level, int optname, void* optval, u32* optlen) {
    return 0;
}

int PosixSocket::Bind(const OrbisNetSockaddr* addr, u32 addrlen) {
    return 0;
}

int PosixSocket::Listen(int backlog) {
    return 0;
}

int PosixSocket::SendPacket(const void* msg, u32 len, int flags, const OrbisNetSockaddr* to,
                            u32 tolen) {
    return 0;
}

int PosixSocket::ReceivePacket(void* buf, u32 len, int flags, OrbisNetSockaddr* from,
                               u32* fromlen) {
    return 0;
}

SocketPtr PosixSocket::Accept(OrbisNetSockaddr* addr, u32* addrlen) {
    return SocketPtr();
}

int PosixSocket::Connect(const OrbisNetSockaddr* addr, u32 namelen) {
    return 0;
}

int PosixSocket::GetSocketAddress(OrbisNetSockaddr* name, u32* namelen) {
    return 0;
}

} // namespace Libraries::Net