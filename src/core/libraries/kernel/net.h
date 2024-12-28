// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <chrono>
#include <sys/types.h>
#include "common/types.h"

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <Ws2tcpip.h>
#include <iphlpapi.h>
#include <winsock2.h>
typedef SOCKET s_socket;
#else
#include <sys/socket.h>
typedef int s_socket;
#endif
#include <map>
#include <mutex>

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

class NetPosixInternal {
public:
    explicit NetPosixInternal() = default;
    ~NetPosixInternal() = default;
    int net_socket(int domain, int type, int protocol);
    int net_bind(int sockfd, const struct sockaddr* addr, socklen_t addrlen);

public:
    s_socket sock;
    std::mutex m_mutex;
    typedef std::map<int, s_socket> NetSockets;
    NetSockets socks;
    int next_id = 0;
};

} // namespace Libraries::Kernel
