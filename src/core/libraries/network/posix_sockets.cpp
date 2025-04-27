// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <common/assert.h>
#include "net.h"
#include "net_error.h"
#include "sockets.h"

namespace Libraries::Net {

#ifdef _WIN32
#define ERROR_CASE(errname)                                                                        \
    case (WSA##errname):                                                                           \
        return ORBIS_NET_ERROR_##errname;
#else
#define ERROR_CASE(errname)                                                                        \
    case (errname):                                                                                \
        return ORBIS_NET_ERROR_##errname;
#endif

static int ConvertReturnErrorCode(int retval) {
    if (retval < 0) {
#ifdef _WIN32
        switch (WSAGetLastError()) {
#else
        switch (errno) {
#endif
#ifndef _WIN32 // These errorcodes don't exist in WinSock
            ERROR_CASE(EPERM)
            ERROR_CASE(ENOENT)
            ERROR_CASE(ESRCH)
            ERROR_CASE(EIO)
            ERROR_CASE(ENXIO)
            ERROR_CASE(E2BIG)
            ERROR_CASE(ENOEXEC)
            ERROR_CASE(EDEADLK)
            ERROR_CASE(ENOMEM)
            ERROR_CASE(ECHILD)
            ERROR_CASE(EBUSY)
            ERROR_CASE(EEXIST)
            ERROR_CASE(EXDEV)
            ERROR_CASE(ENODEV)
            ERROR_CASE(ENOTDIR)
            ERROR_CASE(EISDIR)
            ERROR_CASE(ENFILE)
            ERROR_CASE(ENOTTY)
            ERROR_CASE(ETXTBSY)
            ERROR_CASE(EFBIG)
            ERROR_CASE(ENOSPC)
            ERROR_CASE(ESPIPE)
            ERROR_CASE(EROFS)
            ERROR_CASE(EMLINK)
            ERROR_CASE(EPIPE)
            ERROR_CASE(EDOM)
            ERROR_CASE(ERANGE)
            ERROR_CASE(ENOLCK)
            ERROR_CASE(ENOSYS)
            ERROR_CASE(EIDRM)
            ERROR_CASE(EOVERFLOW)
            ERROR_CASE(EILSEQ)
            ERROR_CASE(ENOTSUP)
            ERROR_CASE(ECANCELED)
            ERROR_CASE(EBADMSG)
            ERROR_CASE(ENODATA)
            ERROR_CASE(ENOSR)
            ERROR_CASE(ENOSTR)
            ERROR_CASE(ETIME)
#endif
            ERROR_CASE(EINTR)
            ERROR_CASE(EBADF)
            ERROR_CASE(EACCES)
            ERROR_CASE(EFAULT)
            ERROR_CASE(EINVAL)
            ERROR_CASE(EMFILE)
            ERROR_CASE(EWOULDBLOCK)
            ERROR_CASE(EINPROGRESS)
            ERROR_CASE(EALREADY)
            ERROR_CASE(ENOTSOCK)
            ERROR_CASE(EDESTADDRREQ)
            ERROR_CASE(EMSGSIZE)
            ERROR_CASE(EPROTOTYPE)
            ERROR_CASE(ENOPROTOOPT)
            ERROR_CASE(EPROTONOSUPPORT)
#if defined(__APPLE__) || defined(_WIN32)
            ERROR_CASE(EOPNOTSUPP)
#endif
            ERROR_CASE(EAFNOSUPPORT)
            ERROR_CASE(EADDRINUSE)
            ERROR_CASE(EADDRNOTAVAIL)
            ERROR_CASE(ENETDOWN)
            ERROR_CASE(ENETUNREACH)
            ERROR_CASE(ENETRESET)
            ERROR_CASE(ECONNABORTED)
            ERROR_CASE(ECONNRESET)
            ERROR_CASE(ENOBUFS)
            ERROR_CASE(EISCONN)
            ERROR_CASE(ENOTCONN)
            ERROR_CASE(ETIMEDOUT)
            ERROR_CASE(ECONNREFUSED)
            ERROR_CASE(ELOOP)
            ERROR_CASE(ENAMETOOLONG)
            ERROR_CASE(EHOSTUNREACH)
            ERROR_CASE(ENOTEMPTY)
        }
        return ORBIS_NET_ERROR_EINTERNAL;
    }
    // if it is 0 or positive return it as it is
    return retval;
}

static void convertOrbisNetSockaddrToPosix(const OrbisNetSockaddr* src, sockaddr* dst) {
    if (src == nullptr || dst == nullptr)
        return;
    memset(dst, 0, sizeof(sockaddr));
    const OrbisNetSockaddrIn* src_in = (const OrbisNetSockaddrIn*)src;
    sockaddr_in* dst_in = (sockaddr_in*)dst;
    dst_in->sin_family = src_in->sin_family;
    dst_in->sin_port = src_in->sin_port;
    memcpy(&dst_in->sin_addr, &src_in->sin_addr, 4);
}

static void convertPosixSockaddrToOrbis(sockaddr* src, OrbisNetSockaddr* dst) {
    if (src == nullptr || dst == nullptr)
        return;
    memset(dst, 0, sizeof(OrbisNetSockaddr));
    OrbisNetSockaddrIn* dst_in = (OrbisNetSockaddrIn*)dst;
    sockaddr_in* src_in = (sockaddr_in*)src;
    dst_in->sin_family = static_cast<unsigned char>(src_in->sin_family);
    dst_in->sin_port = src_in->sin_port;
    memcpy(&dst_in->sin_addr, &src_in->sin_addr, 4);
}

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