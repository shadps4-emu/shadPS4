// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <common/assert.h>
#include <common/logging/log.h>
#include <core/libraries/kernel/kernel.h>
#include <magic_enum/magic_enum.hpp>
#include "common/error.h"
#include "common/singleton.h"
#include "core/file_sys/fs.h"
#include "net_error.h"
#include "sockets.h"
#include "sys_net.h"

namespace Libraries::Net {

using FDTable = Common::Singleton<Core::FileSys::HandleTable>;

int PS4_SYSV_ABI sys_connect(OrbisNetId s, const OrbisNetSockaddr* addr, u32 addrlen) {
    auto file = FDTable::Instance()->GetSocket(s);
    if (!file) {
        *Libraries::Kernel::__Error() = ORBIS_NET_EBADF;
        LOG_ERROR(Lib_Net, "socket id is invalid = {}", s);
        return -1;
    }
    LOG_DEBUG(Lib_Net, "s = {} ({})", s, file->m_guest_name);
    int returncode = file->socket->Connect(addr, addrlen);
    if (returncode >= 0) {
        return returncode;
    }
    LOG_ERROR(Lib_Net, "s = {} ({}) returned error code: {}", s, file->m_guest_name,
              (u32)*Libraries::Kernel::__Error());
    return -1;
}

int PS4_SYSV_ABI sys_bind(OrbisNetId s, const OrbisNetSockaddr* addr, u32 addrlen) {
    auto file = FDTable::Instance()->GetSocket(s);
    if (!file) {
        *Libraries::Kernel::__Error() = ORBIS_NET_EBADF;
        LOG_ERROR(Lib_Net, "socket id is invalid = {}", s);
        return -1;
    }
    LOG_DEBUG(Lib_Net, "s = {} ({})", s, file->m_guest_name);
    int returncode = file->socket->Bind(addr, addrlen);
    if (returncode >= 0) {
        return returncode;
    }
    LOG_ERROR(Lib_Net, "error code returned: {}", (u32)*Libraries::Kernel::__Error());
    return -1;
}

int PS4_SYSV_ABI sys_accept(OrbisNetId s, OrbisNetSockaddr* addr, u32* paddrlen) {
    auto file = FDTable::Instance()->GetSocket(s);
    if (!file) {
        *Libraries::Kernel::__Error() = ORBIS_NET_EBADF;
        LOG_ERROR(Lib_Net, "socket id is invalid = {}", s);
        return -1;
    }
    LOG_DEBUG(Lib_Net, "s = {} ({})", s, file->m_guest_name);
    auto new_sock = file->socket->Accept(addr, paddrlen);
    if (!new_sock) {
        LOG_ERROR(Lib_Net, "s = {} ({}) returned error code creating new socket for accepting: {}",
                  s, file->m_guest_name, (u32)*Libraries::Kernel::__Error());
        return -1;
    }
    auto fd = FDTable::Instance()->CreateHandle();
    auto* new_file = FDTable::Instance()->GetFile(fd);
    new_file->is_opened = true;
    new_file->type = Core::FileSys::FileType::Socket;
    new_file->socket = new_sock;
    return fd;
}

int PS4_SYSV_ABI sys_getpeername(OrbisNetId s, OrbisNetSockaddr* addr, u32* paddrlen) {
    auto file = FDTable::Instance()->GetSocket(s);
    if (!file) {
        *Libraries::Kernel::__Error() = ORBIS_NET_EBADF;
        LOG_ERROR(Lib_Net, "socket id is invalid = {}", s);
        return -1;
    }
    LOG_DEBUG(Lib_Net, "s = {} ({})", s, file->m_guest_name);
    int returncode = file->socket->GetPeerName(addr, paddrlen);
    if (returncode >= 0) {
        return returncode;
    }
    LOG_ERROR(Lib_Net, "error code returned: {}", (u32)*Libraries::Kernel::__Error());
    return -1;
}

int PS4_SYSV_ABI sys_getsockname(OrbisNetId s, OrbisNetSockaddr* addr, u32* paddrlen) {
    auto file = FDTable::Instance()->GetSocket(s);
    if (!file) {
        *Libraries::Kernel::__Error() = ORBIS_NET_EBADF;
        LOG_ERROR(Lib_Net, "socket id is invalid = {}", s);
        return -1;
    }
    LOG_DEBUG(Lib_Net, "s = {} ({})", s, file->m_guest_name);
    int returncode = file->socket->GetSocketAddress(addr, paddrlen);
    if (returncode >= 0) {
        return returncode;
    }
    LOG_ERROR(Lib_Net, "error code returned: {}", (u32)*Libraries::Kernel::__Error());
    return -1;
}

int PS4_SYSV_ABI sys_getsockopt(OrbisNetId s, int level, int optname, void* optval, u32* optlen) {
    auto file = FDTable::Instance()->GetSocket(s);
    if (!file) {
        *Libraries::Kernel::__Error() = ORBIS_NET_EBADF;
        LOG_ERROR(Lib_Net, "socket id is invalid = {}", s);
        return -1;
    }
    auto name = [&] -> std::string_view {
        switch (level) {
        case ORBIS_NET_SOL_SOCKET:
            return NameOf((OrbisNetSocketSoOption)optname);
        case ORBIS_NET_IPPROTO_IP:
            return magic_enum::enum_name((OrbisNetSocketIpOption)optname);
        case ORBIS_NET_IPPROTO_TCP:
            return magic_enum::enum_name((OrbisNetSocketTcpOption)optname);
        default:
            return std::string_view{"unknown"};
        }
    }();
    LOG_DEBUG(Lib_Net, "s = {} ({}), level = {}, optname = {}", s, file->m_guest_name,
              NameOf((OrbisNetProtocol)level), name);
    int returncode = file->socket->GetSocketOptions(level, optname, optval, optlen);
    if (returncode >= 0) {
        if (optname == ORBIS_NET_SO_ERROR) {
            LOG_DEBUG(Lib_Net, "so_error = {}", *reinterpret_cast<s32*>(optval));
        }
        return returncode;
    }
    LOG_ERROR(Lib_Net, "error code returned: {}", (u32)*Libraries::Kernel::__Error());
    return -1;
}

int PS4_SYSV_ABI sys_listen(OrbisNetId s, int backlog) {
    auto file = FDTable::Instance()->GetSocket(s);
    if (!file) {
        *Libraries::Kernel::__Error() = ORBIS_NET_EBADF;
        LOG_ERROR(Lib_Net, "socket id is invalid = {}", s);
        return -1;
    }
    LOG_DEBUG(Lib_Net, "s = {} ({}), backlog = {}", s, file->m_guest_name, backlog);
    int returncode = file->socket->Listen(backlog);
    if (returncode >= 0) {
        return returncode;
    }
    LOG_ERROR(Lib_Net, "error code returned: {}", (u32)*Libraries::Kernel::__Error());
    return -1;
}

int PS4_SYSV_ABI sys_setsockopt(OrbisNetId s, int level, int optname, const void* optval,
                                u32 optlen) {
    auto file = FDTable::Instance()->GetSocket(s);
    if (!file) {
        *Libraries::Kernel::__Error() = ORBIS_NET_EBADF;
        LOG_ERROR(Lib_Net, "socket id is invalid = {}", s);
        return -1;
    }
    auto name = [&] -> std::string_view {
        switch (level) {
        case ORBIS_NET_SOL_SOCKET:
            return NameOf((OrbisNetSocketSoOption)optname);
        case ORBIS_NET_IPPROTO_IP:
            return magic_enum::enum_name((OrbisNetSocketIpOption)optname);
        case ORBIS_NET_IPPROTO_TCP:
            return magic_enum::enum_name((OrbisNetSocketTcpOption)optname);
        default:
            return std::string_view{"unknown"};
        }
    }();
    LOG_DEBUG(Lib_Net, "s = {} ({}), level = {}, optname = {}", s, file->m_guest_name,
              NameOf((OrbisNetProtocol)level), name);
    int returncode = file->socket->SetSocketOptions(level, optname, optval, optlen);
    if (returncode >= 0) {
        return returncode;
    }
    LOG_ERROR(Lib_Net, "error code returned: {}", (u32)*Libraries::Kernel::__Error());
    return -1;
}

int PS4_SYSV_ABI sys_shutdown(OrbisNetId s, int how) {
    return -1;
}

int PS4_SYSV_ABI sys_socketex(const char* name, int family, int type, int protocol) {
    auto sname = name ? name : "anon";
    LOG_INFO(Lib_Net, "name = {} family = {} type = {} protocol = {}", std::string(sname),
             magic_enum::enum_name((OrbisNetFamily)family),
             magic_enum::enum_name((OrbisNetSocketType)type), protocol);
    SocketPtr socket;
    switch (type) {
    case ORBIS_NET_SOCK_STREAM:
    case ORBIS_NET_SOCK_DGRAM:
        if (family == ORBIS_NET_AF_UNIX) {
            socket = std::make_shared<UnixSocket>(family, type, protocol);
            break;
        }
        [[fallthrough]];
    case ORBIS_NET_SOCK_RAW:
        socket = std::make_shared<PosixSocket>(family, type, protocol);
        break;
    case ORBIS_NET_SOCK_DGRAM_P2P:
    case ORBIS_NET_SOCK_STREAM_P2P:
        socket = std::make_shared<P2PSocket>(family, type, protocol);
        break;
    default:
        UNREACHABLE_MSG("Unknown type {}", type);
    }
    if (!socket->IsValid()) {
        *Libraries::Kernel::__Error() = ORBIS_NET_EPROTONOSUPPORT;
        return -1;
    }

    auto fd = FDTable::Instance()->CreateHandle();
    auto* sock = FDTable::Instance()->GetFile(fd);
    sock->is_opened = true;
    sock->type = Core::FileSys::FileType::Socket;
    sock->socket = socket;
    sock->m_guest_name = sname;
    return fd;
}

int PS4_SYSV_ABI sys_socket(int family, int type, int protocol) {
    return sys_socketex(nullptr, family, type, protocol);
}

#ifdef _WIN32
int socketpair(int family, int type, int protocol, net_socket fd[2]) {
    if (family != AF_UNIX && family != AF_INET) {
        *Libraries::Kernel::__Error() = ORBIS_NET_EPROTONOSUPPORT;
        return -1;
    }
    if (type != SOCK_STREAM) {
        *Libraries::Kernel::__Error() = ORBIS_NET_EPROTONOSUPPORT;
        return -1;
    }

    SOCKET listener = INVALID_SOCKET;
    SOCKET sock1 = INVALID_SOCKET;
    SOCKET sock2 = INVALID_SOCKET;
    sockaddr_in addr{};
    int addrlen = sizeof(addr);

    listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener == INVALID_SOCKET)
        goto fail1;

