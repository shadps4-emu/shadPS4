// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::NetCtl {

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
int PS4_SYSV_ABI sceNetCtlGetInfo();
int PS4_SYSV_ABI sceNetCtlGetInfoIpcInt();
int PS4_SYSV_ABI sceNetCtlGetInfoV6IpcInt();
int PS4_SYSV_ABI sceNetCtlGetNatInfo();
int PS4_SYSV_ABI sceNetCtlGetNatInfoIpcInt();
int PS4_SYSV_ABI sceNetCtlGetNetEvConfigInfoIpcInt();
int PS4_SYSV_ABI sceNetCtlGetResult();
int PS4_SYSV_ABI sceNetCtlGetResultIpcInt();
int PS4_SYSV_ABI sceNetCtlGetResultV6IpcInt();
int PS4_SYSV_ABI sceNetCtlGetScanInfoBssidForSsidListScanIpcInt();
int PS4_SYSV_ABI sceNetCtlGetScanInfoBssidIpcInt();
int PS4_SYSV_ABI sceNetCtlGetScanInfoByBssidIpcInt();
int PS4_SYSV_ABI sceNetCtlGetScanInfoForSsidListScanIpcInt();
int PS4_SYSV_ABI sceNetCtlGetScanInfoForSsidScanIpcInt();
int PS4_SYSV_ABI sceNetCtlGetState();
int PS4_SYSV_ABI sceNetCtlGetState2IpcInt();
int PS4_SYSV_ABI sceNetCtlGetStateIpcInt();
int PS4_SYSV_ABI sceNetCtlGetStateV6IpcInt();
int PS4_SYSV_ABI sceNetCtlGetWifiType();
int PS4_SYSV_ABI sceNetCtlInit();
int PS4_SYSV_ABI sceNetCtlIsBandwidthManagementEnabledIpcInt();
int PS4_SYSV_ABI sceNetCtlRegisterCallback();
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
int PS4_SYSV_ABI sceNetCtlRegisterCallbackForNpToolkit();
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

void RegisterlibSceNetCtl(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::NetCtl
