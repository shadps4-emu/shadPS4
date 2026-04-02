// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <common/assert.h>
#include "core/libraries/kernel/kernel.h"
#include "net.h"
#include "net_error.h"
#include "sockets.h"

#ifndef _WIN32
#include <sys/ioctl.h>
#include <sys/stat.h>
#endif

namespace Libraries::Net {

P2PSocket::P2PSocket(int domain, int type, int protocol) : Socket(domain, type, protocol) {
    socket_type = type;
    // Map P2P socket types to real OS socket types
    int os_type;
    int os_protocol;
    if (type == ORBIS_NET_SOCK_DGRAM_P2P) {
        os_type = SOCK_DGRAM;
        os_protocol = IPPROTO_UDP;
    } else {
        // ORBIS_NET_SOCK_STREAM_P2P
        os_type = SOCK_STREAM;
        os_protocol = IPPROTO_TCP;
    }
    sock = ::socket(AF_INET, os_type, os_protocol);
    LOG_INFO(Lib_Net, "P2P socket created: type={} (os_type={}), fd={}", type, os_type, sock);
}

P2PSocket::~P2PSocket() {
#ifdef _WIN32
    if (sock != INVALID_SOCKET) {
        closesocket(sock);
    }
#else
    if (sock != -1) {
        ::close(sock);
    }
#endif
}

bool P2PSocket::IsValid() const {
#ifdef _WIN32
    return sock != INVALID_SOCKET;
#else
    return sock != -1;
#endif
}

int P2PSocket::Close() {
    std::scoped_lock lock{m_mutex};
#ifdef _WIN32
    auto out = closesocket(sock);
    sock = INVALID_SOCKET;
#else
    auto out = ::close(sock);
    sock = -1;
#endif
    return ConvertReturnErrorCode(out);
}

int P2PSocket::SetSocketOptions(int level, int optname, const void* optval, u32 optlen) {
    std::scoped_lock lock{m_mutex};
    int native_level = ConvertLevels(level);

    // Handle PS4-specific options that have no OS equivalent
    if (native_level == SOL_SOCKET) {
        switch (optname) {
        case ORBIS_NET_SO_NBIO: {
            sockopt_so_nbio = *(int*)optval;
            int val = sockopt_so_nbio;
#ifdef _WIN32
            u_long mode = val;
            return ConvertReturnErrorCode(ioctlsocket(sock, FIONBIO, &mode));
#else
            return ConvertReturnErrorCode(ioctl(sock, FIONBIO, &val));
#endif
        }
        case ORBIS_NET_SO_USECRYPTO:
        case ORBIS_NET_SO_USESIGNATURE:
        case ORBIS_NET_SO_REUSEPORT:
            LOG_DEBUG(Lib_Net, "P2P setsockopt: storing PS4-specific option {}", optname);
            return 0;
        case ORBIS_NET_SO_SNDTIMEO:
        case ORBIS_NET_SO_RCVTIMEO: {
            int us = *(int*)optval;
#ifdef _WIN32
            DWORD timeout_ms = us / 1000;
            int native_opt = (optname == ORBIS_NET_SO_SNDTIMEO) ? SO_SNDTIMEO : SO_RCVTIMEO;
            return ConvertReturnErrorCode(
                ::setsockopt(sock, SOL_SOCKET, native_opt, (char*)&timeout_ms, sizeof(timeout_ms)));
#else
            timeval tv{.tv_sec = us / 1000000, .tv_usec = us % 1000000};
            int native_opt = (optname == ORBIS_NET_SO_SNDTIMEO) ? SO_SNDTIMEO : SO_RCVTIMEO;
            return ConvertReturnErrorCode(
                ::setsockopt(sock, SOL_SOCKET, native_opt, &tv, sizeof(tv)));
#endif
        }
        default:
            break;
        }
    }

    if (native_level < 0) {
        LOG_WARNING(Lib_Net, "P2P setsockopt: unknown level {}", level);
        return 0;
    }

    return ConvertReturnErrorCode(
        ::setsockopt(sock, native_level, optname, (const char*)optval, optlen));
}

int P2PSocket::GetSocketOptions(int level, int optname, void* optval, u32* optlen) {
    std::scoped_lock lock{m_mutex};
    int native_level = ConvertLevels(level);

    if (native_level == SOL_SOCKET) {
        switch (optname) {
        case ORBIS_NET_SO_NBIO:
            *(int*)optval = sockopt_so_nbio;
            *optlen = sizeof(int);
            return 0;
        case ORBIS_NET_SO_TYPE:
            *(int*)optval = socket_type;
            *optlen = sizeof(int);
            return 0;
        default:
            break;
        }
    }

    if (native_level < 0) {
        LOG_WARNING(Lib_Net, "P2P getsockopt: unknown level {}", level);
        return 0;
    }

    socklen_t native_optlen = *optlen;
    int ret = ::getsockopt(sock, native_level, optname, (char*)optval, &native_optlen);
    *optlen = native_optlen;
    return ConvertReturnErrorCode(ret);
}

int P2PSocket::Bind(const OrbisNetSockaddr* addr, u32 addrlen) {
    std::scoped_lock lock{m_mutex};
    const auto* orbis_addr = reinterpret_cast<const OrbisNetSockaddrIn*>(addr);
    if (orbis_addr) {
        vport = orbis_addr->sin_vport;
        LOG_INFO(Lib_Net, "P2P bind: port={}, vport={}", ntohs(orbis_addr->sin_port), vport);
    }
    sockaddr native_addr;
    convertOrbisNetSockaddrToPosix(addr, &native_addr);
    return ConvertReturnErrorCode(::bind(sock, &native_addr, sizeof(sockaddr_in)));
}

int P2PSocket::Listen(int backlog) {
    std::scoped_lock lock{m_mutex};
    return ConvertReturnErrorCode(::listen(sock, backlog));
}

int P2PSocket::SendMessage(const OrbisNetMsghdr* msg, int flags) {
    std::scoped_lock lock{m_mutex};
    int native_flags = convertOrbisFlagsToPosix(socket_type, flags);
#ifdef _WIN32
    // Windows: loop through buffers with send()
    int total_sent = 0;
    for (int i = 0; i < msg->msg_iovlen; i++) {
        auto& iov = msg->msg_iov[i];
        int sent = ::send(sock, (const char*)iov.iov_base, iov.iov_len, native_flags);
        if (sent < 0) {
            return total_sent > 0 ? total_sent : ConvertReturnErrorCode(sent);
        }
        total_sent += sent;
    }
    return total_sent;
#else
    return ConvertReturnErrorCode(
        ::sendmsg(sock, reinterpret_cast<const msghdr*>(msg), native_flags));
#endif
}

int P2PSocket::SendPacket(const void* msg, u32 len, int flags, const OrbisNetSockaddr* to,
                          u32 tolen) {
    std::scoped_lock lock{m_mutex};
    int native_flags = convertOrbisFlagsToPosix(socket_type, flags);
    if (to == nullptr) {
        return ConvertReturnErrorCode(::send(sock, (const char*)msg, len, native_flags));
    }
    sockaddr native_addr;
    convertOrbisNetSockaddrToPosix(to, &native_addr);
    return ConvertReturnErrorCode(
        ::sendto(sock, (const char*)msg, len, native_flags, &native_addr, sizeof(sockaddr_in)));
}

int P2PSocket::ReceiveMessage(OrbisNetMsghdr* msg, int flags) {
    std::scoped_lock lock{receive_mutex};
    int native_flags = convertOrbisFlagsToPosix(socket_type, flags);
#ifdef _WIN32
    // Windows: loop through buffers with recv()
    int total_recv = 0;
    for (int i = 0; i < msg->msg_iovlen; i++) {
        auto& iov = msg->msg_iov[i];
        int recvd = ::recv(sock, (char*)iov.iov_base, iov.iov_len, native_flags);
        if (recvd < 0) {
            return total_recv > 0 ? total_recv : ConvertReturnErrorCode(recvd);
        }
        total_recv += recvd;
        if (recvd < (int)iov.iov_len) {
            break;
        }
    }
    return total_recv;
#else
    return ConvertReturnErrorCode(
        ::recvmsg(sock, reinterpret_cast<msghdr*>(msg), native_flags));
#endif
}

int P2PSocket::ReceivePacket(void* buf, u32 len, int flags, OrbisNetSockaddr* from,
                             u32* fromlen) {
    std::scoped_lock lock{receive_mutex};
    int native_flags = convertOrbisFlagsToPosix(socket_type, flags);
    if (from == nullptr) {
        return ConvertReturnErrorCode(::recv(sock, (char*)buf, len, native_flags));
    }
    sockaddr native_addr;
    socklen_t native_addrlen = sizeof(native_addr);
    int ret = ::recvfrom(sock, (char*)buf, len, native_flags, &native_addr, &native_addrlen);
    if (ret >= 0) {
        convertPosixSockaddrToOrbis(&native_addr, from);
        if (fromlen) {
            *fromlen = sizeof(OrbisNetSockaddrIn);
        }
    }
    return ConvertReturnErrorCode(ret);
}

SocketPtr P2PSocket::Accept(OrbisNetSockaddr* addr, u32* addrlen) {
    std::scoped_lock lock{m_mutex};
    sockaddr native_addr;
    socklen_t len = sizeof(native_addr);
    auto new_sock = ::accept(sock, &native_addr, &len);
#ifdef _WIN32
    if (new_sock == INVALID_SOCKET) {
#else
    if (new_sock < 0) {
#endif
        ConvertReturnErrorCode(-1);
        return nullptr;
    }
    if (addr) {
        convertPosixSockaddrToOrbis(&native_addr, addr);
        if (addrlen) {
            *addrlen = sizeof(OrbisNetSockaddrIn);
        }
    }
    return std::make_shared<PosixSocket>(new_sock);
}

int P2PSocket::Connect(const OrbisNetSockaddr* addr, u32 namelen) {
    std::scoped_lock lock{m_mutex};
    const auto* orbis_addr = reinterpret_cast<const OrbisNetSockaddrIn*>(addr);
    if (orbis_addr) {
        vport = orbis_addr->sin_vport;
        LOG_INFO(Lib_Net, "P2P connect: vport={}", vport);
    }
    sockaddr native_addr;
    convertOrbisNetSockaddrToPosix(addr, &native_addr);
    int ret = ::connect(sock, &native_addr, sizeof(sockaddr_in));
#ifdef _WIN32
    if (ret < 0 && WSAGetLastError() == WSAEWOULDBLOCK) {
        WSASetLastError(WSAEINPROGRESS);
    }
#endif
    return ConvertReturnErrorCode(ret);
}

int P2PSocket::GetSocketAddress(OrbisNetSockaddr* name, u32* namelen) {
    std::scoped_lock lock{m_mutex};
    sockaddr native_addr;
    socklen_t len = sizeof(native_addr);
    int ret = ::getsockname(sock, &native_addr, &len);
    if (ret == 0) {
        convertPosixSockaddrToOrbis(&native_addr, name);
        if (namelen) {
            *namelen = sizeof(OrbisNetSockaddrIn);
        }
    }
    return ConvertReturnErrorCode(ret);
}

int P2PSocket::GetPeerName(OrbisNetSockaddr* addr, u32* namelen) {
    std::scoped_lock lock{m_mutex};
    sockaddr native_addr;
    socklen_t len = sizeof(native_addr);
    int ret = ::getpeername(sock, &native_addr, &len);
    if (ret == 0) {
        convertPosixSockaddrToOrbis(&native_addr, addr);
        if (namelen) {
            *namelen = sizeof(OrbisNetSockaddrIn);
        }
    }
    return ConvertReturnErrorCode(ret);
}

int P2PSocket::fstat(Libraries::Kernel::OrbisKernelStat* stat) {
    LOG_DEBUG(Lib_Net, "(STUBBED) called");
    return 0;
}

} // namespace Libraries::Net
