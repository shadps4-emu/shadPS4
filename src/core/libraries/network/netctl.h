// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/network/net_ctl_obj.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::NetCtl {

constexpr int ORBIS_NET_ETHER_ADDR_LEN = 6;

struct OrbisNetEtherAddr {
    u8 data[ORBIS_NET_ETHER_ADDR_LEN];
};

constexpr int ORBIS_NET_CTL_SSID_LEN = 32 + 1;
constexpr int ORBIS_NET_CTL_HOSTNAME_LEN = 255 + 1;
constexpr int ORBIS_NET_CTL_AUTH_NAME_LEN = 127 + 1;
constexpr int ORBIS_NET_CTL_IPV4_ADDR_STR_LEN = 16;

union OrbisNetCtlInfo {
    u32 device;
    OrbisNetEtherAddr ether_addr;
    u32 mtu;
    u32 link;
    OrbisNetEtherAddr bssid;
    char ssid[ORBIS_NET_CTL_SSID_LEN];
    u32 wifi_security;
    u8 rssi_dbm;
    uint8_t rssi_percentage;
    u8 channel;
    u32 ip_config;
    char dhcp_hostname[ORBIS_NET_CTL_HOSTNAME_LEN];
    char pppoe_auth_name[ORBIS_NET_CTL_AUTH_NAME_LEN];
    char ip_address[ORBIS_NET_CTL_IPV4_ADDR_STR_LEN];
    char netmask[ORBIS_NET_CTL_IPV4_ADDR_STR_LEN];
    char default_route[ORBIS_NET_CTL_IPV4_ADDR_STR_LEN];
    char primary_dns[ORBIS_NET_CTL_IPV4_ADDR_STR_LEN];
    char secondary_dns[ORBIS_NET_CTL_IPV4_ADDR_STR_LEN];
    u32 http_proxy_config;
    char http_proxy_server[ORBIS_NET_CTL_HOSTNAME_LEN];
    u16 http_proxy_port;
};

// GetInfo codes
constexpr int ORBIS_NET_CTL_INFO_DEVICE = 1;
constexpr int ORBIS_NET_CTL_INFO_ETHER_ADDR = 2;
constexpr int ORBIS_NET_CTL_INFO_MTU = 3;
constexpr int ORBIS_NET_CTL_INFO_LINK = 4;
constexpr int ORBIS_NET_CTL_INFO_BSSID = 5;
constexpr int ORBIS_NET_CTL_INFO_SSID = 6;
constexpr int ORBIS_NET_CTL_INFO_WIFI_SECURITY = 7;
constexpr int ORBIS_NET_CTL_INFO_RSSI_DBM = 8;
constexpr int ORBIS_NET_CTL_INFO_RSSI_PERCENTAGE = 9;
constexpr int ORBIS_NET_CTL_INFO_CHANNEL = 10;
constexpr int ORBIS_NET_CTL_INFO_IP_CONFIG = 11;
constexpr int ORBIS_NET_CTL_INFO_DHCP_HOSTNAME = 12;
constexpr int ORBIS_NET_CTL_INFO_PPPOE_AUTH_NAME = 13;
constexpr int ORBIS_NET_CTL_INFO_IP_ADDRESS = 14;
constexpr int ORBIS_NET_CTL_INFO_NETMASK = 15;
constexpr int ORBIS_NET_CTL_INFO_DEFAULT_ROUTE = 16;
constexpr int ORBIS_NET_CTL_INFO_PRIMARY_DNS = 17;
constexpr int ORBIS_NET_CTL_INFO_SECONDARY_DNS = 18;
constexpr int ORBIS_NET_CTL_INFO_HTTP_PROXY_CONFIG = 19;
constexpr int ORBIS_NET_CTL_INFO_HTTP_PROXY_SERVER = 20;
constexpr int ORBIS_NET_CTL_INFO_HTTP_PROXY_PORT = 21;