    ZeroMemory(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;

    if (bind(listener, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
        goto fail1;
    if (listen(listener, 1) == SOCKET_ERROR)
        goto fail1;

    if (getsockname(listener, (sockaddr*)&addr, &addrlen) == SOCKET_ERROR)
        goto fail1;

    sock1 = socket(AF_INET, SOCK_STREAM, 0);
    if (sock1 == INVALID_SOCKET)
        goto fail2;

    if (connect(sock1, (sockaddr*)&addr, addrlen) == SOCKET_ERROR)
        goto fail2;

    sock2 = accept(listener, nullptr, nullptr);
    if (sock2 == INVALID_SOCKET)
        goto fail3;

    closesocket(listener);
    fd[0] = sock1;
    fd[1] = sock2;
    return 0;

fail3:
    closesocket(sock2);
fail2:
    closesocket(sock1);
fail1:
    closesocket(listener);
    return -1;
}
#endif

int PS4_SYSV_ABI sys_socketpair(int family, int type, int protocol, int sv[2]) {
    if (sv == nullptr) {
        *Libraries::Kernel::__Error() = ORBIS_NET_EINVAL;
        return -1;
    }
    if (family != ORBIS_NET_AF_UNIX) {
        *Libraries::Kernel::__Error() = ORBIS_NET_EPROTONOSUPPORT;
        return -1;
    }

    net_socket fd[2];
    if (socketpair(family, type, protocol, fd) != 0) {
        LOG_ERROR(Lib_Net, "socketpair failed with {}", Common::GetLastErrorMsg());
        return -1;
    }

    auto fd1 = FDTable::Instance()->CreateHandle();
    auto fd2 = FDTable::Instance()->CreateHandle();
    auto* sock = FDTable::Instance()->GetFile(fd1);
    sock->is_opened = true;
    sock->type = Core::FileSys::FileType::Socket;
    sock->socket = std::make_shared<UnixSocket>(fd[0]);
    sock->m_guest_name = "anon_sock0";
    sock = FDTable::Instance()->GetFile(fd2);
    sock->is_opened = true;
    sock->type = Core::FileSys::FileType::Socket;
    sock->socket = std::make_shared<UnixSocket>(fd[1]);
    sock->m_guest_name = "anon_sock1";
    sv[0] = fd1;
    sv[1] = fd2;
    return 0;
}

int PS4_SYSV_ABI sys_netabort(OrbisNetId s, int flags) {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return -1;
}

int PS4_SYSV_ABI sys_socketclose(OrbisNetId s) {
    auto file = FDTable::Instance()->GetSocket(s);
    if (!file) {
        *Libraries::Kernel::__Error() = ORBIS_NET_EBADF;
        LOG_ERROR(Lib_Net, "socket id is invalid = {}", s);
        return -1;
    }
    LOG_DEBUG(Lib_Net, "s = {} ({})", s, file->m_guest_name);
    int returncode = file->socket->Close();
    if (returncode >= 0) {
        FDTable::Instance()->DeleteHandle(s);
        return returncode;
    }
    LOG_ERROR(Lib_Net, "error code returned: {}", (u32)*Libraries::Kernel::__Error());
    return -1;
}

int PS4_SYSV_ABI sys_send(OrbisNetId s, const void* buf, u64 len, int flags) {
    return sys_sendto(s, buf, len, flags, nullptr, 0);
}

int PS4_SYSV_ABI sys_sendto(OrbisNetId s, const void* buf, u64 len, int flags,
                            const OrbisNetSockaddr* addr, u32 addrlen) {
    auto file = FDTable::Instance()->GetSocket(s);
    if (!file) {
        *Libraries::Kernel::__Error() = ORBIS_NET_EBADF;
        LOG_ERROR(Lib_Net, "socket id is invalid = {}", s);
        return -1;
    }
    LOG_DEBUG(Lib_Net, "s = {} ({}), len = {}, flags = {:#x}", s, file->m_guest_name, len, flags);
    int returncode = file->socket->SendPacket(buf, len, flags, addr, addrlen);
    if (returncode >= 0) {
        return returncode;
    }
    LOG_ERROR(Lib_Net, "error code returned: {}", (u32)*Libraries::Kernel::__Error());
    return -1;
}

int PS4_SYSV_ABI sys_sendmsg(OrbisNetId s, const OrbisNetMsghdr* msg, int flags) {
    auto file = FDTable::Instance()->GetSocket(s);
    if (!file) {
        *Libraries::Kernel::__Error() = ORBIS_NET_EBADF;
        LOG_ERROR(Lib_Net, "socket id is invalid = {}", s);
        return -1;
    }
    LOG_DEBUG(Lib_Net, "s = {} ({}), flags = {:#x}", s, file->m_guest_name, flags);
    int returncode = file->socket->SendMessage(msg, flags);
    if (returncode >= 0) {
        return returncode;
    }
    LOG_ERROR(Lib_Net, "error code returned: {}", (u32)*Libraries::Kernel::__Error());
    return -1;
}

s64 PS4_SYSV_ABI sys_recv(OrbisNetId s, void* buf, u64 len, int flags) {
    return sys_recvfrom(s, buf, len, flags, nullptr, nullptr);
}

s64 PS4_SYSV_ABI sys_recvfrom(OrbisNetId s, void* buf, u64 len, int flags, OrbisNetSockaddr* addr,
                              u32* paddrlen) {
    auto file = FDTable::Instance()->GetSocket(s);
    if (!file) {
        *Libraries::Kernel::__Error() = ORBIS_NET_EBADF;
        LOG_ERROR(Lib_Net, "socket id is invalid = {}", s);
        return -1;
    }
    LOG_DEBUG(Lib_Net, "s = {} ({}), len = {}, flags = {:#x}", s, file->m_guest_name, len, flags);
    int returncode = file->socket->ReceivePacket(buf, len, flags, addr, paddrlen);
    if (returncode >= 0) {
        return returncode;
    }
    LOG_ERROR(Lib_Net, "s = {} ({}) returned error code: {}", s, file->m_guest_name,
              (u32)*Libraries::Kernel::__Error());
    return -1;
}

s64 PS4_SYSV_ABI sys_recvmsg(OrbisNetId s, OrbisNetMsghdr* msg, int flags) {
    auto file = FDTable::Instance()->GetSocket(s);
    if (!file) {
        *Libraries::Kernel::__Error() = ORBIS_NET_EBADF;
        LOG_ERROR(Lib_Net, "socket id is invalid = {}", s);
        return -1;
    }
    LOG_DEBUG(Lib_Net, "s = {} ({}), flags = {:#x}", s, file->m_guest_name, flags);
    int returncode = file->socket->ReceiveMessage(msg, flags);
    if (returncode >= 0) {
        return returncode;
    }
    LOG_ERROR(Lib_Net, "s = {} ({}) returned error code: {}", s, file->m_guest_name,
              (u32)*Libraries::Kernel::__Error());
    return -1;
}

int PS4_SYSV_ABI sys_htons(u16 v) {
    return ((v << 8) & 0xff00) | ((v >> 8) & 0x00ff);
}

int PS4_SYSV_ABI sys_htonl(u32 v) {
    return (v << 24) | ((v & 0xff00) << 8) | ((v >> 8) & 0xff00) | (v >> 24);
}

} // namespace Libraries::Net
