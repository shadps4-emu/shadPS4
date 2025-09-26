// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
// reference
// https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain/blob/master/include/orbis/_types/sys_service.h
#pragma once

#include <mutex>
#include <queue>
#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::SystemService {

enum class OrbisSystemServiceParamId {
    Lang = 1,
    DateFormat = 2,
    TimeFormat = 3,
    TimeZone = 4,
    Summertime = 5,
    SystemName = 6,
    GameParentalLevel = 7,
    EnterButtonAssign = 1000,
};

enum class OrbisSystemParamDateFormat {
    FmtYYYYMMDD = 0,
    FmtDDMMYYYY = 1,
    FmtMMDDYYYY = 2,
};

enum class OrbisSystemParamTimeFormat {
    Fmt12Hour = 0,
    Fmt24Hour = 1,
};

enum class OrbisSystemParamGameParentalLevel {
    Off = 0,
    Level01 = 1,
    Level02 = 2,
    Level03 = 3,
    Level04 = 4,
    Level05 = 5,
    Level06 = 6,
    Level07 = 7,
    Level08 = 8,
    Level09 = 9,
    Level10 = 10,
    Level11 = 11,
};

enum class OrbisSystemParamEnterButtonAssign {
    Circle = 0,
    Cross = 1,
};

enum class OrbisSystemServiceEventType {
    Invalid = -1,
    OnResume = 0x10000000,
    GameLiveStreamingStatusUpdate = 0x10000001,
    SessionInvitation = 0x10000002,
    EntitlementUpdate = 0x10000003,
    GameCustomData = 0x10000004,
    DisplaySafeAreaUpdate = 0x10000005,
    UrlOpen = 0x10000006,
    LaunchApp = 0x10000007,
    AppLaunchLink = 0x10000008,
    AddcontentInstall = 0x10000009,
    ResetVrPosition = 0x1000000a,
    JoinEvent = 0x1000000b,
    PlaygoLocusUpdate = 0x1000000c,
    PlayTogetherHost = 0x1000000d,
    ServiceEntitlementUpdate = 0x1000000e,
    EyeToEyeDistanceUpdate = 0x1000000f,
    JoinMatchEvent = 0x10000010,
    PlayTogetherHostA = 0x10000011,
    WebBrowserClosed = 0x10000012,
    ControllerSettingsClosed = 0x10000013,
    JoinTeamOnTeamMatchEvent = 0x10000014,
    OpenShareMenu = 0x30000000
};

struct OrbisSystemServiceStatus {
    s32 event_num;
    bool is_system_ui_overlaid;
    bool is_in_background_execution;
    bool is_cpu_mode7_cpu_normal;
    bool is_game_live_streaming_on_air;
    bool is_out_of_vr_play_area;
    u8 reserved[];
};

struct OrbisSystemServiceDisplaySafeAreaInfo {
    float ratio;
    uint8_t reserved[128];
};

struct OrbisSystemServiceEvent {
    OrbisSystemServiceEventType event_type;
    union {
        char param[8192];
        struct {
            char source[1024];
            char url[4096];
        } url_open;
        struct {
            u32 size;
            u8 arg[8188];
        } launch_app;
        struct {
            u32 size;
            u8 arg[2020];
        } app_launch_link;
        struct {
            s32 user_id;
            char event_id[37];
            char boot_argument[7169];
        } join_event;
        struct {
            s32 user_id;
            u32 np_service_label;
            u8 reserved[8184];
        } service_entitlement_update;
        struct {
            s32 user_id;
            u32 np_service_label;
            u8 reserved[8184];
        } unified_entitlement_update;
        u8 reserved[8192];
    };
};

bool IsSplashVisible();

