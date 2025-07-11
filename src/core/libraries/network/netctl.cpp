// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#ifdef WIN32
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <Ws2tcpip.h>
#include <iphlpapi.h>
#include <winsock2.h>
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#endif

#include <common/singleton.h>
#include "common/config.h"
#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/network/net_ctl_codes.h"
#include "core/libraries/network/netctl.h"
#include "net_util.h"

namespace Libraries::NetCtl {

static NetCtlInternal netctl;

int PS4_SYSV_ABI sceNetBweCheckCallbackIpcInt() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetBweClearEventIpcInt() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetBweFinishInternetConnectionTestIpcInt() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetBweGetInfoIpcInt() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetBweRegisterCallbackIpcInt() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetBweSetInternetConnectionTestResultIpcInt() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetBweStartInternetConnectionTestBandwidthTestIpcInt() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetBweStartInternetConnectionTestIpcInt() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetBweUnregisterCallbackIpcInt() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlGetInfoV6() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlGetResultV6() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlGetStateV6() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlRegisterCallbackV6() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlUnregisterCallbackV6() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlCheckCallback() {
    LOG_DEBUG(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlCheckCallbackForLibIpcInt() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlClearEventForLibIpcInt() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlClearEventIpcInt() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlConnectConfIpcInt() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlConnectIpcInt() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlConnectWithRetryIpcInt() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlDisableBandwidthManagementIpcInt() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlDisconnectIpcInt() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlEnableBandwidthManagementIpcInt() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlGetBandwidthInfoIpcInt() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlGetEtherLinkMode() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlGetIfStat() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlGetInfo(int code, OrbisNetCtlInfo* info) {
    LOG_DEBUG(Lib_NetCtl, "code = {}", code);

    switch (code) {
    case ORBIS_NET_CTL_INFO_DEVICE:
        info->device = ORBIS_NET_CTL_DEVICE_WIRED;
        break;
    case ORBIS_NET_CTL_INFO_ETHER_ADDR: {
        auto* netinfo = Common::Singleton<NetUtil::NetUtilInternal>::Instance();
        netinfo->RetrieveEthernetAddr();
        memcpy(info->ether_addr.data, netinfo->GetEthernetAddr().data(), 6);
    } break;
    case ORBIS_NET_CTL_INFO_MTU:
        info->mtu = 1500; // default value
        break;
    case ORBIS_NET_CTL_INFO_LINK:
        info->link = Config::getIsConnectedToNetwork() ? ORBIS_NET_CTL_LINK_CONNECTED
                                                       : ORBIS_NET_CTL_LINK_DISCONNECTED;
        break;
    case ORBIS_NET_CTL_INFO_IP_ADDRESS: {
        strcpy(info->ip_address,
               "127.0.0.1"); // placeholder in case gethostbyname can't find another ip
        char devname[80];
        gethostname(devname, 80);
        if (struct hostent* resolved = gethostbyname(devname)) {
            for (int i = 0; resolved->h_addr_list[i] != nullptr; ++i) {
                struct in_addr addrIn;
                memcpy(&addrIn, resolved->h_addr_list[i], sizeof(u32));
                char* addr = inet_ntoa(addrIn);
                if (strcmp(addr, "127.0.0.1") != 0) {
                    strcpy(info->ip_address, addr);
                    break;
                }
            }
        }
        break;
    }

    default:
        LOG_ERROR(Lib_NetCtl, "{} unsupported code", code);
    }
    LOG_DEBUG(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlGetInfoIpcInt() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlGetInfoV6IpcInt() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlGetNatInfo() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlGetNatInfoIpcInt() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlGetNetEvConfigInfoIpcInt() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlGetResult(int eventType, int* errorCode) {
    if (!errorCode) {
        return ORBIS_NET_CTL_ERROR_INVALID_ADDR;
    }
    LOG_DEBUG(Lib_NetCtl, "(STUBBED) called eventType = {} ", eventType);
    *errorCode = 0;
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlGetResultIpcInt() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlGetResultV6IpcInt() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlGetScanInfoBssidForSsidListScanIpcInt() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlGetScanInfoBssidIpcInt() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlGetScanInfoByBssidIpcInt() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlGetScanInfoForSsidListScanIpcInt() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlGetScanInfoForSsidScanIpcInt() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlGetState(int* state) {
    const auto connected = Config::getIsConnectedToNetwork();
    LOG_DEBUG(Lib_NetCtl, "connected = {}", connected);
    const auto current_state =
        connected ? ORBIS_NET_CTL_STATE_IPOBTAINED : ORBIS_NET_CTL_STATE_DISCONNECTED;
    *state = current_state;
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlGetState2IpcInt() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlGetStateIpcInt() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlGetStateV6IpcInt() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlGetWifiType() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlInit() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlIsBandwidthManagementEnabledIpcInt() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlRegisterCallback(OrbisNetCtlCallback func, void* arg, int* cid) {
    if (!func || !cid) {
        return ORBIS_NET_CTL_ERROR_INVALID_ADDR;
    }
    s32 result = netctl.RegisterCallback(func, arg);
    if (result < 0) {
        return result;
    } else {
        *cid = result;
    }
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlRegisterCallbackForLibIpcInt() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlRegisterCallbackIpcInt() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlRegisterCallbackV6IpcInt() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlScanIpcInt() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlSetErrorNotificationEnabledIpcInt() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlSetStunWithPaddingFlagIpcInt() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlTerm() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlUnregisterCallback() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlUnregisterCallbackForLibIpcInt() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlUnregisterCallbackIpcInt() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlUnregisterCallbackV6IpcInt() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlUnsetStunWithPaddingFlagIpcInt() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_D8DCB6973537A3DC() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlCheckCallbackForNpToolkit() {
    LOG_DEBUG(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlClearEventForNpToolkit() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlRegisterCallbackForNpToolkit(OrbisNetCtlCallbackForNpToolkit func,
                                                       void* arg, int* cid) {
    if (!func || !cid) {
        return ORBIS_NET_CTL_ERROR_INVALID_ADDR;
    }
    s32 result = netctl.RegisterNpToolkitCallback(func, arg);
    if (result < 0) {
        return result;
    } else {
        *cid = result;
    }
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlUnregisterCallbackForNpToolkit() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlApCheckCallback() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlApClearEvent() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlApGetConnectInfo() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlApGetInfo() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlApGetResult() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlApGetState() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlApInit() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlApRegisterCallback() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlApStop() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlApTerm() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlApUnregisterCallback() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlApAppInitWpaKey() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlApAppInitWpaKeyForQa() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlApAppStartWithRetry() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlApAppStartWithRetryPid() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlApRestart() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlApRpCheckCallback() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlApRpClearEvent() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlApRpGetInfo() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlApRpGetResult() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlApRpGetState() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlApRpRegisterCallback() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlApRpStart() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlApRpStartConf() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlApRpStartWithRetry() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlApRpStop() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceNetCtlApRpUnregisterCallback() {
    LOG_ERROR(Lib_NetCtl, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("XtClSOC1xcU", "libSceNetBwe", 1, "libSceNetCtl", 1, 1,
                 sceNetBweCheckCallbackIpcInt);
    LIB_FUNCTION("YALqoY4aeY0", "libSceNetBwe", 1, "libSceNetCtl", 1, 1, sceNetBweClearEventIpcInt);
    LIB_FUNCTION("ouyROWhGUbM", "libSceNetBwe", 1, "libSceNetCtl", 1, 1,
                 sceNetBweFinishInternetConnectionTestIpcInt);
    LIB_FUNCTION("G4vltQ0Vs+0", "libSceNetBwe", 1, "libSceNetCtl", 1, 1, sceNetBweGetInfoIpcInt);
    LIB_FUNCTION("GqETL5+INhU", "libSceNetBwe", 1, "libSceNetCtl", 1, 1,
                 sceNetBweRegisterCallbackIpcInt);
    LIB_FUNCTION("mEUt-phGd5E", "libSceNetBwe", 1, "libSceNetCtl", 1, 1,
                 sceNetBweSetInternetConnectionTestResultIpcInt);
    LIB_FUNCTION("pQLJV5SEAqk", "libSceNetBwe", 1, "libSceNetCtl", 1, 1,
                 sceNetBweStartInternetConnectionTestBandwidthTestIpcInt);
    LIB_FUNCTION("c+aYh130SV0", "libSceNetBwe", 1, "libSceNetCtl", 1, 1,
                 sceNetBweStartInternetConnectionTestIpcInt);
    LIB_FUNCTION("0lViPaTB-R8", "libSceNetBwe", 1, "libSceNetCtl", 1, 1,
                 sceNetBweUnregisterCallbackIpcInt);
    LIB_FUNCTION("Jy1EO5GdlcM", "libSceNetCtlV6", 1, "libSceNetCtl", 1, 1, sceNetCtlGetInfoV6);
    LIB_FUNCTION("H5yARg37U5g", "libSceNetCtlV6", 1, "libSceNetCtl", 1, 1, sceNetCtlGetResultV6);
    LIB_FUNCTION("+lxqIKeU9UY", "libSceNetCtlV6", 1, "libSceNetCtl", 1, 1, sceNetCtlGetStateV6);
    LIB_FUNCTION("1NE9OWdBIww", "libSceNetCtlV6", 1, "libSceNetCtl", 1, 1,
                 sceNetCtlRegisterCallbackV6);
    LIB_FUNCTION("hIUVeUNxAwc", "libSceNetCtlV6", 1, "libSceNetCtl", 1, 1,
                 sceNetCtlUnregisterCallbackV6);
    LIB_FUNCTION("iQw3iQPhvUQ", "libSceNetCtl", 1, "libSceNetCtl", 1, 1, sceNetCtlCheckCallback);
    LIB_FUNCTION("UF6H6+kjyQs", "libSceNetCtl", 1, "libSceNetCtl", 1, 1,
                 sceNetCtlCheckCallbackForLibIpcInt);
    LIB_FUNCTION("vv6g8zoanL4", "libSceNetCtl", 1, "libSceNetCtl", 1, 1,
                 sceNetCtlClearEventForLibIpcInt);
    LIB_FUNCTION("8OJ86vFucfo", "libSceNetCtl", 1, "libSceNetCtl", 1, 1, sceNetCtlClearEventIpcInt);
    LIB_FUNCTION("HCD46HVTyQg", "libSceNetCtl", 1, "libSceNetCtl", 1, 1,
                 sceNetCtlConnectConfIpcInt);
    LIB_FUNCTION("ID+Gq3Ddzbg", "libSceNetCtl", 1, "libSceNetCtl", 1, 1, sceNetCtlConnectIpcInt);
    LIB_FUNCTION("aPpic8K75YA", "libSceNetCtl", 1, "libSceNetCtl", 1, 1,
                 sceNetCtlConnectWithRetryIpcInt);
    LIB_FUNCTION("9y4IcsJdTCc", "libSceNetCtl", 1, "libSceNetCtl", 1, 1,
                 sceNetCtlDisableBandwidthManagementIpcInt);
    LIB_FUNCTION("qOefcpoSs0k", "libSceNetCtl", 1, "libSceNetCtl", 1, 1, sceNetCtlDisconnectIpcInt);
    LIB_FUNCTION("x9bSmRSE+hc", "libSceNetCtl", 1, "libSceNetCtl", 1, 1,
                 sceNetCtlEnableBandwidthManagementIpcInt);
    LIB_FUNCTION("eCUIlA2t5CE", "libSceNetCtl", 1, "libSceNetCtl", 1, 1,
                 sceNetCtlGetBandwidthInfoIpcInt);
    LIB_FUNCTION("2EfjPXVPk3s", "libSceNetCtl", 1, "libSceNetCtl", 1, 1, sceNetCtlGetEtherLinkMode);
    LIB_FUNCTION("teuK4QnJTGg", "libSceNetCtl", 1, "libSceNetCtl", 1, 1, sceNetCtlGetIfStat);
    LIB_FUNCTION("obuxdTiwkF8", "libSceNetCtl", 1, "libSceNetCtl", 1, 1, sceNetCtlGetInfo);
    LIB_FUNCTION("xstcTqAhTys", "libSceNetCtl", 1, "libSceNetCtl", 1, 1, sceNetCtlGetInfoIpcInt);
    LIB_FUNCTION("Jy1EO5GdlcM", "libSceNetCtl", 1, "libSceNetCtl", 1, 1, sceNetCtlGetInfoV6);
    LIB_FUNCTION("arAQRFlwqaA", "libSceNetCtl", 1, "libSceNetCtl", 1, 1, sceNetCtlGetInfoV6IpcInt);
    LIB_FUNCTION("JO4yuTuMoKI", "libSceNetCtl", 1, "libSceNetCtl", 1, 1, sceNetCtlGetNatInfo);
    LIB_FUNCTION("x+cnsAxKSHo", "libSceNetCtl", 1, "libSceNetCtl", 1, 1, sceNetCtlGetNatInfoIpcInt);
    LIB_FUNCTION("hhTsdv99azU", "libSceNetCtl", 1, "libSceNetCtl", 1, 1,
                 sceNetCtlGetNetEvConfigInfoIpcInt);
    LIB_FUNCTION("0cBgduPRR+M", "libSceNetCtl", 1, "libSceNetCtl", 1, 1, sceNetCtlGetResult);
    LIB_FUNCTION("NEtnusbZyAs", "libSceNetCtl", 1, "libSceNetCtl", 1, 1, sceNetCtlGetResultIpcInt);
    LIB_FUNCTION("H5yARg37U5g", "libSceNetCtl", 1, "libSceNetCtl", 1, 1, sceNetCtlGetResultV6);
    LIB_FUNCTION("vdsTa93atXY", "libSceNetCtl", 1, "libSceNetCtl", 1, 1,
                 sceNetCtlGetResultV6IpcInt);
    LIB_FUNCTION("wP0Ab2maR1Y", "libSceNetCtl", 1, "libSceNetCtl", 1, 1,
                 sceNetCtlGetScanInfoBssidForSsidListScanIpcInt);
    LIB_FUNCTION("Wn-+887Lt2s", "libSceNetCtl", 1, "libSceNetCtl", 1, 1,
                 sceNetCtlGetScanInfoBssidIpcInt);
    LIB_FUNCTION("FEdkOG1VbQo", "libSceNetCtl", 1, "libSceNetCtl", 1, 1,
                 sceNetCtlGetScanInfoByBssidIpcInt);
    LIB_FUNCTION("irV8voIAHDw", "libSceNetCtl", 1, "libSceNetCtl", 1, 1,
                 sceNetCtlGetScanInfoForSsidListScanIpcInt);
    LIB_FUNCTION("L97eAHI0xxs", "libSceNetCtl", 1, "libSceNetCtl", 1, 1,
                 sceNetCtlGetScanInfoForSsidScanIpcInt);
    LIB_FUNCTION("uBPlr0lbuiI", "libSceNetCtl", 1, "libSceNetCtl", 1, 1, sceNetCtlGetState);
    LIB_FUNCTION("JXlI9EZVjf4", "libSceNetCtl", 1, "libSceNetCtl", 1, 1, sceNetCtlGetState2IpcInt);
    LIB_FUNCTION("gvnJPMkSoAY", "libSceNetCtl", 1, "libSceNetCtl", 1, 1, sceNetCtlGetStateIpcInt);
    LIB_FUNCTION("+lxqIKeU9UY", "libSceNetCtl", 1, "libSceNetCtl", 1, 1, sceNetCtlGetStateV6);
    LIB_FUNCTION("O8Fk4w5MWss", "libSceNetCtl", 1, "libSceNetCtl", 1, 1, sceNetCtlGetStateV6IpcInt);
    LIB_FUNCTION("BXW9b3R1Nw4", "libSceNetCtl", 1, "libSceNetCtl", 1, 1, sceNetCtlGetWifiType);
    LIB_FUNCTION("gky0+oaNM4k", "libSceNetCtl", 1, "libSceNetCtl", 1, 1, sceNetCtlInit);
    LIB_FUNCTION("YtAnCkTR0K4", "libSceNetCtl", 1, "libSceNetCtl", 1, 1,
                 sceNetCtlIsBandwidthManagementEnabledIpcInt);
    LIB_FUNCTION("UJ+Z7Q+4ck0", "libSceNetCtl", 1, "libSceNetCtl", 1, 1, sceNetCtlRegisterCallback);
    LIB_FUNCTION("WRvDk2syatE", "libSceNetCtl", 1, "libSceNetCtl", 1, 1,
                 sceNetCtlRegisterCallbackForLibIpcInt);
    LIB_FUNCTION("rqkh2kXvLSw", "libSceNetCtl", 1, "libSceNetCtl", 1, 1,
                 sceNetCtlRegisterCallbackIpcInt);
    LIB_FUNCTION("1NE9OWdBIww", "libSceNetCtl", 1, "libSceNetCtl", 1, 1,
                 sceNetCtlRegisterCallbackV6);
    LIB_FUNCTION("ipqlpcIqRsQ", "libSceNetCtl", 1, "libSceNetCtl", 1, 1,
                 sceNetCtlRegisterCallbackV6IpcInt);
    LIB_FUNCTION("reIsHryCDx4", "libSceNetCtl", 1, "libSceNetCtl", 1, 1, sceNetCtlScanIpcInt);
    LIB_FUNCTION("LJYiiIS4HB0", "libSceNetCtl", 1, "libSceNetCtl", 1, 1,
                 sceNetCtlSetErrorNotificationEnabledIpcInt);
    LIB_FUNCTION("DjuqqqV08Nk", "libSceNetCtl", 1, "libSceNetCtl", 1, 1,
                 sceNetCtlSetStunWithPaddingFlagIpcInt);
    LIB_FUNCTION("Z4wwCFiBELQ", "libSceNetCtl", 1, "libSceNetCtl", 1, 1, sceNetCtlTerm);
    LIB_FUNCTION("Rqm2OnZMCz0", "libSceNetCtl", 1, "libSceNetCtl", 1, 1,
                 sceNetCtlUnregisterCallback);
    LIB_FUNCTION("urWaUWkEGZg", "libSceNetCtl", 1, "libSceNetCtl", 1, 1,
                 sceNetCtlUnregisterCallbackForLibIpcInt);
    LIB_FUNCTION("by9cbB7JGJE", "libSceNetCtl", 1, "libSceNetCtl", 1, 1,
                 sceNetCtlUnregisterCallbackIpcInt);
    LIB_FUNCTION("hIUVeUNxAwc", "libSceNetCtl", 1, "libSceNetCtl", 1, 1,
                 sceNetCtlUnregisterCallbackV6);
    LIB_FUNCTION("Hjxpy28aID8", "libSceNetCtl", 1, "libSceNetCtl", 1, 1,
                 sceNetCtlUnregisterCallbackV6IpcInt);
    LIB_FUNCTION("1HSvkN9oxO4", "libSceNetCtl", 1, "libSceNetCtl", 1, 1,
                 sceNetCtlUnsetStunWithPaddingFlagIpcInt);
    LIB_FUNCTION("2Ny2lzU3o9w", "libSceNetCtl", 1, "libSceNetCtl", 1, 1, Func_D8DCB6973537A3DC);
    LIB_FUNCTION("u5oqtlIP+Fw", "libSceNetCtlForNpToolkit", 1, "libSceNetCtl", 1, 1,
                 sceNetCtlCheckCallbackForNpToolkit);
    LIB_FUNCTION("saYB0b2ZWtI", "libSceNetCtlForNpToolkit", 1, "libSceNetCtl", 1, 1,
                 sceNetCtlClearEventForNpToolkit);
    LIB_FUNCTION("wIsKy+TfeLs", "libSceNetCtlForNpToolkit", 1, "libSceNetCtl", 1, 1,
                 sceNetCtlRegisterCallbackForNpToolkit);
    LIB_FUNCTION("2oUqKR5odGc", "libSceNetCtlForNpToolkit", 1, "libSceNetCtl", 1, 1,
                 sceNetCtlUnregisterCallbackForNpToolkit);
    LIB_FUNCTION("19Ec7WkMFfQ", "libSceNetCtlAp", 1, "libSceNetCtl", 1, 1,
                 sceNetCtlApCheckCallback);
    LIB_FUNCTION("meFMaDpdsVI", "libSceNetCtlAp", 1, "libSceNetCtl", 1, 1, sceNetCtlApClearEvent);
    LIB_FUNCTION("hfkLVdXmfnU", "libSceNetCtlAp", 1, "libSceNetCtl", 1, 1,
                 sceNetCtlApGetConnectInfo);
    LIB_FUNCTION("LXADzTIzM9I", "libSceNetCtlAp", 1, "libSceNetCtl", 1, 1, sceNetCtlApGetInfo);
    LIB_FUNCTION("4jkLJc954+Q", "libSceNetCtlAp", 1, "libSceNetCtl", 1, 1, sceNetCtlApGetResult);
    LIB_FUNCTION("AKZOzsb9whc", "libSceNetCtlAp", 1, "libSceNetCtl", 1, 1, sceNetCtlApGetState);
    LIB_FUNCTION("FdN+edNRtiw", "libSceNetCtlAp", 1, "libSceNetCtl", 1, 1, sceNetCtlApInit);
    LIB_FUNCTION("pmjobSVHuY0", "libSceNetCtlAp", 1, "libSceNetCtl", 1, 1,
                 sceNetCtlApRegisterCallback);
    LIB_FUNCTION("r-pOyN6AhsM", "libSceNetCtlAp", 1, "libSceNetCtl", 1, 1, sceNetCtlApStop);
    LIB_FUNCTION("cv5Y2efOTeg", "libSceNetCtlAp", 1, "libSceNetCtl", 1, 1, sceNetCtlApTerm);
    LIB_FUNCTION("NpTcFtaQ-0E", "libSceNetCtlAp", 1, "libSceNetCtl", 1, 1,
                 sceNetCtlApUnregisterCallback);
    LIB_FUNCTION("R-4a9Yh4tG8", "libSceNetCtlApIpcInt", 1, "libSceNetCtl", 1, 1,
                 sceNetCtlApAppInitWpaKey);
    LIB_FUNCTION("5oLJoOVBbGU", "libSceNetCtlApIpcInt", 1, "libSceNetCtl", 1, 1,
                 sceNetCtlApAppInitWpaKeyForQa);
    LIB_FUNCTION("YtTwZ3pa4aQ", "libSceNetCtlApIpcInt", 1, "libSceNetCtl", 1, 1,
                 sceNetCtlApAppStartWithRetry);
    LIB_FUNCTION("sgWeDrEt24U", "libSceNetCtlApIpcInt", 1, "libSceNetCtl", 1, 1,
                 sceNetCtlApAppStartWithRetryPid);
    LIB_FUNCTION("amqSGH8l--s", "libSceNetCtlApIpcInt", 1, "libSceNetCtl", 1, 1,
                 sceNetCtlApRestart);
    LIB_FUNCTION("DufQZgH5ISc", "libSceNetCtlApIpcInt", 1, "libSceNetCtl", 1, 1,
                 sceNetCtlApRpCheckCallback);
    LIB_FUNCTION("qhZbOi+2qLY", "libSceNetCtlApIpcInt", 1, "libSceNetCtl", 1, 1,
                 sceNetCtlApRpClearEvent);
    LIB_FUNCTION("VQl16Q+qXeY", "libSceNetCtlApIpcInt", 1, "libSceNetCtl", 1, 1,
                 sceNetCtlApRpGetInfo);
    LIB_FUNCTION("3pxwYqHzGcw", "libSceNetCtlApIpcInt", 1, "libSceNetCtl", 1, 1,
                 sceNetCtlApRpGetResult);
    LIB_FUNCTION("LEn8FGztKWc", "libSceNetCtlApIpcInt", 1, "libSceNetCtl", 1, 1,
                 sceNetCtlApRpGetState);
    LIB_FUNCTION("ofGsK+xoAaM", "libSceNetCtlApIpcInt", 1, "libSceNetCtl", 1, 1,
                 sceNetCtlApRpRegisterCallback);
    LIB_FUNCTION("mjFgpqNavHg", "libSceNetCtlApIpcInt", 1, "libSceNetCtl", 1, 1,
                 sceNetCtlApRpStart);
    LIB_FUNCTION("HMvaHoZWsn8", "libSceNetCtlApIpcInt", 1, "libSceNetCtl", 1, 1,
                 sceNetCtlApRpStartConf);
    LIB_FUNCTION("9Dxg7XSlr2s", "libSceNetCtlApIpcInt", 1, "libSceNetCtl", 1, 1,
                 sceNetCtlApRpStartWithRetry);
    LIB_FUNCTION("6uvAl4RlEyk", "libSceNetCtlApIpcInt", 1, "libSceNetCtl", 1, 1, sceNetCtlApRpStop);
    LIB_FUNCTION("8eyH37Ns8tk", "libSceNetCtlApIpcInt", 1, "libSceNetCtl", 1, 1,
                 sceNetCtlApRpUnregisterCallback);
};

} // namespace Libraries::NetCtl
