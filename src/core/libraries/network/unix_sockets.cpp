// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <common/assert.h>
#include "common/error.h"
#include "core/libraries/kernel/file_system.h"
#include "core/libraries/kernel/kernel.h"
#include "net.h"
#ifndef _WIN32
#include <sys/stat.h>
#include <sys/un.h>
#endif
#include "net_error.h"
#include "sockets.h"

namespace Libraries::Net {

static void convertOrbisNetSockaddrToUnix(const OrbisNetSockaddr* src, sockaddr_un* dst) {
    if (src == nullptr || dst == nullptr)
        return;
    memset(dst, 0, sizeof(sockaddr_un));
    const OrbisNetSockaddrUn* src_in = (const OrbisNetSockaddrUn*)src;
    sockaddr_un* dst_in = (sockaddr_un*)dst;
    dst_in->sun_family = src_in->sun_family;
    memcpy(&dst_in->sun_path, &src_in->sun_path, src_in->sun_len);
}

static void convertUnixSockaddrToOrbis(sockaddr* src, OrbisNetSockaddr* dst) {
    if (src == nullptr || dst == nullptr)
        return;
    memset(dst, 0, sizeof(OrbisNetSockaddrUn));
    OrbisNetSockaddrUn* dst_in = (OrbisNetSockaddrUn*)dst;
    sockaddr_un* src_in = (sockaddr_un*)src;
    dst_in->sun_len = strnlen(src_in->sun_path, 108);
    dst_in->sun_family = src_in->sun_family;
    memcpy(&dst_in->sun_path, &src_in->sun_path, dst_in->sun_len);
}

bool UnixSocket::IsValid() const {
#ifdef _WIN32
    return sock != INVALID_SOCKET;
#else
    return sock != -1;
#endif
}

int UnixSocket::Close() {
    std::scoped_lock lock{m_mutex};
#ifdef _WIN32
    auto out = closesocket(sock);
#else
    auto out = ::close(sock);
#endif
    return ConvertReturnErrorCode(out);
}

int UnixSocket::Bind(const OrbisNetSockaddr* addr, u32 addrlen) {
    std::scoped_lock lock{m_mutex};
    sockaddr_un addr2;
    convertOrbisNetSockaddrToUnix(addr, &addr2);
    return ConvertReturnErrorCode(::bind(sock, (const sockaddr*)&addr2, sizeof(sockaddr_un)));
}

int UnixSocket::Listen(int backlog) {
    std::scoped_lock lock{m_mutex};
    return ConvertReturnErrorCode(::listen(sock, backlog));
}

int UnixSocket::SendMessage(const OrbisNetMsghdr* msg, int flags) {
    std::scoped_lock lock{m_mutex};
#ifdef _WIN32
    DWORD bytesSent = 0;
    LPFN_WSASENDMSG wsasendmsg = nullptr;
    GUID guid = WSAID_WSASENDMSG;
    DWORD bytes = 0;

    if (WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), &wsasendmsg,
                 sizeof(wsasendmsg), &bytes, nullptr, nullptr) != 0) {
        return ConvertReturnErrorCode(-1);
    }

    int res = wsasendmsg(sock, reinterpret_cast<LPWSAMSG>(const_cast<OrbisNetMsghdr*>(msg)), flags,
                         &bytesSent, nullptr, nullptr);

    if (res == SOCKET_ERROR) {
        return ConvertReturnErrorCode(-1);
    }
    return static_cast<int>(bytesSent);
#else
    int res = sendmsg(sock, reinterpret_cast<const msghdr*>(msg), flags);
    return ConvertReturnErrorCode(res);
#endif
}

int UnixSocket::SendPacket(const void* msg, u32 len, int flags, const OrbisNetSockaddr* to,
                           u32 tolen) {
    std::scoped_lock lock{m_mutex};
    LOG_ERROR(Lib_Net, "called");
    if (to != nullptr) {
        sockaddr_un addr;
        convertOrbisNetSockaddrToUnix(to, &addr);
        return ConvertReturnErrorCode(
            sendto(sock, (const char*)msg, len, flags, (sockaddr*)&addr, sizeof(sockaddr_un)));
    } else {
        return ConvertReturnErrorCode(send(sock, (const char*)msg, len, flags));
    }
}

int UnixSocket::ReceiveMessage(OrbisNetMsghdr* msg, int flags) {
    std::scoped_lock lock{receive_mutex};
#ifdef _WIN32
    LPFN_WSARECVMSG wsarecvmsg = nullptr;
    GUID guid = WSAID_WSARECVMSG;
    DWORD bytes = 0;

    if (WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), &wsarecvmsg,
                 sizeof(wsarecvmsg), &bytes, nullptr, nullptr) != 0) {
        return ConvertReturnErrorCode(-1);
    }

    DWORD bytesReceived = 0;
    int res = wsarecvmsg(sock, reinterpret_cast<LPWSAMSG>(msg), &bytesReceived, nullptr, nullptr);

    if (res == SOCKET_ERROR) {
        return ConvertReturnErrorCode(-1);
    }
    return static_cast<int>(bytesReceived);
#else
    int res = recvmsg(sock, reinterpret_cast<msghdr*>(msg), flags);
    return ConvertReturnErrorCode(res);
#endif
}

int UnixSocket::ReceivePacket(void* buf, u32 len, int flags, OrbisNetSockaddr* from, u32* fromlen) {
    std::scoped_lock lock{receive_mutex};
    LOG_ERROR(Lib_Net, "called");
    if (from != nullptr) {
        sockaddr_un addr;
        int res = recvfrom(sock, (char*)buf, len, flags, (sockaddr*)&addr, (socklen_t*)fromlen);
        convertUnixSockaddrToOrbis((sockaddr*)&addr, from);
        *fromlen = sizeof(OrbisNetSockaddrUn);
        return ConvertReturnErrorCode(res);
    } else {
        return ConvertReturnErrorCode(recv(sock, (char*)buf, len, flags));
    }
}