int PS4_SYSV_ABI sceAppMessagingClearEventFlag();
int PS4_SYSV_ABI sceAppMessagingReceiveMsg();
int PS4_SYSV_ABI sceAppMessagingSendMsg();
int PS4_SYSV_ABI sceAppMessagingSendMsgToShellCore();
int PS4_SYSV_ABI sceAppMessagingSendMsgToShellUI();
int PS4_SYSV_ABI sceAppMessagingSetEventFlag();
int PS4_SYSV_ABI sceAppMessagingTryGetEventFlag();
int PS4_SYSV_ABI sceAppMessagingTryReceiveMsg();
int PS4_SYSV_ABI Func_C8E899ABEF7F64C4();
int PS4_SYSV_ABI Func_F74BA759B9C8D2A1();
int PS4_SYSV_ABI sceLncUtilAcquireCpuBudgetOfExtraAudioDevices();
int PS4_SYSV_ABI sceLncUtilAcquireCpuBudgetOfImeForBigApp();
int PS4_SYSV_ABI sceLncUtilAcquireCpuBudgetOfInGameStore();
int PS4_SYSV_ABI sceLncUtilActivateCdlg();
int PS4_SYSV_ABI sceLncUtilAddLocalProcess();
int PS4_SYSV_ABI sceLncUtilBlockAppSuspend();
int PS4_SYSV_ABI sceLncUtilBlockingGetEventForDaemon();
int PS4_SYSV_ABI sceLncUtilContinueApp();
int PS4_SYSV_ABI sceLncUtilCrashSyscore();
int PS4_SYSV_ABI sceLncUtilDeactivateCdlg();
int PS4_SYSV_ABI sceLncUtilDeclareReadyForSuspend();
int PS4_SYSV_ABI sceLncUtilDisableSuspendNotification();
int PS4_SYSV_ABI sceLncUtilEnableSuspendNotification();
int PS4_SYSV_ABI sceLncUtilFinishSpecialResume();
int PS4_SYSV_ABI sceLncUtilForceKillApp();
int PS4_SYSV_ABI sceLncUtilForceKillLocalProcess();
int PS4_SYSV_ABI sceLncUtilGetApp0DirPath();
int PS4_SYSV_ABI sceLncUtilGetAppCategory();
int PS4_SYSV_ABI sceLncUtilGetAppFocusedAppStatus();
int PS4_SYSV_ABI sceLncUtilGetAppId();
int PS4_SYSV_ABI sceLncUtilGetAppIdOfBigApp();
int PS4_SYSV_ABI sceLncUtilGetAppIdOfMiniApp();
int PS4_SYSV_ABI sceLncUtilGetAppLaunchedUser();
int PS4_SYSV_ABI sceLncUtilGetAppStatus();
int PS4_SYSV_ABI sceLncUtilGetAppStatusListForShellUIReboot();
int PS4_SYSV_ABI sceLncUtilGetAppTitleId();
int PS4_SYSV_ABI sceLncUtilGetAppType();
int PS4_SYSV_ABI sceLncUtilGetCdlgExec();
int PS4_SYSV_ABI sceLncUtilGetCoredumpState();
int PS4_SYSV_ABI sceLncUtilGetDbgExecutablePath();
int PS4_SYSV_ABI sceLncUtilGetEventForDaemon();
int PS4_SYSV_ABI sceLncUtilGetEventForShellUI();
int PS4_SYSV_ABI sceLncUtilGetGpuCrashFullDumpAppStatus();
int PS4_SYSV_ABI sceLncUtilGetLocalProcessStatusList();
int PS4_SYSV_ABI sceLncUtilGetParentSocket();
int PS4_SYSV_ABI sceLncUtilGetResultKillApp();
int PS4_SYSV_ABI sceLncUtilGetResultLaunchAppByTitleId();
int PS4_SYSV_ABI sceLncUtilInitialize();
int PS4_SYSV_ABI sceLncUtilIsActiveCdlg();
int PS4_SYSV_ABI sceLncUtilIsAppLaunched();
int PS4_SYSV_ABI sceLncUtilIsAppSuspended();
int PS4_SYSV_ABI sceLncUtilIsCpuBudgetOfExtraAudioDevicesAvailable();
int PS4_SYSV_ABI sceLncUtilIsPs2Emu();
int PS4_SYSV_ABI sceLncUtilIsShellUiFgAndGameBgCpuMode();
int PS4_SYSV_ABI sceLncUtilKickCoredumpOnlyProcMem();
int PS4_SYSV_ABI sceLncUtilKillApp();
int PS4_SYSV_ABI sceLncUtilKillAppWithReason();
int PS4_SYSV_ABI sceLncUtilKillLocalProcess();
int PS4_SYSV_ABI sceLncUtilLaunchApp();
int PS4_SYSV_ABI sceLncUtilLoadExec();
int PS4_SYSV_ABI sceLncUtilNotifyCoredumpRequestEnd();
int PS4_SYSV_ABI sceLncUtilNotifyCoredumpRequestProgress();
int PS4_SYSV_ABI sceLncUtilNotifyVshReady();
int PS4_SYSV_ABI sceLncUtilRaiseException();
int PS4_SYSV_ABI sceLncUtilRaiseExceptionLocalProcess();
int PS4_SYSV_ABI sceLncUtilRegisterCdlgSharedMemoryName();
int PS4_SYSV_ABI sceLncUtilRegisterDaemon();
int PS4_SYSV_ABI sceLncUtilRegisterShellUI();
int PS4_SYSV_ABI sceLncUtilReleaseCpuBudgetOfExtraAudioDevices();
int PS4_SYSV_ABI sceLncUtilReleaseCpuBudgetOfImeForBigApp();
int PS4_SYSV_ABI sceLncUtilReleaseCpuBudgetOfInGameStore();
int PS4_SYSV_ABI sceLncUtilResumeApp();
int PS4_SYSV_ABI sceLncUtilResumeLocalProcess();
int PS4_SYSV_ABI sceLncUtilSetAppFocus();
int PS4_SYSV_ABI sceLncUtilSetCdlgExec();
int PS4_SYSV_ABI sceLncUtilSetControllerFocus();
int PS4_SYSV_ABI sceLncUtilSetControllerFocusPermission();
int PS4_SYSV_ABI sceLncUtilStartKillApp();
int PS4_SYSV_ABI sceLncUtilStartLaunchAppByTitleId();
int PS4_SYSV_ABI sceLncUtilSuspendApp();
int PS4_SYSV_ABI sceLncUtilSuspendBackgroundApp();
int PS4_SYSV_ABI sceLncUtilSuspendLocalProcess();
int PS4_SYSV_ABI sceLncUtilSystemSuspend();
int PS4_SYSV_ABI sceLncUtilTerminate();
int PS4_SYSV_ABI sceLncUtilTryBlockAppSuspend();
int PS4_SYSV_ABI sceLncUtilUnblockAppSuspend();
int PS4_SYSV_ABI sceLncUtilUnregisterCdlgSharedMemoryName();
int PS4_SYSV_ABI sceLncUtilUnregisterDaemon();
int PS4_SYSV_ABI sceLncUtilUnregisterShellUI();
int PS4_SYSV_ABI sceSystemServiceActivateHevcSoft();
int PS4_SYSV_ABI sceSystemServiceActivateHevcSoftAbort();
int PS4_SYSV_ABI sceSystemServiceActivateHevcSoftGetStatus();
int PS4_SYSV_ABI sceSystemServiceActivateHevcSoftInit();
int PS4_SYSV_ABI sceSystemServiceActivateHevcSoftIsActivated();
int PS4_SYSV_ABI sceSystemServiceActivateHevcSoftStart();
int PS4_SYSV_ABI sceSystemServiceActivateHevcSoftTerm();
int PS4_SYSV_ABI sceShellCoreUtilAccessibilityZoomLock();
int PS4_SYSV_ABI sceShellCoreUtilAccessibilityZoomUnlock();
int PS4_SYSV_ABI sceShellCoreUtilAcquireBgmCpuBudget();
int PS4_SYSV_ABI sceShellCoreUtilAcquireRemotePlayCpuBudget();
int PS4_SYSV_ABI sceShellCoreUtilAcquireSharePlayCpuBudget();
int PS4_SYSV_ABI sceShellCoreUtilActivateAbort();
int PS4_SYSV_ABI sceShellCoreUtilActivateGetStatus();
int PS4_SYSV_ABI sceShellCoreUtilActivateInit();
int PS4_SYSV_ABI sceShellCoreUtilActivateIsActivated();
int PS4_SYSV_ABI sceShellCoreUtilActivateRecordActivation();
int PS4_SYSV_ABI sceShellCoreUtilActivateStart();
int PS4_SYSV_ABI sceShellCoreUtilActivateStartAsync();
int PS4_SYSV_ABI sceShellCoreUtilActivateTerm();
int PS4_SYSV_ABI sceShellCoreUtilChangeRunLevel();
int PS4_SYSV_ABI sceShellCoreUtilChangeToStaffModeForIDU();
int PS4_SYSV_ABI sceShellCoreUtilCheckerAbort();
int PS4_SYSV_ABI sceShellCoreUtilCleanupCrashReport();
int PS4_SYSV_ABI sceShellCoreUtilClearAppData();
int PS4_SYSV_ABI sceShellCoreUtilClearPsnAccountInfo();
int PS4_SYSV_ABI sceShellCoreUtilCrashReportRequestCancel();
int PS4_SYSV_ABI sceShellCoreUtilDeclareBeginOfExternalStorageAppMove();
int PS4_SYSV_ABI sceShellCoreUtilDeclareEndOfExternalStorageAppMove();
int PS4_SYSV_ABI sceShellCoreUtilDeleteDiscInstalledTitleWorkaroundFile();
int PS4_SYSV_ABI sceShellCoreUtilDeleteDownloadedHidConfigFile();
int PS4_SYSV_ABI sceShellCoreUtilDeleteDownloadedNetEvConfigFile();
int PS4_SYSV_ABI sceShellCoreUtilDeleteDownloadedTitleWorkaroundFile();
int PS4_SYSV_ABI sceShellCoreUtilDeleteSmrHddDummyData();
int PS4_SYSV_ABI sceShellCoreUtilDoFsck();
int PS4_SYSV_ABI sceShellCoreUtilDownloadHidConfigFileFromServer();
int PS4_SYSV_ABI sceShellCoreUtilDownloadNetEvConfigFileFromServer();
int PS4_SYSV_ABI sceShellCoreUtilDownloadTitleWorkaroundFileFromServer();
int PS4_SYSV_ABI sceShellCoreUtilEnterPowerLockSection();
int PS4_SYSV_ABI sceShellCoreUtilExecuteCrashReport();
int PS4_SYSV_ABI sceShellCoreUtilExfatFormatExternalHdd();
int PS4_SYSV_ABI sceShellCoreUtilExitMiniApp();
int PS4_SYSV_ABI sceShellCoreUtilExitMiniAppWithValue();
int PS4_SYSV_ABI sceShellCoreUtilFillUpSpaceOnSmrHdd();
int PS4_SYSV_ABI sceShellCoreUtilFormatExternalHdd();
int PS4_SYSV_ABI sceShellCoreUtilFormatHddForRestore();
int PS4_SYSV_ABI sceShellCoreUtilFreeUpSpaceOnSmrHdd();
int PS4_SYSV_ABI sceShellCoreUtilGetAppData();
int PS4_SYSV_ABI sceShellCoreUtilGetAppEnableTTS();
int PS4_SYSV_ABI sceShellCoreUtilGetAppEnterButtonAssign();
int PS4_SYSV_ABI sceShellCoreUtilGetAppLaunchedParamInt();
int PS4_SYSV_ABI sceShellCoreUtilGetAppLaunchedParamIntByBudgetType();
int PS4_SYSV_ABI sceShellCoreUtilGetAppLaunchedParamString();
int PS4_SYSV_ABI sceShellCoreUtilGetAppLaunchedParamStringByBudgetType();
int PS4_SYSV_ABI sceShellCoreUtilGetAppLaunchTypeInfo();
int PS4_SYSV_ABI sceShellCoreUtilGetAutoPowerDownRemainingSeconds();
int PS4_SYSV_ABI sceShellCoreUtilGetBasicProductShape();
int PS4_SYSV_ABI sceShellCoreUtilGetCheckerString();
int PS4_SYSV_ABI sceShellCoreUtilGetCheckerStringEx();
int PS4_SYSV_ABI sceShellCoreUtilGetCloudClientStatus();
int PS4_SYSV_ABI sceShellCoreUtilGetCrashReportCoreFileSetSize();
int PS4_SYSV_ABI sceShellCoreUtilGetCrashReportFilterInfoStart();
int PS4_SYSV_ABI sceShellCoreUtilGetCrashReportInfoForBoot();
int PS4_SYSV_ABI sceShellCoreUtilGetCrashReportInfoForBootStart();
int PS4_SYSV_ABI sceShellCoreUtilGetCrashReportInfoStart();
int PS4_SYSV_ABI sceShellCoreutilGetCrashReportProcessInformation();
int PS4_SYSV_ABI sceShellCoreUtilGetCrashReportResult();
int PS4_SYSV_ABI sceShellCoreUtilGetCrashReportStatus();
int PS4_SYSV_ABI sceShellCoreUtilGetCrashReportUploadStatus();
int PS4_SYSV_ABI sceShellCoreUtilGetDeviceIndexBehavior();
int PS4_SYSV_ABI sceShellCoreUtilGetDeviceIndexBehaviorWithTimeout();
int PS4_SYSV_ABI sceShellCoreUtilGetEffectiveTotalSizeOfUserPartition();
int PS4_SYSV_ABI sceShellCoreUtilGetFreeSizeOfAvContentsTmp();
int PS4_SYSV_ABI sceShellCoreUtilGetFreeSizeOfUserPartition();
int PS4_SYSV_ABI sceShellCoreUtilGetFsckProgress();
int PS4_SYSV_ABI sceShellCoreUtilGetGameLiveStreamingStatus();
int PS4_SYSV_ABI sceShellCoreUtilGetGnmCompositorOnScreenProfilerFlag();
int PS4_SYSV_ABI sceShellCoreUtilGetGpuLoadEmulationMode();
int PS4_SYSV_ABI sceShellCoreUtilGetGpuLoadEmulationModeByAppId();
int PS4_SYSV_ABI sceShellCoreUtilGetHidConfigFileInfoString();
int PS4_SYSV_ABI sceShellCoreUtilGetHidConfigFileString();
int PS4_SYSV_ABI sceShellCoreUtilGetHidConfigName();
int PS4_SYSV_ABI sceShellCoreUtilGetHidConfigNum();
int PS4_SYSV_ABI sceShellCoreUtilGetIDUMode();
int PS4_SYSV_ABI sceShellCoreUtilGetImposeMenuFlagForPs2Emu();
int PS4_SYSV_ABI sceShellCoreUtilGetManifestFileStatus();
int PS4_SYSV_ABI sceShellCoreUtilGetNeedSizeOfAppContent();
int PS4_SYSV_ABI sceShellCoreUtilGetNetEvConfigFileInfoString();
int PS4_SYSV_ABI sceShellCoreUtilGetOptimizationStatus();
int PS4_SYSV_ABI sceShellCoreUtilGetOutOfVrPlayZoneWarning();
int PS4_SYSV_ABI sceShellCoreUtilGetPapcGamePcl();
int PS4_SYSV_ABI sceShellCoreUtilGetPbtcUserInfoList();
int PS4_SYSV_ABI sceShellCoreUtilGetPlatformPrivacyDefinitionEventData();
int PS4_SYSV_ABI sceShellCoreUtilGetPlatformPrivacySetting();
int PS4_SYSV_ABI sceShellCoreUtilGetProgressOfFormatExternalHdd();
int PS4_SYSV_ABI sceShellCoreUtilGetProgressOfFsck();
int PS4_SYSV_ABI sceShellCoreUtilGetPsnAccountInfo();
int PS4_SYSV_ABI sceShellCoreUtilGetPsStoreIconLayout();
int PS4_SYSV_ABI sceShellCoreUtilGetPsStoreIconState();
int PS4_SYSV_ABI sceShellCoreUtilGetRegion();
int PS4_SYSV_ABI sceShellCoreUtilGetRemotePlayStatus();
int PS4_SYSV_ABI sceShellCoreUtilGetRunLevel();
int PS4_SYSV_ABI sceShellCoreUtilGetSharePlayStatus();
int PS4_SYSV_ABI sceShellCoreUtilGetShellUIVMStats();
int PS4_SYSV_ABI sceShellCoreUtilGetSmrHddInfoString();
int PS4_SYSV_ABI sceShellCoreUtilGetSocialScreenStatus();
int PS4_SYSV_ABI sceShellCoreUtilGetSplashScreenState();
int PS4_SYSV_ABI sceShellCoreUtilGetSupportSiteURL();
int PS4_SYSV_ABI sceShellCoreUtilGetSuspendConfirmationDialogFlag();
int PS4_SYSV_ABI sceShellCoreUtilGetSystemBGState();
int PS4_SYSV_ABI sceShellCoreUtilGetSystemBGWaveColor();
int PS4_SYSV_ABI sceShellCoreUtilGetSystemBGWaveState();
int PS4_SYSV_ABI sceShellCoreUtilGetTitleWorkaroundFileInfoString();
int PS4_SYSV_ABI sceShellCoreUtilGetTitleWorkaroundFileString();
int PS4_SYSV_ABI sceShellCoreUtilGetUIStatus();
int PS4_SYSV_ABI sceShellCoreUtilGetUserFocus();
int PS4_SYSV_ABI sceShellCoreUtilGetUserIdOfMorpheusUser();
int PS4_SYSV_ABI sceShellCoreUtilGetVersionNumberOfCameraCalibrationData();
int PS4_SYSV_ABI sceShellCoreUtilGoBackToKratosCurrentSessionGame();
int PS4_SYSV_ABI sceShellCoreUtilHideBlocksFromUser();
int PS4_SYSV_ABI sceShellCoreUtilIncrementVersionNumberOfCameraCalibrationData();
int PS4_SYSV_ABI sceShellCoreUtilIsAccessibilityZoomLocked();
int PS4_SYSV_ABI sceShellCoreUtilIsBgmCpuBudgetAcquired();
int PS4_SYSV_ABI sceShellCoreUtilIsBgmCpuBudgetAvailable();
int PS4_SYSV_ABI sceShellCoreUtilIsBgmPlaying();
int PS4_SYSV_ABI sceShellCoreUtilIsExternalStorageAppMoveInProgress();
int PS4_SYSV_ABI sceShellCoreUtilIsEyeToEyeDistanceAdjusted();
int PS4_SYSV_ABI sceShellCoreUtilIsGameLiveStreamingOnAir();
int PS4_SYSV_ABI sceShellCoreUtilIsImposeScreenOverlaid();
int PS4_SYSV_ABI sceShellCoreUtilIsInSystemSuspendBlackList();
int PS4_SYSV_ABI sceShellCoreUtilIsInternalKratosUser();
int PS4_SYSV_ABI sceShellCoreUtilIsKilledOrSuspendedByLogout();
int PS4_SYSV_ABI sceShellCoreUtilIsNeededCrashReport();
int PS4_SYSV_ABI sceShellCoreUtilIsPowerSaveAlertRequested();
int PS4_SYSV_ABI sceShellCoreUtilIsRemotePlayCpuBudgetAcquired();
int PS4_SYSV_ABI sceShellCoreUtilIsScreenSaverOn();
int PS4_SYSV_ABI sceShellCoreUtilIsSharePlayCpuBudgetAcquired();
int PS4_SYSV_ABI sceShellCoreUtilIsShowCrashReport();
int PS4_SYSV_ABI sceShellCoreUtilIsTemperatureDanger();
int PS4_SYSV_ABI sceShellCoreUtilIsTitleWorkaroundEnabled();
int PS4_SYSV_ABI sceShellCoreUtilIsUsbMassStorageMounted();
int PS4_SYSV_ABI sceShellCoreUtilLaunchByUri();
int PS4_SYSV_ABI sceShellCoreUtilLeavePowerLockSection();
int PS4_SYSV_ABI sceShellCoreUtilLog();
int PS4_SYSV_ABI sceShellCoreUtilMakeManifestFile();
int PS4_SYSV_ABI sceShellCoreUtilMountAppRight();
int PS4_SYSV_ABI sceShellCoreUtilMountDownloadDataForShellUI();
int PS4_SYSV_ABI sceShellCoreUtilMountHddForBackup();
int PS4_SYSV_ABI sceShellCoreUtilMountHddForRestore();
int PS4_SYSV_ABI sceShellCoreUtilNavigateToAnotherApp();
int PS4_SYSV_ABI sceShellCoreUtilNavigateToGoHome();
int PS4_SYSV_ABI sceShellCoreUtilNavigateToLaunchedApp();
int PS4_SYSV_ABI sceShellCoreUtilNotificationCancelForIDU();
int PS4_SYSV_ABI sceShellCoreUtilNotificationRequestedForIDU();
int PS4_SYSV_ABI sceShellCoreUtilNotifyBgmCoreTermination();
int PS4_SYSV_ABI sceShellCoreUtilNotifyFarsightUIDone();
int PS4_SYSV_ABI sceShellCoreUtilNotifyFsReadError();
int PS4_SYSV_ABI sceShellCoreUtilNotifyPsnAccountInfoReceived();
int PS4_SYSV_ABI sceShellCoreUtilNotifyYouTubeAccountLinkStatusChanged();
int PS4_SYSV_ABI sceShellCoreUtilPfAuthClientConsoleTokenClearCache();
int PS4_SYSV_ABI sceShellCoreUtilPostActivityForPsNow();
int PS4_SYSV_ABI sceShellCoreUtilPostErrorLog();
int PS4_SYSV_ABI sceShellCoreUtilPostLaunchConfirmResult();
int PS4_SYSV_ABI sceShellCoreUtilPostPsmEventToShellUI();
int PS4_SYSV_ABI sceShellCoreUtilPreNotifyOfGameLiveStreaming();
int PS4_SYSV_ABI sceShellCoreUtilPreNotifyOfRemotePlay();
int PS4_SYSV_ABI sceShellCoreUtilPreNotifyOfSharePlay();
int PS4_SYSV_ABI sceShellCoreUtilReleaseBgmCpuBudget();
int PS4_SYSV_ABI sceShellCoreUtilReleaseRemotePlayCpuBudget();
int PS4_SYSV_ABI sceShellCoreUtilReleaseSharePlayCpuBudget();
int PS4_SYSV_ABI sceShellCoreUtilReportSessionErrorToGaikaiController();
int PS4_SYSV_ABI sceShellCoreUtilReportUnexpectedFatalErrorToSystemTelemetry();
int PS4_SYSV_ABI sceShellCoreUtilRequestCameraCalibration();
int PS4_SYSV_ABI sceShellCoreUtilRequestEjectDevice();
int PS4_SYSV_ABI sceShellCoreUtilRequestRebootApp();
int PS4_SYSV_ABI sceShellCoreUtilRequestShutdown();
int PS4_SYSV_ABI sceShellCoreUtilResetAutoPowerDownTimer();
int PS4_SYSV_ABI sceShellCoreUtilResetBgdcConfig();
int PS4_SYSV_ABI sceShellCoreUtilSetAppData();
int PS4_SYSV_ABI sceShellCoreUtilSetBgmProhibition();
int PS4_SYSV_ABI sceShellCoreUtilSetDeviceIndexBehavior();
int PS4_SYSV_ABI sceShellCoreUtilSetGameLiveStreamingOnAirFlag();
int PS4_SYSV_ABI sceShellCoreUtilSetGameLiveStreamingStatus();
int PS4_SYSV_ABI sceShellCoreUtilSetGnmCompositorOnScreenProfilerFlag();
int PS4_SYSV_ABI sceShellCoreUtilSetGpuLoadEmulationMode();
int PS4_SYSV_ABI sceShellCoreUtilSetGpuLoadEmulationModeByAppId();
int PS4_SYSV_ABI sceShellCoreUtilSetIDUMode();
int PS4_SYSV_ABI sceShellCoreUtilSetImposeStatusFlag();
int PS4_SYSV_ABI sceShellCoreUtilSetPsStoreIconLayout();
int PS4_SYSV_ABI sceShellCoreUtilSetPsStoreIconState();
int PS4_SYSV_ABI sceShellCoreUtilSetRemotePlayStatus();
int PS4_SYSV_ABI sceShellCoreUtilSetSharePlayStatus();
int PS4_SYSV_ABI sceShellCoreUtilSetSkipUpdateCheck();
int PS4_SYSV_ABI sceShellCoreUtilSetSocialScreenStatus();
int PS4_SYSV_ABI sceShellCoreUtilSetSplashScreenState();
int PS4_SYSV_ABI sceShellCoreUtilSetSystemBGState();
int PS4_SYSV_ABI sceShellCoreUtilSetSystemBGWaveColor();
int PS4_SYSV_ABI sceShellCoreUtilSetSystemBGWaveState();
int PS4_SYSV_ABI sceShellCoreUtilSetUIStatus();
int PS4_SYSV_ABI sceShellCoreUtilSetUserFocus();
int PS4_SYSV_ABI sceShellCoreUtilShowCriticalErrorDialog();
int PS4_SYSV_ABI sceShellCoreUtilShowErrorDialog();
int PS4_SYSV_ABI sceShellCoreUtilShowErrorDialogWithFormatArgs();
int PS4_SYSV_ABI sceShellCoreUtilShowErrorDialogWithParam();
int PS4_SYSV_ABI sceShellCoreUtilShowPsUnderLockIndicator();
int PS4_SYSV_ABI sceShellCoreUtilSignalUserInput();
int PS4_SYSV_ABI sceShellCoreUtilStartOptimization();
int PS4_SYSV_ABI sceShellCoreUtilStartPsNowGame();
int PS4_SYSV_ABI sceShellCoreUtilStopOptimization();
int PS4_SYSV_ABI sceShellCoreUtilStopPsNowGame();
int PS4_SYSV_ABI sceShellCoreUtilTestBusTransferSpeed();
int PS4_SYSV_ABI sceShellCoreUtilTickHeartBeat();
int PS4_SYSV_ABI sceShellCoreUtilTriggerPapcRecalculation();
int PS4_SYSV_ABI sceShellCoreUtilTriggerPapcUpdate();
int PS4_SYSV_ABI sceShellCoreUtilTurnOffScreenSaver();
int PS4_SYSV_ABI sceShellCoreUtilUnmountAppRight();
int PS4_SYSV_ABI sceShellCoreUtilUnmountDownloadDataForShellUI();
int PS4_SYSV_ABI sceShellCoreUtilUnmountHddForBackup();
int PS4_SYSV_ABI sceShellCoreUtilUnmountHddForRestore();
int PS4_SYSV_ABI sceShellCoreUtilWriteSmrHddDummyData();
int PS4_SYSV_ABI Func_1E5CA5A71FA7F028();
int PS4_SYSV_ABI Func_6D43644F75C38346();
int PS4_SYSV_ABI Func_739FB849CB28F445();
int PS4_SYSV_ABI Func_B20628FF35C74111();
int PS4_SYSV_ABI sceSystemServiceAddLocalProcessForJvm();
int PS4_SYSV_ABI sceSystemServiceGetParentSocketForJvm();
int PS4_SYSV_ABI sceSystemServiceKillLocalProcessForJvm();
int PS4_SYSV_ABI sceSystemServiceAcquireFb0();
int PS4_SYSV_ABI sceSystemServiceAddLocalProcess();
int PS4_SYSV_ABI sceSystemServiceAddLocalProcessForPsmKit();
int PS4_SYSV_ABI sceSystemServiceChangeAcpClock();
int PS4_SYSV_ABI sceSystemServiceChangeCpuClock();
int PS4_SYSV_ABI sceSystemServiceChangeGpuClock();
int PS4_SYSV_ABI sceSystemServiceChangeMemoryClock();
int PS4_SYSV_ABI sceSystemServiceChangeMemoryClockToBaseMode();
int PS4_SYSV_ABI sceSystemServiceChangeMemoryClockToDefault();
int PS4_SYSV_ABI sceSystemServiceChangeMemoryClockToMultiMediaMode();
int PS4_SYSV_ABI sceSystemServiceChangeNumberOfGpuCu();
int PS4_SYSV_ABI sceSystemServiceChangeSamuClock();
int PS4_SYSV_ABI sceSystemServiceChangeUvdClock();
int PS4_SYSV_ABI sceSystemServiceChangeVceClock();
int PS4_SYSV_ABI sceSystemServiceDisableMusicPlayer();
int PS4_SYSV_ABI sceSystemServiceDisablePersonalEyeToEyeDistanceSetting();
int PS4_SYSV_ABI sceSystemServiceDisableSuspendConfirmationDialog();
int PS4_SYSV_ABI sceSystemServiceEnablePersonalEyeToEyeDistanceSetting();
int PS4_SYSV_ABI sceSystemServiceEnableSuspendConfirmationDialog();
int PS4_SYSV_ABI sceSystemServiceGetAppFocusedAppStatus();
int PS4_SYSV_ABI sceSystemServiceGetAppIdOfBigApp();
int PS4_SYSV_ABI sceSystemServiceGetAppIdOfMiniApp();
int PS4_SYSV_ABI sceSystemServiceGetAppStatus();
int PS4_SYSV_ABI sceSystemServiceGetAppType();
s32 PS4_SYSV_ABI
sceSystemServiceGetDisplaySafeAreaInfo(OrbisSystemServiceDisplaySafeAreaInfo* info);
int PS4_SYSV_ABI sceSystemServiceGetEventForDaemon();
int PS4_SYSV_ABI sceSystemServiceGetGpuLoadEmulationMode();
int PS4_SYSV_ABI sceSystemServiceGetHdrToneMapLuminance();
int PS4_SYSV_ABI sceSystemServiceGetLocalProcessStatusList();
int PS4_SYSV_ABI sceSystemServiceGetParentSocket();
int PS4_SYSV_ABI sceSystemServiceGetParentSocketForPsmKit();
int PS4_SYSV_ABI sceSystemServiceGetPSButtonEvent();
int PS4_SYSV_ABI sceSystemServiceGetRenderingMode();
s32 PS4_SYSV_ABI sceSystemServiceGetStatus(OrbisSystemServiceStatus* status);
int PS4_SYSV_ABI sceSystemServiceGetTitleWorkaroundInfo();
int PS4_SYSV_ABI sceSystemServiceGetVersionNumberOfCameraCalibrationData();
s32 PS4_SYSV_ABI sceSystemServiceHideSplashScreen();
int PS4_SYSV_ABI sceSystemServiceIsAppSuspended();
int PS4_SYSV_ABI sceSystemServiceIsBgmPlaying();
int PS4_SYSV_ABI sceSystemServiceIsEyeToEyeDistanceAdjusted();
int PS4_SYSV_ABI sceSystemServiceIsScreenSaverOn();
int PS4_SYSV_ABI sceSystemServiceIsShellUiFgAndGameBgCpuMode();
int PS4_SYSV_ABI sceSystemServiceKillApp();
int PS4_SYSV_ABI sceSystemServiceKillLocalProcess();
int PS4_SYSV_ABI sceSystemServiceKillLocalProcessForPsmKit();
int PS4_SYSV_ABI sceSystemServiceLaunchApp();
int PS4_SYSV_ABI sceSystemServiceLaunchEventDetails();
int PS4_SYSV_ABI sceSystemServiceLaunchTournamentList();
int PS4_SYSV_ABI sceSystemServiceLaunchTournamentsTeamProfile();
int PS4_SYSV_ABI sceSystemServiceLaunchWebBrowser();
int PS4_SYSV_ABI sceSystemServiceLoadExec(const char* path, const char* argv[]);
int PS4_SYSV_ABI sceSystemServiceNavigateToAnotherApp();
int PS4_SYSV_ABI sceSystemServiceNavigateToGoBack();
int PS4_SYSV_ABI sceSystemServiceNavigateToGoBackWithValue();
int PS4_SYSV_ABI sceSystemServiceNavigateToGoHome();
s32 PS4_SYSV_ABI sceSystemServiceParamGetInt(OrbisSystemServiceParamId param_id, int* value);
int PS4_SYSV_ABI sceSystemServiceParamGetString();
int PS4_SYSV_ABI sceSystemServicePowerTick();
int PS4_SYSV_ABI sceSystemServiceRaiseExceptionLocalProcess();
s32 PS4_SYSV_ABI sceSystemServiceReceiveEvent(OrbisSystemServiceEvent* event);
int PS4_SYSV_ABI sceSystemServiceReenableMusicPlayer();
int PS4_SYSV_ABI sceSystemServiceRegisterDaemon();
int PS4_SYSV_ABI sceSystemServiceReleaseFb0();
int PS4_SYSV_ABI sceSystemServiceReportAbnormalTermination();
int PS4_SYSV_ABI sceSystemServiceRequestCameraCalibration();
int PS4_SYSV_ABI sceSystemServiceRequestToChangeRenderingMode();
int PS4_SYSV_ABI sceSystemServiceResumeLocalProcess();
int PS4_SYSV_ABI sceSystemServiceSetControllerFocusPermission();
int PS4_SYSV_ABI sceSystemServiceSetGpuLoadEmulationMode();
int PS4_SYSV_ABI sceSystemServiceSetOutOfVrPlayAreaFlag();
int PS4_SYSV_ABI sceSystemServiceSetOutOfVrPlayZoneWarning();
int PS4_SYSV_ABI sceSystemServiceShowControllerSettings();
int PS4_SYSV_ABI sceSystemServiceShowDisplaySafeAreaSettings();
int PS4_SYSV_ABI sceSystemServiceShowEyeToEyeDistanceSetting();
int PS4_SYSV_ABI sceSystemServiceSuspendBackgroundApp();
int PS4_SYSV_ABI sceSystemServiceSuspendLocalProcess();
int PS4_SYSV_ABI sceSystemServiceTickVideoPlayback();
int PS4_SYSV_ABI sceSystemServiceTurnOffScreenSaver();
int PS4_SYSV_ABI Func_9031A344CB540F1A();
int PS4_SYSV_ABI Func_A9D4CF2568EAB837();
int PS4_SYSV_ABI sceSystemServiceLaunchWebApp();
int PS4_SYSV_ABI Func_B8495C766861FDCF();
int PS4_SYSV_ABI sceSystemServiceGetDbgExecutablePath();
int PS4_SYSV_ABI sceSystemServiceActivateHevc();
int PS4_SYSV_ABI sceSystemServiceActivateHevcAbort();
int PS4_SYSV_ABI sceSystemServiceActivateHevcGetStatus();
int PS4_SYSV_ABI sceSystemServiceActivateHevcInit();
int PS4_SYSV_ABI sceSystemServiceActivateHevcIsActivated();
int PS4_SYSV_ABI sceSystemServiceActivateHevcStart();
int PS4_SYSV_ABI sceSystemServiceActivateHevcTerm();
int PS4_SYSV_ABI sceSystemServiceActivateMpeg2Abort();
int PS4_SYSV_ABI sceSystemServiceActivateMpeg2GetStatus();
int PS4_SYSV_ABI sceSystemServiceActivateMpeg2Init();
int PS4_SYSV_ABI sceSystemServiceActivateMpeg2IsActivated();
int PS4_SYSV_ABI sceSystemServiceActivateMpeg2Start();
int PS4_SYSV_ABI sceSystemServiceActivateMpeg2Term();
int PS4_SYSV_ABI sceSystemStateMgrCancelShutdownTimer();
int PS4_SYSV_ABI sceSystemStateMgrEnterMediaPlaybackMode();
int PS4_SYSV_ABI sceSystemStateMgrEnterStandby();
int PS4_SYSV_ABI sceSystemStateMgrExtendShutdownTimer();
int PS4_SYSV_ABI sceSystemStateMgrExtendShutdownTimerForPostAutoUpdateProcess();
int PS4_SYSV_ABI sceSystemStateMgrGetCurrentState();
int PS4_SYSV_ABI sceSystemStateMgrGetTriggerCode();
int PS4_SYSV_ABI sceSystemStateMgrIsBdDriveReady();
int PS4_SYSV_ABI sceSystemStateMgrIsGpuPerformanceNormal();
int PS4_SYSV_ABI sceSystemStateMgrIsShellUIShutdownInProgress();
int PS4_SYSV_ABI sceSystemStateMgrIsStandbyModeEnabled();
int PS4_SYSV_ABI sceSystemStateMgrLeaveMediaPlaybackMode();
int PS4_SYSV_ABI sceSystemStateMgrNotifySystemSuspendResumeProgress();
int PS4_SYSV_ABI sceSystemStateMgrReboot();
int PS4_SYSV_ABI sceSystemStateMgrSendCecOneTouchPlayCommand();
int PS4_SYSV_ABI sceSystemStateMgrStartRebootTimer();
int PS4_SYSV_ABI sceSystemStateMgrStartShutdownTimer();
int PS4_SYSV_ABI sceSystemStateMgrStartStadbyTimer();
int PS4_SYSV_ABI sceSystemStateMgrStartVshAutoUpdateTimer();
int PS4_SYSV_ABI sceSystemStateMgrTickMusicPlayback();
int PS4_SYSV_ABI sceSystemStateMgrTickPartyChat();
int PS4_SYSV_ABI sceSystemStateMgrTurnOff();
int PS4_SYSV_ABI sceSystemStateMgrVshAutoUpdate();
int PS4_SYSV_ABI sceSystemStateMgrWaitVshAutoUpdateVerifyDone();
int PS4_SYSV_ABI sceSystemStateMgrWakeUp();
int PS4_SYSV_ABI Func_89F262179C22B49E();
int PS4_SYSV_ABI Func_AC8A8FAB4A1696B8();
int PS4_SYSV_ABI sceSystemServiceInvokeAppLaunchLink();
int PS4_SYSV_ABI sceSystemServiceShowClosedCaptionAdvancedSettings();
int PS4_SYSV_ABI sceSystemServiceShowClosedCaptionSettings();
int PS4_SYSV_ABI sceSystemServiceSetPowerSaveLevel();
int PS4_SYSV_ABI sceSystemServiceInitializeForShellCore();
int PS4_SYSV_ABI Func_7C1183FC73629929();
int PS4_SYSV_ABI sceSystemServiceDisablePartyVoice();
int PS4_SYSV_ABI sceSystemServiceReenablePartyVoice();
int PS4_SYSV_ABI sceSystemServiceGetPlatformPrivacyDefinitionData();
int PS4_SYSV_ABI sceSystemServiceGetPlatformPrivacyDefinitionVersion();
int PS4_SYSV_ABI sceSystemServiceGetPlatformPrivacySetting();
int PS4_SYSV_ABI sceSystemServiceDeclareReadyForSuspend();
int PS4_SYSV_ABI sceSystemServiceDisableSuspendNotification();
int PS4_SYSV_ABI sceSystemServiceEnableSuspendNotification();
int PS4_SYSV_ABI sceSystemServiceRequestPowerOff();
int PS4_SYSV_ABI sceSystemServiceRequestReboot();
int PS4_SYSV_ABI sceSystemServiceAddLocalProcessForPs2Emu();
int PS4_SYSV_ABI sceSystemServiceGetParentSocketForPs2Emu();
int PS4_SYSV_ABI sceSystemServiceKillLocalProcessForPs2Emu();
int PS4_SYSV_ABI sceSystemServiceShowImposeMenuForPs2Emu();
int PS4_SYSV_ABI sceSystemServiceSaveVideoToken();
int PS4_SYSV_ABI sceSystemServiceLaunchStore();
int PS4_SYSV_ABI sceSystemServiceTelemetrySetData();
int PS4_SYSV_ABI Func_C67FC780F5B6F71E();
int PS4_SYSV_ABI sceSystemServiceLaunchUdsApp();
int PS4_SYSV_ABI sceSystemServiceLoadExecVideoServiceWebApp();
int PS4_SYSV_ABI sceSystemServiceDisableVoiceRecognition();
int PS4_SYSV_ABI sceSystemServiceReenableVoiceRecognition();
int PS4_SYSV_ABI Func_6B1CDB955F0EBD65();
int PS4_SYSV_ABI Func_CB5E885E225F69F0();

void PushSystemServiceEvent(const OrbisSystemServiceEvent& event);

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::SystemService
