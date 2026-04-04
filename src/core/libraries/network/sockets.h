// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#ifdef _WIN32
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <Ws2tcpip.h>
#include <afunix.h>
#include <iphlpapi.h>
#include <mstcpip.h>
#include <winsock2.h>
typedef SOCKET net_socket;
typedef int socklen_t;
#ifndef LPFN_WSASENDMSG
typedef INT(PASCAL* LPFN_WSASENDMSG)(SOCKET s, LPWSAMSG lpMsg, DWORD dwFlags,
                                     LPDWORD lpNumberOfBytesSent, LPWSAOVERLAPPED lpOverlapped,
                                     LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);
#endif
#ifndef WSAID_WSASENDMSG
static const GUID WSAID_WSASENDMSG = {
    0xa441e712, 0x754f, 0x43ca, {0x84, 0xa7, 0x0d, 0xee, 0x44, 0xcf, 0x60, 0x6d}};
#endif
#ifndef LPFN_WSARECVMSG
typedef INT(PASCAL* LPFN_WSARECVMSG)(SOCKET s, LPWSAMSG lpMsg, LPDWORD lpdwNumberOfBytesRecvd,
                                     LPWSAOVERLAPPED lpOverlapped,
                                     LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);
#endif

#ifndef WSAID_WSARECVMSG
static const GUID WSAID_WSARECVMSG = {
    0xf689d7c8, 0x6f1f, 0x436b, {0x8a, 0x53, 0xe5, 0x4f, 0xe3, 0x51, 0xc3, 0x22}};
#endif
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

// Shared socket helper functions defined in posix_sockets.cpp
int ConvertReturnErrorCode(int retval);
int ConvertLevels(int level);
void convertOrbisNetSockaddrToPosix(const OrbisNetSockaddr* src, sockaddr* dst);
void convertPosixSockaddrToOrbis(sockaddr* src, OrbisNetSockaddr* dst);
int convertOrbisFlagsToPosix(int sock_type, int sce_flags);
#ifndef _WIN32
// Build a native msghdr from OrbisNetMsghdr. OrbisNetMsghdr uses
// int-sized fields (msg_iovlen, msg_controllen) where native msghdr uses size_t,
// so a reinterpret_cast would corrupt the layout on 64-bit systems.
inline msghdr ConvertOrbisToNativeMsghdr(const OrbisNetMsghdr* msg) {
    msghdr native_msg{};
    native_msg.msg_name = msg->msg_name;
    native_msg.msg_namelen = msg->msg_namelen;
    native_msg.msg_iov = reinterpret_cast<iovec*>(msg->msg_iov);
    native_msg.msg_iovlen = msg->msg_iovlen;
    native_msg.msg_control = msg->msg_control;
    native_msg.msg_controllen = msg->msg_controllen;
    native_msg.msg_flags = msg->msg_flags;
    return native_msg;
}
#endif

struct OrbisNetLinger {
    s32 l_onoff;
    s32 l_linger;
};
struct Socket {
    explicit Socket(int domain, int type, int protocol) : socket_type(type) {}
    virtual ~Socket() = default;
    virtual bool IsValid() const = 0;
    virtual int Close() = 0;
    virtual int SetSocketOptions(int level, int optname, const void* optval, u32 optlen) = 0;
    virtual int GetSocketOptions(int level, int optname, void* optval, u32* optlen) = 0;
    virtual int Bind(const OrbisNetSockaddr* addr, u32 addrlen) = 0;
    virtual int Listen(int backlog) = 0;
    virtual int SendMessage(const OrbisNetMsghdr* msg, int flags) = 0;
    virtual int SendPacket(const void* msg, u32 len, int flags, const OrbisNetSockaddr* to,
                           u32 tolen) = 0;
    virtual SocketPtr Accept(OrbisNetSockaddr* addr, u32* addrlen) = 0;
    virtual int ReceiveMessage(OrbisNetMsghdr* msg, int flags) = 0;
    virtual int ReceivePacket(void* buf, u32 len, int flags, OrbisNetSockaddr* from,
                              u32* fromlen) = 0;
    virtual int Connect(const OrbisNetSockaddr* addr, u32 namelen) = 0;
    virtual int GetSocketAddress(OrbisNetSockaddr* name, u32* namelen) = 0;
    virtual int GetPeerName(OrbisNetSockaddr* addr, u32* namelen) = 0;
    virtual int fstat(Libraries::Kernel::OrbisKernelStat* stat) = 0;
    virtual std::optional<net_socket> Native() = 0;
    std::mutex m_mutex;
    std::mutex receive_mutex;
    int socket_type;
};

// Owns the OS socket fd and implements methods identical across all socket types.
struct NativeSocket : public Socket {
    net_socket sock;

