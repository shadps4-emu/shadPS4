// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#ifdef _WIN32
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <Ws2tcpip.h>
#include <iphlpapi.h>
#include <winsock2.h>
typedef SOCKET net_socket;
typedef int socklen_t;
#else
#include <cerrno>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
typedef int net_socket;
#endif
#include <map>
#include <mutex>

namespace Libraries::Net {

struct Socket;

typedef std::shared_ptr<Socket> SocketPtr;

struct Socket {
    explicit Socket(int domain, int type, int protocol) {}
    virtual ~Socket() = default;
    virtual int SetSocketOptions(int level, int optname, const void* optval,
                                 unsigned int optlen) = 0;
};

struct PosixSocket : public Socket {
    net_socket sock;
    explicit PosixSocket(int domain, int type, int protocol)
        : Socket(domain, type, protocol), sock(socket(domain, type, protocol)) {}
    int SetSocketOptions(int level, int optname, const void* optval, unsigned int optlen) override;
};

class NetInternal {
public:
    explicit NetInternal() = default;
    ~NetInternal() = default;
    SocketPtr FindSocket(int sockid) {
        std::scoped_lock lock{m_mutex};
        const auto it = socks.find(sockid);
        if (it != socks.end()) {
            return it->second;
        }
        return nullptr;
    }

public:
    std::mutex m_mutex;
    typedef std::map<int, SocketPtr> NetSockets;
    NetSockets socks;
    int next_sock_id = 0;
};
} // namespace Libraries::Net