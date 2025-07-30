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
#include "net.h"

namespace Libraries::Kernel {
struct OrbisKernelStat;
}

namespace Libraries::Net {

struct Socket;

typedef std::shared_ptr<Socket> SocketPtr;

struct OrbisNetLinger {
    s32 l_onoff;
    s32 l_linger;
};
struct Socket {
    explicit Socket(int domain, int type, int protocol) {}
    virtual ~Socket() = default;
    virtual bool IsValid() const = 0;
    virtual int Close() = 0;
    virtual int SetSocketOptions(int level, int optname, const void* optval, u32 optlen) = 0;
    virtual int GetSocketOptions(int level, int optname, void* optval, u32* optlen) = 0;
    virtual int Bind(const OrbisNetSockaddr* addr, u32 addrlen) = 0;
    virtual int Listen(int backlog) = 0;
    virtual int SendPacket(const void* msg, u32 len, int flags, const OrbisNetSockaddr* to,
                           u32 tolen) = 0;
    virtual SocketPtr Accept(OrbisNetSockaddr* addr, u32* addrlen) = 0;
    virtual int ReceivePacket(void* buf, u32 len, int flags, OrbisNetSockaddr* from,
                              u32* fromlen) = 0;
    virtual int Connect(const OrbisNetSockaddr* addr, u32 namelen) = 0;
    virtual int GetSocketAddress(OrbisNetSockaddr* name, u32* namelen) = 0;
    virtual int GetPeerName(OrbisNetSockaddr* addr, u32* namelen) = 0;
    virtual int fstat(Libraries::Kernel::OrbisKernelStat* stat) = 0;
    virtual std::optional<net_socket> Native() = 0;
    std::mutex m_mutex;
    std::mutex receive_mutex;
};

struct PosixSocket : public Socket {
    net_socket sock;
    int sockopt_so_connecttimeo = 0;
    int sockopt_so_reuseport = 0;
    int sockopt_so_onesbcast = 0;
    int sockopt_so_usecrypto = 0;
    int sockopt_so_usesignature = 0;
    int sockopt_so_nbio = 0;
    int sockopt_ip_ttlchk = 0;
    int sockopt_ip_maxttl = 0;
    int sockopt_tcp_mss_to_advertise = 0;
    int socket_type;
    explicit PosixSocket(int domain, int type, int protocol)
        : Socket(domain, type, protocol), sock(socket(domain, type, protocol)) {
        socket_type = type;
    }
    explicit PosixSocket(net_socket sock) : Socket(0, 0, 0), sock(sock) {}
    bool IsValid() const override;
    int Close() override;
    int SetSocketOptions(int level, int optname, const void* optval, u32 optlen) override;
    int GetSocketOptions(int level, int optname, void* optval, u32* optlen) override;
    int Bind(const OrbisNetSockaddr* addr, u32 addrlen) override;
    int Listen(int backlog) override;
    int SendPacket(const void* msg, u32 len, int flags, const OrbisNetSockaddr* to,
                   u32 tolen) override;
    int ReceivePacket(void* buf, u32 len, int flags, OrbisNetSockaddr* from, u32* fromlen) override;
    SocketPtr Accept(OrbisNetSockaddr* addr, u32* addrlen) override;
    int Connect(const OrbisNetSockaddr* addr, u32 namelen) override;
    int GetSocketAddress(OrbisNetSockaddr* name, u32* namelen) override;
    int GetPeerName(OrbisNetSockaddr* addr, u32* namelen) override;
    int fstat(Libraries::Kernel::OrbisKernelStat* stat) override;
    std::optional<net_socket> Native() override {
        return sock;
    }
};

struct P2PSocket : public Socket {
    explicit P2PSocket(int domain, int type, int protocol) : Socket(domain, type, protocol) {}
    bool IsValid() const override {
        return true;
    }
    int Close() override;
    int SetSocketOptions(int level, int optname, const void* optval, u32 optlen) override;
    int GetSocketOptions(int level, int optname, void* optval, u32* optlen) override;
    int Bind(const OrbisNetSockaddr* addr, u32 addrlen) override;
    int Listen(int backlog) override;
    int SendPacket(const void* msg, u32 len, int flags, const OrbisNetSockaddr* to,
                   u32 tolen) override;
    int ReceivePacket(void* buf, u32 len, int flags, OrbisNetSockaddr* from, u32* fromlen) override;
    SocketPtr Accept(OrbisNetSockaddr* addr, u32* addrlen) override;
    int Connect(const OrbisNetSockaddr* addr, u32 namelen) override;
    int GetSocketAddress(OrbisNetSockaddr* name, u32* namelen) override;
    int GetPeerName(OrbisNetSockaddr* addr, u32* namelen) override;
    int fstat(Libraries::Kernel::OrbisKernelStat* stat) override;
    std::optional<net_socket> Native() override {
        return {};
    }
};

} // namespace Libraries::Net