    NativeSocket(int domain, int type, int protocol)
        : Socket(domain, type, protocol), sock(::socket(domain, type, protocol)) {}

    // Wrap an already-existing fd. Queries SO_TYPE from the OS automatically.
    // Note: Socket base domain/protocol remain 0 — only socket_type is recovered.
    explicit NativeSocket(net_socket sock) : Socket(0, 0, 0), sock(sock) {
        int type = 0;
        socklen_t len = sizeof(type);
        ::getsockopt(sock, SOL_SOCKET, SO_TYPE, (char*)&type, &len);
        socket_type = type;
    }

    bool IsValid() const override {
#ifdef _WIN32
        return sock != INVALID_SOCKET;
#else
        return sock != -1;
#endif
    }

    int Close() override;
    int Listen(int backlog) override;
    int fstat(Libraries::Kernel::OrbisKernelStat* stat) override;

    std::optional<net_socket> Native() override {
        return sock;
    }
};

struct PosixSocket : public NativeSocket {
    int sockopt_so_connecttimeo = 0;
    int sockopt_so_reuseport = 0;
    int sockopt_so_onesbcast = 0;
    int sockopt_so_usecrypto = 0;
    int sockopt_so_usesignature = 0;
    int sockopt_so_nbio = 0;
    int sockopt_ip_ttlchk = 0;
    int sockopt_ip_maxttl = 0;
    int sockopt_tcp_mss_to_advertise = 0;

    explicit PosixSocket(int domain, int type, int protocol)
        : NativeSocket(domain, type, protocol) {}
    explicit PosixSocket(net_socket sock) : NativeSocket(sock) {}

    int SetSocketOptions(int level, int optname, const void* optval, u32 optlen) override;
    int GetSocketOptions(int level, int optname, void* optval, u32* optlen) override;
    int Bind(const OrbisNetSockaddr* addr, u32 addrlen) override;
    int SendMessage(const OrbisNetMsghdr* msg, int flags) override;
    int SendPacket(const void* msg, u32 len, int flags, const OrbisNetSockaddr* to,
                   u32 tolen) override;
    int ReceiveMessage(OrbisNetMsghdr* msg, int flags) override;
    int ReceivePacket(void* buf, u32 len, int flags, OrbisNetSockaddr* from, u32* fromlen) override;
    SocketPtr Accept(OrbisNetSockaddr* addr, u32* addrlen) override;
    int Connect(const OrbisNetSockaddr* addr, u32 namelen) override;
    int GetSocketAddress(OrbisNetSockaddr* name, u32* namelen) override;
    int GetPeerName(OrbisNetSockaddr* addr, u32* namelen) override;
};

struct P2PSocket : public PosixSocket {
    u16 vport = 0; // PS4 virtual port

    explicit P2PSocket(int domain, int type, int protocol);
    // Wrap an already-accepted fd. The PS4 type must be passed explicitly because
    // the OS only knows SOCK_STREAM/SOCK_DGRAM, not PS4-specific P2P socket types.
    P2PSocket(net_socket accepted_sock, int ps4_type, u16 parent_vport)
        : PosixSocket(accepted_sock), vport(parent_vport) {
        socket_type = ps4_type;
    }
    int GetSocketOptions(int level, int optname, void* optval, u32* optlen) override;
    int Bind(const OrbisNetSockaddr* addr, u32 addrlen) override;
    int Connect(const OrbisNetSockaddr* addr, u32 namelen) override;
    SocketPtr Accept(OrbisNetSockaddr* addr, u32* addrlen) override;
};

struct UnixSocket : public NativeSocket {
    explicit UnixSocket(int domain, int type, int protocol)
        : NativeSocket(domain, type, protocol) {}
    explicit UnixSocket(net_socket sock) : NativeSocket(sock) {}

    int SetSocketOptions(int level, int optname, const void* optval, u32 optlen) override;
    int GetSocketOptions(int level, int optname, void* optval, u32* optlen) override;
    int Bind(const OrbisNetSockaddr* addr, u32 addrlen) override;
    int SendMessage(const OrbisNetMsghdr* msg, int flags) override;
    int SendPacket(const void* msg, u32 len, int flags, const OrbisNetSockaddr* to,
                   u32 tolen) override;
    int ReceiveMessage(OrbisNetMsghdr* msg, int flags) override;
    int ReceivePacket(void* buf, u32 len, int flags, OrbisNetSockaddr* from, u32* fromlen) override;
    SocketPtr Accept(OrbisNetSockaddr* addr, u32* addrlen) override;
    int Connect(const OrbisNetSockaddr* addr, u32 namelen) override;
    int GetSocketAddress(OrbisNetSockaddr* name, u32* namelen) override;
    int GetPeerName(OrbisNetSockaddr* addr, u32* namelen) override;
};

} // namespace Libraries::Net
