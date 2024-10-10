// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shareplay.h"

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"

namespace Libraries::SharePlay {

int PS4_SYSV_ABI sceSharePlayCrashDaemon() {
    LOG_ERROR(Lib_SharePlay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSharePlayGetCurrentConnectionInfo(OrbisSharePlayConnectionInfo* pInfo) {
    memset(pInfo, 0, sizeof(*pInfo));
    pInfo->status = ORBIS_SHARE_PLAY_CONNECTION_STATUS_DORMANT;
    LOG_DEBUG(Lib_SharePlay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSharePlayGetCurrentConnectionInfoA() {
    LOG_ERROR(Lib_SharePlay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSharePlayGetCurrentInfo() {
    LOG_ERROR(Lib_SharePlay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSharePlayGetEvent() {
    LOG_ERROR(Lib_SharePlay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSharePlayInitialize() {
    LOG_ERROR(Lib_SharePlay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSharePlayNotifyDialogOpen() {
    LOG_ERROR(Lib_SharePlay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSharePlayNotifyForceCloseForCdlg() {
    LOG_ERROR(Lib_SharePlay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSharePlayNotifyOpenQuickMenu() {
    LOG_ERROR(Lib_SharePlay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSharePlayResumeScreenForCdlg() {
    LOG_ERROR(Lib_SharePlay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSharePlayServerLock() {
    LOG_ERROR(Lib_SharePlay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSharePlayServerUnLock() {
    LOG_ERROR(Lib_SharePlay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSharePlaySetMode() {
    LOG_ERROR(Lib_SharePlay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSharePlaySetProhibition() {
    LOG_ERROR(Lib_SharePlay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSharePlaySetProhibitionModeWithAppId() {
    LOG_ERROR(Lib_SharePlay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSharePlayStartStandby() {
    LOG_ERROR(Lib_SharePlay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSharePlayStartStreaming() {
    LOG_ERROR(Lib_SharePlay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSharePlayStopStandby() {
    LOG_ERROR(Lib_SharePlay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSharePlayStopStreaming() {
    LOG_ERROR(Lib_SharePlay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSharePlayTerminate() {
    LOG_ERROR(Lib_SharePlay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_2E93C0EA6A6B67C4() {
    LOG_ERROR(Lib_SharePlay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_C1C236728D88E177() {
    LOG_ERROR(Lib_SharePlay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_E9E80C474781F115() {
    LOG_ERROR(Lib_SharePlay, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_F3DD6199DA15ED44() {
    LOG_ERROR(Lib_SharePlay, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterlibSceSharePlay(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("ggnCfalLU-8", "libSceSharePlay", 1, "libSceSharePlay", 0, 0,
                 sceSharePlayCrashDaemon);
    LIB_FUNCTION("OOrLKB0bSDs", "libSceSharePlay", 1, "libSceSharePlay", 0, 0,
                 sceSharePlayGetCurrentConnectionInfo);
    LIB_FUNCTION("+MCXJlWdi+s", "libSceSharePlay", 1, "libSceSharePlay", 0, 0,
                 sceSharePlayGetCurrentConnectionInfoA);
    LIB_FUNCTION("vUMkWXQff3w", "libSceSharePlay", 1, "libSceSharePlay", 0, 0,
                 sceSharePlayGetCurrentInfo);
    LIB_FUNCTION("Md7Mdkr8LBc", "libSceSharePlay", 1, "libSceSharePlay", 0, 0,
                 sceSharePlayGetEvent);
    LIB_FUNCTION("isruqthpYcw", "libSceSharePlay", 1, "libSceSharePlay", 0, 0,
                 sceSharePlayInitialize);
    LIB_FUNCTION("9zwJpai7jGc", "libSceSharePlay", 1, "libSceSharePlay", 0, 0,
                 sceSharePlayNotifyDialogOpen);
    LIB_FUNCTION("VUW2V9cUTP4", "libSceSharePlay", 1, "libSceSharePlay", 0, 0,
                 sceSharePlayNotifyForceCloseForCdlg);
    LIB_FUNCTION("XL0WwUJoQPg", "libSceSharePlay", 1, "libSceSharePlay", 0, 0,
                 sceSharePlayNotifyOpenQuickMenu);
    LIB_FUNCTION("6-1fKaa5HlY", "libSceSharePlay", 1, "libSceSharePlay", 0, 0,
                 sceSharePlayResumeScreenForCdlg);
    LIB_FUNCTION("U28jAuLHj6c", "libSceSharePlay", 1, "libSceSharePlay", 0, 0,
                 sceSharePlayServerLock);
    LIB_FUNCTION("3Oaux9ITEtY", "libSceSharePlay", 1, "libSceSharePlay", 0, 0,
                 sceSharePlayServerUnLock);
    LIB_FUNCTION("QZy+KmyqKPU", "libSceSharePlay", 1, "libSceSharePlay", 0, 0, sceSharePlaySetMode);
    LIB_FUNCTION("co2NCj--pnc", "libSceSharePlay", 1, "libSceSharePlay", 0, 0,
                 sceSharePlaySetProhibition);
    LIB_FUNCTION("KADsbjNCgPo", "libSceSharePlay", 1, "libSceSharePlay", 0, 0,
                 sceSharePlaySetProhibitionModeWithAppId);
    LIB_FUNCTION("-F6NddfUsa4", "libSceSharePlay", 1, "libSceSharePlay", 0, 0,
                 sceSharePlayStartStandby);
    LIB_FUNCTION("rWVNHNnEx6g", "libSceSharePlay", 1, "libSceSharePlay", 0, 0,
                 sceSharePlayStartStreaming);
    LIB_FUNCTION("zEDkUWLVwFI", "libSceSharePlay", 1, "libSceSharePlay", 0, 0,
                 sceSharePlayStopStandby);
    LIB_FUNCTION("aGlema+JxUU", "libSceSharePlay", 1, "libSceSharePlay", 0, 0,
                 sceSharePlayStopStreaming);
    LIB_FUNCTION("UaLjloJinow", "libSceSharePlay", 1, "libSceSharePlay", 0, 0,
                 sceSharePlayTerminate);
    LIB_FUNCTION("LpPA6mprZ8Q", "libSceSharePlay", 1, "libSceSharePlay", 0, 0,
                 Func_2E93C0EA6A6B67C4);
    LIB_FUNCTION("wcI2co2I4Xc", "libSceSharePlay", 1, "libSceSharePlay", 0, 0,
                 Func_C1C236728D88E177);
    LIB_FUNCTION("6egMR0eB8RU", "libSceSharePlay", 1, "libSceSharePlay", 0, 0,
                 Func_E9E80C474781F115);
    LIB_FUNCTION("891hmdoV7UQ", "libSceSharePlay", 1, "libSceSharePlay", 0, 0,
                 Func_F3DD6199DA15ED44);
    LIB_FUNCTION("OOrLKB0bSDs", "libSceSharePlayCompat", 1, "libSceSharePlay", 0, 0,
                 sceSharePlayGetCurrentConnectionInfo);
};

} // namespace Libraries::SharePlay