int PS4_SYSV_ABI sceNetBweCheckCallbackIpcInt();
int PS4_SYSV_ABI sceNetBweClearEventIpcInt();
int PS4_SYSV_ABI sceNetBweFinishInternetConnectionTestIpcInt();
int PS4_SYSV_ABI sceNetBweGetInfoIpcInt();
int PS4_SYSV_ABI sceNetBweRegisterCallbackIpcInt();
int PS4_SYSV_ABI sceNetBweSetInternetConnectionTestResultIpcInt();
int PS4_SYSV_ABI sceNetBweStartInternetConnectionTestBandwidthTestIpcInt();
int PS4_SYSV_ABI sceNetBweStartInternetConnectionTestIpcInt();
int PS4_SYSV_ABI sceNetBweUnregisterCallbackIpcInt();
int PS4_SYSV_ABI sceNetCtlGetInfoV6();
int PS4_SYSV_ABI sceNetCtlGetResultV6();
int PS4_SYSV_ABI sceNetCtlGetStateV6();
int PS4_SYSV_ABI sceNetCtlRegisterCallbackV6();
int PS4_SYSV_ABI sceNetCtlUnregisterCallbackV6();
int PS4_SYSV_ABI sceNetCtlCheckCallback();
int PS4_SYSV_ABI sceNetCtlCheckCallbackForLibIpcInt();
int PS4_SYSV_ABI sceNetCtlClearEventForLibIpcInt();
int PS4_SYSV_ABI sceNetCtlClearEventIpcInt();
int PS4_SYSV_ABI sceNetCtlConnectConfIpcInt();
int PS4_SYSV_ABI sceNetCtlConnectIpcInt();
int PS4_SYSV_ABI sceNetCtlConnectWithRetryIpcInt();
int PS4_SYSV_ABI sceNetCtlDisableBandwidthManagementIpcInt();
int PS4_SYSV_ABI sceNetCtlDisconnectIpcInt();
int PS4_SYSV_ABI sceNetCtlEnableBandwidthManagementIpcInt();
int PS4_SYSV_ABI sceNetCtlGetBandwidthInfoIpcInt();
int PS4_SYSV_ABI sceNetCtlGetEtherLinkMode();
int PS4_SYSV_ABI sceNetCtlGetIfStat();
int PS4_SYSV_ABI sceNetCtlGetInfo(int code, OrbisNetCtlInfo* info);
int PS4_SYSV_ABI sceNetCtlGetInfoIpcInt();
int PS4_SYSV_ABI sceNetCtlGetInfoV6IpcInt();
int PS4_SYSV_ABI sceNetCtlGetNatInfo();
int PS4_SYSV_ABI sceNetCtlGetNatInfoIpcInt();
int PS4_SYSV_ABI sceNetCtlGetNetEvConfigInfoIpcInt();
int PS4_SYSV_ABI sceNetCtlGetResult(int eventType, int* errorCode);
int PS4_SYSV_ABI sceNetCtlGetResultIpcInt();
int PS4_SYSV_ABI sceNetCtlGetResultV6IpcInt();
int PS4_SYSV_ABI sceNetCtlGetScanInfoBssidForSsidListScanIpcInt();
int PS4_SYSV_ABI sceNetCtlGetScanInfoBssidIpcInt();
int PS4_SYSV_ABI sceNetCtlGetScanInfoByBssidIpcInt();
int PS4_SYSV_ABI sceNetCtlGetScanInfoForSsidListScanIpcInt();
int PS4_SYSV_ABI sceNetCtlGetScanInfoForSsidScanIpcInt();
int PS4_SYSV_ABI sceNetCtlGetState(int* state);
int PS4_SYSV_ABI sceNetCtlGetState2IpcInt();
int PS4_SYSV_ABI sceNetCtlGetStateIpcInt();
int PS4_SYSV_ABI sceNetCtlGetStateV6IpcInt();
int PS4_SYSV_ABI sceNetCtlGetWifiType();
int PS4_SYSV_ABI sceNetCtlInit();
int PS4_SYSV_ABI sceNetCtlIsBandwidthManagementEnabledIpcInt();
int PS4_SYSV_ABI sceNetCtlRegisterCallback(OrbisNetCtlCallback func, void* arg, int* cid);
int PS4_SYSV_ABI sceNetCtlRegisterCallbackForLibIpcInt();
int PS4_SYSV_ABI sceNetCtlRegisterCallbackIpcInt();
int PS4_SYSV_ABI sceNetCtlRegisterCallbackV6IpcInt();
int PS4_SYSV_ABI sceNetCtlScanIpcInt();
int PS4_SYSV_ABI sceNetCtlSetErrorNotificationEnabledIpcInt();
int PS4_SYSV_ABI sceNetCtlSetStunWithPaddingFlagIpcInt();
int PS4_SYSV_ABI sceNetCtlTerm();
int PS4_SYSV_ABI sceNetCtlUnregisterCallback();
int PS4_SYSV_ABI sceNetCtlUnregisterCallbackForLibIpcInt();
int PS4_SYSV_ABI sceNetCtlUnregisterCallbackIpcInt();
int PS4_SYSV_ABI sceNetCtlUnregisterCallbackV6IpcInt();
int PS4_SYSV_ABI sceNetCtlUnsetStunWithPaddingFlagIpcInt();
int PS4_SYSV_ABI Func_D8DCB6973537A3DC();
int PS4_SYSV_ABI sceNetCtlCheckCallbackForNpToolkit();
int PS4_SYSV_ABI sceNetCtlClearEventForNpToolkit();
int PS4_SYSV_ABI sceNetCtlRegisterCallbackForNpToolkit(OrbisNetCtlCallbackForNpToolkit func,
                                                       void* arg, int* cid);
