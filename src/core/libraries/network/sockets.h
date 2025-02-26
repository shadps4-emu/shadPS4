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
#include <memory>
#include <mutex>

namespace Libraries::Net {

struct Socket;

typedef std::shared_ptr<Socket> SocketPtr;

struct Socket {
    explicit Socket(int domain, int type, int protocol) {}
    virtual ~Socket() = default;
    virtual int SetSocketOptions(int level, int optname, const void* optval,
                                 unsigned int optlen) = 0;
    virtual int Bind(const OrbisNetSockaddr* addr, unsigned int addrlen) = 0;
    virtual int Listen(int backlog) = 0;
    virtual int SendPacket(const void* msg, u32 len, int flags, const OrbisNetSockaddr* to,
                           u32 tolen) = 0;
    virtual SocketPtr Accept(OrbisNetSockaddr* addr, u32* addrlen) = 0;
    virtual int ReceivePacket(void* buf, u32 len, int flags, OrbisNetSockaddr* from,
                              u32* fromlen) = 0;
};

struct PosixSocket : public Socket {
    net_socket sock;
    int sockopt_so_nbio = 0;
    int sockopt_so_onesbcast = 0;
    explicit PosixSocket(int domain, int type, int protocol)
        : Socket(domain, type, protocol), sock(socket(domain, type, protocol)) {}
    explicit PosixSocket(net_socket sock) : Socket(0, 0, 0), sock(sock) {}
    int SetSocketOptions(int level, int optname, const void* optval, unsigned int optlen) override;
    int Bind(const OrbisNetSockaddr* addr, unsigned int addrlen) override;
    int Listen(int backlog) override;
    int SendPacket(const void* msg, u32 len, int flags, const OrbisNetSockaddr* to,
                   u32 tolen) override;
    int ReceivePacket(void* buf, u32 len, int flags, OrbisNetSockaddr* from, u32* fromlen) override;
    SocketPtr Accept(OrbisNetSockaddr* addr, u32* addrlen) override;
};

struct P2PSocket : public Socket {
    explicit P2PSocket(int domain, int type, int protocol) : Socket(domain, type, protocol) {}
    int SetSocketOptions(int level, int optname, const void* optval, unsigned int optlen) override;
    int Bind(const OrbisNetSockaddr* addr, unsigned int addrlen) override;
    int Listen(int backlog) override;
    int SendPacket(const void* msg, u32 len, int flags, const OrbisNetSockaddr* to,
                   u32 tolen) override;
    int ReceivePacket(void* buf, u32 len, int flags, OrbisNetSockaddr* from, u32* fromlen) override;
    SocketPtr Accept(OrbisNetSockaddr* addr, u32* addrlen) override;
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
        return 0;
    }

public:
    std::mutex m_mutex;
    typedef std::map<int, SocketPtr> NetSockets;
    NetSockets socks;
    int next_sock_id = 0;
};
} // namespace Libraries::Net