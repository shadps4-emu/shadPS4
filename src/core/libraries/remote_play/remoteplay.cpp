// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "remoteplay.h"

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"

namespace Libraries::Remoteplay {

int PS4_SYSV_ABI sceRemoteplayApprove() {
    LOG_ERROR(Lib_Remoteplay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRemoteplayChangeEnterKey() {
    LOG_ERROR(Lib_Remoteplay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRemoteplayClearAllRegistData() {
    LOG_ERROR(Lib_Remoteplay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRemoteplayClearConnectHistory() {
    LOG_ERROR(Lib_Remoteplay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRemoteplayConfirmDeviceRegist() {
    LOG_ERROR(Lib_Remoteplay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRemoteplayDisconnect() {
    LOG_ERROR(Lib_Remoteplay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRemoteplayGeneratePinCode() {
    LOG_ERROR(Lib_Remoteplay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRemoteplayGetApMode() {
    LOG_ERROR(Lib_Remoteplay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRemoteplayGetConnectHistory() {
    LOG_ERROR(Lib_Remoteplay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRemoteplayGetConnectionStatus(s32 userId, int* pStatus) {
    *pStatus = ORBIS_REMOTEPLAY_CONNECTION_STATUS_DISCONNECT;
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRemoteplayGetConnectUserId() {
    LOG_ERROR(Lib_Remoteplay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRemoteplayGetMbusDeviceInfo() {
    LOG_ERROR(Lib_Remoteplay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRemoteplayGetOperationStatus() {
    LOG_ERROR(Lib_Remoteplay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRemoteplayGetRemoteplayStatus() {
    LOG_ERROR(Lib_Remoteplay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRemoteplayGetRpMode() {
    LOG_ERROR(Lib_Remoteplay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRemoteplayImeClose() {
    LOG_ERROR(Lib_Remoteplay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRemoteplayImeFilterResult() {
    LOG_ERROR(Lib_Remoteplay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRemoteplayImeGetEvent() {
    LOG_ERROR(Lib_Remoteplay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRemoteplayImeNotify() {
    LOG_ERROR(Lib_Remoteplay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRemoteplayImeNotifyEventResult() {
    LOG_ERROR(Lib_Remoteplay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRemoteplayImeOpen() {
    LOG_ERROR(Lib_Remoteplay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRemoteplayImeSetCaret() {
    LOG_ERROR(Lib_Remoteplay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRemoteplayImeSetText() {
    LOG_ERROR(Lib_Remoteplay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRemoteplayInitialize() {
    LOG_ERROR(Lib_Remoteplay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRemoteplayIsRemoteOskReady() {
    LOG_ERROR(Lib_Remoteplay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRemoteplayIsRemotePlaying() {
    LOG_ERROR(Lib_Remoteplay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRemoteplayNotifyMbusDeviceRegistComplete() {
    LOG_ERROR(Lib_Remoteplay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRemoteplayNotifyNpPushWakeup() {
    LOG_ERROR(Lib_Remoteplay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRemoteplayNotifyPinCodeError() {
    LOG_ERROR(Lib_Remoteplay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRemoteplayNotifyUserDelete() {
    LOG_ERROR(Lib_Remoteplay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRemoteplayPrintAllRegistData() {
    LOG_ERROR(Lib_Remoteplay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRemoteplayProhibit() {
    LOG_ERROR(Lib_Remoteplay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRemoteplayProhibitStreaming() {
    LOG_ERROR(Lib_Remoteplay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRemoteplayServerLock() {
    LOG_ERROR(Lib_Remoteplay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRemoteplayServerUnLock() {
    LOG_ERROR(Lib_Remoteplay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRemoteplaySetApMode() {
    LOG_ERROR(Lib_Remoteplay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRemoteplaySetLogLevel() {
    LOG_ERROR(Lib_Remoteplay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRemoteplaySetProhibition() {
    LOG_ERROR(Lib_Remoteplay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRemoteplaySetProhibitionForVsh() {
    LOG_ERROR(Lib_Remoteplay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRemoteplaySetRpMode() {
    LOG_ERROR(Lib_Remoteplay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceRemoteplayTerminate() {
    LOG_ERROR(Lib_Remoteplay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_1D5EE365ED5FADB3() {
    LOG_ERROR(Lib_Remoteplay, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterlibSceRemoteplay(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("xQeIryTX7dY", "libSceRemoteplay", 1, "libSceRemoteplay", 0, 0,
                 sceRemoteplayApprove);
    LIB_FUNCTION("IYZ+Mu+8tPo", "libSceRemoteplay", 1, "libSceRemoteplay", 0, 0,
                 sceRemoteplayChangeEnterKey);
    LIB_FUNCTION("ZYUsJtcAnqA", "libSceRemoteplay", 1, "libSceRemoteplay", 0, 0,
                 sceRemoteplayClearAllRegistData);
    LIB_FUNCTION("cCheyCbF7qw", "libSceRemoteplay", 1, "libSceRemoteplay", 0, 0,
                 sceRemoteplayClearConnectHistory);
    LIB_FUNCTION("tPYT-kGbZh8", "libSceRemoteplay", 1, "libSceRemoteplay", 0, 0,
                 sceRemoteplayConfirmDeviceRegist);
    LIB_FUNCTION("6Lg4BNleJWc", "libSceRemoteplay", 1, "libSceRemoteplay", 0, 0,
                 sceRemoteplayDisconnect);
    LIB_FUNCTION("j98LdSGy4eY", "libSceRemoteplay", 1, "libSceRemoteplay", 0, 0,
                 sceRemoteplayGeneratePinCode);
    LIB_FUNCTION("L+cL-M-DP3w", "libSceRemoteplay", 1, "libSceRemoteplay", 0, 0,
                 sceRemoteplayGetApMode);
    LIB_FUNCTION("g4K51cY+PEw", "libSceRemoteplay", 1, "libSceRemoteplay", 0, 0,
                 sceRemoteplayGetConnectHistory);
    LIB_FUNCTION("g3PNjYKWqnQ", "libSceRemoteplay", 1, "libSceRemoteplay", 0, 0,
                 sceRemoteplayGetConnectionStatus);
    LIB_FUNCTION("3eBNV9A0BUM", "libSceRemoteplay", 1, "libSceRemoteplay", 0, 0,
                 sceRemoteplayGetConnectUserId);
    LIB_FUNCTION("ufesWMVX6iU", "libSceRemoteplay", 1, "libSceRemoteplay", 0, 0,
                 sceRemoteplayGetMbusDeviceInfo);
    LIB_FUNCTION("DxU4JGh4S2k", "libSceRemoteplay", 1, "libSceRemoteplay", 0, 0,
                 sceRemoteplayGetOperationStatus);
    LIB_FUNCTION("n5OxFJEvPlc", "libSceRemoteplay", 1, "libSceRemoteplay", 0, 0,
                 sceRemoteplayGetRemoteplayStatus);
    LIB_FUNCTION("Cekhs6LSHC0", "libSceRemoteplay", 1, "libSceRemoteplay", 0, 0,
                 sceRemoteplayGetRpMode);
    LIB_FUNCTION("ig1ocbR7Ptw", "libSceRemoteplay", 1, "libSceRemoteplay", 0, 0,
                 sceRemoteplayImeClose);
    LIB_FUNCTION("gV9-8cJPM3I", "libSceRemoteplay", 1, "libSceRemoteplay", 0, 0,
                 sceRemoteplayImeFilterResult);
    LIB_FUNCTION("cMk57DZXe6c", "libSceRemoteplay", 1, "libSceRemoteplay", 0, 0,
                 sceRemoteplayImeGetEvent);
    LIB_FUNCTION("-gwkQpOCl68", "libSceRemoteplay", 1, "libSceRemoteplay", 0, 0,
                 sceRemoteplayImeNotify);
    LIB_FUNCTION("58v9tSlRxc8", "libSceRemoteplay", 1, "libSceRemoteplay", 0, 0,
                 sceRemoteplayImeNotifyEventResult);
    LIB_FUNCTION("C3r2zT5ebMg", "libSceRemoteplay", 1, "libSceRemoteplay", 0, 0,
                 sceRemoteplayImeOpen);
    LIB_FUNCTION("oB730zwoz0s", "libSceRemoteplay", 1, "libSceRemoteplay", 0, 0,
                 sceRemoteplayImeSetCaret);
    LIB_FUNCTION("rOTg1Nljp8w", "libSceRemoteplay", 1, "libSceRemoteplay", 0, 0,
                 sceRemoteplayImeSetText);
    LIB_FUNCTION("k1SwgkMSOM8", "libSceRemoteplay", 1, "libSceRemoteplay", 0, 0,
                 sceRemoteplayInitialize);
    LIB_FUNCTION("R8RZC1ZIkzU", "libSceRemoteplay", 1, "libSceRemoteplay", 0, 0,
                 sceRemoteplayIsRemoteOskReady);
    LIB_FUNCTION("uYhiELUtLgA", "libSceRemoteplay", 1, "libSceRemoteplay", 0, 0,
                 sceRemoteplayIsRemotePlaying);
    LIB_FUNCTION("d-BBSEq1nfc", "libSceRemoteplay", 1, "libSceRemoteplay", 0, 0,
                 sceRemoteplayNotifyMbusDeviceRegistComplete);
    LIB_FUNCTION("Yytq7NE38R8", "libSceRemoteplay", 1, "libSceRemoteplay", 0, 0,
                 sceRemoteplayNotifyNpPushWakeup);
    LIB_FUNCTION("Wg-w8xjMZA4", "libSceRemoteplay", 1, "libSceRemoteplay", 0, 0,
                 sceRemoteplayNotifyPinCodeError);
    LIB_FUNCTION("yheulqylKwI", "libSceRemoteplay", 1, "libSceRemoteplay", 0, 0,
                 sceRemoteplayNotifyUserDelete);
    LIB_FUNCTION("t5ZvUiZ1hpE", "libSceRemoteplay", 1, "libSceRemoteplay", 0, 0,
                 sceRemoteplayPrintAllRegistData);
    LIB_FUNCTION("mrNh78tBpmg", "libSceRemoteplay", 1, "libSceRemoteplay", 0, 0,
                 sceRemoteplayProhibit);
    LIB_FUNCTION("7QLrixwVHcU", "libSceRemoteplay", 1, "libSceRemoteplay", 0, 0,
                 sceRemoteplayProhibitStreaming);
    LIB_FUNCTION("-ThIlThsN80", "libSceRemoteplay", 1, "libSceRemoteplay", 0, 0,
                 sceRemoteplayServerLock);
    LIB_FUNCTION("0Z-Pm5rZJOI", "libSceRemoteplay", 1, "libSceRemoteplay", 0, 0,
                 sceRemoteplayServerUnLock);
    LIB_FUNCTION("xSrhtSLIjOc", "libSceRemoteplay", 1, "libSceRemoteplay", 0, 0,
                 sceRemoteplaySetApMode);
    LIB_FUNCTION("5-2agAeaE+c", "libSceRemoteplay", 1, "libSceRemoteplay", 0, 0,
                 sceRemoteplaySetLogLevel);
    LIB_FUNCTION("Rf0XMVR7xPw", "libSceRemoteplay", 1, "libSceRemoteplay", 0, 0,
                 sceRemoteplaySetProhibition);
    LIB_FUNCTION("n4l3FTZtNQM", "libSceRemoteplay", 1, "libSceRemoteplay", 0, 0,
                 sceRemoteplaySetProhibitionForVsh);
    LIB_FUNCTION("-BPcEQ1w8xc", "libSceRemoteplay", 1, "libSceRemoteplay", 0, 0,
                 sceRemoteplaySetRpMode);
    LIB_FUNCTION("BOwybKVa3Do", "libSceRemoteplay", 1, "libSceRemoteplay", 0, 0,
                 sceRemoteplayTerminate);
    LIB_FUNCTION("HV7jZe1frbM", "libSceRemoteplay", 1, "libSceRemoteplay", 0, 0,
                 Func_1D5EE365ED5FADB3);
};

} // namespace Libraries::Remoteplay