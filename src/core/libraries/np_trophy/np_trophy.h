// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::NpTrophy {

int PS4_SYSV_ABI sceNpTrophyAbortHandle();
int PS4_SYSV_ABI sceNpTrophyCaptureScreenshot();
int PS4_SYSV_ABI sceNpTrophyConfigGetTrophyDetails();
int PS4_SYSV_ABI sceNpTrophyConfigGetTrophyFlagArray();
int PS4_SYSV_ABI sceNpTrophyConfigGetTrophyGroupArray();
int PS4_SYSV_ABI sceNpTrophyConfigGetTrophyGroupDetails();
int PS4_SYSV_ABI sceNpTrophyConfigGetTrophySetInfo();
int PS4_SYSV_ABI sceNpTrophyConfigGetTrophySetInfoInGroup();
int PS4_SYSV_ABI sceNpTrophyConfigGetTrophySetVersion();
int PS4_SYSV_ABI sceNpTrophyConfigGetTrophyTitleDetails();
int PS4_SYSV_ABI sceNpTrophyConfigHasGroupFeature();
int PS4_SYSV_ABI sceNpTrophyCreateContext();
int PS4_SYSV_ABI sceNpTrophyCreateHandle();
int PS4_SYSV_ABI sceNpTrophyDestroyContext();
int PS4_SYSV_ABI sceNpTrophyDestroyHandle();
int PS4_SYSV_ABI sceNpTrophyGetGameIcon();
int PS4_SYSV_ABI sceNpTrophyGetGameInfo();
int PS4_SYSV_ABI sceNpTrophyGetGroupIcon();
int PS4_SYSV_ABI sceNpTrophyGetGroupInfo();
int PS4_SYSV_ABI sceNpTrophyGetTrophyIcon();
int PS4_SYSV_ABI sceNpTrophyGetTrophyInfo();
int PS4_SYSV_ABI sceNpTrophyGetTrophyUnlockState();
int PS4_SYSV_ABI sceNpTrophyGroupArrayGetNum();
int PS4_SYSV_ABI sceNpTrophyIntAbortHandle();
int PS4_SYSV_ABI sceNpTrophyIntCheckNetSyncTitles();
int PS4_SYSV_ABI sceNpTrophyIntCreateHandle();
int PS4_SYSV_ABI sceNpTrophyIntDestroyHandle();
int PS4_SYSV_ABI sceNpTrophyIntGetLocalTrophySummary();
int PS4_SYSV_ABI sceNpTrophyIntGetProgress();
int PS4_SYSV_ABI sceNpTrophyIntGetRunningTitle();
int PS4_SYSV_ABI sceNpTrophyIntGetRunningTitles();
int PS4_SYSV_ABI sceNpTrophyIntGetTrpIconByUri();
int PS4_SYSV_ABI sceNpTrophyIntNetSyncTitle();
int PS4_SYSV_ABI sceNpTrophyIntNetSyncTitles();
int PS4_SYSV_ABI sceNpTrophyNumInfoGetTotal();
int PS4_SYSV_ABI sceNpTrophyRegisterContext();
int PS4_SYSV_ABI sceNpTrophySetInfoGetTrophyFlagArray();
int PS4_SYSV_ABI sceNpTrophySetInfoGetTrophyNum();
int PS4_SYSV_ABI sceNpTrophyShowTrophyList();
int PS4_SYSV_ABI sceNpTrophySystemAbortHandle();
int PS4_SYSV_ABI sceNpTrophySystemBuildGroupIconUri();
int PS4_SYSV_ABI sceNpTrophySystemBuildNetTrophyIconUri();
int PS4_SYSV_ABI sceNpTrophySystemBuildTitleIconUri();
int PS4_SYSV_ABI sceNpTrophySystemBuildTrophyIconUri();
int PS4_SYSV_ABI sceNpTrophySystemCheckNetSyncTitles();
int PS4_SYSV_ABI sceNpTrophySystemCheckRecoveryRequired();
int PS4_SYSV_ABI sceNpTrophySystemCloseStorage();
int PS4_SYSV_ABI sceNpTrophySystemCreateContext();
int PS4_SYSV_ABI sceNpTrophySystemCreateHandle();
int PS4_SYSV_ABI sceNpTrophySystemDbgCtl();
int PS4_SYSV_ABI sceNpTrophySystemDebugLockTrophy();
int PS4_SYSV_ABI sceNpTrophySystemDebugUnlockTrophy();
int PS4_SYSV_ABI sceNpTrophySystemDestroyContext();
int PS4_SYSV_ABI sceNpTrophySystemDestroyHandle();
int PS4_SYSV_ABI sceNpTrophySystemDestroyTrophyConfig();
int PS4_SYSV_ABI sceNpTrophySystemGetDbgParam();
int PS4_SYSV_ABI sceNpTrophySystemGetDbgParamInt();
int PS4_SYSV_ABI sceNpTrophySystemGetGroupIcon();
int PS4_SYSV_ABI sceNpTrophySystemGetLocalTrophySummary();
int PS4_SYSV_ABI sceNpTrophySystemGetNextTitleFileEntryStatus();
int PS4_SYSV_ABI sceNpTrophySystemGetProgress();
int PS4_SYSV_ABI sceNpTrophySystemGetTitleFileStatus();
int PS4_SYSV_ABI sceNpTrophySystemGetTitleIcon();
int PS4_SYSV_ABI sceNpTrophySystemGetTitleSyncStatus();
int PS4_SYSV_ABI sceNpTrophySystemGetTrophyConfig();
int PS4_SYSV_ABI sceNpTrophySystemGetTrophyData();
int PS4_SYSV_ABI sceNpTrophySystemGetTrophyGroupData();
int PS4_SYSV_ABI sceNpTrophySystemGetTrophyIcon();
int PS4_SYSV_ABI sceNpTrophySystemGetTrophyTitleData();
int PS4_SYSV_ABI sceNpTrophySystemGetTrophyTitleIds();
int PS4_SYSV_ABI sceNpTrophySystemGetUserFileInfo();
int PS4_SYSV_ABI sceNpTrophySystemGetUserFileStatus();
int PS4_SYSV_ABI sceNpTrophySystemIsServerAvailable();
int PS4_SYSV_ABI sceNpTrophySystemNetSyncTitle();
int PS4_SYSV_ABI sceNpTrophySystemNetSyncTitles();
int PS4_SYSV_ABI sceNpTrophySystemOpenStorage();
int PS4_SYSV_ABI sceNpTrophySystemPerformRecovery();
int PS4_SYSV_ABI sceNpTrophySystemRemoveAll();
int PS4_SYSV_ABI sceNpTrophySystemRemoveTitleData();
int PS4_SYSV_ABI sceNpTrophySystemRemoveUserData();
int PS4_SYSV_ABI sceNpTrophySystemSetDbgParam();
int PS4_SYSV_ABI sceNpTrophySystemSetDbgParamInt();
int PS4_SYSV_ABI sceNpTrophyUnlockTrophy();
int PS4_SYSV_ABI Func_149656DA81D41C59();
int PS4_SYSV_ABI Func_9F80071876FFA5F6();
int PS4_SYSV_ABI Func_F8EF6F5350A91990();
int PS4_SYSV_ABI Func_FA7A2DD770447552();

void RegisterlibSceNpTrophy(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::NpTrophy