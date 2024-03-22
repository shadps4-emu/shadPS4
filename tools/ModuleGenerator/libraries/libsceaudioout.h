// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "library_common.h" 

namespace Libraries::AudioOut{

int PS4_SYSV_ABI sceAudioOutDeviceIdOpen();
int PS4_SYSV_ABI sceAudioDeviceControlGet();
int PS4_SYSV_ABI sceAudioDeviceControlSet();
int PS4_SYSV_ABI sceAudioOutA3dControl();
int PS4_SYSV_ABI sceAudioOutA3dExit();
int PS4_SYSV_ABI sceAudioOutA3dInit();
int PS4_SYSV_ABI sceAudioOutAttachToApplicationByPid();
int PS4_SYSV_ABI sceAudioOutChangeAppModuleState();
int PS4_SYSV_ABI sceAudioOutClose();
int PS4_SYSV_ABI sceAudioOutDetachFromApplicationByPid();
int PS4_SYSV_ABI sceAudioOutExConfigureOutputMode();
int PS4_SYSV_ABI sceAudioOutExGetSystemInfo();
int PS4_SYSV_ABI sceAudioOutExPtClose();
int PS4_SYSV_ABI sceAudioOutExPtGetLastOutputTime();
int PS4_SYSV_ABI sceAudioOutExPtOpen();
int PS4_SYSV_ABI sceAudioOutExSystemInfoIsSupportedAudioOutExMode();
int PS4_SYSV_ABI sceAudioOutGetFocusEnablePid();
int PS4_SYSV_ABI sceAudioOutGetHandleStatusInfo();
int PS4_SYSV_ABI sceAudioOutGetInfo();
int PS4_SYSV_ABI sceAudioOutGetInfoOpenNum();
int PS4_SYSV_ABI sceAudioOutGetLastOutputTime();
int PS4_SYSV_ABI sceAudioOutGetPortState();
int PS4_SYSV_ABI sceAudioOutGetSimulatedBusUsableStatusByBusType();
int PS4_SYSV_ABI sceAudioOutGetSimulatedHandleStatusInfo();
int PS4_SYSV_ABI sceAudioOutGetSimulatedHandleStatusInfo2();
int PS4_SYSV_ABI sceAudioOutGetSparkVss();
int PS4_SYSV_ABI sceAudioOutGetSystemState();
int PS4_SYSV_ABI sceAudioOutInit();
int PS4_SYSV_ABI sceAudioOutInitIpmiGetSession();
int PS4_SYSV_ABI sceAudioOutMasteringGetState();
int PS4_SYSV_ABI sceAudioOutMasteringInit();
int PS4_SYSV_ABI sceAudioOutMasteringSetParam();
int PS4_SYSV_ABI sceAudioOutMasteringTerm();
int PS4_SYSV_ABI sceAudioOutMbusInit();
int PS4_SYSV_ABI sceAudioOutOpen();
int PS4_SYSV_ABI sceAudioOutOpenEx();
int PS4_SYSV_ABI sceAudioOutOutput();
int PS4_SYSV_ABI sceAudioOutOutputs();
int PS4_SYSV_ABI sceAudioOutPtClose();
int PS4_SYSV_ABI sceAudioOutPtGetLastOutputTime();
int PS4_SYSV_ABI sceAudioOutPtOpen();
int PS4_SYSV_ABI sceAudioOutSetConnections();
int PS4_SYSV_ABI sceAudioOutSetConnectionsForUser();
int PS4_SYSV_ABI sceAudioOutSetDevConnection();
int PS4_SYSV_ABI sceAudioOutSetHeadphoneOutMode();
int PS4_SYSV_ABI sceAudioOutSetJediJackVolume();
int PS4_SYSV_ABI sceAudioOutSetJediSpkVolume();
int PS4_SYSV_ABI sceAudioOutSetMainOutput();
int PS4_SYSV_ABI sceAudioOutSetMixLevelPadSpk();
int PS4_SYSV_ABI sceAudioOutSetMorpheusParam();
int PS4_SYSV_ABI sceAudioOutSetMorpheusWorkingMode();
int PS4_SYSV_ABI sceAudioOutSetPortConnections();
int PS4_SYSV_ABI sceAudioOutSetPortStatuses();
int PS4_SYSV_ABI sceAudioOutSetRecMode();
int PS4_SYSV_ABI sceAudioOutSetSparkParam();
int PS4_SYSV_ABI sceAudioOutSetUsbVolume();
int PS4_SYSV_ABI sceAudioOutSetVolume();
int PS4_SYSV_ABI sceAudioOutSetVolumeDown();
int PS4_SYSV_ABI sceAudioOutStartAuxBroadcast();
int PS4_SYSV_ABI sceAudioOutStartSharePlay();
int PS4_SYSV_ABI sceAudioOutStopAuxBroadcast();
int PS4_SYSV_ABI sceAudioOutStopSharePlay();
int PS4_SYSV_ABI sceAudioOutSuspendResume();
int PS4_SYSV_ABI sceAudioOutSysConfigureOutputMode();
int PS4_SYSV_ABI sceAudioOutSysGetHdmiMonitorInfo();
int PS4_SYSV_ABI sceAudioOutSysGetSystemInfo();
int PS4_SYSV_ABI sceAudioOutSysHdmiMonitorInfoIsSupportedAudioOutMode();
int PS4_SYSV_ABI sceAudioOutSystemControlGet();
int PS4_SYSV_ABI sceAudioOutSystemControlSet();
int PS4_SYSV_ABI sceAudioOutSparkControlSetEqCoef();
int PS4_SYSV_ABI sceAudioOutSetSystemDebugState();

void RegisterlibSceAudioOut(Core::Loader::SymbolsResolver * sym);
}