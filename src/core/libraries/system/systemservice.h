// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later
// reference
// https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain/blob/master/include/orbis/_types/sys_service.h
#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::SystemService {

enum OrbisSystemServiceParamId {
    ORBIS_SYSTEM_SERVICE_PARAM_ID_LANG = 1,
    ORBIS_SYSTEM_SERVICE_PARAM_ID_DATE_FORMAT = 2,
    ORBIS_SYSTEM_SERVICE_PARAM_ID_TIME_FORMAT = 3,
    ORBIS_SYSTEM_SERVICE_PARAM_ID_TIME_ZONE = 4,
    ORBIS_SYSTEM_SERVICE_PARAM_ID_SUMMERTIME = 5,
    ORBIS_SYSTEM_SERVICE_PARAM_ID_SYSTEM_NAME = 6,
    ORBIS_SYSTEM_SERVICE_PARAM_ID_GAME_PARENTAL_LEVEL = 7,
    ORBIS_SYSTEM_SERVICE_PARAM_ID_ENTER_BUTTON_ASSIGN = 1000
};

enum OrbisSystemParamDateFormat {
    ORBIS_SYSTEM_PARAM_DATE_FORMAT_YYYYMMDD = 0,
    ORBIS_SYSTEM_PARAM_DATE_FORMAT_DDMMYYYY = 1,
    ORBIS_SYSTEM_PARAM_DATE_FORMAT_MMDDYYYY = 2
};

enum OrbisSystemParamTimeFormat {
    ORBIS_SYSTEM_PARAM_TIME_FORMAT_12HOUR = 0,
    ORBIS_SYSTEM_PARAM_TIME_FORMAT_24HOUR = 1
};

enum OrbisSystemParamGameParentalLevel {
    ORBIS_SYSTEM_PARAM_GAME_PARENTAL_OFF = 0,
    ORBIS_SYSTEM_PARAM_GAME_PARENTAL_LEVEL01 = 1,
    ORBIS_SYSTEM_PARAM_GAME_PARENTAL_LEVEL02 = 2,
    ORBIS_SYSTEM_PARAM_GAME_PARENTAL_LEVEL03 = 3,
    ORBIS_SYSTEM_PARAM_GAME_PARENTAL_LEVEL04 = 4,
    ORBIS_SYSTEM_PARAM_GAME_PARENTAL_LEVEL05 = 5,
    ORBIS_SYSTEM_PARAM_GAME_PARENTAL_LEVEL06 = 6,
    ORBIS_SYSTEM_PARAM_GAME_PARENTAL_LEVEL07 = 7,
    ORBIS_SYSTEM_PARAM_GAME_PARENTAL_LEVEL08 = 8,
    ORBIS_SYSTEM_PARAM_GAME_PARENTAL_LEVEL09 = 9,
    ORBIS_SYSTEM_PARAM_GAME_PARENTAL_LEVEL10 = 10,
    ORBIS_SYSTEM_PARAM_GAME_PARENTAL_LEVEL11 = 11
};

enum OrbisSystemParamEnterButtonAssign {
    ORBIS_SYSTEM_PARAM_ENTER_BUTTON_ASSIGN_CIRCLE = 0,
    ORBIS_SYSTEM_PARAM_ENTER_BUTTON_ASSIGN_CROSS = 1
};

enum OrbisSystemParamLanguage {
    ORBIS_SYSTEM_PARAM_LANG_JAPANESE = 0,
    ORBIS_SYSTEM_PARAM_LANG_ENGLISH_US = 1,
    ORBIS_SYSTEM_PARAM_LANG_FRENCH = 2,
    ORBIS_SYSTEM_PARAM_LANG_SPANISH = 3,
    ORBIS_SYSTEM_PARAM_LANG_GERMAN = 4,
    ORBIS_SYSTEM_PARAM_LANG_ITALIAN = 5,
    ORBIS_SYSTEM_PARAM_LANG_DUTCH = 6,
    ORBIS_SYSTEM_PARAM_LANG_PORTUGUESE_PT = 7,
    ORBIS_SYSTEM_PARAM_LANG_RUSSIAN = 8,
    ORBIS_SYSTEM_PARAM_LANG_KOREAN = 9,
    ORBIS_SYSTEM_PARAM_LANG_CHINESE_T = 10,
    ORBIS_SYSTEM_PARAM_LANG_CHINESE_S = 11,
    ORBIS_SYSTEM_PARAM_LANG_FINNISH = 12,
    ORBIS_SYSTEM_PARAM_LANG_SWEDISH = 13,
    ORBIS_SYSTEM_PARAM_LANG_DANISH = 14,
    ORBIS_SYSTEM_PARAM_LANG_NORWEGIAN = 15,
    ORBIS_SYSTEM_PARAM_LANG_POLISH = 16,
    ORBIS_SYSTEM_PARAM_LANG_PORTUGUESE_BR = 17,
    ORBIS_SYSTEM_PARAM_LANG_ENGLISH_GB = 18,
    ORBIS_SYSTEM_PARAM_LANG_TURKISH = 19,
    ORBIS_SYSTEM_PARAM_LANG_SPANISH_LA = 20,
    ORBIS_SYSTEM_PARAM_LANG_ARABIC = 21,
    ORBIS_SYSTEM_PARAM_LANG_FRENCH_CA = 22,
    ORBIS_SYSTEM_PARAM_LANG_CZECH = 23,
    ORBIS_SYSTEM_PARAM_LANG_HUNGARIAN = 24,
    ORBIS_SYSTEM_PARAM_LANG_GREEK = 25,
    ORBIS_SYSTEM_PARAM_LANG_ROMANIAN = 26,
    ORBIS_SYSTEM_PARAM_LANG_THAI = 27,
    ORBIS_SYSTEM_PARAM_LANG_VIETNAMESE = 28,
    ORBIS_SYSTEM_PARAM_LANG_INDONESIAN = 29
};

