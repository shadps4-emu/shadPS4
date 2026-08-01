// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

// Kept ahead of the Windows networking headers below, which define ASSERT unless it already is:
// reaching ours second makes it the redefinition.
#include "common/assert.h"

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
#include <condition_variable>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <vector>
#include "net.h"

namespace Libraries::Kernel {
struct OrbisKernelStat;
}

namespace Libraries::Net {

struct Socket;

typedef std::shared_ptr<Socket> SocketPtr;

/// Translates the last host socket error into the guest errno and returns -1; a non-negative
/// return value passes through unchanged.
int ConvertReturnErrorCode(int retval);

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
    int SendMessage(const OrbisNetMsghdr* msg, int flags) override;
    int SendPacket(const void* msg, u32 len, int flags, const OrbisNetSockaddr* to,
                   u32 tolen) override;
    int ReceiveMessage(OrbisNetMsghdr* msg, int flags) override;
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

class P2PPort;

struct P2PSocket : public Socket {
    explicit P2PSocket(int domain, int type, int protocol);
    ~P2PSocket() override;
    bool IsValid() const override;

    // The host socket this vport is multiplexed on, shared with every other P2P socket bound to
    // the same (address, port) pair. Null until the socket is bound.
    std::shared_ptr<P2PPort> port;
    // Datagrams for this vport are forwarded here by the port's receive thread, which is why the
    // guest can poll the socket with epoll/select like any other: this is what Native() returns.
    net_socket inbox;
    sockaddr_in inbox_addr{};
    u32 bound_addr{};  // network byte order
    u16 bound_port{};  // network byte order
    u16 bound_vport{}; // network byte order
    bool bound{};
    // Connect() on a datagram socket only records the default destination, so the socket keeps
    // receiving from everyone; without it sceNetSend (which passes no address) has nowhere to go.
    u32 peer_addr{};  // network byte order
    u16 peer_port{};  // network byte order
    u16 peer_vport{}; // network byte order
    bool connected{};
    // Cached socket options. SO_NBIO decides whether an empty recv blocks; the SO_REUSE* pair
    // decides whether several sockets are allowed to share one vport.
    int sockopt_so_nbio{};
    int sockopt_so_reuseaddr{};
    int sockopt_so_reuseport{};
    int Close() override;
    int SetSocketOptions(int level, int optname, const void* optval, u32 optlen) override;
    int GetSocketOptions(int level, int optname, void* optval, u32* optlen) override;
    int Bind(const OrbisNetSockaddr* addr, u32 addrlen) override;
    int Listen(int backlog) override;
    int SendMessage(const OrbisNetMsghdr* msg, int flags) override;
    int SendPacket(const void* msg, u32 len, int flags, const OrbisNetSockaddr* to,
                   u32 tolen) override;
    int ReceiveMessage(OrbisNetMsghdr* msg, int flags) override;
    int ReceivePacket(void* buf, u32 len, int flags, OrbisNetSockaddr* from, u32* fromlen) override;
    SocketPtr Accept(OrbisNetSockaddr* addr, u32* addrlen) override;
    int Connect(const OrbisNetSockaddr* addr, u32 namelen) override;
    int GetSocketAddress(OrbisNetSockaddr* name, u32* namelen) override;
    int GetPeerName(OrbisNetSockaddr* addr, u32* namelen) override;
    int fstat(Libraries::Kernel::OrbisKernelStat* stat) override;
    std::optional<net_socket> Native() override {
        return inbox;
    }

private:
    /// Binds to an OS-chosen port and an ephemeral vport, the way a send from an unbound socket
    /// implicitly binds it. Returns 0, or -1 with the guest errno set.
    int EnsureBound();
};

struct UnixSocket : public Socket {
    net_socket sock;
    int socket_type;
    explicit UnixSocket(int domain, int type, int protocol)
        : Socket(domain, type, protocol), sock(socket(domain, type, protocol)) {
        socket_type = type;
    }
    explicit UnixSocket(net_socket sock) : Socket(0, 0, 0), sock(sock) {}
    bool IsValid() const override;
    int Close() override;
    int SetSocketOptions(int level, int optname, const void* optval, u32 optlen) override;
    int GetSocketOptions(int level, int optname, void* optval, u32* optlen) override;
    int Bind(const OrbisNetSockaddr* addr, u32 addrlen) override;
    int Listen(int backlog) override;
    int SendMessage(const OrbisNetMsghdr* msg, int flags) override;
    int SendPacket(const void* msg, u32 len, int flags, const OrbisNetSockaddr* to,
                   u32 tolen) override;
    int ReceiveMessage(OrbisNetMsghdr* msg, int flags) override;
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

u16 GetP2PConfiguredPort();
u32 GetP2PAdvertisedAddr();
bool EnsureP2PTransport();
bool P2PTransportIsReady();
int P2PSignalingSendTo(const void* data, u32 len, u32 dest_addr, u16 dest_port);
int P2PSignalingRecvFrom(void* buf, u32 len, u32* from_addr, u16* from_port);
int P2PControlSendTo(const void* data, u32 len, u32 dest_addr, u16 dest_port);
int P2PControlRecvFrom(void* buf, u32 len, u32* from_addr, u16* from_port);
int P2PMatching2SendTo(const void* data, u32 len, u32 dest_addr, u16 dest_port);
int P2PMatching2RecvFrom(void* buf, u32 len, u32* from_addr, u16* from_port);

} // namespace Libraries::Net