SocketPtr UnixSocket::Accept(OrbisNetSockaddr* addr, u32* addrlen) {
    std::scoped_lock lock{m_mutex};
    sockaddr_un addr2;
    socklen_t len = sizeof(addr2);
    net_socket new_socket = ::accept(sock, (sockaddr*)&addr2, &len);
#ifdef _WIN32
    if (new_socket != INVALID_SOCKET) {
#else
    if (new_socket >= 0) {
#endif
        if (addr && addrlen) {
            convertUnixSockaddrToOrbis((sockaddr*)&addr2, addr);
            *addrlen = sizeof(OrbisNetSockaddrUn);
        }
        return std::make_shared<UnixSocket>(new_socket);
    }
    ConvertReturnErrorCode(new_socket);
    return nullptr;
}

int UnixSocket::Connect(const OrbisNetSockaddr* addr, u32 namelen) {
    std::scoped_lock lock{m_mutex};
    sockaddr_un addr2;
    convertOrbisNetSockaddrToUnix(addr, &addr2);
    int result = 0;
    do {
        result = ::connect(sock, (sockaddr*)&addr2, sizeof(sockaddr_un));
        LOG_DEBUG(Lib_Net, "raw connect result = {}, errno = {}", result,
                  result == -1 ? Common::GetLastErrorMsg() : "none");
    } while (result == -1 && (errno == EINTR || errno == EINPROGRESS));
    return ConvertReturnErrorCode(result);
}

int UnixSocket::GetSocketAddress(OrbisNetSockaddr* name, u32* namelen) {
    std::scoped_lock lock{m_mutex};

    sockaddr_un addr;
    convertOrbisNetSockaddrToUnix(name, &addr);
    if (name != nullptr) {
        *namelen = sizeof(sockaddr_un);
    }
    int res = getsockname(sock, (sockaddr*)&addr, (socklen_t*)namelen);
    if (res >= 0) {
        convertUnixSockaddrToOrbis((sockaddr*)&addr, name);
        *namelen = sizeof(OrbisNetSockaddrUn);
    }
    return ConvertReturnErrorCode(res);
}

#define CASE_SETSOCKOPT(opt)                                                                       \
    case ORBIS_NET_##opt:                                                                          \
        return ConvertReturnErrorCode(                                                             \
            setsockopt(sock, native_level, opt, (const char*)optval, optlen))

#define CASE_SETSOCKOPT_VALUE(opt, value)                                                          \
    case opt:                                                                                      \
        if (optlen != sizeof(*value)) {                                                            \
            *Libraries::Kernel::__Error() = ORBIS_NET_EFAULT;                                      \
            return -1;                                                                             \
        }                                                                                          \
        memcpy(value, optval, optlen);                                                             \
        return 0

int UnixSocket::SetSocketOptions(int level, int optname, const void* optval, u32 optlen) {
    std::scoped_lock lock{m_mutex};
    LOG_ERROR(Lib_Net, "Unknown level = {} optname = {}", level, optname);
    return -1;
}

#define CASE_GETSOCKOPT(opt)                                                                       \
    case ORBIS_NET_##opt: {                                                                        \
        socklen_t optlen_temp = *optlen;                                                           \
        auto retval = ConvertReturnErrorCode(                                                      \
            getsockopt(sock, native_level, opt, (char*)optval, &optlen_temp));                     \
        *optlen = optlen_temp;                                                                     \
        return retval;                                                                             \
    }
#define CASE_GETSOCKOPT_VALUE(opt, value)                                                          \
    case opt:                                                                                      \
        if (*optlen < sizeof(value)) {                                                             \
            *optlen = sizeof(value);                                                               \
            *Libraries::Kernel::__Error() = ORBIS_NET_EFAULT;                                      \
            return -1;                                                                             \
        }                                                                                          \
        *optlen = sizeof(value);                                                                   \
        *(decltype(value)*)optval = value;                                                         \
        return 0;

int UnixSocket::GetSocketOptions(int level, int optname, void* optval, u32* optlen) {
    std::scoped_lock lock{m_mutex};
    LOG_ERROR(Lib_Net, "Unknown level = {} optname = {}", level, optname);
    return ConvertReturnErrorCode(-1);
}

int UnixSocket::GetPeerName(OrbisNetSockaddr* name, u32* namelen) {
    std::scoped_lock lock{m_mutex};
    LOG_DEBUG(Lib_Net, "called");

    sockaddr_un addr;
    convertOrbisNetSockaddrToUnix(name, &addr);
    if (name != nullptr) {
        *namelen = sizeof(sockaddr_un);
    }
    int res = ::getpeername(sock, (sockaddr*)&addr, (socklen_t*)namelen);
    if (res >= 0) {
        convertUnixSockaddrToOrbis((sockaddr*)&addr, name);
        *namelen = sizeof(OrbisNetSockaddrUn);
    }
    return ConvertReturnErrorCode(res);
}

int UnixSocket::fstat(Libraries::Kernel::OrbisKernelStat* sb) {
#ifdef _WIN32
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    sb->st_mode = 0000777u | 0140000u;
    return 0;
#else
    struct stat st{};
    int result = ::fstat(sock, &st);
    sb->st_mode = 0000777u | 0140000u;
    sb->st_size = st.st_size;
    sb->st_blocks = st.st_blocks;
    sb->st_blksize = st.st_blksize;
    // sb->st_flags = st.st_flags;
    return ConvertReturnErrorCode(result);
#endif
}

} // namespace Libraries::Net