#include "sys_net.h"
// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <common/assert.h>
#include <common/logging/log.h>
#include <core/libraries/kernel/kernel.h>
#include "common/singleton.h"
#include "net_error.h"
#include "sockets.h"
#include "sys_net.h"

namespace Libraries::Net {

int PS4_SYSV_ABI sys_connect(OrbisNetId s, const OrbisNetSockaddr* addr, u32 addrlen) {
    auto* netcall = Common::Singleton<NetInternal>::Instance();
    auto sock = netcall->FindSocket(s);
    if (!sock) {
        *Libraries::Kernel::__Error() = ORBIS_NET_ERROR_EBADF;
        LOG_ERROR(Lib_Net, "socket id is invalid = {}", s);
        return -1;
    }
    int returncode = sock->Connect(addr, addrlen);
    if (returncode >= 0) {
        return returncode;
    }
    *Libraries::Kernel::__Error() = returncode;
    LOG_ERROR(Lib_Net, "error code returned : {:#x}", (u32)returncode);
    return -1;
}
int PS4_SYSV_ABI sys_bind(OrbisNetId s, const OrbisNetSockaddr* addr, u32 addrlen) {
    auto* netcall = Common::Singleton<NetInternal>::Instance();
    auto sock = netcall->FindSocket(s);
    if (!sock) {
        *Libraries::Kernel::__Error() = ORBIS_NET_ERROR_EBADF;
        LOG_ERROR(Lib_Net, "socket id is invalid = {}", s);
        return -1;
    }
    int returncode = sock->Bind(addr, addrlen);
    if (returncode >= 0) {
        return returncode;
    }
    *Libraries::Kernel::__Error() = returncode;
    LOG_ERROR(Lib_Net, "error code returned : {:#x}", (u32)returncode);
    return -1;
}
int PS4_SYSV_ABI sys_accept(OrbisNetId s, OrbisNetSockaddr* addr, u32* paddrlen) {
    auto* netcall = Common::Singleton<NetInternal>::Instance();
    auto sock = netcall->FindSocket(s);
    if (!sock) {
        *Libraries::Kernel::__Error() = ORBIS_NET_EBADF;
        LOG_ERROR(Lib_Net, "socket id is invalid = {}", s);
        return -1;
    }
    auto new_sock = sock->Accept(addr, paddrlen);
    if (!new_sock) {
        *Libraries::Kernel::__Error() = ORBIS_NET_EBADF;
        LOG_ERROR(Lib_Net, "error creating new socket for accepting");
        return -1;
    }
    auto id = ++netcall->next_sock_id;
    netcall->socks.emplace(id, new_sock);
    return id;
}
int PS4_SYSV_ABI sys_getpeername(OrbisNetId s, const OrbisNetSockaddr* addr, u32* paddrlen) {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return -1;
}
int PS4_SYSV_ABI sys_getsockname(OrbisNetId s, OrbisNetSockaddr* addr, u32* paddrlen) {
    auto* netcall = Common::Singleton<NetInternal>::Instance();
    auto sock = netcall->FindSocket(s);
    if (!sock) {
        *Libraries::Kernel::__Error() = ORBIS_NET_ERROR_EBADF;
        LOG_ERROR(Lib_Net, "socket id is invalid = {}", s);
        return -1;
    }
    int returncode = sock->GetSocketAddress(addr, paddrlen);
    if (returncode >= 0) {
        return returncode;
    }
    *Libraries::Kernel::__Error() = returncode;
    LOG_ERROR(Lib_Net, "error code returned : {:#x}", (u32)returncode);
    return -1;
}
int PS4_SYSV_ABI sys_getsockopt(OrbisNetId s, int level, int optname, void* optval, u32* optlen) {
    auto* netcall = Common::Singleton<NetInternal>::Instance();
    auto sock = netcall->FindSocket(s);
    if (!sock) {
        *Libraries::Kernel::__Error() = ORBIS_NET_ERROR_EBADF;
        LOG_ERROR(Lib_Net, "socket id is invalid = {}", s);
        return -1;
    }
    int returncode = sock->GetSocketOptions(level, optname, optval, optlen);
    if (returncode >= 0) {
        return returncode;
    }
    *Libraries::Kernel::__Error() = returncode;
    LOG_ERROR(Lib_Net, "error code returned : {:#x}", (u32)returncode);
    return -1;
}
int PS4_SYSV_ABI sys_listen(OrbisNetId s, int backlog) {
    auto* netcall = Common::Singleton<NetInternal>::Instance();
    auto sock = netcall->FindSocket(s);
    if (!sock) {
        *Libraries::Kernel::__Error() = ORBIS_NET_ERROR_EBADF;
        LOG_ERROR(Lib_Net, "socket id is invalid = {}", s);
        return -1;
    }
    int returncode = sock->Listen(backlog);
    if (returncode >= 0) {
        return returncode;
    }
    *Libraries::Kernel::__Error() = returncode;
    LOG_ERROR(Lib_Net, "error code returned : {:#x}", (u32)returncode);
    return -1;
}
int PS4_SYSV_ABI sys_setsockopt(OrbisNetId s, int level, int optname, const void* optval,
                                u32 optlen) {
    auto* netcall = Common::Singleton<NetInternal>::Instance();
    auto sock = netcall->FindSocket(s);
    if (!sock) {
        *Libraries::Kernel::__Error() = ORBIS_NET_ERROR_EBADF;
        LOG_ERROR(Lib_Net, "socket id is invalid = {}", s);
        return -1;
    }
    int returncode = sock->SetSocketOptions(level, optname, optval, optlen);
    if (returncode >= 0) {
        return returncode;
    }
    *Libraries::Kernel::__Error() = returncode;
    LOG_ERROR(Lib_Net, "error code returned : {:#x}", (u32)returncode);
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
    SocketPtr sock;
    switch (type) {
    case ORBIS_NET_SOCK_STREAM:
    case ORBIS_NET_SOCK_DGRAM:
    case ORBIS_NET_SOCK_RAW:
        sock = std::make_shared<PosixSocket>(family, type, protocol);
        break;
    case ORBIS_NET_SOCK_DGRAM_P2P:
    case ORBIS_NET_SOCK_STREAM_P2P:
        sock = std::make_shared<P2PSocket>(family, type, protocol);
        break;
    default:
        UNREACHABLE_MSG("Unknown type {}", type);
    }
    auto* netcall = Common::Singleton<NetInternal>::Instance();
    auto id = ++netcall->next_sock_id;
    netcall->socks.emplace(id, sock);
    return id;
}
int PS4_SYSV_ABI sys_socket(int family, int type, int protocol) {
    return sys_socketex(nullptr, family, type, protocol);
}
int PS4_SYSV_ABI sys_netabort(OrbisNetId s, int flags) {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return -1;
}
int PS4_SYSV_ABI sys_socketclose(OrbisNetId s) {
    auto* netcall = Common::Singleton<NetInternal>::Instance();
    auto sock = netcall->FindSocket(s);
    if (!sock) {
        *Libraries::Kernel::__Error() = ORBIS_NET_ERROR_EBADF;
        LOG_ERROR(Lib_Net, "socket id is invalid = {}", s);
        return -1;
    }
    int returncode = sock->Close();
    if (returncode >= 0) {
        return returncode;
    }
    *Libraries::Kernel::__Error() = returncode;
    LOG_ERROR(Lib_Net, "error code returned : {:#x}", (u32)returncode);
    return -1;
}
int PS4_SYSV_ABI sys_sendto(OrbisNetId s, const void* buf, u64 len, int flags,
                            const OrbisNetSockaddr* addr, u32 addrlen) {
    auto* netcall = Common::Singleton<NetInternal>::Instance();
    auto sock = netcall->FindSocket(s);
    if (!sock) {
        *Libraries::Kernel::__Error() = ORBIS_NET_ERROR_EBADF;
        LOG_ERROR(Lib_Net, "socket id is invalid = {}", s);
        return -1;
    }
    int returncode = sock->SendPacket(buf, len, flags, addr, addrlen);
    if (returncode >= 0) {
        return returncode;
    }
    *Libraries::Kernel::__Error() = returncode;
    LOG_ERROR(Lib_Net, "error code returned : {:#x}", (u32)returncode);
    return -1;
}
int PS4_SYSV_ABI sys_sendmsg(OrbisNetId s, const OrbisNetMsghdr* msg, int flags) {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return -1;
}
int PS4_SYSV_ABI sys_recvfrom(OrbisNetId s, void* buf, u64 len, int flags, OrbisNetSockaddr* addr,
                              u32* paddrlen) {
    auto* netcall = Common::Singleton<NetInternal>::Instance();
    auto sock = netcall->FindSocket(s);
    if (!sock) {
        *Libraries::Kernel::__Error() = ORBIS_NET_ERROR_EBADF;
        LOG_ERROR(Lib_Net, "socket id is invalid = {}", s);
        return -1;
    }
    int returncode = sock->ReceivePacket(buf, len, flags, addr, paddrlen);
    if (returncode >= 0) {
        return returncode;
    }
    *Libraries::Kernel::__Error() = returncode;
    LOG_ERROR(Lib_Net, "error code returned : {:#x}", (u32)returncode);
    return -1;
}
int PS4_SYSV_ABI sys_recvmsg(OrbisNetId s, OrbisNetMsghdr* msg, int flags) {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return -1;
}
} // namespace Libraries::Net