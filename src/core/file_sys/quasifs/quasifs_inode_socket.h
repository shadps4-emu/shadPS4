// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
// INAA License @marecl 2026

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

#include "common/assert.h"
#include "core/libraries/network/net.h"

#include "quasi_types.h"
#include "quasifs_inode.h"

namespace QuasiFS {

#define SOCKET_STUB()                                                                              \
    {                                                                                              \
        LOG_ERROR(Kernel_Fs, "(STUBBED) called");                                                  \
        return -POSIX_ENOSYS;                                                                      \
    }

using namespace Libraries::Net;

struct OrbisNetLinger {
    s32 l_onoff;
    s32 l_linger;
};

class Socket : public Inode {

    std::mutex m_mutex;
    std::mutex receive_mutex;

public:
    Socket(int domain, int type, int protocol);
    ~Socket();

    static socket_ptr Create() {
        return std::make_shared<Socket>();
    }

    socket_ptr Clone() const {
        auto _out = std::make_shared<Socket>(*this);
        _out->st.st_ino = -1;
        _out->st.st_nlink = 0;
        return _out;
    }

    // clang-format off
    virtual bool IsValid() const { SOCKET_STUB(); }
    virtual int Close() { SOCKET_STUB(); }
    virtual int SetSocketOptions(int level, int optname, const void* optval, u32 optlen) { SOCKET_STUB(); }
    virtual int GetSocketOptions(int level, int optname, void* optval, u32* optlen) { SOCKET_STUB(); }
    virtual int Bind(const OrbisNetSockaddr* addr, u32 addrlen) { SOCKET_STUB(); }
    virtual int Listen(int backlog) { SOCKET_STUB(); }
    virtual int SendMessage(const OrbisNetMsghdr* msg, int flags) { SOCKET_STUB(); }
    virtual int SendPacket(const void* msg, u32 len, int flags, const OrbisNetSockaddr* to,
                           u32 tolen) { SOCKET_STUB(); }
    virtual socket_ptr Accept(OrbisNetSockaddr* addr, u32* addrlen) {
        LOG_ERROR(Kernel_Fs, "(STUBBED) called");
        return nullptr;
    };
    virtual int ReceiveMessage(OrbisNetMsghdr* msg, int flags) { SOCKET_STUB(); }
    virtual int ReceivePacket(void* buf, u32 len, int flags, OrbisNetSockaddr* from, u32* fromlen) { SOCKET_STUB(); }
    virtual int Connect(const OrbisNetSockaddr* addr, u32 namelen) { SOCKET_STUB(); }
    virtual int GetSocketAddress(OrbisNetSockaddr* name, u32* namelen) { SOCKET_STUB(); }
    virtual int GetPeerName(OrbisNetSockaddr* addr, u32* namelen) { SOCKET_STUB(); }
    virtual int fstat(Libraries::Kernel::OrbisKernelStat* stat) { SOCKET_STUB(); }
    virtual std::optional<net_socket> Native() { SOCKET_STUB(); }
    // clang-format on
};

} // namespace QuasiFS