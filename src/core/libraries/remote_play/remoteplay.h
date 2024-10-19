// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

// returning codes in sceRemoteplayGetConnectionStatus pstatus
constexpr int ORBIS_REMOTEPLAY_CONNECTION_STATUS_DISCONNECT = 0;
constexpr int ORBIS_REMOTEPLAY_CONNECTION_STATUS_CONNECT = 1;

namespace Libraries::Remoteplay {

int PS4_SYSV_ABI sceRemoteplayApprove();
int PS4_SYSV_ABI sceRemoteplayChangeEnterKey();
int PS4_SYSV_ABI sceRemoteplayClearAllRegistData();
int PS4_SYSV_ABI sceRemoteplayClearConnectHistory();
int PS4_SYSV_ABI sceRemoteplayConfirmDeviceRegist();
int PS4_SYSV_ABI sceRemoteplayDisconnect();
int PS4_SYSV_ABI sceRemoteplayGeneratePinCode();
int PS4_SYSV_ABI sceRemoteplayGetApMode();
int PS4_SYSV_ABI sceRemoteplayGetConnectHistory();
int PS4_SYSV_ABI sceRemoteplayGetConnectionStatus(s32 userId, int* pStatus);
int PS4_SYSV_ABI sceRemoteplayGetConnectUserId();
int PS4_SYSV_ABI sceRemoteplayGetMbusDeviceInfo();
int PS4_SYSV_ABI sceRemoteplayGetOperationStatus();
int PS4_SYSV_ABI sceRemoteplayGetRemoteplayStatus();
int PS4_SYSV_ABI sceRemoteplayGetRpMode();
int PS4_SYSV_ABI sceRemoteplayImeClose();
int PS4_SYSV_ABI sceRemoteplayImeFilterResult();
int PS4_SYSV_ABI sceRemoteplayImeGetEvent();
int PS4_SYSV_ABI sceRemoteplayImeNotify();
int PS4_SYSV_ABI sceRemoteplayImeNotifyEventResult();
int PS4_SYSV_ABI sceRemoteplayImeOpen();
int PS4_SYSV_ABI sceRemoteplayImeSetCaret();
int PS4_SYSV_ABI sceRemoteplayImeSetText();
int PS4_SYSV_ABI sceRemoteplayInitialize();
int PS4_SYSV_ABI sceRemoteplayIsRemoteOskReady();
int PS4_SYSV_ABI sceRemoteplayIsRemotePlaying();
int PS4_SYSV_ABI sceRemoteplayNotifyMbusDeviceRegistComplete();
int PS4_SYSV_ABI sceRemoteplayNotifyNpPushWakeup();
int PS4_SYSV_ABI sceRemoteplayNotifyPinCodeError();
int PS4_SYSV_ABI sceRemoteplayNotifyUserDelete();
int PS4_SYSV_ABI sceRemoteplayPrintAllRegistData();
int PS4_SYSV_ABI sceRemoteplayProhibit();
int PS4_SYSV_ABI sceRemoteplayProhibitStreaming();
int PS4_SYSV_ABI sceRemoteplayServerLock();
int PS4_SYSV_ABI sceRemoteplayServerUnLock();
int PS4_SYSV_ABI sceRemoteplaySetApMode();
int PS4_SYSV_ABI sceRemoteplaySetLogLevel();
int PS4_SYSV_ABI sceRemoteplaySetProhibition();
int PS4_SYSV_ABI sceRemoteplaySetProhibitionForVsh();
int PS4_SYSV_ABI sceRemoteplaySetRpMode();
int PS4_SYSV_ABI sceRemoteplayTerminate();
int PS4_SYSV_ABI Func_1D5EE365ED5FADB3();

void RegisterlibSceRemoteplay(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Remoteplay