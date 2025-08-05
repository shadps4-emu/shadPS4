// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <common/assert.h>
#include <common/logging/log.h>
#include <core/libraries/kernel/kernel.h>
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
    int returncode = file->socket->Connect(addr, addrlen);
    if (returncode >= 0) {
        return returncode;
    }
    LOG_ERROR(Lib_Net, "error code returned : {:#x}", (u32)*Libraries::Kernel::__Error());
    return -1;
}

int PS4_SYSV_ABI sys_bind(OrbisNetId s, const OrbisNetSockaddr* addr, u32 addrlen) {
    auto file = FDTable::Instance()->GetSocket(s);
    if (!file) {
        *Libraries::Kernel::__Error() = ORBIS_NET_EBADF;
        LOG_ERROR(Lib_Net, "socket id is invalid = {}", s);
        return -1;
    }
    int returncode = file->socket->Bind(addr, addrlen);
    if (returncode >= 0) {
        return returncode;
    }
    LOG_ERROR(Lib_Net, "error code returned : {:#x}", (u32)*Libraries::Kernel::__Error());
    return -1;
}

int PS4_SYSV_ABI sys_accept(OrbisNetId s, OrbisNetSockaddr* addr, u32* paddrlen) {
    auto file = FDTable::Instance()->GetSocket(s);
    if (!file) {
        *Libraries::Kernel::__Error() = ORBIS_NET_EBADF;
        LOG_ERROR(Lib_Net, "socket id is invalid = {}", s);
        return -1;
    }
    auto new_sock = file->socket->Accept(addr, paddrlen);
    if (!new_sock) {
        LOG_ERROR(Lib_Net, "error creating new socket for accepting: {:#x}",
                  (u32)*Libraries::Kernel::__Error());
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
    LOG_INFO(Lib_Net, "s = {}", s);

    auto file = FDTable::Instance()->GetSocket(s);
    if (!file) {
        *Libraries::Kernel::__Error() = ORBIS_NET_EBADF;
        LOG_ERROR(Lib_Net, "socket id is invalid = {}", s);
        return -1;
    }
    int returncode = file->socket->GetPeerName(addr, paddrlen);
    if (returncode >= 0) {
        return returncode;
    }
    LOG_ERROR(Lib_Net, "error code returned : {:#x}", (u32)*Libraries::Kernel::__Error());
    return -1;
}

int PS4_SYSV_ABI sys_getsockname(OrbisNetId s, OrbisNetSockaddr* addr, u32* paddrlen) {
    auto file = FDTable::Instance()->GetSocket(s);
    if (!file) {
        *Libraries::Kernel::__Error() = ORBIS_NET_EBADF;
        LOG_ERROR(Lib_Net, "socket id is invalid = {}", s);
        return -1;
    }
    int returncode = file->socket->GetSocketAddress(addr, paddrlen);
    if (returncode >= 0) {
        return returncode;
    }
    LOG_ERROR(Lib_Net, "error code returned : {:#x}", (u32)*Libraries::Kernel::__Error());
    return -1;
}

int PS4_SYSV_ABI sys_getsockopt(OrbisNetId s, int level, int optname, void* optval, u32* optlen) {
    auto file = FDTable::Instance()->GetSocket(s);
    if (!file) {
        *Libraries::Kernel::__Error() = ORBIS_NET_EBADF;
        LOG_ERROR(Lib_Net, "socket id is invalid = {}", s);
        return -1;
    }
    int returncode = file->socket->GetSocketOptions(level, optname, optval, optlen);
    if (returncode >= 0) {
        return returncode;
    }
    LOG_ERROR(Lib_Net, "error code returned : {:#x}", (u32)*Libraries::Kernel::__Error());
    return -1;
}

int PS4_SYSV_ABI sys_listen(OrbisNetId s, int backlog) {
    auto file = FDTable::Instance()->GetSocket(s);
    if (!file) {
        *Libraries::Kernel::__Error() = ORBIS_NET_EBADF;
        LOG_ERROR(Lib_Net, "socket id is invalid = {}", s);
        return -1;
    }
    int returncode = file->socket->Listen(backlog);
    if (returncode >= 0) {
        return returncode;
    }
    LOG_ERROR(Lib_Net, "error code returned : {:#x}", (u32)*Libraries::Kernel::__Error());
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
    int returncode = file->socket->SetSocketOptions(level, optname, optval, optlen);
    if (returncode >= 0) {
        return returncode;
    }
    LOG_ERROR(Lib_Net, "error code returned : {:#x}", (u32)*Libraries::Kernel::__Error());
    return -1;
}

int PS4_SYSV_ABI sys_shutdown(OrbisNetId s, int how) {
    return -1;
}

int PS4_SYSV_ABI sys_socketex(const char* name, int family, int type, int protocol) {
    if (name == nullptr) {
        LOG_INFO(Lib_Net, "name = no-named family = {} type = {} protocol = {}", family, type,
                 protocol);
    } else {
        LOG_INFO(Lib_Net, "name = {} family = {} type = {} protocol = {}", std::string(name),
                 family, type, protocol);
    }
    SocketPtr socket;
    switch (type) {
    case ORBIS_NET_SOCK_STREAM:
    case ORBIS_NET_SOCK_DGRAM:
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
    sock->m_guest_name = name ? name : "anon_sock";
    return fd;
}

int PS4_SYSV_ABI sys_socket(int family, int type, int protocol) {
    return sys_socketex(nullptr, family, type, protocol);
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
    int returncode = file->socket->Close();
    if (returncode >= 0) {
        return returncode;
    }
    LOG_ERROR(Lib_Net, "error code returned : {:#x}", (u32)*Libraries::Kernel::__Error());
    return -1;
}

int PS4_SYSV_ABI sys_sendto(OrbisNetId s, const void* buf, u64 len, int flags,
                            const OrbisNetSockaddr* addr, u32 addrlen) {
    auto file = FDTable::Instance()->GetSocket(s);
    if (!file) {
        *Libraries::Kernel::__Error() = ORBIS_NET_EBADF;
        LOG_ERROR(Lib_Net, "socket id is invalid = {}", s);
        return -1;
    }
    int returncode = file->socket->SendPacket(buf, len, flags, addr, addrlen);
    if (returncode >= 0) {
        return returncode;
    }
    LOG_ERROR(Lib_Net, "error code returned : {:#x}", (u32)*Libraries::Kernel::__Error());
    return -1;
}

int PS4_SYSV_ABI sys_sendmsg(OrbisNetId s, const OrbisNetMsghdr* msg, int flags) {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return -1;
}

s64 PS4_SYSV_ABI sys_recvfrom(OrbisNetId s, void* buf, u64 len, int flags, OrbisNetSockaddr* addr,
                              u32* paddrlen) {
    auto file = FDTable::Instance()->GetSocket(s);
    if (!file) {
        *Libraries::Kernel::__Error() = ORBIS_NET_EBADF;
        LOG_ERROR(Lib_Net, "socket id is invalid = {}", s);
        return -1;
    }
    int returncode = file->socket->ReceivePacket(buf, len, flags, addr, paddrlen);
    if (returncode >= 0) {
        return returncode;
    }
    LOG_ERROR(Lib_Net, "error code returned : {:#x}", (u32)*Libraries::Kernel::__Error());
    return -1;
}

s64 PS4_SYSV_ABI sys_recvmsg(OrbisNetId s, OrbisNetMsghdr* msg, int flags) {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return -1;
}

int PS4_SYSV_ABI sys_htons(u16 v) {
    return ((v << 8) & 0xff00) | ((v >> 8) & 0x00ff);
}

int PS4_SYSV_ABI sys_htonl(u32 v) {
    return (v << 24) | ((v & 0xff00) << 8) | ((v >> 8) & 0xff00) | (v >> 24);
}

} // namespace Libraries::Net
