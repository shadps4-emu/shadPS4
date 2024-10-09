// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::SharePlay {

int PS4_SYSV_ABI sceSharePlayCrashDaemon();
int PS4_SYSV_ABI sceSharePlayGetCurrentConnectionInfo();
int PS4_SYSV_ABI sceSharePlayGetCurrentConnectionInfoA();
int PS4_SYSV_ABI sceSharePlayGetCurrentInfo();
int PS4_SYSV_ABI sceSharePlayGetEvent();
int PS4_SYSV_ABI sceSharePlayInitialize();
int PS4_SYSV_ABI sceSharePlayNotifyDialogOpen();
int PS4_SYSV_ABI sceSharePlayNotifyForceCloseForCdlg();
int PS4_SYSV_ABI sceSharePlayNotifyOpenQuickMenu();
int PS4_SYSV_ABI sceSharePlayResumeScreenForCdlg();
int PS4_SYSV_ABI sceSharePlayServerLock();
int PS4_SYSV_ABI sceSharePlayServerUnLock();
int PS4_SYSV_ABI sceSharePlaySetMode();
int PS4_SYSV_ABI sceSharePlaySetProhibition();
int PS4_SYSV_ABI sceSharePlaySetProhibitionModeWithAppId();
int PS4_SYSV_ABI sceSharePlayStartStandby();
int PS4_SYSV_ABI sceSharePlayStartStreaming();
int PS4_SYSV_ABI sceSharePlayStopStandby();
int PS4_SYSV_ABI sceSharePlayStopStreaming();
int PS4_SYSV_ABI sceSharePlayTerminate();
int PS4_SYSV_ABI Func_2E93C0EA6A6B67C4();
int PS4_SYSV_ABI Func_C1C236728D88E177();
int PS4_SYSV_ABI Func_E9E80C474781F115();
int PS4_SYSV_ABI Func_F3DD6199DA15ED44();

void RegisterlibSceSharePlay(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::SharePlay