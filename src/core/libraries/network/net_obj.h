// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#ifdef WIN32
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <Ws2tcpip.h>
#include <iphlpapi.h>
#include <winsock2.h>
typedef SOCKET s_socket;
#else
#include <cerrno>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
typedef int s_socket;
#endif
#include <map>
#include <mutex>
#include "net.h"

namespace Libraries::Net {

class NetInternal {
public:
    explicit NetInternal() = default;
    ~NetInternal() = default;
    int net_socket(int domain, int type, int protocol);
    int send_packet(s_socket sock,const void* msg, unsigned int len, int flags,
                    const OrbisNetSockaddr* to,
                    unsigned int tolen);
    s_socket findsock(int sockid);

public:
    s_socket sock;
    std::mutex m_mutex;
    typedef std::map<int, s_socket> NetSockets;
    NetSockets socks;
    int next_sock_id = 0;
};
} // namespace Libraries::Net