int PS4_SYSV_ABI sceNetCtlUnregisterCallbackForNpToolkit();
int PS4_SYSV_ABI sceNetCtlApCheckCallback();
int PS4_SYSV_ABI sceNetCtlApClearEvent();
int PS4_SYSV_ABI sceNetCtlApGetConnectInfo();
int PS4_SYSV_ABI sceNetCtlApGetInfo();
int PS4_SYSV_ABI sceNetCtlApGetResult();
int PS4_SYSV_ABI sceNetCtlApGetState();
int PS4_SYSV_ABI sceNetCtlApInit();
int PS4_SYSV_ABI sceNetCtlApRegisterCallback();
int PS4_SYSV_ABI sceNetCtlApStop();
int PS4_SYSV_ABI sceNetCtlApTerm();
int PS4_SYSV_ABI sceNetCtlApUnregisterCallback();
int PS4_SYSV_ABI sceNetCtlApAppInitWpaKey();
int PS4_SYSV_ABI sceNetCtlApAppInitWpaKeyForQa();
int PS4_SYSV_ABI sceNetCtlApAppStartWithRetry();
int PS4_SYSV_ABI sceNetCtlApAppStartWithRetryPid();
int PS4_SYSV_ABI sceNetCtlApRestart();
int PS4_SYSV_ABI sceNetCtlApRpCheckCallback();
int PS4_SYSV_ABI sceNetCtlApRpClearEvent();
int PS4_SYSV_ABI sceNetCtlApRpGetInfo();
int PS4_SYSV_ABI sceNetCtlApRpGetResult();
int PS4_SYSV_ABI sceNetCtlApRpGetState();
int PS4_SYSV_ABI sceNetCtlApRpRegisterCallback();
int PS4_SYSV_ABI sceNetCtlApRpStart();
int PS4_SYSV_ABI sceNetCtlApRpStartConf();
int PS4_SYSV_ABI sceNetCtlApRpStartWithRetry();
int PS4_SYSV_ABI sceNetCtlApRpStop();
int PS4_SYSV_ABI sceNetCtlApRpUnregisterCallback();

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::NetCtl
