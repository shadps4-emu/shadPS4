// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/config.h"
#include "common/logging/log.h"
#include "common/singleton.h"
#include "core/file_sys/fs.h"
#include "core/libraries/libs.h"
#include "core/libraries/system/systemservice.h"
#include "core/libraries/system/systemservice_error.h"
#include "emulator.h"

namespace Libraries::SystemService {

bool g_splash_status{true};
std::queue<OrbisSystemServiceEvent> g_event_queue;
std::mutex g_event_queue_mutex;

bool IsSplashVisible() {
    return Config::showSplash() && g_splash_status;
}

int PS4_SYSV_ABI sceAppMessagingClearEventFlag() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAppMessagingReceiveMsg() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAppMessagingSendMsg() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAppMessagingSendMsgToShellCore() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAppMessagingSendMsgToShellUI() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAppMessagingSetEventFlag() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAppMessagingTryGetEventFlag() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceAppMessagingTryReceiveMsg() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_C8E899ABEF7F64C4() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_F74BA759B9C8D2A1() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilAcquireCpuBudgetOfExtraAudioDevices() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilAcquireCpuBudgetOfImeForBigApp() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilAcquireCpuBudgetOfInGameStore() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilActivateCdlg() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilAddLocalProcess() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilBlockAppSuspend() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilBlockingGetEventForDaemon() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilContinueApp() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilCrashSyscore() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilDeactivateCdlg() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilDeclareReadyForSuspend() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilDisableSuspendNotification() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilEnableSuspendNotification() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilFinishSpecialResume() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilForceKillApp() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilForceKillLocalProcess() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilGetApp0DirPath() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilGetAppCategory() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilGetAppFocusedAppStatus() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilGetAppId() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilGetAppIdOfBigApp() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilGetAppIdOfMiniApp() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilGetAppLaunchedUser() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilGetAppStatus() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilGetAppStatusListForShellUIReboot() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilGetAppTitleId() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilGetAppType() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilGetCdlgExec() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilGetCoredumpState() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilGetDbgExecutablePath() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilGetEventForDaemon() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilGetEventForShellUI() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilGetGpuCrashFullDumpAppStatus() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilGetLocalProcessStatusList() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilGetParentSocket() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilGetResultKillApp() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilGetResultLaunchAppByTitleId() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilInitialize() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilIsActiveCdlg() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilIsAppLaunched() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilIsAppSuspended() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilIsCpuBudgetOfExtraAudioDevicesAvailable() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilIsPs2Emu() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilIsShellUiFgAndGameBgCpuMode() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilKickCoredumpOnlyProcMem() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilKillApp() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilKillAppWithReason() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilKillLocalProcess() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilLaunchApp() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilLoadExec() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilNotifyCoredumpRequestEnd() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilNotifyCoredumpRequestProgress() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilNotifyVshReady() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilRaiseException() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilRaiseExceptionLocalProcess() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilRegisterCdlgSharedMemoryName() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilRegisterDaemon() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilRegisterShellUI() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilReleaseCpuBudgetOfExtraAudioDevices() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilReleaseCpuBudgetOfImeForBigApp() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilReleaseCpuBudgetOfInGameStore() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilResumeApp() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilResumeLocalProcess() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilSetAppFocus() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilSetCdlgExec() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilSetControllerFocus() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilSetControllerFocusPermission() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilStartKillApp() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilStartLaunchAppByTitleId() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilSuspendApp() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilSuspendBackgroundApp() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilSuspendLocalProcess() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilSystemSuspend() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilTerminate() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilTryBlockAppSuspend() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilUnblockAppSuspend() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilUnregisterCdlgSharedMemoryName() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilUnregisterDaemon() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceLncUtilUnregisterShellUI() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceActivateHevcSoft() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceActivateHevcSoftAbort() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceActivateHevcSoftGetStatus() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceActivateHevcSoftInit() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceActivateHevcSoftIsActivated() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceActivateHevcSoftStart() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceActivateHevcSoftTerm() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilAccessibilityZoomLock() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilAccessibilityZoomUnlock() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilAcquireBgmCpuBudget() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilAcquireRemotePlayCpuBudget() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilAcquireSharePlayCpuBudget() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilActivateAbort() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilActivateGetStatus() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilActivateInit() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilActivateIsActivated() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilActivateRecordActivation() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilActivateStart() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilActivateStartAsync() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilActivateTerm() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilChangeRunLevel() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilChangeToStaffModeForIDU() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilCheckerAbort() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilCleanupCrashReport() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilClearAppData() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilClearPsnAccountInfo() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilCrashReportRequestCancel() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilDeclareBeginOfExternalStorageAppMove() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilDeclareEndOfExternalStorageAppMove() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilDeleteDiscInstalledTitleWorkaroundFile() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilDeleteDownloadedHidConfigFile() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilDeleteDownloadedNetEvConfigFile() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilDeleteDownloadedTitleWorkaroundFile() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilDeleteSmrHddDummyData() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilDoFsck() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilDownloadHidConfigFileFromServer() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilDownloadNetEvConfigFileFromServer() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilDownloadTitleWorkaroundFileFromServer() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilEnterPowerLockSection() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilExecuteCrashReport() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilExfatFormatExternalHdd() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilExitMiniApp() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilExitMiniAppWithValue() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilFillUpSpaceOnSmrHdd() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilFormatExternalHdd() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilFormatHddForRestore() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilFreeUpSpaceOnSmrHdd() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetAppData() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetAppEnableTTS() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetAppEnterButtonAssign() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetAppLaunchedParamInt() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetAppLaunchedParamIntByBudgetType() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetAppLaunchedParamString() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetAppLaunchedParamStringByBudgetType() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetAppLaunchTypeInfo() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetAutoPowerDownRemainingSeconds() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetBasicProductShape() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetCheckerString() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetCheckerStringEx() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetCloudClientStatus() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetCrashReportCoreFileSetSize() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetCrashReportFilterInfoStart() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetCrashReportInfoForBoot() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetCrashReportInfoForBootStart() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetCrashReportInfoStart() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreutilGetCrashReportProcessInformation() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetCrashReportResult() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetCrashReportStatus() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetCrashReportUploadStatus() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetDeviceIndexBehavior() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetDeviceIndexBehaviorWithTimeout() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetEffectiveTotalSizeOfUserPartition() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetFreeSizeOfAvContentsTmp() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetFreeSizeOfUserPartition() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetFsckProgress() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetGameLiveStreamingStatus() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetGnmCompositorOnScreenProfilerFlag() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetGpuLoadEmulationMode() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetGpuLoadEmulationModeByAppId() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetHidConfigFileInfoString() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetHidConfigFileString() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetHidConfigName() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetHidConfigNum() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetIDUMode() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetImposeMenuFlagForPs2Emu() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetManifestFileStatus() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetNeedSizeOfAppContent() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetNetEvConfigFileInfoString() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetOptimizationStatus() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetOutOfVrPlayZoneWarning() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetPapcGamePcl() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetPbtcUserInfoList() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetPlatformPrivacyDefinitionEventData() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetPlatformPrivacySetting() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetProgressOfFormatExternalHdd() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetProgressOfFsck() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetPsnAccountInfo() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetPsStoreIconLayout() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetPsStoreIconState() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetRegion() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetRemotePlayStatus() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetRunLevel() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetSharePlayStatus() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetShellUIVMStats() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetSmrHddInfoString() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetSocialScreenStatus() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetSplashScreenState() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetSupportSiteURL() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetSuspendConfirmationDialogFlag() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetSystemBGState() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetSystemBGWaveColor() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetSystemBGWaveState() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetTitleWorkaroundFileInfoString() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetTitleWorkaroundFileString() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetUIStatus() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetUserFocus() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetUserIdOfMorpheusUser() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGetVersionNumberOfCameraCalibrationData() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilGoBackToKratosCurrentSessionGame() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilHideBlocksFromUser() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilIncrementVersionNumberOfCameraCalibrationData() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilIsAccessibilityZoomLocked() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilIsBgmCpuBudgetAcquired() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilIsBgmCpuBudgetAvailable() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilIsBgmPlaying() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilIsExternalStorageAppMoveInProgress() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilIsEyeToEyeDistanceAdjusted() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilIsGameLiveStreamingOnAir() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilIsImposeScreenOverlaid() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilIsInSystemSuspendBlackList() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilIsInternalKratosUser() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilIsKilledOrSuspendedByLogout() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilIsNeededCrashReport() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilIsPowerSaveAlertRequested() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilIsRemotePlayCpuBudgetAcquired() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilIsScreenSaverOn() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilIsSharePlayCpuBudgetAcquired() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilIsShowCrashReport() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilIsTemperatureDanger() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilIsTitleWorkaroundEnabled() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilIsUsbMassStorageMounted() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilLaunchByUri() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilLeavePowerLockSection() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilLog() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilMakeManifestFile() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilMountAppRight() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilMountDownloadDataForShellUI() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilMountHddForBackup() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilMountHddForRestore() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilNavigateToAnotherApp() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilNavigateToGoHome() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilNavigateToLaunchedApp() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilNotificationCancelForIDU() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilNotificationRequestedForIDU() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilNotifyBgmCoreTermination() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilNotifyFarsightUIDone() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilNotifyFsReadError() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilNotifyPsnAccountInfoReceived() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilNotifyYouTubeAccountLinkStatusChanged() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilPfAuthClientConsoleTokenClearCache() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilPostActivityForPsNow() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilPostErrorLog() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilPostLaunchConfirmResult() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilPostPsmEventToShellUI() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilPreNotifyOfGameLiveStreaming() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilPreNotifyOfRemotePlay() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilPreNotifyOfSharePlay() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilReleaseBgmCpuBudget() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilReleaseRemotePlayCpuBudget() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilReleaseSharePlayCpuBudget() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilReportSessionErrorToGaikaiController() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilReportUnexpectedFatalErrorToSystemTelemetry() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilRequestCameraCalibration() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilRequestEjectDevice() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilRequestRebootApp() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilRequestShutdown() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilResetAutoPowerDownTimer() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilResetBgdcConfig() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilSetAppData() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilSetBgmProhibition() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilSetDeviceIndexBehavior() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilSetGameLiveStreamingOnAirFlag() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilSetGameLiveStreamingStatus() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilSetGnmCompositorOnScreenProfilerFlag() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilSetGpuLoadEmulationMode() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilSetGpuLoadEmulationModeByAppId() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilSetIDUMode() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilSetImposeStatusFlag() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilSetPsStoreIconLayout() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilSetPsStoreIconState() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilSetRemotePlayStatus() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilSetSharePlayStatus() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilSetSkipUpdateCheck() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilSetSocialScreenStatus() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilSetSplashScreenState() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilSetSystemBGState() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilSetSystemBGWaveColor() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilSetSystemBGWaveState() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilSetUIStatus() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilSetUserFocus() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilShowCriticalErrorDialog() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilShowErrorDialog() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilShowErrorDialogWithFormatArgs() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilShowErrorDialogWithParam() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilShowPsUnderLockIndicator() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilSignalUserInput() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilStartOptimization() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilStartPsNowGame() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilStopOptimization() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilStopPsNowGame() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilTestBusTransferSpeed() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilTickHeartBeat() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilTriggerPapcRecalculation() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilTriggerPapcUpdate() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilTurnOffScreenSaver() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilUnmountAppRight() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilUnmountDownloadDataForShellUI() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilUnmountHddForBackup() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilUnmountHddForRestore() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceShellCoreUtilWriteSmrHddDummyData() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_1E5CA5A71FA7F028() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_6D43644F75C38346() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_739FB849CB28F445() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_B20628FF35C74111() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceAddLocalProcessForJvm() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceGetParentSocketForJvm() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceKillLocalProcessForJvm() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceAcquireFb0() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceAddLocalProcess() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceAddLocalProcessForPsmKit() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceChangeAcpClock() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceChangeCpuClock() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceChangeGpuClock() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceChangeMemoryClock() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceChangeMemoryClockToBaseMode() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceChangeMemoryClockToDefault() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceChangeMemoryClockToMultiMediaMode() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceChangeNumberOfGpuCu() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceChangeSamuClock() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceChangeUvdClock() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceChangeVceClock() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceDisableMusicPlayer() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceDisablePersonalEyeToEyeDistanceSetting() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceDisableSuspendConfirmationDialog() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceEnablePersonalEyeToEyeDistanceSetting() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceEnableSuspendConfirmationDialog() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceGetAppFocusedAppStatus() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceGetAppIdOfBigApp() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceGetAppIdOfMiniApp() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceGetAppStatus() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceGetAppType() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI
sceSystemServiceGetDisplaySafeAreaInfo(OrbisSystemServiceDisplaySafeAreaInfo* info) {
    LOG_DEBUG(Lib_SystemService, "called");
    if (info == nullptr) {
        LOG_ERROR(Lib_SystemService, "OrbisSystemServiceDisplaySafeAreaInfo is null");
        return ORBIS_SYSTEM_SERVICE_ERROR_PARAMETER;
    }
    info->ratio = 1.0f;
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceGetEventForDaemon() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceGetGpuLoadEmulationMode() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceGetHdrToneMapLuminance() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceGetLocalProcessStatusList() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceGetParentSocket() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceGetParentSocketForPsmKit() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceGetPSButtonEvent() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceGetRenderingMode() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSystemServiceGetStatus(OrbisSystemServiceStatus* status) {
    LOG_TRACE(Lib_SystemService, "called");
    if (status == nullptr) {
        LOG_ERROR(Lib_SystemService, "OrbisSystemServiceStatus is null");
        return ORBIS_SYSTEM_SERVICE_ERROR_PARAMETER;
    }

    std::lock_guard<std::mutex> lock(g_event_queue_mutex);
    status->event_num = static_cast<s32>(g_event_queue.size());
    status->is_system_ui_overlaid = false;
    status->is_in_background_execution = false;
    status->is_cpu_mode7_cpu_normal = true;
    status->is_game_live_streaming_on_air = false;
    status->is_out_of_vr_play_area = false;
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceGetTitleWorkaroundInfo() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceGetVersionNumberOfCameraCalibrationData() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSystemServiceHideSplashScreen() {
    LOG_INFO(Lib_SystemService, "called");
    g_splash_status = false;
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceIsAppSuspended() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceIsBgmPlaying() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceIsEyeToEyeDistanceAdjusted() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceIsScreenSaverOn() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceIsShellUiFgAndGameBgCpuMode() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceKillApp() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceKillLocalProcess() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceKillLocalProcessForPsmKit() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceLaunchApp() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceLaunchEventDetails() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceLaunchTournamentList() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceLaunchTournamentsTeamProfile() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceLaunchWebBrowser() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceLoadExec(const char* path, const char* argv[]) {
    LOG_DEBUG(Lib_SystemService, "called");
    auto emu = Common::Singleton<Core::Emulator>::Instance();
    auto mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();
    auto hostPath = mnt->GetHostPath(std::string_view(path));
    std::vector<std::string> args;
    if (argv != nullptr) {
        for (const char** ptr = argv; *ptr != nullptr; ptr++) {
            args.push_back(std::string(*ptr));
        }
    }
    emu->Restart(hostPath, args);
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceNavigateToAnotherApp() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceNavigateToGoBack() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceNavigateToGoBackWithValue() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceNavigateToGoHome() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSystemServiceParamGetInt(OrbisSystemServiceParamId param_id, int* value) {
    // TODO this probably should be stored in config for UI configuration
    LOG_DEBUG(Lib_SystemService, "called param_id {}", u32(param_id));
    if (value == nullptr) {
        LOG_ERROR(Lib_SystemService, "value is null");
        return ORBIS_SYSTEM_SERVICE_ERROR_PARAMETER;
    }
    switch (param_id) {
    case OrbisSystemServiceParamId::Lang:
        *value = Config::GetLanguage();
        break;
    case OrbisSystemServiceParamId::DateFormat:
        *value = u32(OrbisSystemParamDateFormat::FmtDDMMYYYY);
        break;
    case OrbisSystemServiceParamId::TimeFormat:
        *value = u32(OrbisSystemParamTimeFormat::Fmt24Hour);
        break;
    case OrbisSystemServiceParamId::TimeZone:
        *value = +120;
        break;
    case OrbisSystemServiceParamId::Summertime:
        *value = 1;
        break;
    case OrbisSystemServiceParamId::GameParentalLevel:
        *value = u32(OrbisSystemParamGameParentalLevel::Off);
        break;
    case OrbisSystemServiceParamId::EnterButtonAssign:
        *value = u32(OrbisSystemParamEnterButtonAssign::Cross);
        break;
    default:
        LOG_ERROR(Lib_SystemService, "param_id {} unsupported!", u32(param_id));
        *value = 0;
    }

    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceParamGetString() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServicePowerTick() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceRaiseExceptionLocalProcess() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceSystemServiceReceiveEvent(OrbisSystemServiceEvent* event) {
    LOG_TRACE(Lib_SystemService, "called");
    if (event == nullptr) {
        return ORBIS_SYSTEM_SERVICE_ERROR_PARAMETER;
    }

    std::lock_guard<std::mutex> lock(g_event_queue_mutex);
    if (g_event_queue.empty()) {
        return ORBIS_SYSTEM_SERVICE_ERROR_NO_EVENT;
    }

    *event = g_event_queue.front();
    g_event_queue.pop();
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceReenableMusicPlayer() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceRegisterDaemon() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceReleaseFb0() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceReportAbnormalTermination() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceRequestCameraCalibration() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceRequestToChangeRenderingMode() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceResumeLocalProcess() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceSetControllerFocusPermission() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceSetGpuLoadEmulationMode() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceSetOutOfVrPlayAreaFlag() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceSetOutOfVrPlayZoneWarning() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceShowControllerSettings() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceShowDisplaySafeAreaSettings() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceShowEyeToEyeDistanceSetting() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceSuspendBackgroundApp() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceSuspendLocalProcess() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceTickVideoPlayback() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceTurnOffScreenSaver() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_9031A344CB540F1A() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_A9D4CF2568EAB837() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceLaunchWebApp() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_B8495C766861FDCF() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceGetDbgExecutablePath() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceActivateHevc() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceActivateHevcAbort() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceActivateHevcGetStatus() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceActivateHevcInit() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceActivateHevcIsActivated() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceActivateHevcStart() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceActivateHevcTerm() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceActivateMpeg2Abort() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceActivateMpeg2GetStatus() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceActivateMpeg2Init() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceActivateMpeg2IsActivated() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceActivateMpeg2Start() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceActivateMpeg2Term() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemStateMgrCancelShutdownTimer() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemStateMgrEnterMediaPlaybackMode() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemStateMgrEnterStandby() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemStateMgrExtendShutdownTimer() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemStateMgrExtendShutdownTimerForPostAutoUpdateProcess() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemStateMgrGetCurrentState() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemStateMgrGetTriggerCode() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemStateMgrIsBdDriveReady() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemStateMgrIsGpuPerformanceNormal() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemStateMgrIsShellUIShutdownInProgress() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemStateMgrIsStandbyModeEnabled() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemStateMgrLeaveMediaPlaybackMode() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemStateMgrNotifySystemSuspendResumeProgress() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemStateMgrReboot() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemStateMgrSendCecOneTouchPlayCommand() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemStateMgrStartRebootTimer() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemStateMgrStartShutdownTimer() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemStateMgrStartStadbyTimer() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemStateMgrStartVshAutoUpdateTimer() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemStateMgrTickMusicPlayback() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemStateMgrTickPartyChat() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemStateMgrTurnOff() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemStateMgrVshAutoUpdate() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemStateMgrWaitVshAutoUpdateVerifyDone() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemStateMgrWakeUp() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_89F262179C22B49E() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_AC8A8FAB4A1696B8() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceInvokeAppLaunchLink() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceShowClosedCaptionAdvancedSettings() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceShowClosedCaptionSettings() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceSetPowerSaveLevel() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceInitializeForShellCore() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_7C1183FC73629929() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceDisablePartyVoice() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceReenablePartyVoice() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceGetPlatformPrivacyDefinitionData() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceGetPlatformPrivacyDefinitionVersion() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceGetPlatformPrivacySetting() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceDeclareReadyForSuspend() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceDisableSuspendNotification() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceEnableSuspendNotification() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceRequestPowerOff() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceRequestReboot() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceAddLocalProcessForPs2Emu() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceGetParentSocketForPs2Emu() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceKillLocalProcessForPs2Emu() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceShowImposeMenuForPs2Emu() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceSaveVideoToken() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceLaunchStore() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceTelemetrySetData() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_C67FC780F5B6F71E() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceLaunchUdsApp() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceLoadExecVideoServiceWebApp() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceDisableVoiceRecognition() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSystemServiceReenableVoiceRecognition() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_6B1CDB955F0EBD65() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_CB5E885E225F69F0() {
    LOG_ERROR(Lib_SystemService, "(STUBBED) called");
    return ORBIS_OK;
}

void PushSystemServiceEvent(const OrbisSystemServiceEvent& event) {
    std::lock_guard<std::mutex> lock(g_event_queue_mutex);
    g_event_queue.push(event);
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("alZfRdr2RP8", "libSceAppMessaging", 1, "libSceSystemService",
                 sceAppMessagingClearEventFlag);
    LIB_FUNCTION("jKgAUl6cLy0", "libSceAppMessaging", 1, "libSceSystemService",
                 sceAppMessagingReceiveMsg);
    LIB_FUNCTION("+zuv20FsXrA", "libSceAppMessaging", 1, "libSceSystemService",
                 sceAppMessagingSendMsg);
    LIB_FUNCTION("HIwEvx4kf6o", "libSceAppMessaging", 1, "libSceSystemService",
                 sceAppMessagingSendMsgToShellCore);
    LIB_FUNCTION("5ygy1IPUh5c", "libSceAppMessaging", 1, "libSceSystemService",
                 sceAppMessagingSendMsgToShellUI);
    LIB_FUNCTION("hdoMbMFIDdE", "libSceAppMessaging", 1, "libSceSystemService",
                 sceAppMessagingSetEventFlag);
    LIB_FUNCTION("iKNXKsUtOjY", "libSceAppMessaging", 1, "libSceSystemService",
                 sceAppMessagingTryGetEventFlag);
    LIB_FUNCTION("ZVRXXqj1n80", "libSceAppMessaging", 1, "libSceSystemService",
                 sceAppMessagingTryReceiveMsg);
    LIB_FUNCTION("yOiZq+9-ZMQ", "libSceAppMessaging", 1, "libSceSystemService",
                 Func_C8E899ABEF7F64C4);
    LIB_FUNCTION("90unWbnI0qE", "libSceAppMessaging", 1, "libSceSystemService",
                 Func_F74BA759B9C8D2A1);
    LIB_FUNCTION("V350H0h35IU", "libSceLncUtil", 1, "libSceSystemService",
                 sceLncUtilAcquireCpuBudgetOfExtraAudioDevices);
    LIB_FUNCTION("GmKMHwvxLlo", "libSceLncUtil", 1, "libSceSystemService",
                 sceLncUtilAcquireCpuBudgetOfImeForBigApp);
    LIB_FUNCTION("mC3BKJFlbNI", "libSceLncUtil", 1, "libSceSystemService",
                 sceLncUtilAcquireCpuBudgetOfInGameStore);
    LIB_FUNCTION("4dWfNKOT1sg", "libSceLncUtil", 1, "libSceSystemService", sceLncUtilActivateCdlg);
    LIB_FUNCTION("j72lst7BFuc", "libSceLncUtil", 1, "libSceSystemService",
                 sceLncUtilAddLocalProcess);
    LIB_FUNCTION("u1JVDP28ycg", "libSceLncUtil", 1, "libSceSystemService",
                 sceLncUtilBlockAppSuspend);
    LIB_FUNCTION("MxXZ-poqGNs", "libSceLncUtil", 1, "libSceSystemService",
                 sceLncUtilBlockingGetEventForDaemon);
    LIB_FUNCTION("93MnzhkAAgk", "libSceLncUtil", 1, "libSceSystemService", sceLncUtilContinueApp);
    LIB_FUNCTION("uaqZvga3Fkg", "libSceLncUtil", 1, "libSceSystemService", sceLncUtilCrashSyscore);
    LIB_FUNCTION("4oofFQudfx0", "libSceLncUtil", 1, "libSceSystemService",
                 sceLncUtilDeactivateCdlg);
    LIB_FUNCTION("GHUqRrCB2hM", "libSceLncUtil", 1, "libSceSystemService",
                 sceLncUtilDeclareReadyForSuspend);
    LIB_FUNCTION("GkcNZBoiDcs", "libSceLncUtil", 1, "libSceSystemService",
                 sceLncUtilDisableSuspendNotification);
    LIB_FUNCTION("AGnsy1zV34o", "libSceLncUtil", 1, "libSceSystemService",
                 sceLncUtilEnableSuspendNotification);
    LIB_FUNCTION("UukL0EXLQls", "libSceLncUtil", 1, "libSceSystemService",
                 sceLncUtilFinishSpecialResume);
    LIB_FUNCTION("vquYrvImjPg", "libSceLncUtil", 1, "libSceSystemService", sceLncUtilForceKillApp);
    LIB_FUNCTION("NS-XWAN9uoc", "libSceLncUtil", 1, "libSceSystemService",
                 sceLncUtilForceKillLocalProcess);
    LIB_FUNCTION("i-esdF3Kz-g", "libSceLncUtil", 1, "libSceSystemService",
                 sceLncUtilGetApp0DirPath);
    LIB_FUNCTION("vbMEQcz6O8g", "libSceLncUtil", 1, "libSceSystemService",
                 sceLncUtilGetAppCategory);
    LIB_FUNCTION("i+1kluDITlQ", "libSceLncUtil", 1, "libSceSystemService",
                 sceLncUtilGetAppFocusedAppStatus);
    LIB_FUNCTION("MVF+elex8Sw", "libSceLncUtil", 1, "libSceSystemService", sceLncUtilGetAppId);
    LIB_FUNCTION("Wu+zDz8VIFk", "libSceLncUtil", 1, "libSceSystemService",
                 sceLncUtilGetAppIdOfBigApp);
    LIB_FUNCTION("ppWFdoDMMSs", "libSceLncUtil", 1, "libSceSystemService",
                 sceLncUtilGetAppIdOfMiniApp);
    LIB_FUNCTION("oYQC9Quj6No", "libSceLncUtil", 1, "libSceSystemService",
                 sceLncUtilGetAppLaunchedUser);
    LIB_FUNCTION("DxRki7T2E44", "libSceLncUtil", 1, "libSceSystemService", sceLncUtilGetAppStatus);
    LIB_FUNCTION("cyO5ShJxdnE", "libSceLncUtil", 1, "libSceSystemService",
                 sceLncUtilGetAppStatusListForShellUIReboot);
    LIB_FUNCTION("g0wTG9KImzI", "libSceLncUtil", 1, "libSceSystemService", sceLncUtilGetAppTitleId);
    LIB_FUNCTION("1AQf7o8gpHc", "libSceLncUtil", 1, "libSceSystemService", sceLncUtilGetAppType);
    LIB_FUNCTION("7yXjWLWJFHU", "libSceLncUtil", 1, "libSceSystemService", sceLncUtilGetCdlgExec);
    LIB_FUNCTION("CgVdl9Sp1G0", "libSceLncUtil", 1, "libSceSystemService",
                 sceLncUtilGetCoredumpState);
    LIB_FUNCTION("deCYc7iaC5Q", "libSceLncUtil", 1, "libSceSystemService",
                 sceLncUtilGetDbgExecutablePath);
    LIB_FUNCTION("yUh0BIPbhVo", "libSceLncUtil", 1, "libSceSystemService",
                 sceLncUtilGetEventForDaemon);
    LIB_FUNCTION("ZucoOmNsb7w", "libSceLncUtil", 1, "libSceSystemService",
                 sceLncUtilGetEventForShellUI);
    LIB_FUNCTION("ojmvNKQZNUw", "libSceLncUtil", 1, "libSceSystemService",
                 sceLncUtilGetGpuCrashFullDumpAppStatus);
    LIB_FUNCTION("wGobSSrBM4s", "libSceLncUtil", 1, "libSceSystemService",
                 sceLncUtilGetLocalProcessStatusList);
    LIB_FUNCTION("HRXjUojlG70", "libSceLncUtil", 1, "libSceSystemService",
                 sceLncUtilGetParentSocket);
    LIB_FUNCTION("kOd75qDlxBM", "libSceLncUtil", 1, "libSceSystemService",
                 sceLncUtilGetResultKillApp);
    LIB_FUNCTION("LZs6hfPMnso", "libSceLncUtil", 1, "libSceSystemService",
                 sceLncUtilGetResultLaunchAppByTitleId);
    LIB_FUNCTION("f-Q8Nd33FBc", "libSceLncUtil", 1, "libSceSystemService", sceLncUtilInitialize);
    LIB_FUNCTION("PyNH7p4LVw8", "libSceLncUtil", 1, "libSceSystemService", sceLncUtilIsActiveCdlg);
    LIB_FUNCTION("IGrJsPNL6n4", "libSceLncUtil", 1, "libSceSystemService", sceLncUtilIsAppLaunched);
    LIB_FUNCTION("teGoPWnEgd4", "libSceLncUtil", 1, "libSceSystemService",
                 sceLncUtilIsAppSuspended);
    LIB_FUNCTION("iUsONHVCDbQ", "libSceLncUtil", 1, "libSceSystemService",
                 sceLncUtilIsCpuBudgetOfExtraAudioDevicesAvailable);
    LIB_FUNCTION("i4tm7MB0ZK0", "libSceLncUtil", 1, "libSceSystemService", sceLncUtilIsPs2Emu);
    LIB_FUNCTION("Ry4u8KxkVY4", "libSceLncUtil", 1, "libSceSystemService",
                 sceLncUtilIsShellUiFgAndGameBgCpuMode);
    LIB_FUNCTION("gNn+EZtm1i0", "libSceLncUtil", 1, "libSceSystemService",
                 sceLncUtilKickCoredumpOnlyProcMem);
    LIB_FUNCTION("SZ2uH5Abws8", "libSceLncUtil", 1, "libSceSystemService", sceLncUtilKillApp);
    LIB_FUNCTION("RBlEzB--JeA", "libSceLncUtil", 1, "libSceSystemService",
                 sceLncUtilKillAppWithReason);
    LIB_FUNCTION("IhlLdSAX+Jk", "libSceLncUtil", 1, "libSceSystemService",
                 sceLncUtilKillLocalProcess);
    LIB_FUNCTION("+nRJUD-7qCk", "libSceLncUtil", 1, "libSceSystemService", sceLncUtilLaunchApp);
    LIB_FUNCTION("wwpRNgW81Cs", "libSceLncUtil", 1, "libSceSystemService", sceLncUtilLoadExec);
    LIB_FUNCTION("+8LJld9LIt4", "libSceLncUtil", 1, "libSceSystemService",
                 sceLncUtilNotifyCoredumpRequestEnd);
    LIB_FUNCTION("HKZmLmGfkd4", "libSceLncUtil", 1, "libSceSystemService",
                 sceLncUtilNotifyCoredumpRequestProgress);
    LIB_FUNCTION("-3moAnxKYkc", "libSceLncUtil", 1, "libSceSystemService",
                 sceLncUtilNotifyVshReady);
    LIB_FUNCTION("P563r-eGAh4", "libSceLncUtil", 1, "libSceSystemService",
                 sceLncUtilRaiseException);
    LIB_FUNCTION("CJ45DLRQOD8", "libSceLncUtil", 1, "libSceSystemService",
                 sceLncUtilRaiseExceptionLocalProcess);
    LIB_FUNCTION("Qn5JIRI6ZNU", "libSceLncUtil", 1, "libSceSystemService",
                 sceLncUtilRegisterCdlgSharedMemoryName);
    LIB_FUNCTION("V25-9U+YauY", "libSceLncUtil", 1, "libSceSystemService",
                 sceLncUtilRegisterDaemon);
    LIB_FUNCTION("awS+eYVuXJA", "libSceLncUtil", 1, "libSceSystemService",
                 sceLncUtilRegisterShellUI);
    LIB_FUNCTION("QvUYLdPhylQ", "libSceLncUtil", 1, "libSceSystemService",
                 sceLncUtilReleaseCpuBudgetOfExtraAudioDevices);
    LIB_FUNCTION("1PQhPdyNCj8", "libSceLncUtil", 1, "libSceSystemService",
                 sceLncUtilReleaseCpuBudgetOfImeForBigApp);
    LIB_FUNCTION("QsLhZ+8WvSM", "libSceLncUtil", 1, "libSceSystemService",
                 sceLncUtilReleaseCpuBudgetOfInGameStore);
    LIB_FUNCTION("lD-k3hDhlqA", "libSceLncUtil", 1, "libSceSystemService", sceLncUtilResumeApp);
    LIB_FUNCTION("XaC9s-Nr2u4", "libSceLncUtil", 1, "libSceSystemService",
                 sceLncUtilResumeLocalProcess);
    LIB_FUNCTION("v7DYuX0G5TQ", "libSceLncUtil", 1, "libSceSystemService", sceLncUtilSetAppFocus);
    LIB_FUNCTION("3mHuKF7fsd8", "libSceLncUtil", 1, "libSceSystemService", sceLncUtilSetCdlgExec);
    LIB_FUNCTION("X8gYbyLG1wk", "libSceLncUtil", 1, "libSceSystemService",
                 sceLncUtilSetControllerFocus);
    LIB_FUNCTION("NJYAQeP3z7c", "libSceLncUtil", 1, "libSceSystemService",
                 sceLncUtilSetControllerFocusPermission);
    LIB_FUNCTION("3+64z-ckBS8", "libSceLncUtil", 1, "libSceSystemService", sceLncUtilStartKillApp);
    LIB_FUNCTION("r07vD4SP2sc", "libSceLncUtil", 1, "libSceSystemService",
                 sceLncUtilStartLaunchAppByTitleId);
    LIB_FUNCTION("Y8onQYjuvOU", "libSceLncUtil", 1, "libSceSystemService", sceLncUtilSuspendApp);
    LIB_FUNCTION("8vYXkdXmh-Q", "libSceLncUtil", 1, "libSceSystemService",
                 sceLncUtilSuspendBackgroundApp);
    LIB_FUNCTION("rd+-SzL202E", "libSceLncUtil", 1, "libSceSystemService",
                 sceLncUtilSuspendLocalProcess);
    LIB_FUNCTION("Kt1k5aBzrcE", "libSceLncUtil", 1, "libSceSystemService", sceLncUtilSystemSuspend);
    LIB_FUNCTION("cCod+B3EdhI", "libSceLncUtil", 1, "libSceSystemService", sceLncUtilTerminate);
    LIB_FUNCTION("msW-hp1U0zo", "libSceLncUtil", 1, "libSceSystemService",
                 sceLncUtilTryBlockAppSuspend);
    LIB_FUNCTION("iRZduYIV1hs", "libSceLncUtil", 1, "libSceSystemService",
                 sceLncUtilUnblockAppSuspend);
    LIB_FUNCTION("aVRNp1nOOKY", "libSceLncUtil", 1, "libSceSystemService",
                 sceLncUtilUnregisterCdlgSharedMemoryName);
    LIB_FUNCTION("BnMaW5wfnlQ", "libSceLncUtil", 1, "libSceSystemService",
                 sceLncUtilUnregisterDaemon);
    LIB_FUNCTION("cqui4JUJtbY", "libSceLncUtil", 1, "libSceSystemService",
                 sceLncUtilUnregisterShellUI);
    LIB_FUNCTION("f-WtMqIKo20", "libSceSystemServiceActivateHevcSoft", 1, "libSceSystemService",
                 sceSystemServiceActivateHevcSoft);
    LIB_FUNCTION("s6ucQ90BW3g", "libSceSystemServiceActivateHevcSoft", 1, "libSceSystemService",
                 sceSystemServiceActivateHevcSoftAbort);
    LIB_FUNCTION("MyDvxh8+ckI", "libSceSystemServiceActivateHevcSoft", 1, "libSceSystemService",
                 sceSystemServiceActivateHevcSoftGetStatus);
    LIB_FUNCTION("ytMU6x1nlmU", "libSceSystemServiceActivateHevcSoft", 1, "libSceSystemService",
                 sceSystemServiceActivateHevcSoftInit);
    LIB_FUNCTION("djVe06YjzkI", "libSceSystemServiceActivateHevcSoft", 1, "libSceSystemService",
                 sceSystemServiceActivateHevcSoftIsActivated);
    LIB_FUNCTION("PNO2xlDVdzg", "libSceSystemServiceActivateHevcSoft", 1, "libSceSystemService",
                 sceSystemServiceActivateHevcSoftStart);
    LIB_FUNCTION("P-awBIrXrTQ", "libSceSystemServiceActivateHevcSoft", 1, "libSceSystemService",
                 sceSystemServiceActivateHevcSoftTerm);
    LIB_FUNCTION("5SfMtsW8h7A", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilAccessibilityZoomLock);
    LIB_FUNCTION("Uku2JpZmoqc", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilAccessibilityZoomUnlock);
    LIB_FUNCTION("qVBNhnqUz-4", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilAcquireBgmCpuBudget);
    LIB_FUNCTION("TfVHoRVX2HM", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilAcquireRemotePlayCpuBudget);
    LIB_FUNCTION("fBuukeGZ2FE", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilAcquireSharePlayCpuBudget);
    LIB_FUNCTION("mpkohyVqCRM", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilActivateAbort);
    LIB_FUNCTION("fkcM5YcqjV8", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilActivateGetStatus);
    LIB_FUNCTION("x5hqKRKziYU", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilActivateInit);
    LIB_FUNCTION("jktCMQNgyFc", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilActivateIsActivated);
    LIB_FUNCTION("xIMClZZz50k", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilActivateRecordActivation);
    LIB_FUNCTION("MRVnLsn-GRI", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilActivateStart);
    LIB_FUNCTION("CZrOHqt6oCY", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilActivateStartAsync);
    LIB_FUNCTION("ibXh+Mc4wbs", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilActivateTerm);
    LIB_FUNCTION("wtNEh1E9ALA", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilChangeRunLevel);
    LIB_FUNCTION("v81dfnaMfUY", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilChangeToStaffModeForIDU);
    LIB_FUNCTION("9VDzY7m1NN8", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilCheckerAbort);
    LIB_FUNCTION("AgYSGAQGtXs", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilCleanupCrashReport);
    LIB_FUNCTION("l5bdg4tUTGc", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilClearAppData);
    LIB_FUNCTION("RnY2HTwqz3A", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilClearPsnAccountInfo);
    LIB_FUNCTION("SYSL4KtzcAU", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilCrashReportRequestCancel);
    LIB_FUNCTION("KTCPKqvFTok", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilDeclareBeginOfExternalStorageAppMove);
    LIB_FUNCTION("F20xA1NsG9s", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilDeclareEndOfExternalStorageAppMove);
    LIB_FUNCTION("XlcBqhyaJyI", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilDeleteDiscInstalledTitleWorkaroundFile);
    LIB_FUNCTION("yO7OIU45UnQ", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilDeleteDownloadedHidConfigFile);
    LIB_FUNCTION("4SgLbJPUxNw", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilDeleteDownloadedNetEvConfigFile);
    LIB_FUNCTION("ctTYL9lomv8", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilDeleteDownloadedTitleWorkaroundFile);
    LIB_FUNCTION("gYXxtLzFU8Y", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilDeleteSmrHddDummyData);
    LIB_FUNCTION("0QN4BUnzF14", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilDoFsck);
    LIB_FUNCTION("WN1v3xYoGDw", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilDownloadHidConfigFileFromServer);
    LIB_FUNCTION("A3wbbLmrQV4", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilDownloadNetEvConfigFileFromServer);
    LIB_FUNCTION("5YNnX5Pfquo", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilDownloadTitleWorkaroundFileFromServer);
    LIB_FUNCTION("9plZCCRm9x4", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilEnterPowerLockSection);
    LIB_FUNCTION("SOmyRqRpKIM", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilExecuteCrashReport);
    LIB_FUNCTION("+jVaKSG0nHk", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilExfatFormatExternalHdd);
    LIB_FUNCTION("0g6-uh4JTP8", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilExitMiniApp);
    LIB_FUNCTION("dtx5tcGFVII", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilExitMiniAppWithValue);
    LIB_FUNCTION("F-g-G0oJegs", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilFillUpSpaceOnSmrHdd);
    LIB_FUNCTION("UG9I-iHI-ME", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilFormatExternalHdd);
    LIB_FUNCTION("LlFmfrkpjW0", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilFormatHddForRestore);
    LIB_FUNCTION("FmjFl9Nvwcw", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilFreeUpSpaceOnSmrHdd);
    LIB_FUNCTION("WISL-JH-6Ic", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetAppData);
    LIB_FUNCTION("XGxXS135WR8", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetAppEnableTTS);
    LIB_FUNCTION("V9b3HfN19vM", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetAppEnterButtonAssign);
    LIB_FUNCTION("u474-bA7ul0", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetAppLaunchedParamInt);
    LIB_FUNCTION("kyFOaxSaP0A", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetAppLaunchedParamIntByBudgetType);
    LIB_FUNCTION("f5Z7FIeoHdw", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetAppLaunchedParamString);
    LIB_FUNCTION("dZ3RfDzgmCY", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetAppLaunchedParamStringByBudgetType);
    LIB_FUNCTION("ZIKGk+35UDU", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetAppLaunchTypeInfo);
    LIB_FUNCTION("5gIVIzipgsw", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetAutoPowerDownRemainingSeconds);
    LIB_FUNCTION("lAvSrKAjxCA", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetBasicProductShape);
    LIB_FUNCTION("EwfSRaPlCE4", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetCheckerString);
    LIB_FUNCTION("gAyT42nwElM", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetCheckerStringEx);
    LIB_FUNCTION("Mg3P1Z4Xavs", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetCloudClientStatus);
    LIB_FUNCTION("FcAheKO8u7c", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetCrashReportCoreFileSetSize);
    LIB_FUNCTION("jCJ+gks483A", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetCrashReportFilterInfoStart);
    LIB_FUNCTION("-ROAAenn4Xg", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetCrashReportInfoForBoot);
    LIB_FUNCTION("V9LadIvu5Ko", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetCrashReportInfoForBootStart);
    LIB_FUNCTION("J5OPALFNdFE", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetCrashReportInfoStart);
    LIB_FUNCTION("368es-zmQuc", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreutilGetCrashReportProcessInformation);
    LIB_FUNCTION("NTttBlD2Xbk", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetCrashReportResult);
    LIB_FUNCTION("4YQ-w9Xwn7s", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetCrashReportStatus);
    LIB_FUNCTION("L6R0jU7yTTQ", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetCrashReportUploadStatus);
    LIB_FUNCTION("oINHTqU1qvY", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetDeviceIndexBehavior);
    LIB_FUNCTION("vPxKoc5MyxQ", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetDeviceIndexBehaviorWithTimeout);
    LIB_FUNCTION("rkV5b-p490g", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetEffectiveTotalSizeOfUserPartition);
    LIB_FUNCTION("guf+xcMoCas", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetFreeSizeOfAvContentsTmp);
    LIB_FUNCTION("ZbY5LxmH6uA", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetFreeSizeOfUserPartition);
    LIB_FUNCTION("1qbUFXlBXFw", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetFsckProgress);
    LIB_FUNCTION("-g0pBZ2JdAc", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetGameLiveStreamingStatus);
    LIB_FUNCTION("g8T4x0GD9Wo", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetGnmCompositorOnScreenProfilerFlag);
    LIB_FUNCTION("beQ90Sx6c8g", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetGpuLoadEmulationMode);
    LIB_FUNCTION("ns8HVzVqaNo", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetGpuLoadEmulationModeByAppId);
    LIB_FUNCTION("kn3vBOTg-ok", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetHidConfigFileInfoString);
    LIB_FUNCTION("97L0D3+iBDE", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetHidConfigFileString);
    LIB_FUNCTION("NZWLG-imDlU", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetHidConfigName);
    LIB_FUNCTION("RM1Xb5Hcq4w", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetHidConfigNum);
    LIB_FUNCTION("93trbeNjx7c", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetIDUMode);
    LIB_FUNCTION("Ac3I81u9ky4", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetImposeMenuFlagForPs2Emu);
    LIB_FUNCTION("39lewWn5+G0", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetManifestFileStatus);
    LIB_FUNCTION("kuErIHXWIpc", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetNeedSizeOfAppContent);
    LIB_FUNCTION("wFvgq-KXT0Q", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetNetEvConfigFileInfoString);
    LIB_FUNCTION("m5OsHQx9Ni4", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetOptimizationStatus);
    LIB_FUNCTION("qEUJBsB7yMk", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetOutOfVrPlayZoneWarning);
    LIB_FUNCTION("lgbdvT36kTE", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetPapcGamePcl);
    LIB_FUNCTION("bTmtBchzFps", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetPbtcUserInfoList);
    LIB_FUNCTION("lXlP+jhO8QI", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetPlatformPrivacyDefinitionEventData);
    LIB_FUNCTION("CKTyfq2tb7k", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetPlatformPrivacySetting);
    LIB_FUNCTION("VxWJ7DUrEIQ", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetProgressOfFormatExternalHdd);
    LIB_FUNCTION("3M1tCF9VfoE", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetProgressOfFsck);
    LIB_FUNCTION("x6STXhIEG0M", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetPsnAccountInfo);
    LIB_FUNCTION("1G3xnMBZpYI", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetPsStoreIconLayout);
    LIB_FUNCTION("zS6ZPLgQJcA", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetPsStoreIconState);
    LIB_FUNCTION("9coQ7gTfwhA", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetRegion);
    LIB_FUNCTION("ai7LcBcf6Rs", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetRemotePlayStatus);
    LIB_FUNCTION("HeZzL2xbRJY", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetRunLevel);
    LIB_FUNCTION("L5mESo+Iq+k", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetSharePlayStatus);
    LIB_FUNCTION("hQClZK9mdkk", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetShellUIVMStats);
    LIB_FUNCTION("fRurGDbUulc", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetSmrHddInfoString);
    LIB_FUNCTION("nG+HNBwQ4sw", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetSocialScreenStatus);
    LIB_FUNCTION("WH6O4hriE-0", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetSplashScreenState);
    LIB_FUNCTION("PPQxiE4lbyY", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetSupportSiteURL);
    LIB_FUNCTION("9i+R1rj6Trk", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetSuspendConfirmationDialogFlag);
    LIB_FUNCTION("WKxOVIkISxA", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetSystemBGState);
    LIB_FUNCTION("W5HtGRCZ1iE", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetSystemBGWaveColor);
    LIB_FUNCTION("D-6S9JHI6A0", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetSystemBGWaveState);
    LIB_FUNCTION("ZGbkd2hWhJU", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetTitleWorkaroundFileInfoString);
    LIB_FUNCTION("vq8ubGb2XjQ", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetTitleWorkaroundFileString);
    LIB_FUNCTION("fORZmlh1TQo", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetUIStatus);
    LIB_FUNCTION("E4I7uCoWbkM", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetUserFocus);
    LIB_FUNCTION("plK52OfeEIc", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetUserIdOfMorpheusUser);
    LIB_FUNCTION("VbEHW7RrJ+w", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGetVersionNumberOfCameraCalibrationData);
    LIB_FUNCTION("0y01ndm0BA8", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilGoBackToKratosCurrentSessionGame);
    LIB_FUNCTION("oh68H-4hEAE", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilHideBlocksFromUser);
    LIB_FUNCTION("DviyPC-JJ1k", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilIncrementVersionNumberOfCameraCalibrationData);
    LIB_FUNCTION("2b-b5AouLv4", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilIsAccessibilityZoomLocked);
    LIB_FUNCTION("soq7GTbVMkw", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilIsBgmCpuBudgetAcquired);
    LIB_FUNCTION("vYHJtZyhhEI", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilIsBgmCpuBudgetAvailable);
    LIB_FUNCTION("-Lpr5gHkHkc", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilIsBgmPlaying);
    LIB_FUNCTION("mpeGML7ulA8", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilIsExternalStorageAppMoveInProgress);
    LIB_FUNCTION("PGsAGnnRstY", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilIsEyeToEyeDistanceAdjusted);
    LIB_FUNCTION("KyQY2KfMxKw", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilIsGameLiveStreamingOnAir);
    LIB_FUNCTION("izo3BrmWZDM", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilIsImposeScreenOverlaid);
    LIB_FUNCTION("wCbG33VsbqQ", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilIsInSystemSuspendBlackList);
    LIB_FUNCTION("mTZxVC3pebc", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilIsInternalKratosUser);
    LIB_FUNCTION("44PCVgTBBCw", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilIsKilledOrSuspendedByLogout);
    LIB_FUNCTION("KH0InA0uStg", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilIsNeededCrashReport);
    LIB_FUNCTION("3JNHzrEDnrk", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilIsPowerSaveAlertRequested);
    LIB_FUNCTION("CWcxjT6X+1c", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilIsRemotePlayCpuBudgetAcquired);
    LIB_FUNCTION("WIEUJ61AwvU", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilIsScreenSaverOn);
    LIB_FUNCTION("gWMlFq4N9Lw", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilIsSharePlayCpuBudgetAcquired);
    LIB_FUNCTION("GEZ9sIz3wuM", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilIsShowCrashReport);
    LIB_FUNCTION("vzWoetyaUuA", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilIsTemperatureDanger);
    LIB_FUNCTION("4dsNPwVODKM", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilIsTitleWorkaroundEnabled);
    LIB_FUNCTION("IHHSdVBTwBc", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilIsUsbMassStorageMounted);
    LIB_FUNCTION("GYUk4t27Myw", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilLaunchByUri);
    LIB_FUNCTION("nENvUAsAKdY", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilLeavePowerLockSection);
    LIB_FUNCTION("2rOYe6lVCVQ", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilLog);
    LIB_FUNCTION("-Sp1aaqI1SQ", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilMakeManifestFile);
    LIB_FUNCTION("aCkM+OaGv3g", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilMountAppRight);
    LIB_FUNCTION("juqlPZWkJGc", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilMountDownloadDataForShellUI);
    LIB_FUNCTION("qNe8uNe3EpQ", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilMountHddForBackup);
    LIB_FUNCTION("bRCLw49N4hE", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilMountHddForRestore);
    LIB_FUNCTION("1e7CmFlJveU", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilNavigateToAnotherApp);
    LIB_FUNCTION("n9xRQPtUP0g", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilNavigateToGoHome);
    LIB_FUNCTION("Mi9-CweviUo", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilNavigateToLaunchedApp);
    LIB_FUNCTION("V-5cjs+9kI0", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilNotificationCancelForIDU);
    LIB_FUNCTION("VQRWOxYGays", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilNotificationRequestedForIDU);
    LIB_FUNCTION("roUQwCYYegE", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilNotifyBgmCoreTermination);
    LIB_FUNCTION("4CzZUVleMcE", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilNotifyFarsightUIDone);
    LIB_FUNCTION("awBTm0vNaos", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilNotifyFsReadError);
    LIB_FUNCTION("dk-PIxWMp8k", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilNotifyPsnAccountInfoReceived);
    LIB_FUNCTION("IldAc7Eq5-c", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilNotifyYouTubeAccountLinkStatusChanged);
    LIB_FUNCTION("Tgs5zOUfQSc", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilPfAuthClientConsoleTokenClearCache);
    LIB_FUNCTION("pb2XPMV5beI", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilPostActivityForPsNow);
    LIB_FUNCTION("-hrMXdpZuDU", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilPostErrorLog);
    LIB_FUNCTION("fCeSFo0IM-w", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilPostLaunchConfirmResult);
    LIB_FUNCTION("cZCJTMamDOE", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilPostPsmEventToShellUI);
    LIB_FUNCTION("PGHjjtZxKvs", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilPreNotifyOfGameLiveStreaming);
    LIB_FUNCTION("K-QFvDXYSbg", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilPreNotifyOfRemotePlay);
    LIB_FUNCTION("HBA-tCWUXP8", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilPreNotifyOfSharePlay);
    LIB_FUNCTION("EFhM9SF2aSQ", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilReleaseBgmCpuBudget);
    LIB_FUNCTION("cfJZThTSxQ0", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilReleaseRemotePlayCpuBudget);
    LIB_FUNCTION("jKnwOdgck5g", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilReleaseSharePlayCpuBudget);
    LIB_FUNCTION("yO-ekZ5toaQ", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilReportSessionErrorToGaikaiController);
    LIB_FUNCTION("lF96Sr8Jf0s", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilReportUnexpectedFatalErrorToSystemTelemetry);
    LIB_FUNCTION("-yYPJb0ejO8", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilRequestCameraCalibration);
    LIB_FUNCTION("8+CmlQdq7u8", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilRequestEjectDevice);
    LIB_FUNCTION("+2paAsKqXOQ", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilRequestRebootApp);
    LIB_FUNCTION("7JgSJnaByIs", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilRequestShutdown);
    LIB_FUNCTION("IzQN+F5q3wg", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilResetAutoPowerDownTimer);
    LIB_FUNCTION("7yUQmZWoqVg", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilResetBgdcConfig);
    LIB_FUNCTION("xKSgaSVX1io", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilSetAppData);
    LIB_FUNCTION("dS1+1D1LRHs", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilSetBgmProhibition);
    LIB_FUNCTION("l96YlUEtMPk", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilSetDeviceIndexBehavior);
    LIB_FUNCTION("bC8vo608P2E", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilSetGameLiveStreamingOnAirFlag);
    LIB_FUNCTION("K33+EwitWlo", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilSetGameLiveStreamingStatus);
    LIB_FUNCTION("m65uKv7IAkI", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilSetGnmCompositorOnScreenProfilerFlag);
    LIB_FUNCTION("MeboioVomns", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilSetGpuLoadEmulationMode);
    LIB_FUNCTION("zd4oVXWGD2Y", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilSetGpuLoadEmulationModeByAppId);
    LIB_FUNCTION("4Pd0g-lGEM0", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilSetIDUMode);
    LIB_FUNCTION("TJp3kdSGsIw", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilSetImposeStatusFlag);
    LIB_FUNCTION("qtjjorW1V94", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilSetPsStoreIconLayout);
    LIB_FUNCTION("g787tMBA1TE", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilSetPsStoreIconState);
    LIB_FUNCTION("jqj5vbglbZU", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilSetRemotePlayStatus);
    LIB_FUNCTION("l22TAIbbtFw", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilSetSharePlayStatus);
    LIB_FUNCTION("IWSCO20RwIY", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilSetSkipUpdateCheck);
    LIB_FUNCTION("nA5rRwLrgIU", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilSetSocialScreenStatus);
    LIB_FUNCTION("2Pms7iCE-Fo", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilSetSplashScreenState);
    LIB_FUNCTION("kfyuElAEnis", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilSetSystemBGState);
    LIB_FUNCTION("Ujz25JX-jPM", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilSetSystemBGWaveColor);
    LIB_FUNCTION("GB19cfR-Tis", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilSetSystemBGWaveState);
    LIB_FUNCTION("atiUTsTFJ3k", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilSetUIStatus);
    LIB_FUNCTION("-9djWj1NU4E", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilSetUserFocus);
    LIB_FUNCTION("lW+8pdTQMmg", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilShowCriticalErrorDialog);
    LIB_FUNCTION("0ptZiu0jBJs", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilShowErrorDialog);
    LIB_FUNCTION("chZFHnGa9x4", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilShowErrorDialogWithFormatArgs);
    LIB_FUNCTION("yxiUUPJoyYI", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilShowErrorDialogWithParam);
    LIB_FUNCTION("GjOxjIVZA1Y", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilShowPsUnderLockIndicator);
    LIB_FUNCTION("R013D1VIETQ", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilSignalUserInput);
    LIB_FUNCTION("nu542EmGFD4", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilStartOptimization);
    LIB_FUNCTION("HhBo--ix7Lg", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilStartPsNowGame);
    LIB_FUNCTION("kozqEeuRwrk", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilStopOptimization);
    LIB_FUNCTION("9dvVBukqOsw", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilStopPsNowGame);
    LIB_FUNCTION("dbwyzALlKOQ", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilTestBusTransferSpeed);
    LIB_FUNCTION("DWVv0MlE1sw", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilTickHeartBeat);
    LIB_FUNCTION("VxRZE4CZQw8", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilTriggerPapcRecalculation);
    LIB_FUNCTION("CSl1MAdUbYs", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilTriggerPapcUpdate);
    LIB_FUNCTION("lcp9E77DAB4", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilTurnOffScreenSaver);
    LIB_FUNCTION("T9xeifEUF3w", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilUnmountAppRight);
    LIB_FUNCTION("qqL5VYwFLgo", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilUnmountDownloadDataForShellUI);
    LIB_FUNCTION("YvCj4cb1-jU", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilUnmountHddForBackup);
    LIB_FUNCTION("oeyHRt5PP+Q", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilUnmountHddForRestore);
    LIB_FUNCTION("JTctYix8NXU", "libSceShellCoreUtil", 1, "libSceSystemService",
                 sceShellCoreUtilWriteSmrHddDummyData);
    LIB_FUNCTION("Hlylpx+n8Cg", "libSceShellCoreUtil", 1, "libSceSystemService",
                 Func_1E5CA5A71FA7F028);
    LIB_FUNCTION("bUNkT3XDg0Y", "libSceShellCoreUtil", 1, "libSceSystemService",
                 Func_6D43644F75C38346);
    LIB_FUNCTION("c5+4Scso9EU", "libSceShellCoreUtil", 1, "libSceSystemService",
                 Func_739FB849CB28F445);
    LIB_FUNCTION("sgYo-zXHQRE", "libSceShellCoreUtil", 1, "libSceSystemService",
                 Func_B20628FF35C74111);
    LIB_FUNCTION("jPKapVQLX70", "libSceSystemService_jvm", 1, "libSceSystemService",
                 sceSystemServiceAddLocalProcessForJvm);
    LIB_FUNCTION("zqjkZ5VKFSg", "libSceSystemService_jvm", 1, "libSceSystemService",
                 sceSystemServiceGetParentSocketForJvm);
    LIB_FUNCTION("2TJ5KzC73gY", "libSceSystemService_jvm", 1, "libSceSystemService",
                 sceSystemServiceKillLocalProcessForJvm);
    LIB_FUNCTION("0z7srulNt7U", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceAcquireFb0);
    LIB_FUNCTION("0cl8SuwosPQ", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceAddLocalProcess);
    LIB_FUNCTION("cltshBrDLC0", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceAddLocalProcessForPsmKit);
    LIB_FUNCTION("FI+VqGdttvI", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceChangeAcpClock);
    LIB_FUNCTION("ec72vt3WEQo", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceChangeCpuClock);
    LIB_FUNCTION("Z5RgV4Chwxg", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceChangeGpuClock);
    LIB_FUNCTION("LFo00RWzqRU", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceChangeMemoryClock);
    LIB_FUNCTION("MyBXslDE+2o", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceChangeMemoryClockToBaseMode);
    LIB_FUNCTION("qv+X8gozqF4", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceChangeMemoryClockToDefault);
    LIB_FUNCTION("fOsE5pTieqY", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceChangeMemoryClockToMultiMediaMode);
    LIB_FUNCTION("5MLppFJZyX4", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceChangeNumberOfGpuCu);
    LIB_FUNCTION("lgTlIAEJ33M", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceChangeSamuClock);
    LIB_FUNCTION("BQUi7AW+2tQ", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceChangeUvdClock);
    LIB_FUNCTION("fzguXBQzNvI", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceChangeVceClock);
    LIB_FUNCTION("x1UB9bwDSOw", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceDisableMusicPlayer);
    LIB_FUNCTION("Mr1IgQaRff0", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceDisablePersonalEyeToEyeDistanceSetting);
    LIB_FUNCTION("PQ+SjXAg3EM", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceDisableSuspendConfirmationDialog);
    LIB_FUNCTION("O3irWUQ2s-g", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceEnablePersonalEyeToEyeDistanceSetting);
    LIB_FUNCTION("Rn32O5PDlmo", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceEnableSuspendConfirmationDialog);
    LIB_FUNCTION("xjE7xLfrLUk", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceGetAppFocusedAppStatus);
    LIB_FUNCTION("f4oDTxAJCHE", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceGetAppIdOfBigApp);
    LIB_FUNCTION("BBSmGrxok5o", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceGetAppIdOfMiniApp);
    LIB_FUNCTION("t5ShV0jWEFE", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceGetAppStatus);
    LIB_FUNCTION("YLbhAXS20C0", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceGetAppType);
    LIB_FUNCTION("1n37q1Bvc5Y", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceGetDisplaySafeAreaInfo);
    LIB_FUNCTION("JFg3az5ITN4", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceGetEventForDaemon);
    LIB_FUNCTION("4imyVMxX5-8", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceGetGpuLoadEmulationMode);
    LIB_FUNCTION("mPpPxv5CZt4", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceGetHdrToneMapLuminance);
    LIB_FUNCTION("ZNIuJjqdtgI", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceGetLocalProcessStatusList);
    LIB_FUNCTION("UMIlrOlGNQU", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceGetParentSocket);
    LIB_FUNCTION("4ZYuSI8i2aM", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceGetParentSocketForPsmKit);
    LIB_FUNCTION("gbUBqHCEgAI", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceGetPSButtonEvent);
    LIB_FUNCTION("jA629PcMCKU", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceGetRenderingMode);
    LIB_FUNCTION("rPo6tV8D9bM", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceGetStatus);
    LIB_FUNCTION("VrvpoJEoSSU", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceGetTitleWorkaroundInfo);
    LIB_FUNCTION("s4OcLqLsKn0", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceGetVersionNumberOfCameraCalibrationData);
    LIB_FUNCTION("Vo5V8KAwCmk", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceHideSplashScreen);
    LIB_FUNCTION("d-15YTCUMVU", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceIsAppSuspended);
    LIB_FUNCTION("SYqaqLuQU6w", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceIsBgmPlaying);
    LIB_FUNCTION("O4x1B7aXRYE", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceIsEyeToEyeDistanceAdjusted);
    LIB_FUNCTION("bMDbofWFNfQ", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceIsScreenSaverOn);
    LIB_FUNCTION("KQFyDkgAjVs", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceIsShellUiFgAndGameBgCpuMode);
    LIB_FUNCTION("N4RkyJh7FtA", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceKillApp);
    LIB_FUNCTION("6jpZY0WUwLM", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceKillLocalProcess);
    LIB_FUNCTION("7cTc7seJLfQ", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceKillLocalProcessForPsmKit);
    LIB_FUNCTION("l4FB3wNa-Ac", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceLaunchApp);
    LIB_FUNCTION("wX9wVFaegaM", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceLaunchEventDetails);
    LIB_FUNCTION("G5AwzWnHxks", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceLaunchTournamentList);
    LIB_FUNCTION("wIc92b0x6hk", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceLaunchTournamentsTeamProfile);
    LIB_FUNCTION("-+3hY+y8bNo", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceLaunchWebBrowser);
    LIB_FUNCTION("JoBqSQt1yyA", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceLoadExec);
    LIB_FUNCTION("9ScDVErRRgw", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceNavigateToAnotherApp);
    LIB_FUNCTION("e4E3MIEAS2A", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceNavigateToGoBack);
    LIB_FUNCTION("ZeubLhPDitw", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceNavigateToGoBackWithValue);
    LIB_FUNCTION("x2-o9eBw3ZU", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceNavigateToGoHome);
    LIB_FUNCTION("fZo48un7LK4", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceParamGetInt);
    LIB_FUNCTION("SsC-m-S9JTA", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceParamGetString);
    LIB_FUNCTION("XbbJC3E+L5M", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServicePowerTick);
    LIB_FUNCTION("2xenlv7M-UU", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceRaiseExceptionLocalProcess);
    LIB_FUNCTION("656LMQSrg6U", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceReceiveEvent);
    LIB_FUNCTION("9kPCz7Or+1Y", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceReenableMusicPlayer);
    LIB_FUNCTION("Pi3K47Xw0ss", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceRegisterDaemon);
    LIB_FUNCTION("Oms065qIClY", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceReleaseFb0);
    LIB_FUNCTION("3s8cHiCBKBE", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceReportAbnormalTermination);
    LIB_FUNCTION("3ZFpzcRqYsk", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceRequestCameraCalibration);
    LIB_FUNCTION("P71fvnHyFTQ", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceRequestToChangeRenderingMode);
    LIB_FUNCTION("tMuzuZcUIcA", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceResumeLocalProcess);
    LIB_FUNCTION("DNE77sfNw5Y", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceSetControllerFocusPermission);
    LIB_FUNCTION("eLWnPuja+Y8", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceSetGpuLoadEmulationMode);
    LIB_FUNCTION("Xn-eH9-Fu60", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceSetOutOfVrPlayAreaFlag);
    LIB_FUNCTION("sgRPNJjrWjg", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceSetOutOfVrPlayZoneWarning);
    LIB_FUNCTION("w9wlKcHrmm8", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceShowControllerSettings);
    LIB_FUNCTION("tPfQU2pD4-M", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceShowDisplaySafeAreaSettings);
    LIB_FUNCTION("f8eZvJ8hV6o", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceShowEyeToEyeDistanceSetting);
    LIB_FUNCTION("vY1-RZtvvbk", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceSuspendBackgroundApp);
    LIB_FUNCTION("kTiAx7e2zU4", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceSuspendLocalProcess);
    LIB_FUNCTION("zlXqkzPY-ds", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceTickVideoPlayback);
    LIB_FUNCTION("vOhqz-IMiW4", "libSceSystemService", 1, "libSceSystemService",
                 sceSystemServiceTurnOffScreenSaver);
    LIB_FUNCTION("kDGjRMtUDxo", "libSceSystemService", 1, "libSceSystemService",
                 Func_9031A344CB540F1A);
    LIB_FUNCTION("qdTPJWjquDc", "libSceSystemService", 1, "libSceSystemService",
                 Func_A9D4CF2568EAB837);
    LIB_FUNCTION("f34qn7XA3QE", "libSceSystemServiceWebApp", 1, "libSceSystemService",
                 sceSystemServiceLaunchWebApp);
    LIB_FUNCTION("uElcdmhh-c8", "libSceSystemServiceWebApp", 1, "libSceSystemService",
                 Func_B8495C766861FDCF);
    LIB_FUNCTION("0TDfP7R4fiQ", "libSceSystemServiceDbg", 1, "libSceSystemService",
                 sceSystemServiceGetDbgExecutablePath);
    LIB_FUNCTION("+2uXfrrQCyk", "libSceSystemServiceActivateHevc", 1, "libSceSystemService",
                 sceSystemServiceActivateHevc);
    LIB_FUNCTION("VXA8STT529w", "libSceSystemServiceActivateHevc", 1, "libSceSystemService",
                 sceSystemServiceActivateHevcAbort);
    LIB_FUNCTION("-9LzYPdangA", "libSceSystemServiceActivateHevc", 1, "libSceSystemService",
                 sceSystemServiceActivateHevcGetStatus);
    LIB_FUNCTION("BgjPgbXKYjE", "libSceSystemServiceActivateHevc", 1, "libSceSystemService",
                 sceSystemServiceActivateHevcInit);
    LIB_FUNCTION("2HHfdrT+rnQ", "libSceSystemServiceActivateHevc", 1, "libSceSystemService",
                 sceSystemServiceActivateHevcIsActivated);
    LIB_FUNCTION("E9FdusyklCA", "libSceSystemServiceActivateHevc", 1, "libSceSystemService",
                 sceSystemServiceActivateHevcStart);
    LIB_FUNCTION("tImUgGSSHpc", "libSceSystemServiceActivateHevc", 1, "libSceSystemService",
                 sceSystemServiceActivateHevcTerm);
    LIB_FUNCTION("F-nn3DvNKww", "libSceSystemServiceActivateMpeg2", 1, "libSceSystemService",
                 sceSystemServiceActivateMpeg2Abort);
    LIB_FUNCTION("W-U8F5o2SHg", "libSceSystemServiceActivateMpeg2", 1, "libSceSystemService",
                 sceSystemServiceActivateMpeg2GetStatus);
    LIB_FUNCTION("PkRTWNBI4IQ", "libSceSystemServiceActivateMpeg2", 1, "libSceSystemService",
                 sceSystemServiceActivateMpeg2Init);
    LIB_FUNCTION("aVZb961bWBU", "libSceSystemServiceActivateMpeg2", 1, "libSceSystemService",
                 sceSystemServiceActivateMpeg2IsActivated);
    LIB_FUNCTION("-7zMNJ1Ap1c", "libSceSystemServiceActivateMpeg2", 1, "libSceSystemService",
                 sceSystemServiceActivateMpeg2Start);
    LIB_FUNCTION("JjIspXDbL6o", "libSceSystemServiceActivateMpeg2", 1, "libSceSystemService",
                 sceSystemServiceActivateMpeg2Term);
    LIB_FUNCTION("eBFzDYThras", "libSceSystemStateMgr", 1, "libSceSystemService",
                 sceSystemStateMgrCancelShutdownTimer);
    LIB_FUNCTION("Ap5dJ0zHRVY", "libSceSystemStateMgr", 1, "libSceSystemService",
                 sceSystemStateMgrEnterMediaPlaybackMode);
    LIB_FUNCTION("Laac0S4FuhE", "libSceSystemStateMgr", 1, "libSceSystemService",
                 sceSystemStateMgrEnterStandby);
    LIB_FUNCTION("rSquvOtwQmk", "libSceSystemStateMgr", 1, "libSceSystemService",
                 sceSystemStateMgrExtendShutdownTimer);
    LIB_FUNCTION("FzjISMWw5Xg", "libSceSystemStateMgr", 1, "libSceSystemService",
                 sceSystemStateMgrExtendShutdownTimerForPostAutoUpdateProcess);
    LIB_FUNCTION("ze0ky5Q1yE8", "libSceSystemStateMgr", 1, "libSceSystemService",
                 sceSystemStateMgrGetCurrentState);
    LIB_FUNCTION("wlxvESTUplk", "libSceSystemStateMgr", 1, "libSceSystemService",
                 sceSystemStateMgrGetTriggerCode);
    LIB_FUNCTION("cmjuYpVujQs", "libSceSystemStateMgr", 1, "libSceSystemService",
                 sceSystemStateMgrIsBdDriveReady);
    LIB_FUNCTION("texLPLDXDso", "libSceSystemStateMgr", 1, "libSceSystemService",
                 sceSystemStateMgrIsGpuPerformanceNormal);
    LIB_FUNCTION("asLBe0esmIY", "libSceSystemStateMgr", 1, "libSceSystemService",
                 sceSystemStateMgrIsShellUIShutdownInProgress);
    LIB_FUNCTION("j3IrOCL+DmM", "libSceSystemStateMgr", 1, "libSceSystemService",
                 sceSystemStateMgrIsStandbyModeEnabled);
    LIB_FUNCTION("88y5DztlXBE", "libSceSystemStateMgr", 1, "libSceSystemService",
                 sceSystemStateMgrLeaveMediaPlaybackMode);
    LIB_FUNCTION("H2f6ZwIqLJg", "libSceSystemStateMgr", 1, "libSceSystemService",
                 sceSystemStateMgrNotifySystemSuspendResumeProgress);
    LIB_FUNCTION("uR1wFHXX1XQ", "libSceSystemStateMgr", 1, "libSceSystemService",
                 sceSystemStateMgrReboot);
    LIB_FUNCTION("gPx1b36zyMY", "libSceSystemStateMgr", 1, "libSceSystemService",
                 sceSystemStateMgrSendCecOneTouchPlayCommand);
    LIB_FUNCTION("PcJ5DLzZXSs", "libSceSystemStateMgr", 1, "libSceSystemService",
                 sceSystemStateMgrStartRebootTimer);
    LIB_FUNCTION("7qf7mhzOQPo", "libSceSystemStateMgr", 1, "libSceSystemService",
                 sceSystemStateMgrStartShutdownTimer);
    LIB_FUNCTION("ZwhQSHTqGpE", "libSceSystemStateMgr", 1, "libSceSystemService",
                 sceSystemStateMgrStartStadbyTimer);
    LIB_FUNCTION("YWftBq50hcA", "libSceSystemStateMgr", 1, "libSceSystemService",
                 sceSystemStateMgrStartVshAutoUpdateTimer);
    LIB_FUNCTION("ypl-BoZZKOM", "libSceSystemStateMgr", 1, "libSceSystemService",
                 sceSystemStateMgrTickMusicPlayback);
    LIB_FUNCTION("GvqPsPX4EUI", "libSceSystemStateMgr", 1, "libSceSystemService",
                 sceSystemStateMgrTickPartyChat);
    LIB_FUNCTION("gK3EX6ZKtKc", "libSceSystemStateMgr", 1, "libSceSystemService",
                 sceSystemStateMgrTurnOff);
    LIB_FUNCTION("U1dZXAjkBVo", "libSceSystemStateMgr", 1, "libSceSystemService",
                 sceSystemStateMgrVshAutoUpdate);
    LIB_FUNCTION("geg26leOsvw", "libSceSystemStateMgr", 1, "libSceSystemService",
                 sceSystemStateMgrWaitVshAutoUpdateVerifyDone);
    LIB_FUNCTION("6gtqLPVTdJY", "libSceSystemStateMgr", 1, "libSceSystemService",
                 sceSystemStateMgrWakeUp);
    LIB_FUNCTION("ifJiF5witJ4", "libSceSystemStateMgr", 1, "libSceSystemService",
                 Func_89F262179C22B49E);
    LIB_FUNCTION("rIqPq0oWlrg", "libSceSystemStateMgr", 1, "libSceSystemService",
                 Func_AC8A8FAB4A1696B8);
    LIB_FUNCTION("rTa0Vp-4nKA", "libSceSystemServiceAppLaunchLink", 1, "libSceSystemService",
                 sceSystemServiceInvokeAppLaunchLink);
    LIB_FUNCTION("uhD7g7zXIQo", "libSceSystemServiceClosedCaption", 1, "libSceSystemService",
                 sceSystemServiceShowClosedCaptionAdvancedSettings);
    LIB_FUNCTION("5W6LurzMZaY", "libSceSystemServiceClosedCaption", 1, "libSceSystemService",
                 sceSystemServiceShowClosedCaptionSettings);
    LIB_FUNCTION("Mr1IgQaRff0", "libSceSystemServiceEyeToEyeDistance", 1, "libSceSystemService",
                 sceSystemServiceDisablePersonalEyeToEyeDistanceSetting);
    LIB_FUNCTION("O3irWUQ2s-g", "libSceSystemServiceEyeToEyeDistance", 1, "libSceSystemService",
                 sceSystemServiceEnablePersonalEyeToEyeDistanceSetting);
    LIB_FUNCTION("nT-7-iG55M8", "libSceSystemServicePowerSaveLevel", 1, "libSceSystemService",
                 sceSystemServiceSetPowerSaveLevel);
    LIB_FUNCTION("gD4wh2+nuuU", "libSceSystemServiceForShellCoreOnly", 1, "libSceSystemService",
                 sceSystemServiceInitializeForShellCore);
    LIB_FUNCTION("fBGD-HNimSk", "libSceSystemServicePadspkRouting", 1, "libSceSystemService",
                 Func_7C1183FC73629929);
    LIB_FUNCTION("45QrFvUkrjg", "libSceSystemServicePartyVoice", 1, "libSceSystemService",
                 sceSystemServiceDisablePartyVoice);
    LIB_FUNCTION("hU3bSlF2OKs", "libSceSystemServicePartyVoice", 1, "libSceSystemService",
                 sceSystemServiceReenablePartyVoice);
    LIB_FUNCTION("5u2WeL-PR2w", "libSceSystemServicePlatformPrivacy", 1, "libSceSystemService",
                 sceSystemServiceGetPlatformPrivacyDefinitionData);
    LIB_FUNCTION("t5K+IeMVD1Q", "libSceSystemServicePlatformPrivacy", 1, "libSceSystemService",
                 sceSystemServiceGetPlatformPrivacyDefinitionVersion);
    LIB_FUNCTION("hvoLYhc4cq0", "libSceSystemServicePlatformPrivacy", 1, "libSceSystemService",
                 sceSystemServiceGetPlatformPrivacySetting);
    LIB_FUNCTION("EqcPA3ugRP8", "libSceSystemServiceSuspend", 1, "libSceSystemService",
                 sceSystemServiceDeclareReadyForSuspend);
    LIB_FUNCTION("Mi0qwCb+rvo", "libSceSystemServiceSuspend", 1, "libSceSystemService",
                 sceSystemServiceDisableSuspendNotification);
    LIB_FUNCTION("a5Kjjq6HgcU", "libSceSystemServiceSuspend", 1, "libSceSystemService",
                 sceSystemServiceEnableSuspendNotification);
    LIB_FUNCTION("d4imyunHryo", "libSceSystemServicePowerControl", 1, "libSceSystemService",
                 sceSystemServiceRequestPowerOff);
    LIB_FUNCTION("oEJqGsNtFIw", "libSceSystemServicePowerControl", 1, "libSceSystemService",
                 sceSystemServiceRequestReboot);
    LIB_FUNCTION("tIdXUhSLyOU", "libSceSystemServicePs2Emu", 1, "libSceSystemService",
                 sceSystemServiceAddLocalProcessForPs2Emu);
    LIB_FUNCTION("qhPJ1EfqLjQ", "libSceSystemServicePs2Emu", 1, "libSceSystemService",
                 sceSystemServiceGetParentSocketForPs2Emu);
    LIB_FUNCTION("fKqJTnoZ8C8", "libSceSystemServicePs2Emu", 1, "libSceSystemService",
                 sceSystemServiceKillLocalProcessForPs2Emu);
    LIB_FUNCTION("YtDk7X3FF08", "libSceSystemServicePs2Emu", 1, "libSceSystemService",
                 sceSystemServiceShowImposeMenuForPs2Emu);
    LIB_FUNCTION("DILuzcvXjGQ", "libSceSystemServiceVideoToken", 1, "libSceSystemService",
                 sceSystemServiceSaveVideoToken);
    LIB_FUNCTION("cltshBrDLC0", "libSceSystemServicePsmKit", 1, "libSceSystemService",
                 sceSystemServiceAddLocalProcessForPsmKit);
    LIB_FUNCTION("4ZYuSI8i2aM", "libSceSystemServicePsmKit", 1, "libSceSystemService",
                 sceSystemServiceGetParentSocketForPsmKit);
    LIB_FUNCTION("7cTc7seJLfQ", "libSceSystemServicePsmKit", 1, "libSceSystemService",
                 sceSystemServiceKillLocalProcessForPsmKit);
    LIB_FUNCTION("Zj5FGJQPFxs", "libSceSystemServiceStore", 1, "libSceSystemService",
                 sceSystemServiceLaunchStore);
    LIB_FUNCTION("3nn7rnOdt1g", "libSceSystemServiceTelemetry", 1, "libSceSystemService",
                 sceSystemServiceTelemetrySetData);
    LIB_FUNCTION("G5AwzWnHxks", "libSceSystemServiceTournamentMlg", 1, "libSceSystemService",
                 sceSystemServiceLaunchTournamentList);
    LIB_FUNCTION("xn-HgPW29x4", "libSceSystemServiceTournamentMlg", 1, "libSceSystemService",
                 Func_C67FC780F5B6F71E);
    LIB_FUNCTION("YNoDjc1BPJI", "libSceSystemServiceUdsApp", 1, "libSceSystemService",
                 sceSystemServiceLaunchUdsApp);
    LIB_FUNCTION("AmTvo3RT5ss", "libSceSystemServiceVideoServiceWebApp", 1, "libSceSystemService",
                 sceSystemServiceLoadExecVideoServiceWebApp);
    LIB_FUNCTION("d3OnoKtNjGg", "libSceSystemServiceVoiceRecognition", 1, "libSceSystemService",
                 sceSystemServiceDisableVoiceRecognition);
    LIB_FUNCTION("c-aFKhn74h0", "libSceSystemServiceVoiceRecognition", 1, "libSceSystemService",
                 sceSystemServiceReenableVoiceRecognition);
    LIB_FUNCTION("-+3hY+y8bNo", "libSceSystemServiceWebBrowser", 1, "libSceSystemService",
                 sceSystemServiceLaunchWebBrowser);
    LIB_FUNCTION("axzblV8OvWU", "libSceSystemServiceYouTubeAccountLinkStatus", 1,
                 "libSceSystemService", Func_6B1CDB955F0EBD65);
    LIB_FUNCTION("y16IXiJfafA", "libSceSystemServiceYouTubeAccountLinkStatus", 1,
                 "libSceSystemService", Func_CB5E885E225F69F0);
};

} // namespace Libraries::SystemService
