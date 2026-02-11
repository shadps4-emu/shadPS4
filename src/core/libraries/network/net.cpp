// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#ifdef WIN32
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <Ws2tcpip.h>
#include <iphlpapi.h>
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

#include <core/libraries/kernel/kernel.h>
#include <magic_enum/magic_enum.hpp>
#include "common/assert.h"
#include "common/error.h"
#include "common/logging/log.h"
#include "common/singleton.h"
#include "core/file_sys/fs.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/network/net.h"
#include "net_epoll.h"
#include "net_error.h"
#include "net_resolver.h"
#include "net_util.h"
#include "netctl.h"
#include "sockets.h"
#include "sys_net.h"

namespace Libraries::Net {

using FDTable = Common::Singleton<Core::FileSys::HandleTable>;

static thread_local int32_t net_errno = 0;

static bool g_isNetInitialized = true; // TODO init it properly

static int ConvertFamilies(int family) {
    switch (family) {
    case ORBIS_NET_AF_INET:
        return AF_INET;
    case ORBIS_NET_AF_INET6:
        return AF_INET6;
    default:
        UNREACHABLE_MSG("unsupported socket family {}", family);
    }
}

auto NetErrorHandler(auto f) -> decltype(f()) {
    auto result = 0;
    int err;
    int positiveErr;

    do {
        result = f();

        if (result >= 0) {
            return result; // Success
        }

        err = *Libraries::Kernel::__Error(); // Standard errno

        // Convert to positive error for comparison
        positiveErr = (err < 0) ? -err : err;

        if ((positiveErr & 0xfff0000) != 0) {
            // Unknown/fatal error range
            *sceNetErrnoLoc() = ORBIS_NET_ERETURN;
            return -positiveErr;
        }

        // Retry if interrupted
    } while (positiveErr == ORBIS_NET_EINTR);

    if (positiveErr == ORBIS_NET_ENOTSOCK) {
        result = -ORBIS_NET_EBADF;
    } else if (positiveErr == ORBIS_NET_ENETINTR) {
        result = -ORBIS_NET_EINTR;
    } else {
        result = -positiveErr;
    }

    *sceNetErrnoLoc() = -result;

    return (-result) | ORBIS_NET_ERROR_BASE; // Convert to official ORBIS_NET_ERROR code
}

int PS4_SYSV_ABI in6addr_any() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI in6addr_loopback() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sce_net_dummy() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sce_net_in6addr_any() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sce_net_in6addr_linklocal_allnodes() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sce_net_in6addr_linklocal_allrouters() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sce_net_in6addr_loopback() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sce_net_in6addr_nodelocal_allnodes() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

OrbisNetId PS4_SYSV_ABI sceNetAccept(OrbisNetId s, OrbisNetSockaddr* addr, u32* paddrlen) {
    if (!g_isNetInitialized) {
        return ORBIS_NET_ERROR_ENOTINIT;
    }

    return NetErrorHandler([&] { return sys_accept(s, addr, paddrlen); });
}

int PS4_SYSV_ABI sceNetAddrConfig6GetInfo() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetAddrConfig6Start() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetAddrConfig6Stop() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetAllocateAllRouteInfo() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetBandwidthControlGetDataTraffic() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetBandwidthControlGetDefaultParam() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetBandwidthControlGetIfParam() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetBandwidthControlGetPolicy() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetBandwidthControlSetDefaultParam() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetBandwidthControlSetIfParam() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetBandwidthControlSetPolicy() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetBind(OrbisNetId s, const OrbisNetSockaddr* addr, u32 addrlen) {
    if (!g_isNetInitialized) {
        return ORBIS_NET_ERROR_ENOTINIT;
    }

    return NetErrorHandler([&] { return sys_bind(s, addr, addrlen); });
}

int PS4_SYSV_ABI sceNetClearDnsCache() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigAddArp() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigAddArpWithInterface() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigAddIfaddr() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigAddMRoute() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigAddRoute() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigAddRoute6() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigAddRouteWithInterface() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigCleanUpAllInterfaces() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigDelArp() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigDelArpWithInterface() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigDelDefaultRoute() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigDelDefaultRoute6() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigDelIfaddr() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigDelIfaddr6() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigDelMRoute() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigDelRoute() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigDelRoute6() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigDownInterface() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigEtherGetLinkMode() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigEtherPostPlugInOutEvent() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigEtherSetLinkMode() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigFlushRoute() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigGetDefaultRoute() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigGetDefaultRoute6() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigGetIfaddr() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigGetIfaddr6() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigRoutingShowRoutingConfig() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigRoutingShowtCtlVar() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigRoutingStart() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigRoutingStop() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigSetDefaultRoute() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigSetDefaultRoute6() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigSetDefaultScope() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigSetIfaddr() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigSetIfaddr6() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigSetIfaddr6WithFlags() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigSetIfFlags() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigSetIfLinkLocalAddr6() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigSetIfmtu() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigUnsetIfFlags() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigUpInterface() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigUpInterfaceWithFlags() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigWlanAdhocClearWakeOnWlan() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigWlanAdhocCreate() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigWlanAdhocGetWakeOnWlanInfo() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigWlanAdhocJoin() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigWlanAdhocLeave() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigWlanAdhocPspEmuClearWakeOnWlan() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigWlanAdhocPspEmuGetWakeOnWlanInfo() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigWlanAdhocPspEmuSetWakeOnWlan() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigWlanAdhocScanJoin() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigWlanAdhocSetExtInfoElement() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigWlanAdhocSetWakeOnWlan() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigWlanApStart() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigWlanApStop() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigWlanBackgroundScanQuery() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigWlanBackgroundScanStart() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigWlanBackgroundScanStop() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigWlanDiagGetDeviceInfo() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigWlanDiagSetAntenna() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigWlanDiagSetTxFixedRate() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigWlanGetDeviceConfig() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigWlanInfraGetRssiInfo() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigWlanInfraLeave() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigWlanInfraScanJoin() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigWlanScan() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConfigWlanSetDeviceConfig() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetConnect(OrbisNetId s, const OrbisNetSockaddr* addr, u32 addrlen) {
    if (!g_isNetInitialized) {
        return ORBIS_NET_ERROR_ENOTINIT;
    }

    return NetErrorHandler([&] { return sys_connect(s, addr, addrlen); });
}

int PS4_SYSV_ABI sceNetControl() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetDhcpdStart() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetDhcpdStop() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetDhcpGetAutoipInfo() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetDhcpGetInfo() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetDhcpGetInfoEx() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetDhcpStart() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetDhcpStop() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetDumpAbort() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetDumpCreate() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetDumpDestroy() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetDumpRead() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetDuplicateIpStart() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetDuplicateIpStop() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetEpollAbort() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetEpollControl(OrbisNetId epollid, OrbisNetEpollFlag op, OrbisNetId id,
                                    OrbisNetEpollEvent* event) {
    auto file = FDTable::Instance()->GetEpoll(epollid);
    if (!file) {
        *sceNetErrnoLoc() = ORBIS_NET_EBADF;
        return ORBIS_NET_ERROR_EBADF;
    }
    auto epoll = file->epoll;
    LOG_WARNING(Lib_Net, "called, epollid = {} ({}), op = {}, id = {}", epollid, epoll->name,
                magic_enum::enum_name(op), id);

    auto find_id = [&](OrbisNetId id) {
        return std::ranges::find_if(epoll->events, [&](auto& el) { return el.first == id; });
    };

    switch (op) {
    case ORBIS_NET_EPOLL_CTL_ADD: {
        if (event == nullptr) {
            *sceNetErrnoLoc() = ORBIS_NET_EINVAL;
            return ORBIS_NET_ERROR_EINVAL;
        }
        if (find_id(id) != epoll->events.end()) {
            *sceNetErrnoLoc() = ORBIS_NET_EEXIST;
            return ORBIS_NET_ERROR_EEXIST;
        }

        auto file = FDTable::Instance()->GetFile(id);
        if (!file) {
            *sceNetErrnoLoc() = ORBIS_NET_EBADF;
            LOG_ERROR(Lib_Net, "file id is invalid = {}", id);
            return ORBIS_NET_ERROR_EBADF;
        }

        switch (file->type) {
        case Core::FileSys::FileType::Socket: {
            auto native_handle = file->socket->Native();
            if (!native_handle) {
                // P2P socket, cannot be added to epoll
                LOG_ERROR(Lib_Net, "P2P socket cannot be added to epoll (unimplemented)");
                *sceNetErrnoLoc() = ORBIS_NET_EBADF;
                return ORBIS_NET_ERROR_EBADF;
            }

            epoll_event native_event = {.events = ConvertEpollEventsIn(event->events),
                                        .data = {.fd = id}};
            ASSERT(epoll_ctl(epoll->epoll_fd, EPOLL_CTL_ADD, *native_handle, &native_event) == 0);
            epoll->events.emplace_back(id, *event);
            break;
        }
        case Core::FileSys::FileType::Resolver: {
            epoll->async_resolutions.emplace_back(id);
            epoll->events.emplace_back(id, *event);
            break;
        }
        default: {
            LOG_ERROR(Lib_Net, "file type {} ({}) passed", magic_enum::enum_name(file->type.load()),
                      file->m_guest_name);
            break;
        }
        }
        break;
    }
    case ORBIS_NET_EPOLL_CTL_MOD: {
        if (event == nullptr) {
            *sceNetErrnoLoc() = ORBIS_NET_EINVAL;
            return ORBIS_NET_ERROR_EINVAL;
        }

        const auto it = find_id(id);
        if (it == epoll->events.end()) {
            *sceNetErrnoLoc() = ORBIS_NET_EBADF;
            return ORBIS_NET_ERROR_EBADF;
        }

        auto file = FDTable::Instance()->GetFile(id);
        if (!file) {
            *sceNetErrnoLoc() = ORBIS_NET_EBADF;
            LOG_ERROR(Lib_Net, "file id is invalid = {}", id);
            return ORBIS_NET_ERROR_EBADF;
        }

        switch (file->type) {
        case Core::FileSys::FileType::Socket: {
            auto native_handle = file->socket->Native();
            if (!native_handle) {
                // P2P socket, cannot be modified in epoll
                LOG_ERROR(Lib_Net, "P2P socket cannot be modified in epoll (unimplemented)");
                *sceNetErrnoLoc() = ORBIS_NET_EBADF;
                return ORBIS_NET_ERROR_EBADF;
            }

            epoll_event native_event = {.events = ConvertEpollEventsIn(event->events),
                                        .data = {.fd = id}};
            ASSERT(epoll_ctl(epoll->epoll_fd, EPOLL_CTL_MOD, *native_handle, &native_event) == 0);
            *it = {id, *event};
            break;
        }
        default:
            LOG_ERROR(Lib_Net, "file type {} ({}) passed", magic_enum::enum_name(file->type.load()),
                      file->m_guest_name);
            break;
        }
        break;
    }
    case ORBIS_NET_EPOLL_CTL_DEL: {
        if (event != nullptr) {
            *sceNetErrnoLoc() = ORBIS_NET_EINVAL;
            return ORBIS_NET_ERROR_EINVAL;
        }

        auto it = find_id(id);
        if (it == epoll->events.end()) {
            *sceNetErrnoLoc() = ORBIS_NET_EBADF;
            return ORBIS_NET_ERROR_EBADF;
        }

        auto file = FDTable::Instance()->GetFile(id);
        if (!file) {
            *sceNetErrnoLoc() = ORBIS_NET_EBADF;
            LOG_ERROR(Lib_Net, "file id is invalid = {}", id);
            return ORBIS_NET_ERROR_EBADF;
        }

        switch (file->type) {
        case Core::FileSys::FileType::Socket: {
            auto native_handle = file->socket->Native();
            if (!native_handle) {
                // P2P socket, cannot be removed from epoll
                LOG_ERROR(Lib_Net, "P2P socket cannot be removed from epoll (unimplemented)");
                *sceNetErrnoLoc() = ORBIS_NET_EBADF;
                return ORBIS_NET_ERROR_EBADF;
            }

            ASSERT(epoll_ctl(epoll->epoll_fd, EPOLL_CTL_DEL, *native_handle, nullptr) == 0);
            epoll->events.erase(it);
            break;
        }
        case Core::FileSys::FileType::Resolver: {
            std::erase(epoll->async_resolutions, id);
            epoll->events.erase(it);
            break;
        }
        default:
            LOG_ERROR(Lib_Net, "file type {} ({}) passed", magic_enum::enum_name(file->type.load()),
                      file->m_guest_name);
            break;
        }
        break;
    }
    default:
        *sceNetErrnoLoc() = ORBIS_NET_EINVAL;
        return ORBIS_NET_ERROR_EINVAL;
    }

    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetEpollCreate(const char* name, int flags) {
    LOG_INFO(Lib_Net, "called, name = {}, flags = {}", name, flags);
    if (flags != 0) {
        *sceNetErrnoLoc() = ORBIS_NET_EINVAL;
        return ORBIS_NET_ERROR_EINVAL;
    }

    auto fd = FDTable::Instance()->CreateHandle();
    auto* epoll = FDTable::Instance()->GetFile(fd);
    epoll->is_opened = true;
    epoll->type = Core::FileSys::FileType::Epoll;
    epoll->epoll = std::make_shared<Epoll>(name);
    epoll->m_guest_name = name;
    return fd;
}

int PS4_SYSV_ABI sceNetEpollDestroy(OrbisNetId epollid) {
    auto file = FDTable::Instance()->GetEpoll(epollid);
    if (!file) {
        *sceNetErrnoLoc() = ORBIS_NET_EBADF;
        return ORBIS_NET_ERROR_EBADF;
    }

    LOG_DEBUG(Lib_Net, "called, epollid = {} ({})", epollid, file->epoll->name);

    file->epoll->Destroy();
    FDTable::Instance()->DeleteHandle(epollid);

    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetEpollWait(OrbisNetId epollid, OrbisNetEpollEvent* events, int maxevents,
                                 int timeout) {
    auto file = FDTable::Instance()->GetEpoll(epollid);
    if (!file) {
        *sceNetErrnoLoc() = ORBIS_NET_EBADF;
        return ORBIS_NET_ERROR_EBADF;
    }
    auto epoll = file->epoll;
    LOG_DEBUG(Lib_Net, "called, epollid = {} ({}), maxevents = {}, timeout = {}", epollid,
              epoll->name, maxevents, timeout);

    int sockets_waited_on = (epoll->events.size() - epoll->async_resolutions.size()) > 0;

    std::vector<epoll_event> native_events{static_cast<size_t>(maxevents)};
    int result = ORBIS_OK;
    if (sockets_waited_on) {
#ifdef __linux__
        const timespec epoll_timeout{.tv_sec = timeout / 1000000,
                                     .tv_nsec = (timeout % 1000000) * 1000};
        result = epoll_pwait2(epoll->epoll_fd, native_events.data(), maxevents,
                              timeout < 0 ? nullptr : &epoll_timeout, nullptr);
#else
        result = epoll_wait(epoll->epoll_fd, native_events.data(), maxevents,
                            timeout < 0 ? timeout : timeout / 1000);
#endif
    }

    int i = 0;

    if (result < 0) {
        LOG_ERROR(Lib_Net, "epoll_wait failed with {}", Common::GetLastErrorMsg());
        switch (errno) {
        case EINTR:
            *sceNetErrnoLoc() = ORBIS_NET_EINTR;
            return ORBIS_NET_ERROR_EINTR;
        case EINVAL:
            *sceNetErrnoLoc() = ORBIS_NET_EINVAL;
            return ORBIS_NET_ERROR_EINVAL;
        case EBADF:
            *sceNetErrnoLoc() = ORBIS_NET_EBADF;
            return ORBIS_NET_ERROR_EBADF;
        default:
            *sceNetErrnoLoc() = ORBIS_NET_EINTERNAL;
            return ORBIS_NET_ERROR_EINTERNAL;
        }
    } else if (result == 0) {
        LOG_TRACE(Lib_Net, "timed out");
    } else {
        for (; i < result; ++i) {
            const auto& current_event = native_events[i];
            LOG_DEBUG(Lib_Net, "native_event[{}] = ( .events = {}, .data = {:#x} )", i,
                      current_event.events, current_event.data.u64);
            const auto it = std::ranges::find_if(
                epoll->events, [&](auto& el) { return el.first == current_event.data.fd; });
            ASSERT(it != epoll->events.end());
            events[i] = {
                .events = ConvertEpollEventsOut(current_event.events),
                .ident = static_cast<u64>(current_event.data.fd),
                .data = it->second.data,
            };
            LOG_DEBUG(Lib_Net, "event[{}] = ( .events = {:#x}, .ident = {}, .data = {:#x} )", i,
                      events[i].events, events[i].ident, events[i].data.data_u64);
        }
    }

    if (result >= 0) {
        while (!epoll->async_resolutions.empty()) {
            if (i == maxevents) {
                break;
            }
            auto rid = epoll->async_resolutions.front();
            epoll->async_resolutions.pop_front();
            auto file = FDTable::Instance()->GetResolver(rid);
            if (!file) {
                LOG_ERROR(Lib_Net, "resolver {} does not exist", rid);
                continue;
            }

            file->resolver->Resolve();

            const auto it =
                std::ranges::find_if(epoll->events, [&](auto& el) { return el.first == rid; });
            ASSERT(it != epoll->events.end());
            events[i] = {
                .events = ORBIS_NET_EPOLLDESCID,
                .ident = static_cast<u64>(rid),
                .data = it->second.data,
            };
            LOG_DEBUG(Lib_Net, "event[{}] = ( .events = {:#x}, .ident = {}, .data = {:#x} )", i,
                      events[i].events, events[i].ident, events[i].data.data_u64);
            ++i;
        }
    }

    return i;
}

int* PS4_SYSV_ABI sceNetErrnoLoc() {
    LOG_TRACE(Lib_Net, "called");
    return &net_errno;
}

int PS4_SYSV_ABI sceNetEtherNtostr() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetEtherStrton() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetEventCallbackCreate() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetEventCallbackDestroy() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetEventCallbackGetError() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetEventCallbackWaitCB() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetFreeAllRouteInfo() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetGetArpInfo() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetGetDns6Info() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetGetDnsInfo() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetGetIfList() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetGetIfListOnce() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetGetIfName() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetGetIfnameNumList() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetGetMacAddress(Libraries::NetCtl::OrbisNetEtherAddr* addr, int flags) {
    if (addr == nullptr) {
        LOG_ERROR(Lib_Net, "addr is null!");
        return ORBIS_NET_EINVAL;
    }
    LOG_DEBUG(Lib_Net, "called");

    auto* netinfo = Common::Singleton<NetUtil::NetUtilInternal>::Instance();
    netinfo->RetrieveEthernetAddr();
    memcpy(addr->data, netinfo->GetEthernetAddr().data(), 6);

    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetGetMemoryPoolStats() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetGetNameToIndex() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetGetpeername(OrbisNetId s, OrbisNetSockaddr* addr, u32* paddrlen) {
    if (!g_isNetInitialized) {
        return ORBIS_NET_ERROR_ENOTINIT;
    }

    return NetErrorHandler([&] { return sys_getpeername(s, addr, paddrlen); });
}

int PS4_SYSV_ABI sceNetGetRandom() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetGetRouteInfo() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetGetSockInfo() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetGetSockInfo6() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetGetsockname(OrbisNetId s, OrbisNetSockaddr* addr, u32* paddrlen) {
    if (!g_isNetInitialized) {
        return ORBIS_NET_ERROR_ENOTINIT;
    }

    return NetErrorHandler([&] { return sys_getsockname(s, addr, paddrlen); });
}

int PS4_SYSV_ABI sceNetGetsockopt(OrbisNetId s, int level, int optname, void* optval, u32* optlen) {
    if (!g_isNetInitialized) {
        return ORBIS_NET_ERROR_ENOTINIT;
    }

    return NetErrorHandler([&] { return sys_getsockopt(s, level, optname, optval, optlen); });
}

int PS4_SYSV_ABI sceNetGetStatisticsInfo() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetGetStatisticsInfoInternal() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetGetSystemTime() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

u32 PS4_SYSV_ABI sceNetHtonl(u32 host32) {
    return htonl(host32);
}

u64 PS4_SYSV_ABI sceNetHtonll(u64 host64) {
    return HTONLL(host64);
}

u16 PS4_SYSV_ABI sceNetHtons(u16 host16) {
    return htons(host16);
}

#if defined(WIN32) || defined(__linux__)
// there isn't a strlcpy function in windows/glibc so implement one
u64 strlcpy(char* dst, const char* src, u64 size) {
    u64 src_len = strlen(src);

    if (size > 0) {
        u64 copy_len = (src_len >= size) ? (size - 1) : src_len;
        memcpy(dst, src, copy_len);
        dst[copy_len] = '\0';
    }

    return src_len;
}

#endif

const char* freebsd_inet_ntop4(const unsigned char* src, char* dst, u64 size) {
    static const char fmt[] = "%u.%u.%u.%u";
    char tmp[sizeof "255.255.255.255"];
    int l;

    l = snprintf(tmp, sizeof(tmp), fmt, src[0], src[1], src[2], src[3]);
    if (l <= 0 || (socklen_t)l >= size) {
        return nullptr;
    }
    strlcpy(dst, tmp, size);
    return (dst);
}

const char* freebsd_inet_ntop6(const unsigned char* src, char* dst, u64 size) {
    /*
     * Note that int32_t and int16_t need only be "at least" large enough
     * to contain a value of the specified size.  On some systems, like
     * Crays, there is no such thing as an integer variable with 16 bits.
     * Keep this in mind if you think this function should have been coded
     * to use pointer overlays.  All the world's not a VAX.
     */
    char tmp[sizeof "ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255"], *tp;
    struct {
        int base, len;
    } best, cur;
#define NS_IN6ADDRSZ 16
#define NS_INT16SZ 2
    u_int words[NS_IN6ADDRSZ / NS_INT16SZ];
    int i;

    /*
     * Preprocess:
     *	Copy the input (bytewise) array into a wordwise array.
     *	Find the longest run of 0x00's in src[] for :: shorthanding.
     */
    memset(words, '\0', sizeof words);
    for (i = 0; i < NS_IN6ADDRSZ; i++)
        words[i / 2] |= (src[i] << ((1 - (i % 2)) << 3));
    best.base = -1;
    best.len = 0;
    cur.base = -1;
    cur.len = 0;
    for (i = 0; i < (NS_IN6ADDRSZ / NS_INT16SZ); i++) {
        if (words[i] == 0) {
            if (cur.base == -1)
                cur.base = i, cur.len = 1;
            else
                cur.len++;
        } else {
            if (cur.base != -1) {
                if (best.base == -1 || cur.len > best.len)
                    best = cur;
                cur.base = -1;
            }
        }
    }
    if (cur.base != -1) {
        if (best.base == -1 || cur.len > best.len)
            best = cur;
    }
    if (best.base != -1 && best.len < 2)
        best.base = -1;

    /*
     * Format the result.
     */
    tp = tmp;
    for (i = 0; i < (NS_IN6ADDRSZ / NS_INT16SZ); i++) {
        /* Are we inside the best run of 0x00's? */
        if (best.base != -1 && i >= best.base && i < (best.base + best.len)) {
            if (i == best.base)
                *tp++ = ':';
            continue;
        }
        /* Are we following an initial run of 0x00s or any real hex? */
        if (i != 0)
            *tp++ = ':';
        /* Is this address an encapsulated IPv4? */
        if (i == 6 && best.base == 0 &&
            (best.len == 6 || (best.len == 7 && words[7] != 0x0001) ||
             (best.len == 5 && words[5] == 0xffff))) {
            if (!freebsd_inet_ntop4(src + 12, tp, sizeof tmp - (tp - tmp)))
                return nullptr;
            tp += strlen(tp);
            break;
        }
        tp += snprintf(tp, sizeof(tmp) - (tp - tmp), "%x", words[i]);
    }
    /* Was it a trailing run of 0x00's? */
    if (best.base != -1 && (best.base + best.len) == (NS_IN6ADDRSZ / NS_INT16SZ))
        *tp++ = ':';
    *tp++ = '\0';

    /*
     * Check for overflow, copy, and we're done.
     */
    if ((u64)(tp - tmp) > size) {
        return nullptr;
    }
    strcpy(dst, tmp);
    return (dst);
}
const char* PS4_SYSV_ABI sceNetInetNtop(int af, const void* src, char* dst, u32 size) {
    if (!(src && dst)) {
        *sceNetErrnoLoc() = ORBIS_NET_ENOSPC;
        LOG_ERROR(Lib_Net, "returned ORBIS_NET_ENOSPC");
        return nullptr;
    }

    const char* returnvalue = nullptr;
    switch (af) {
    case ORBIS_NET_AF_INET:
        returnvalue = freebsd_inet_ntop4((const unsigned char*)src, dst, size);
        break;
    case ORBIS_NET_AF_INET6:
        returnvalue = freebsd_inet_ntop6((const unsigned char*)src, dst, size);
        break;
    default:
        *sceNetErrnoLoc() = ORBIS_NET_EAFNOSUPPORT;
        LOG_ERROR(Lib_Net, "returned ORBIS_NET_EAFNOSUPPORT");
        return nullptr;
    }
    if (returnvalue == nullptr) {
        *sceNetErrnoLoc() = ORBIS_NET_ENOSPC;
        LOG_ERROR(Lib_Net, "returned ORBIS_NET_ENOSPC");
    } else {
        LOG_DEBUG(Lib_Net, "{}: {}", magic_enum::enum_name((OrbisNetFamily)af), dst);
    }
    return returnvalue;
}

int PS4_SYSV_ABI sceNetInetNtopWithScopeId() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetInetPton(int af, const char* src, void* dst) {
#ifdef WIN32
    int res = InetPtonA(ConvertFamilies(af), src, dst);
#else
    int res = inet_pton(ConvertFamilies(af), src, dst);
#endif
    if (res < 0) {
        UNREACHABLE_MSG("af = {}, src = {}, dst = {}", af, src, fmt::ptr(dst));
    }
    return res;
}

int PS4_SYSV_ABI sceNetInetPtonEx(int af, const char* src, void* dst, int flags) {
    LOG_WARNING(Lib_Net, "ignored flags, redirecting to sceNetInetPton");
    return sceNetInetPton(af, src, dst);
}

int PS4_SYSV_ABI sceNetInetPtonWithScopeId() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetInfoDumpStart() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetInfoDumpStop() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetInit() {
    LOG_ERROR(Lib_Net, "(DUMMY) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetInitParam() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetIoctl() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetListen(OrbisNetId s, int backlog) {
    if (!g_isNetInitialized) {
        return ORBIS_NET_ERROR_ENOTINIT;
    }

    return NetErrorHandler([&] { return sys_listen(s, backlog); });
}

int PS4_SYSV_ABI sceNetMemoryAllocate() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetMemoryFree() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

u32 PS4_SYSV_ABI sceNetNtohl(u32 net32) {
    return ntohl(net32);
}

int PS4_SYSV_ABI sceNetNtohll() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

u16 PS4_SYSV_ABI sceNetNtohs(u16 net16) {
    return ntohs(net16);
}

int PS4_SYSV_ABI sceNetPoolCreate(const char* name, int size, int flags) {
    LOG_ERROR(Lib_Net, "(DUMMY) name = {} size = {} flags = {} ", std::string(name), size, flags);
    static s32 id = 1;
    return id++;
}

int PS4_SYSV_ABI sceNetPoolDestroy() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetPppoeStart() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetPppoeStop() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetRecv(OrbisNetId s, void* buf, u64 len, int flags) {
    if (!g_isNetInitialized) {
        return ORBIS_NET_ERROR_ENOTINIT;
    }

    return NetErrorHandler(
        [&] { return sys_recvfrom(s, buf, len, flags | 0x40000000, nullptr, 0); });
}

int PS4_SYSV_ABI sceNetRecvfrom(OrbisNetId s, void* buf, u64 len, int flags, OrbisNetSockaddr* addr,
                                u32* paddrlen) {
    if (!g_isNetInitialized) {
        return ORBIS_NET_ERROR_ENOTINIT;
    }

    return NetErrorHandler(
        [&] { return sys_recvfrom(s, buf, len, flags | 0x40000000, addr, paddrlen); });
}

int PS4_SYSV_ABI sceNetRecvmsg(OrbisNetId s, OrbisNetMsghdr* msg, int flags) {
    if (!g_isNetInitialized) {
        return ORBIS_NET_ERROR_ENOTINIT;
    }

    return NetErrorHandler([&] { return sys_recvmsg(s, msg, flags | 0x40000000); });
}

int PS4_SYSV_ABI sceNetResolverAbort() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetResolverConnect() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetResolverConnectAbort() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetResolverConnectCreate() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetResolverConnectDestroy() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetResolverCreate(const char* name, int poolid, int flags) {
    const char* safe_name = name ? name : "";
    LOG_INFO(Lib_Net, "name = {}, poolid = {}, flags = {}", safe_name, poolid, flags);

    if (flags != 0) {
        *sceNetErrnoLoc() = ORBIS_NET_EINVAL;
        return ORBIS_NET_ERROR_EINVAL;
    }

    auto fd = FDTable::Instance()->CreateHandle();
    auto* resolver = FDTable::Instance()->GetFile(fd);
    resolver->is_opened = true;
    resolver->type = Core::FileSys::FileType::Resolver;
    resolver->resolver = std::make_shared<Resolver>(safe_name, poolid, flags);
    resolver->m_guest_name = safe_name;
    return fd;
}

int PS4_SYSV_ABI sceNetResolverDestroy(OrbisNetId resolverid) {
    LOG_ERROR(Lib_Net, "(STUBBED) called rid = {}", resolverid);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetResolverGetError(OrbisNetId resolverid, s32* status) {
    LOG_ERROR(Lib_Net, "(STUBBED) called rid = {}", resolverid);
    *status = 0;
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetResolverStartAton() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetResolverStartAton6() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetResolverStartNtoa(OrbisNetId resolverid, const char* hostname,
                                         OrbisNetInAddr* addr, int timeout, int retry, int flags) {
    LOG_INFO(Lib_Net,
             "called, resolverid = {}, hostname = {}, timeout = {}, retry = {}, flags = {}",
             resolverid, hostname, timeout, retry, flags);

    auto file = FDTable::Instance()->GetResolver(resolverid);
    if (!file) {
        *sceNetErrnoLoc() = ORBIS_NET_EBADF;
        return ORBIS_NET_ERROR_EBADF;
    }

    if ((flags & ORBIS_NET_RESOLVER_ASYNC) != 0) {
        return file->resolver->ResolveAsync(hostname, addr, timeout, retry, flags);
    }

    auto* netinfo = Common::Singleton<NetUtil::NetUtilInternal>::Instance();
    auto ret = netinfo->ResolveHostname(hostname, addr);

    if (ret != 0) {
        *sceNetErrnoLoc() = ret;
        ret = -ret | ORBIS_NET_ERROR_BASE;
    }

    return ret;
}

int PS4_SYSV_ABI sceNetResolverStartNtoa6() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetResolverStartNtoaMultipleRecords(OrbisNetId resolverid, const char* hostname,
                                                        OrbisNetResolverInfo* info, int timeout,
                                                        int retry, int flags) {
    LOG_WARNING(Lib_Net, "redirected to sceNetResolverStartNtoa");

    OrbisNetInAddr addr{};
    auto result = sceNetResolverStartNtoa(resolverid, hostname, &addr, timeout, retry, flags);

    if (result == ORBIS_OK) {
        info->addrs[0] = {.u = {.addr = addr}, .af = ORBIS_NET_AF_INET};
        info->records = 1;
        info->recordsv4 = 1;
    }

    return result;
}

int PS4_SYSV_ABI sceNetResolverStartNtoaMultipleRecordsEx() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetSend(OrbisNetId s, const void* buf, u64 len, int flags) {
    if (!g_isNetInitialized) {
        return ORBIS_NET_ERROR_ENOTINIT;
    }

    return NetErrorHandler([&] { return sys_sendto(s, buf, len, flags | 0x40020000, nullptr, 0); });
}

int PS4_SYSV_ABI sceNetSendmsg(OrbisNetId s, const OrbisNetMsghdr* msg, int flags) {
    if (!g_isNetInitialized) {
        return ORBIS_NET_ERROR_ENOTINIT;
    }

    return NetErrorHandler([&] { return sys_sendmsg(s, msg, flags | 0x40020000); });
}

int PS4_SYSV_ABI sceNetSendto(OrbisNetId s, const void* buf, u64 len, int flags,
                              const OrbisNetSockaddr* addr, u32 addrlen) {
    if (!g_isNetInitialized) {
        return ORBIS_NET_ERROR_ENOTINIT;
    }

    return NetErrorHandler(
        [&] { return sys_sendto(s, buf, len, flags | 0x40020000, addr, addrlen); });
}

int PS4_SYSV_ABI sceNetSetDns6Info() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetSetDns6InfoToKernel() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetSetDnsInfo() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetSetDnsInfoToKernel() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetSetsockopt(OrbisNetId s, int level, int optname, const void* optval,
                                  u32 optlen) {
    if (!g_isNetInitialized) {
        return ORBIS_NET_ERROR_ENOTINIT;
    }

    return NetErrorHandler([&] { return sys_setsockopt(s, level, optname, optval, optlen); });
}

int PS4_SYSV_ABI sceNetShowIfconfig() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetShowIfconfigForBuffer() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetShowIfconfigWithMemory() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetShowNetstat() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetShowNetstatEx() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetShowNetstatExForBuffer() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetShowNetstatForBuffer() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetShowNetstatWithMemory() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetShowPolicy() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetShowPolicyWithMemory() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetShowRoute() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetShowRoute6() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetShowRoute6ForBuffer() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetShowRoute6WithMemory() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetShowRouteForBuffer() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetShowRouteWithMemory() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetShutdown(OrbisNetId s, int how) {
    if (!g_isNetInitialized) {
        return ORBIS_NET_ERROR_ENOTINIT;
    }

    return NetErrorHandler([&] { return sys_shutdown(s, how); });
}

OrbisNetId PS4_SYSV_ABI sceNetSocket(const char* name, int family, int type, int protocol) {
    if (!g_isNetInitialized) {
        return ORBIS_NET_ERROR_ENOTINIT;
    }

    return NetErrorHandler([&] { return sys_socketex(name, family, type, protocol); });
}

int PS4_SYSV_ABI sceNetSocketAbort(OrbisNetId s, int flags) {
    if (!g_isNetInitialized) {
        return ORBIS_NET_ERROR_ENOTINIT;
    }

    return NetErrorHandler([&] { return sys_netabort(s, flags); });
}

int PS4_SYSV_ABI sceNetSocketClose(OrbisNetId s) {
    if (!g_isNetInitialized) {
        return ORBIS_NET_ERROR_ENOTINIT;
    }

    return NetErrorHandler([&] { return sys_socketclose(s); });
}

int PS4_SYSV_ABI sceNetSyncCreate() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetSyncDestroy() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetSyncGet() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetSyncSignal() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetSyncWait() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetSysctl() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetTerm() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetThreadCreate() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetThreadExit() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetThreadJoin() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetUsleep() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_0E707A589F751C68() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetEmulationGet() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetEmulationSet() {
    LOG_ERROR(Lib_Net, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("ZRAJo-A-ukc", "libSceNet", 1, "libSceNet", in6addr_any);
    LIB_FUNCTION("XCuA-GqjA-k", "libSceNet", 1, "libSceNet", in6addr_loopback);
    LIB_FUNCTION("VZgoeBxPXUQ", "libSceNet", 1, "libSceNet", sce_net_dummy);
    LIB_FUNCTION("GAtITrgxKDE", "libSceNet", 1, "libSceNet", sce_net_in6addr_any);
    LIB_FUNCTION("84MgU4MMTLQ", "libSceNet", 1, "libSceNet", sce_net_in6addr_linklocal_allnodes);
    LIB_FUNCTION("2uSWyOKYc1M", "libSceNet", 1, "libSceNet", sce_net_in6addr_linklocal_allrouters);
    LIB_FUNCTION("P3AeWBvPrkg", "libSceNet", 1, "libSceNet", sce_net_in6addr_loopback);
    LIB_FUNCTION("PgNI+j4zxzM", "libSceNet", 1, "libSceNet", sce_net_in6addr_nodelocal_allnodes);
    LIB_FUNCTION("PIWqhn9oSxc", "libSceNet", 1, "libSceNet", sceNetAccept);
    LIB_FUNCTION("BTUvkWzrP68", "libSceNet", 1, "libSceNet", sceNetAddrConfig6GetInfo);
    LIB_FUNCTION("3qG7UJy2Fq8", "libSceNet", 1, "libSceNet", sceNetAddrConfig6Start);
    LIB_FUNCTION("P+0ePpDfUAQ", "libSceNet", 1, "libSceNet", sceNetAddrConfig6Stop);
    LIB_FUNCTION("PcdLABhYga4", "libSceNet", 1, "libSceNet", sceNetAllocateAllRouteInfo);
    LIB_FUNCTION("xHq87H78dho", "libSceNet", 1, "libSceNet", sceNetBandwidthControlGetDataTraffic);
    LIB_FUNCTION("c8IRpl4L74I", "libSceNet", 1, "libSceNet", sceNetBandwidthControlGetDefaultParam);
    LIB_FUNCTION("b9Ft65tqvLk", "libSceNet", 1, "libSceNet", sceNetBandwidthControlGetIfParam);
    LIB_FUNCTION("PDkapOwggRw", "libSceNet", 1, "libSceNet", sceNetBandwidthControlGetPolicy);
    LIB_FUNCTION("P4zZXE7bpsA", "libSceNet", 1, "libSceNet", sceNetBandwidthControlSetDefaultParam);
    LIB_FUNCTION("g4DKkzV2qC4", "libSceNet", 1, "libSceNet", sceNetBandwidthControlSetIfParam);
    LIB_FUNCTION("7Z1hhsEmkQU", "libSceNet", 1, "libSceNet", sceNetBandwidthControlSetPolicy);
    LIB_FUNCTION("bErx49PgxyY", "libSceNet", 1, "libSceNet", sceNetBind);
    LIB_FUNCTION("eyLyLJrdEOU", "libSceNet", 1, "libSceNet", sceNetClearDnsCache);
    LIB_FUNCTION("Ea2NaVMQNO8", "libSceNet", 1, "libSceNet", sceNetConfigAddArp);
    LIB_FUNCTION("0g0qIuPN3ZQ", "libSceNet", 1, "libSceNet", sceNetConfigAddArpWithInterface);
    LIB_FUNCTION("ge7g15Sqhks", "libSceNet", 1, "libSceNet", sceNetConfigAddIfaddr);
    LIB_FUNCTION("FDHr4Iz7dQU", "libSceNet", 1, "libSceNet", sceNetConfigAddMRoute);
    LIB_FUNCTION("Cyjl1yzi1qY", "libSceNet", 1, "libSceNet", sceNetConfigAddRoute);
    LIB_FUNCTION("Bu+L5r1lKRg", "libSceNet", 1, "libSceNet", sceNetConfigAddRoute6);
    LIB_FUNCTION("wIGold7Lro0", "libSceNet", 1, "libSceNet", sceNetConfigAddRouteWithInterface);
    LIB_FUNCTION("MzA1YrRE6rA", "libSceNet", 1, "libSceNet", sceNetConfigCleanUpAllInterfaces);
    LIB_FUNCTION("HJt+4x-CnY0", "libSceNet", 1, "libSceNet", sceNetConfigDelArp);
    LIB_FUNCTION("xTcttXJ3Utg", "libSceNet", 1, "libSceNet", sceNetConfigDelArpWithInterface);
    LIB_FUNCTION("RuVwHEW6dM4", "libSceNet", 1, "libSceNet", sceNetConfigDelDefaultRoute);
    LIB_FUNCTION("UMlVCy7RX1s", "libSceNet", 1, "libSceNet", sceNetConfigDelDefaultRoute6);
    LIB_FUNCTION("0239JNsI6PE", "libSceNet", 1, "libSceNet", sceNetConfigDelIfaddr);
    LIB_FUNCTION("hvCXMwd45oc", "libSceNet", 1, "libSceNet", sceNetConfigDelIfaddr6);
    LIB_FUNCTION("5Yl1uuh5i-A", "libSceNet", 1, "libSceNet", sceNetConfigDelMRoute);
    LIB_FUNCTION("QO7+2E3cD-U", "libSceNet", 1, "libSceNet", sceNetConfigDelRoute);
    LIB_FUNCTION("4wDGvfhmkmk", "libSceNet", 1, "libSceNet", sceNetConfigDelRoute6);
    LIB_FUNCTION("3WzWV86AJ3w", "libSceNet", 1, "libSceNet", sceNetConfigDownInterface);
    LIB_FUNCTION("mOUkgTaSkJU", "libSceNet", 1, "libSceNet", sceNetConfigEtherGetLinkMode);
    LIB_FUNCTION("pF3Vy1iZ5bs", "libSceNet", 1, "libSceNet", sceNetConfigEtherPostPlugInOutEvent);
    LIB_FUNCTION("QltDK6wWqF0", "libSceNet", 1, "libSceNet", sceNetConfigEtherSetLinkMode);
    LIB_FUNCTION("18KNgSvYx+Y", "libSceNet", 1, "libSceNet", sceNetConfigFlushRoute);
    LIB_FUNCTION("lFJb+BlPK1c", "libSceNet", 1, "libSceNet", sceNetConfigGetDefaultRoute);
    LIB_FUNCTION("mCLdiNIKtW0", "libSceNet", 1, "libSceNet", sceNetConfigGetDefaultRoute6);
    LIB_FUNCTION("ejwa0hWWhDs", "libSceNet", 1, "libSceNet", sceNetConfigGetIfaddr);
    LIB_FUNCTION("FU6NK4RHQVE", "libSceNet", 1, "libSceNet", sceNetConfigGetIfaddr6);
    LIB_FUNCTION("vbZLomImmEE", "libSceNet", 1, "libSceNet", sceNetConfigRoutingShowRoutingConfig);
    LIB_FUNCTION("a6sS6iSE0IA", "libSceNet", 1, "libSceNet", sceNetConfigRoutingShowtCtlVar);
    LIB_FUNCTION("eszLdtIMfQE", "libSceNet", 1, "libSceNet", sceNetConfigRoutingStart);
    LIB_FUNCTION("toi8xxcSfJ0", "libSceNet", 1, "libSceNet", sceNetConfigRoutingStop);
    LIB_FUNCTION("EAl7xvi7nXg", "libSceNet", 1, "libSceNet", sceNetConfigSetDefaultRoute);
    LIB_FUNCTION("4zLOHbt3UFk", "libSceNet", 1, "libSceNet", sceNetConfigSetDefaultRoute6);
    LIB_FUNCTION("yaVAdLDxUj0", "libSceNet", 1, "libSceNet", sceNetConfigSetDefaultScope);
    LIB_FUNCTION("8Kh+1eidI3c", "libSceNet", 1, "libSceNet", sceNetConfigSetIfaddr);
    LIB_FUNCTION("QJbV3vfBQ8Q", "libSceNet", 1, "libSceNet", sceNetConfigSetIfaddr6);
    LIB_FUNCTION("POrSEl8zySw", "libSceNet", 1, "libSceNet", sceNetConfigSetIfaddr6WithFlags);
    LIB_FUNCTION("0sesmAYH3Lk", "libSceNet", 1, "libSceNet", sceNetConfigSetIfFlags);
    LIB_FUNCTION("uNTluLfYgS8", "libSceNet", 1, "libSceNet", sceNetConfigSetIfLinkLocalAddr6);
    LIB_FUNCTION("s31rYkpIMMQ", "libSceNet", 1, "libSceNet", sceNetConfigSetIfmtu);
    LIB_FUNCTION("tvdzQkm+UaY", "libSceNet", 1, "libSceNet", sceNetConfigUnsetIfFlags);
    LIB_FUNCTION("oGEBX0eXGFs", "libSceNet", 1, "libSceNet", sceNetConfigUpInterface);
    LIB_FUNCTION("6HNbayHPL7c", "libSceNet", 1, "libSceNet", sceNetConfigUpInterfaceWithFlags);
    LIB_FUNCTION("6A6EweB3Dto", "libSceNet", 1, "libSceNet", sceNetConfigWlanAdhocClearWakeOnWlan);
    LIB_FUNCTION("ZLdJyQJUMkM", "libSceNet", 1, "libSceNet", sceNetConfigWlanAdhocCreate);
    LIB_FUNCTION("Yr3UeApLWTY", "libSceNet", 1, "libSceNet",
                 sceNetConfigWlanAdhocGetWakeOnWlanInfo);
    LIB_FUNCTION("Xma8yHmV+TQ", "libSceNet", 1, "libSceNet", sceNetConfigWlanAdhocJoin);
    LIB_FUNCTION("K4o48GTNbSc", "libSceNet", 1, "libSceNet", sceNetConfigWlanAdhocLeave);
    LIB_FUNCTION("ZvKgNrrLCCQ", "libSceNet", 1, "libSceNet",
                 sceNetConfigWlanAdhocPspEmuClearWakeOnWlan);
    LIB_FUNCTION("1j4DZ5dXbeQ", "libSceNet", 1, "libSceNet",
                 sceNetConfigWlanAdhocPspEmuGetWakeOnWlanInfo);
    LIB_FUNCTION("C-+JPjaEhdA", "libSceNet", 1, "libSceNet",
                 sceNetConfigWlanAdhocPspEmuSetWakeOnWlan);
    LIB_FUNCTION("7xYdUWg1WdY", "libSceNet", 1, "libSceNet", sceNetConfigWlanAdhocScanJoin);
    LIB_FUNCTION("Q7ee2Uav5f8", "libSceNet", 1, "libSceNet",
                 sceNetConfigWlanAdhocSetExtInfoElement);
    LIB_FUNCTION("xaOTiuxIQNY", "libSceNet", 1, "libSceNet", sceNetConfigWlanAdhocSetWakeOnWlan);
    LIB_FUNCTION("QlRJWya+dtE", "libSceNet", 1, "libSceNet", sceNetConfigWlanApStart);
    LIB_FUNCTION("6uYcvVjH7Ms", "libSceNet", 1, "libSceNet", sceNetConfigWlanApStop);
    LIB_FUNCTION("MDbg-oAj8Aw", "libSceNet", 1, "libSceNet", sceNetConfigWlanBackgroundScanQuery);
    LIB_FUNCTION("cMA8f6jI6s0", "libSceNet", 1, "libSceNet", sceNetConfigWlanBackgroundScanStart);
    LIB_FUNCTION("3T5aIe-7L84", "libSceNet", 1, "libSceNet", sceNetConfigWlanBackgroundScanStop);
    LIB_FUNCTION("+3KMyS93TOs", "libSceNet", 1, "libSceNet", sceNetConfigWlanDiagGetDeviceInfo);
    LIB_FUNCTION("9oiOWQ5FMws", "libSceNet", 1, "libSceNet", sceNetConfigWlanDiagSetAntenna);
    LIB_FUNCTION("fHr45B97n0U", "libSceNet", 1, "libSceNet", sceNetConfigWlanDiagSetTxFixedRate);
    LIB_FUNCTION("PNDDxnqqtk4", "libSceNet", 1, "libSceNet", sceNetConfigWlanGetDeviceConfig);
    LIB_FUNCTION("Pkx0lwWVzmQ", "libSceNet", 1, "libSceNet", sceNetConfigWlanInfraGetRssiInfo);
    LIB_FUNCTION("IkBCxG+o4Nk", "libSceNet", 1, "libSceNet", sceNetConfigWlanInfraLeave);
    LIB_FUNCTION("273-I-zD8+8", "libSceNet", 1, "libSceNet", sceNetConfigWlanInfraScanJoin);
    LIB_FUNCTION("-Mi5hNiWC4c", "libSceNet", 1, "libSceNet", sceNetConfigWlanScan);
    LIB_FUNCTION("U1q6DrPbY6k", "libSceNet", 1, "libSceNet", sceNetConfigWlanSetDeviceConfig);
    LIB_FUNCTION("OXXX4mUk3uk", "libSceNet", 1, "libSceNet", sceNetConnect);
    LIB_FUNCTION("lDTIbqNs0ps", "libSceNet", 1, "libSceNet", sceNetControl);
    LIB_FUNCTION("Q6T-zIblNqk", "libSceNet", 1, "libSceNet", sceNetDhcpdStart);
    LIB_FUNCTION("xwWm8jzrpeM", "libSceNet", 1, "libSceNet", sceNetDhcpdStop);
    LIB_FUNCTION("KhQxhlEslo0", "libSceNet", 1, "libSceNet", sceNetDhcpGetAutoipInfo);
    LIB_FUNCTION("ix4LWXd12F0", "libSceNet", 1, "libSceNet", sceNetDhcpGetInfo);
    LIB_FUNCTION("DrZuCQDnm3w", "libSceNet", 1, "libSceNet", sceNetDhcpGetInfoEx);
    LIB_FUNCTION("Wzv6dngR-DQ", "libSceNet", 1, "libSceNet", sceNetDhcpStart);
    LIB_FUNCTION("6AN7OlSMWk0", "libSceNet", 1, "libSceNet", sceNetDhcpStop);
    LIB_FUNCTION("+ezgWao0wo8", "libSceNet", 1, "libSceNet", sceNetDumpAbort);
    LIB_FUNCTION("bghgkeLKq1Q", "libSceNet", 1, "libSceNet", sceNetDumpCreate);
    LIB_FUNCTION("xZ54Il-u1vs", "libSceNet", 1, "libSceNet", sceNetDumpDestroy);
    LIB_FUNCTION("YWTpt45PxbI", "libSceNet", 1, "libSceNet", sceNetDumpRead);
    LIB_FUNCTION("TwjkDIPdZ1Q", "libSceNet", 1, "libSceNet", sceNetDuplicateIpStart);
    LIB_FUNCTION("QCbvCx9HL30", "libSceNet", 1, "libSceNet", sceNetDuplicateIpStop);
    LIB_FUNCTION("w21YgGGNtBk", "libSceNet", 1, "libSceNet", sceNetEpollAbort);
    LIB_FUNCTION("ZVw46bsasAk", "libSceNet", 1, "libSceNet", sceNetEpollControl);
    LIB_FUNCTION("SF47kB2MNTo", "libSceNet", 1, "libSceNet", sceNetEpollCreate);
    LIB_FUNCTION("Inp1lfL+Jdw", "libSceNet", 1, "libSceNet", sceNetEpollDestroy);
    LIB_FUNCTION("drjIbDbA7UQ", "libSceNet", 1, "libSceNet", sceNetEpollWait);
    LIB_FUNCTION("HQOwnfMGipQ", "libSceNet", 1, "libSceNet", sceNetErrnoLoc);
    LIB_FUNCTION("v6M4txecCuo", "libSceNet", 1, "libSceNet", sceNetEtherNtostr);
    LIB_FUNCTION("b-bFZvNV59I", "libSceNet", 1, "libSceNet", sceNetEtherStrton);
    LIB_FUNCTION("cWGGXoeZUzA", "libSceNet", 1, "libSceNet", sceNetEventCallbackCreate);
    LIB_FUNCTION("jzP0MoZpYnI", "libSceNet", 1, "libSceNet", sceNetEventCallbackDestroy);
    LIB_FUNCTION("tB3BB8AsrjU", "libSceNet", 1, "libSceNet", sceNetEventCallbackGetError);
    LIB_FUNCTION("5isaotjMWlA", "libSceNet", 1, "libSceNet", sceNetEventCallbackWaitCB);
    LIB_FUNCTION("2ee14ktE1lw", "libSceNet", 1, "libSceNet", sceNetFreeAllRouteInfo);
    LIB_FUNCTION("q8j9OSdnN1Y", "libSceNet", 1, "libSceNet", sceNetGetArpInfo);
    LIB_FUNCTION("wmoIm94hqik", "libSceNet", 1, "libSceNet", sceNetGetDns6Info);
    LIB_FUNCTION("nCL0NyZsd5A", "libSceNet", 1, "libSceNet", sceNetGetDnsInfo);
    LIB_FUNCTION("HoV-GJyx7YY", "libSceNet", 1, "libSceNet", sceNetGetIfList);
    LIB_FUNCTION("ahiOMqoYYMc", "libSceNet", 1, "libSceNet", sceNetGetIfListOnce);
    LIB_FUNCTION("0MT2l3uIX7c", "libSceNet", 1, "libSceNet", sceNetGetIfName);
    LIB_FUNCTION("5lrSEHdqyos", "libSceNet", 1, "libSceNet", sceNetGetIfnameNumList);
    LIB_FUNCTION("6Oc0bLsIYe0", "libSceNet", 1, "libSceNet", sceNetGetMacAddress);
    LIB_FUNCTION("rMyh97BU5pY", "libSceNet", 1, "libSceNet", sceNetGetMemoryPoolStats);
    LIB_FUNCTION("+S-2-jlpaBo", "libSceNet", 1, "libSceNet", sceNetGetNameToIndex);
    LIB_FUNCTION("TCkRD0DWNLg", "libSceNet", 1, "libSceNet", sceNetGetpeername);
    LIB_FUNCTION("G3O2j9f5z00", "libSceNet", 1, "libSceNet", sceNetGetRandom);
    LIB_FUNCTION("6Nx1hIQL9h8", "libSceNet", 1, "libSceNet", sceNetGetRouteInfo);
    LIB_FUNCTION("hLuXdjHnhiI", "libSceNet", 1, "libSceNet", sceNetGetSockInfo);
    LIB_FUNCTION("Cidi9Y65mP8", "libSceNet", 1, "libSceNet", sceNetGetSockInfo6);
    LIB_FUNCTION("hoOAofhhRvE", "libSceNet", 1, "libSceNet", sceNetGetsockname);
    LIB_FUNCTION("xphrZusl78E", "libSceNet", 1, "libSceNet", sceNetGetsockopt);
    LIB_FUNCTION("GA5ZDaLtUBE", "libSceNet", 1, "libSceNet", sceNetGetStatisticsInfo);
    LIB_FUNCTION("9mIcUExH34w", "libSceNet", 1, "libSceNet", sceNetGetStatisticsInfoInternal);
    LIB_FUNCTION("p2vxsE2U3RQ", "libSceNet", 1, "libSceNet", sceNetGetSystemTime);
    LIB_FUNCTION("9T2pDF2Ryqg", "libSceNet", 1, "libSceNet", sceNetHtonl);
    LIB_FUNCTION("3CHi1K1wsCQ", "libSceNet", 1, "libSceNet", sceNetHtonll);
    LIB_FUNCTION("iWQWrwiSt8A", "libSceNet", 1, "libSceNet", sceNetHtons);
    LIB_FUNCTION("9vA2aW+CHuA", "libSceNet", 1, "libSceNet", sceNetInetNtop);
    LIB_FUNCTION("Eh+Vqkrrc00", "libSceNet", 1, "libSceNet", sceNetInetNtopWithScopeId);
    LIB_FUNCTION("8Kcp5d-q1Uo", "libSceNet", 1, "libSceNet", sceNetInetPton);
    LIB_FUNCTION("Xn2TA2QhxHc", "libSceNet", 1, "libSceNet", sceNetInetPtonEx);
    LIB_FUNCTION("b+LixqREH6A", "libSceNet", 1, "libSceNet", sceNetInetPtonWithScopeId);
    LIB_FUNCTION("cYW1ISGlOmo", "libSceNet", 1, "libSceNet", sceNetInfoDumpStart);
    LIB_FUNCTION("XfV-XBCuhDo", "libSceNet", 1, "libSceNet", sceNetInfoDumpStop);
    LIB_FUNCTION("Nlev7Lg8k3A", "libSceNet", 1, "libSceNet", sceNetInit);
    LIB_FUNCTION("6MojQ8uFHEI", "libSceNet", 1, "libSceNet", sceNetInitParam);
    LIB_FUNCTION("ghqRRVQxqKo", "libSceNet", 1, "libSceNet", sceNetIoctl);
    LIB_FUNCTION("kOj1HiAGE54", "libSceNet", 1, "libSceNet", sceNetListen);
    LIB_FUNCTION("HKIa-WH0AZ4", "libSceNet", 1, "libSceNet", sceNetMemoryAllocate);
    LIB_FUNCTION("221fvqVs+sQ", "libSceNet", 1, "libSceNet", sceNetMemoryFree);
    LIB_FUNCTION("pQGpHYopAIY", "libSceNet", 1, "libSceNet", sceNetNtohl);
    LIB_FUNCTION("tOrRi-v3AOM", "libSceNet", 1, "libSceNet", sceNetNtohll);
    LIB_FUNCTION("Rbvt+5Y2iEw", "libSceNet", 1, "libSceNet", sceNetNtohs);
    LIB_FUNCTION("dgJBaeJnGpo", "libSceNet", 1, "libSceNet", sceNetPoolCreate);
    LIB_FUNCTION("K7RlrTkI-mw", "libSceNet", 1, "libSceNet", sceNetPoolDestroy);
    LIB_FUNCTION("QGOqGPnk5a4", "libSceNet", 1, "libSceNet", sceNetPppoeStart);
    LIB_FUNCTION("FIV95WE1EuE", "libSceNet", 1, "libSceNet", sceNetPppoeStop);
    LIB_FUNCTION("9wO9XrMsNhc", "libSceNet", 1, "libSceNet", sceNetRecv);
    LIB_FUNCTION("304ooNZxWDY", "libSceNet", 1, "libSceNet", sceNetRecvfrom);
    LIB_FUNCTION("wvuUDv0jrMI", "libSceNet", 1, "libSceNet", sceNetRecvmsg);
    LIB_FUNCTION("AzqoBha7js4", "libSceNet", 1, "libSceNet", sceNetResolverAbort);
    LIB_FUNCTION("JQk8ck8vnPY", "libSceNet", 1, "libSceNet", sceNetResolverConnect);
    LIB_FUNCTION("bonnMiDoOZg", "libSceNet", 1, "libSceNet", sceNetResolverConnectAbort);
    LIB_FUNCTION("V5q6gvEJpw4", "libSceNet", 1, "libSceNet", sceNetResolverConnectCreate);
    LIB_FUNCTION("QFPjG6rqeZg", "libSceNet", 1, "libSceNet", sceNetResolverConnectDestroy);
    LIB_FUNCTION("C4UgDHHPvdw", "libSceNet", 1, "libSceNet", sceNetResolverCreate);
    LIB_FUNCTION("kJlYH5uMAWI", "libSceNet", 1, "libSceNet", sceNetResolverDestroy);
    LIB_FUNCTION("J5i3hiLJMPk", "libSceNet", 1, "libSceNet", sceNetResolverGetError);
    LIB_FUNCTION("Apb4YDxKsRI", "libSceNet", 1, "libSceNet", sceNetResolverStartAton);
    LIB_FUNCTION("zvzWA5IZMsg", "libSceNet", 1, "libSceNet", sceNetResolverStartAton6);
    LIB_FUNCTION("Nd91WaWmG2w", "libSceNet", 1, "libSceNet", sceNetResolverStartNtoa);
    LIB_FUNCTION("zl35YNs9jnI", "libSceNet", 1, "libSceNet", sceNetResolverStartNtoa6);
    LIB_FUNCTION("RCCY01Xd+58", "libSceNet", 1, "libSceNet",
                 sceNetResolverStartNtoaMultipleRecords);
    LIB_FUNCTION("sT4nBQKUPqM", "libSceNet", 1, "libSceNet",
                 sceNetResolverStartNtoaMultipleRecordsEx);
    LIB_FUNCTION("beRjXBn-z+o", "libSceNet", 1, "libSceNet", sceNetSend);
    LIB_FUNCTION("2eKbgcboJso", "libSceNet", 1, "libSceNet", sceNetSendmsg);
    LIB_FUNCTION("gvD1greCu0A", "libSceNet", 1, "libSceNet", sceNetSendto);
    LIB_FUNCTION("15Ywg-ZsSl0", "libSceNet", 1, "libSceNet", sceNetSetDns6Info);
    LIB_FUNCTION("E3oH1qsdqCA", "libSceNet", 1, "libSceNet", sceNetSetDns6InfoToKernel);
    LIB_FUNCTION("B-M6KjO8-+w", "libSceNet", 1, "libSceNet", sceNetSetDnsInfo);
    LIB_FUNCTION("8s+T0bJeyLQ", "libSceNet", 1, "libSceNet", sceNetSetDnsInfoToKernel);
    LIB_FUNCTION("2mKX2Spso7I", "libSceNet", 1, "libSceNet", sceNetSetsockopt);
    LIB_FUNCTION("k1V1djYpk7k", "libSceNet", 1, "libSceNet", sceNetShowIfconfig);
    LIB_FUNCTION("j6pkkO2zJtg", "libSceNet", 1, "libSceNet", sceNetShowIfconfigForBuffer);
    LIB_FUNCTION("E8dTcvQw3hg", "libSceNet", 1, "libSceNet", sceNetShowIfconfigWithMemory);
    LIB_FUNCTION("WxislcDAW5I", "libSceNet", 1, "libSceNet", sceNetShowNetstat);
    LIB_FUNCTION("rX30iWQqqzg", "libSceNet", 1, "libSceNet", sceNetShowNetstatEx);
    LIB_FUNCTION("vjwKTGa21f0", "libSceNet", 1, "libSceNet", sceNetShowNetstatExForBuffer);
    LIB_FUNCTION("mqoB+LN0pW8", "libSceNet", 1, "libSceNet", sceNetShowNetstatForBuffer);
    LIB_FUNCTION("H5WHYRfDkR0", "libSceNet", 1, "libSceNet", sceNetShowNetstatWithMemory);
    LIB_FUNCTION("tk0p0JmiBkM", "libSceNet", 1, "libSceNet", sceNetShowPolicy);
    LIB_FUNCTION("dbrSNEuZfXI", "libSceNet", 1, "libSceNet", sceNetShowPolicyWithMemory);
    LIB_FUNCTION("cEMX1VcPpQ8", "libSceNet", 1, "libSceNet", sceNetShowRoute);
    LIB_FUNCTION("fCa7-ihdRdc", "libSceNet", 1, "libSceNet", sceNetShowRoute6);
    LIB_FUNCTION("nTJqXsbSS1I", "libSceNet", 1, "libSceNet", sceNetShowRoute6ForBuffer);
    LIB_FUNCTION("TCZyE2YI1uM", "libSceNet", 1, "libSceNet", sceNetShowRoute6WithMemory);
    LIB_FUNCTION("n-IAZb7QB1Y", "libSceNet", 1, "libSceNet", sceNetShowRouteForBuffer);
    LIB_FUNCTION("0-XSSp1kEFM", "libSceNet", 1, "libSceNet", sceNetShowRouteWithMemory);
    LIB_FUNCTION("TSM6whtekok", "libSceNet", 1, "libSceNet", sceNetShutdown);
    LIB_FUNCTION("Q4qBuN-c0ZM", "libSceNet", 1, "libSceNet", sceNetSocket);
    LIB_FUNCTION("zJGf8xjFnQE", "libSceNet", 1, "libSceNet", sceNetSocketAbort);
    LIB_FUNCTION("45ggEzakPJQ", "libSceNet", 1, "libSceNet", sceNetSocketClose);
    LIB_FUNCTION("6AJE2jKg-c0", "libSceNet", 1, "libSceNet", sceNetSyncCreate);
    LIB_FUNCTION("atGfzCaXMak", "libSceNet", 1, "libSceNet", sceNetSyncDestroy);
    LIB_FUNCTION("sAleh-BoxLA", "libSceNet", 1, "libSceNet", sceNetSyncGet);
    LIB_FUNCTION("Z-8Jda650Vk", "libSceNet", 1, "libSceNet", sceNetSyncSignal);
    LIB_FUNCTION("NP5gxDeYhIM", "libSceNet", 1, "libSceNet", sceNetSyncWait);
    LIB_FUNCTION("3zRdT3O2Kxo", "libSceNet", 1, "libSceNet", sceNetSysctl);
    LIB_FUNCTION("cTGkc6-TBlI", "libSceNet", 1, "libSceNet", sceNetTerm);
    LIB_FUNCTION("j-Op3ibRJaQ", "libSceNet", 1, "libSceNet", sceNetThreadCreate);
    LIB_FUNCTION("KirVfZbqniw", "libSceNet", 1, "libSceNet", sceNetThreadExit);
    LIB_FUNCTION("pRbEzaV30qI", "libSceNet", 1, "libSceNet", sceNetThreadJoin);
    LIB_FUNCTION("bjrzRLFali0", "libSceNet", 1, "libSceNet", sceNetUsleep);
    LIB_FUNCTION("DnB6WJ91HGg", "libSceNet", 1, "libSceNet", Func_0E707A589F751C68);
    LIB_FUNCTION("JK1oZe4UysY", "libSceNetDebug", 1, "libSceNet", sceNetEmulationGet);
    LIB_FUNCTION("pfn3Fha1ydc", "libSceNetDebug", 1, "libSceNet", sceNetEmulationSet);
};

} // namespace Libraries::Net