enum OrbisSystemServiceEventType {
    ORBIS_SYSTEM_SERVICE_EVENT_INVALID = -1,
    ORBIS_SYSTEM_SERVICE_EVENT_ON_RESUME = 0x10000000,
    ORBIS_SYSTEM_SERVICE_EVENT_GAME_LIVE_STREAMING_STATUS_UPDATE = 0x10000001,
    ORBIS_SYSTEM_SERVICE_EVENT_SESSION_INVITATION = 0x10000002,
    ORBIS_SYSTEM_SERVICE_EVENT_ENTITLEMENT_UPDATE = 0x10000003,
    ORBIS_SYSTEM_SERVICE_EVENT_GAME_CUSTOM_DATA = 0x10000004,
    ORBIS_SYSTEM_SERVICE_EVENT_DISPLAY_SAFE_AREA_UPDATE = 0x10000005,
    ORBIS_SYSTEM_SERVICE_EVENT_URL_OPEN = 0x10000006,
    ORBIS_SYSTEM_SERVICE_EVENT_LAUNCH_APP = 0x10000007,
    ORBIS_SYSTEM_SERVICE_EVENT_APP_LAUNCH_LINK = 0x10000008,
    ORBIS_SYSTEM_SERVICE_EVENT_ADDCONTENT_INSTALL = 0x10000009,
    ORBIS_SYSTEM_SERVICE_EVENT_RESET_VR_POSITION = 0x1000000a,
    ORBIS_SYSTEM_SERVICE_EVENT_JOIN_EVENT = 0x1000000b,
    ORBIS_SYSTEM_SERVICE_EVENT_PLAYGO_LOCUS_UPDATE = 0x1000000c,
    ORBIS_SYSTEM_SERVICE_EVENT_PLAY_TOGETHER_HOST = 0x1000000d,
    ORBIS_SYSTEM_SERVICE_EVENT_SERVICE_ENTITLEMENT_UPDATE = 0x1000000e,
    ORBIS_SYSTEM_SERVICE_EVENT_EYE_TO_EYE_DISTANCE_UPDATE = 0x1000000f,
    ORBIS_SYSTEM_SERVICE_EVENT_JOIN_MATCH_EVENT = 0x10000010,
    ORBIS_SYSTEM_SERVICE_EVENT_PLAY_TOGETHER_HOST_A = 0x10000011,
    ORBIS_SYSTEM_SERVICE_EVENT_WEBBROWSER_CLOSED = 0x10000012,
    ORBIS_SYSTEM_SERVICE_EVENT_CONTROLLER_SETTINGS_CLOSED = 0x10000013,
    ORBIS_SYSTEM_SERVICE_EVENT_JOIN_TEAM_ON_TEAM_MATCH_EVENT = 0x10000014,
    ORBIS_SYSTEM_SERVICE_EVENT_OPEN_SHARE_MENU = 0x30000000
};

struct OrbisSystemServiceStatus {
    s32 eventNum;
    bool isSystemUiOverlaid;
    bool isInBackgroundExecution;
    bool isCpuMode7CpuNormal;
    bool isGameLiveStreamingOnAir;
    bool isOutOfVrPlayArea;
    u8 reserved[];
};

struct OrbisSystemServiceDisplaySafeAreaInfo {
    float ratio;
    uint8_t reserved[128];
};

struct OrbisSystemServiceEvent {
    OrbisSystemServiceEventType eventType;
    union {
        char param[8192];
        struct {
            char source[1024];
            char url[4096];
        } urlOpen;
        struct {
            u32 size;
            u8 arg[8188];
        } launchApp;
        struct {
            u32 size;
            u8 arg[2020];
        } appLaunchLink;
        struct {
            s32 userId;
            char eventId[37];
            char bootArgument[7169];
        } joinEvent;
        struct {
            s32 userId;
            u32 npServiceLabel;
            u8 reserved[8184];
        } serviceEntitlementUpdate;
        struct {
            s32 userId;
            u32 npServiceLabel;
            u8 reserved[8184];
        } unifiedEntitlementUpdate;
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
int PS4_SYSV_ABI sceSystemServiceLoadExec();
int PS4_SYSV_ABI sceSystemServiceNavigateToAnotherApp();
int PS4_SYSV_ABI sceSystemServiceNavigateToGoBack();
int PS4_SYSV_ABI sceSystemServiceNavigateToGoBackWithValue();
int PS4_SYSV_ABI sceSystemServiceNavigateToGoHome();
s32 PS4_SYSV_ABI sceSystemServiceParamGetInt(int param_id, int* value);
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

void RegisterlibSceSystemService(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::SystemService
