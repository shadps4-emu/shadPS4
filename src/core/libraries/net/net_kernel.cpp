// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>
#include <source_location>
#include <vector>

#include "common/logging/log.h"
#include "common/singleton.h"
#include "core/emulator_settings.h"
#include "core/file_sys/fs.h"
#include "core/libraries/kernel/file_system.h"
#include "core/libraries/kernel/kernel.h"
#include "core/libraries/net/net.h"
#include "core/libraries/net/net_error.h"
#include "core/libraries/net/net_kernel.h"
#include "core/libraries/net/net_log.h"
#include "core/libraries/net/net_resolver.h"
#include "core/libraries/net/net_translate.h"
#include "core/libraries/net/net_upnp.h"
#include "core/libraries/net/net_util.h"
#include "core/net/guest_net.h"

namespace Libraries::Net {

namespace {

using FDTable = Common::Singleton<Core::FileSys::HandleTable>;
using Core::Net::Host::Error;

s64 Posix(s64 result) {
    if (result < 0) {
        *Libraries::Kernel::__Error() = *sceNetErrnoLoc();
        return -1;
    }
    return result;
}

class KeepNetErrno {
public:
    KeepNetErrno() : saved_{*sceNetErrnoLoc()} {}
    ~KeepNetErrno() {
        *sceNetErrnoLoc() = saved_;
    }
    KeepNetErrno(const KeepNetErrno&) = delete;
    KeepNetErrno& operator=(const KeepNetErrno&) = delete;

private:
    s32 saved_;
};

s32 PosixError(int orbis_errno,
               const std::source_location where = std::source_location::current()) {
    *Libraries::Kernel::__Error() = orbis_errno;
    LogFailure(orbis_errno, where);
    return -1;
}

bool IsNetObject(s32 fd) {
    return Core::Net::GetObjectKind(fd).has_value();
}

} // namespace

void InstallKernelIntegration() {
    // Every net object is a Socket entry, Core::Net knows the real kind.
    Core::Net::SetIdAllocator({
        .allocate =
            [] {
                auto* table = FDTable::Instance();
                const int fd = table->CreateHandle();
                auto* file = table->GetFile(fd);
                file->type = Core::FileSys::FileType::Socket;
                file->m_guest_name = "net";
                file->is_opened = true;
                return fd;
            },
        .release = [](s32 fd) { FDTable::Instance()->DeleteHandle(fd); },
    });
    SetKernelErrnoHook([](int orbis_errno) { *Libraries::Kernel::__Error() = orbis_errno; });
    SetSystemHooks({
        .is_online = [] { return EmulatorSettings.IsConnectedToNetwork(); },
        .mac_address =
            [](std::array<u8, 6>* mac) {
                auto* netinfo = Common::Singleton<NetUtil::NetUtilInternal>::Instance();
                if (!netinfo->RetrieveEthernetAddr()) {
                    return false;
                }
                *mac = netinfo->GetEthernetAddr();
                return true;
            },
        .p2p_port =
            [] {
                const int port = EmulatorSettings.GetP2PPort();
                if (port < 0 || port > 0xffff) {
                    LOG_WARNING(Lib_Net, "p2p_port {} is not a UDP port, using 3658", port);
                    return u16{3658};
                }
                return static_cast<u16>(port);
            },
        .p2p_started =
            [](u16 port) {
                if (!EmulatorSettings.IsConnectedToNetwork() || !EmulatorSettings.IsUPnPEnabled()) {
                    LOG_INFO(Lib_Net, "P2P on UDP port {}: UPnP forwarding off", port);
                    return;
                }
                LOG_INFO(Lib_Net, "P2P on UDP port {}: asking the router to forward it (UPnP)",
                         port);
                auto& upnp = UPnPClient::Instance();
                upnp.Start(); // no-op if NP already started it
                upnp.MapPortAsync(port);
            },
        .p2p_stopped =
            [](u16 port) {
                LOG_DEBUG(Lib_Net, "P2P on UDP port {} stopped: removing any UPnP mapping", port);
                UPnPClient::Instance().UnmapPort(port);
            },
        .public_addr =
            [] {
                // Prefer the router's answer over the NP server's.
                if (const u32 upnp = UPnPClient::Instance().GetExternalIp(); upnp != 0) {
                    return upnp;
                }
                return Common::Singleton<NetUtil::NetUtilInternal>::Instance()->GetExternalIp();
            },
    });
}

s32 KernelClose(s32 fd) {
    const auto kind = Core::Net::GetObjectKind(fd);
    LOG_DEBUG(Lib_Net, "close({}): {}", fd,
              !kind                                       ? "not a net object"
              : *kind == Core::Net::NetObjectKind::Socket ? "socket"
              : *kind == Core::Net::NetObjectKind::Epoll  ? "epoll"
                                                          : "resolver");
    if (!kind) {
        return PosixError(ORBIS_NET_EBADF);
    }
    if (*kind == Core::Net::NetObjectKind::External) {
        const int e = DestroyResolver(fd);
        if (e != 0) {
            return PosixError(e);
        }
        ReleaseResolverPool(fd);
        return 0;
    }
    const auto r = Core::Net::CloseObject(fd);
    if (r.error == Error::Ok && *kind == Core::Net::NetObjectKind::Socket) {
        LogSocketClosed(fd);
    }
    return r.error == Error::Ok ? 0 : PosixError(ToOrbisErrno(r.error));
}

s64 KernelRead(s32 fd, void* buf, u64 nbytes) {
    const KeepNetErrno keep;
    if (Core::Net::GetObjectKind(fd) != Core::Net::NetObjectKind::Socket) {
        return PosixError(ORBIS_NET_EBADF);
    }
    return Posix(sceNetRecvfrom(fd, buf, nbytes, 0, nullptr, nullptr));
}

s64 KernelWrite(s32 fd, const void* buf, u64 nbytes) {
    const KeepNetErrno keep;
    if (Core::Net::GetObjectKind(fd) != Core::Net::NetObjectKind::Socket) {
        return PosixError(ORBIS_NET_EBADF);
    }
    return Posix(sceNetSendto(fd, buf, nbytes, 0, nullptr, 0));
}

s32 KernelFstat(s32 fd, Libraries::Kernel::OrbisKernelStat* sb) {
    if (!sb) {
        return PosixError(ORBIS_NET_EFAULT);
    }
    if (!IsNetObject(fd)) {
        return PosixError(ORBIS_NET_EBADF);
    }
    std::memset(sb, 0, sizeof(*sb));
    sb->st_mode = 0140000u | 0777u;
    sb->st_nlink = 1;
    sb->st_blksize = 4096;
    sb->st_size = 0;
    return 0;
}

s64 KernelSelect(std::span<Core::Net::SelectEntry> entries, s64 timeout_us) {
    std::vector<Core::Net::SelectEntry> sockets;
    sockets.reserve(entries.size());
    for (auto& e : entries) {
        e.ready = 0;
        if (Core::Net::GetObjectKind(e.id) == Core::Net::NetObjectKind::Socket) {
            sockets.push_back(e);
        }
    }
    const auto r = Core::Net::SocketSelect(sockets, timeout_us);
    if (r.error != Error::Ok) {
        return PosixError(static_cast<int>(r.error));
    }
    for (const auto& s : sockets) {
        for (auto& e : entries) {
            if (e.id == s.id) {
                e.ready = s.ready;
            }
        }
    }
    return r.value;
}

// libkernel / libScePosix socket calls

int PS4_SYSV_ABI sys_connect(OrbisNetId s, const OrbisNetSockaddr* addr, u32 addrlen) {
    const KeepNetErrno keep;
    return static_cast<int>(Posix(sceNetConnect(s, addr, addrlen)));
}

int PS4_SYSV_ABI sys_bind(OrbisNetId s, const OrbisNetSockaddr* addr, u32 addrlen) {
    const KeepNetErrno keep;
    return static_cast<int>(Posix(sceNetBind(s, addr, addrlen)));
}

int PS4_SYSV_ABI sys_accept(OrbisNetId s, OrbisNetSockaddr* addr, u32* paddrlen) {
    const KeepNetErrno keep;
    return static_cast<int>(Posix(sceNetAccept(s, addr, paddrlen)));
}

int PS4_SYSV_ABI sys_getpeername(OrbisNetId s, OrbisNetSockaddr* addr, u32* paddrlen) {
    const KeepNetErrno keep;
    return static_cast<int>(Posix(sceNetGetpeername(s, addr, paddrlen)));
}

int PS4_SYSV_ABI sys_getsockname(OrbisNetId s, OrbisNetSockaddr* addr, u32* paddrlen) {
    const KeepNetErrno keep;
    return static_cast<int>(Posix(sceNetGetsockname(s, addr, paddrlen)));
}

int PS4_SYSV_ABI sys_getsockopt(OrbisNetId s, int level, int optname, void* optval, u32* optlen) {
    const KeepNetErrno keep;
    return static_cast<int>(Posix(sceNetGetsockopt(s, level, optname, optval, optlen)));
}

int PS4_SYSV_ABI sys_listen(OrbisNetId s, int backlog) {
    const KeepNetErrno keep;
    return static_cast<int>(Posix(sceNetListen(s, backlog)));
}

int PS4_SYSV_ABI sys_setsockopt(OrbisNetId s, int level, int optname, const void* optval,
                                u32 optlen) {
    const KeepNetErrno keep;
    return static_cast<int>(Posix(sceNetSetsockopt(s, level, optname, optval, optlen)));
}

int PS4_SYSV_ABI sys_shutdown(OrbisNetId s, int how) {
    const KeepNetErrno keep;
    return static_cast<int>(Posix(sceNetShutdown(s, how)));
}

int PS4_SYSV_ABI sys_socketex(const char* name, int family, int type, int protocol) {
    const KeepNetErrno keep;
    return static_cast<int>(Posix(sceNetSocket(name, family, type, protocol)));
}

int PS4_SYSV_ABI sys_socket(int family, int type, int protocol) {
    const KeepNetErrno keep;
    return sys_socketex(nullptr, family, type, protocol);
}

int PS4_SYSV_ABI sys_socketpair(int family, int type, int protocol, int sv[2]) {
    LOG_DEBUG(Lib_Net, "family = {}, type = {}, protocol = {}", family, type, protocol);
    if (sv == nullptr) {
        return PosixError(ORBIS_NET_EFAULT);
    }
    if (family != ORBIS_NET_AF_UNIX) {
        return PosixError(ORBIS_NET_EAFNOSUPPORT); // AF_UNIX only on FreeBSD
    }
    int error = 0;
    const auto kind = ToHostSocketKind(family, type, protocol, &error);
    if (!kind) {
        return PosixError(error);
    }
    s32 ids[2];
    const auto r = Core::Net::SocketCreatePair(kind->family, kind->type, kind->protocol, ids);
    if (r.error != Error::Ok) {
        return PosixError(ToOrbisErrno(r.error));
    }
    sv[0] = ids[0];
    sv[1] = ids[1];
    return 0;
}

int PS4_SYSV_ABI sys_netabort(OrbisNetId s, int flags) {
    const KeepNetErrno keep;
    return static_cast<int>(Posix(sceNetSocketAbort(s, flags)));
}

int PS4_SYSV_ABI sys_socketclose(OrbisNetId s) {
    const KeepNetErrno keep;
    return static_cast<int>(Posix(sceNetSocketClose(s)));
}

int PS4_SYSV_ABI sys_send(OrbisNetId s, const void* buf, u64 len, int flags) {
    const KeepNetErrno keep;
    return static_cast<int>(Posix(sceNetSend(s, buf, len, flags)));
}

int PS4_SYSV_ABI sys_sendto(OrbisNetId s, const void* buf, u64 len, int flags,
                            const OrbisNetSockaddr* addr, u32 addrlen) {
    const KeepNetErrno keep;
    return static_cast<int>(Posix(sceNetSendto(s, buf, len, flags, addr, addrlen)));
}

int PS4_SYSV_ABI sys_sendmsg(OrbisNetId s, const OrbisNetMsghdr* msg, int flags) {
    const KeepNetErrno keep;
    return static_cast<int>(Posix(sceNetSendmsg(s, msg, flags)));
}

s64 PS4_SYSV_ABI sys_recv(OrbisNetId s, void* buf, u64 len, int flags) {
    const KeepNetErrno keep;
    return Posix(sceNetRecv(s, buf, len, flags));
}

s64 PS4_SYSV_ABI sys_recvfrom(OrbisNetId s, void* buf, u64 len, int flags, OrbisNetSockaddr* addr,
                              u32* paddrlen) {
    const KeepNetErrno keep;
    return Posix(sceNetRecvfrom(s, buf, len, flags, addr, paddrlen));
}

s64 PS4_SYSV_ABI sys_recvmsg(OrbisNetId s, OrbisNetMsghdr* msg, int flags) {
    const KeepNetErrno keep;
    return Posix(sceNetRecvmsg(s, msg, flags));
}

int PS4_SYSV_ABI sys_htons(u16 v) {
    return sceNetHtons(v);
}

int PS4_SYSV_ABI sys_htonl(u32 v) {
    return static_cast<int>(sceNetHtonl(v));
}

} // namespace Libraries::Net
