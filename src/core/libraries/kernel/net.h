// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <chrono>
#include <sys/types.h>
#include "common/types.h"

#ifdef WIN32
#include <WS2tcpip.h>
#else
#include <sys/socket.h>
#endif

namespace Common {
class NativeClock;
}

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Kernel {
int PS4_SYSV_ABI posix_socket(int domain, int type, int protocol);
int PS4_SYSV_ABI posix_connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen);
u32 PS4_SYSV_ABI posix_htonl(u32 hostlong);
u16 PS4_SYSV_ABI posix_htons(u16 hostshort);
int PS4_SYSV_ABI posix_bind(int sockfd, const struct sockaddr* addr, socklen_t addrlen);
int PS4_SYSV_ABI posix_listen(int sockfd, int backlog);
int PS4_SYSV_ABI posix_accept(int sockfd, struct sockaddr* addr, socklen_t* addrlen);

void RegisterNet(Core::Loader::SymbolsResolver* sym);

} // namespace Libraries::Kernel
