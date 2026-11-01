// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <core/libraries/system/userservice.h>
#include "common/types.h"
#include "core/libraries/np/np_types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::SharePlay {

constexpr s32 ORBIS_SHARE_PLAY_ERROR_INVALID_ARGUMENT = static_cast<s32>(0x810e0001);
constexpr s32 ORBIS_SHARE_PLAY_ERROR_NOT_INITIALIZED = static_cast<s32>(0x810e0004);

constexpr int ORBIS_SHARE_PLAY_CONNECTION_STATUS_DORMANT = 0x00;
constexpr int ORBIS_SHARE_PLAY_CONNECTION_STATUS_READY = 0x01;
constexpr int ORBIS_SHARE_PLAY_CONNECTION_STATUS_CONNECTED = 0x02;

struct OrbisSharePlayConnectionInfo {
    int status;
    int mode;
    char hostOnlineId[Libraries::Np::ORBIS_NP_ONLINEID_MAX_LENGTH];
    Libraries::UserService::OrbisUserServiceUserId hostUserId;
    char visitorOnlineId[Libraries::Np::ORBIS_NP_ONLINEID_MAX_LENGTH];
    Libraries::UserService::OrbisUserServiceUserId visitorUserId;
    int reserved[2];
};
static_assert(sizeof(OrbisSharePlayConnectionInfo) == 0x38);

struct OrbisSharePlayConnectionInfoA {
    int status;
    int mode;
    char hostOnlineId[Libraries::Np::ORBIS_NP_ONLINEID_MAX_LENGTH];
    Libraries::UserService::OrbisUserServiceUserId hostUserId;
    char visitorOnlineId[Libraries::Np::ORBIS_NP_ONLINEID_MAX_LENGTH];
    Libraries::UserService::OrbisUserServiceUserId visitorUserId;
    u8 reserved[24];
};
static_assert(sizeof(OrbisSharePlayConnectionInfoA) == 0x48);

int PS4_SYSV_ABI sceSharePlayCrashDaemon();
int PS4_SYSV_ABI sceSharePlayGetCurrentConnectionInfo(OrbisSharePlayConnectionInfo* pInfo);
int PS4_SYSV_ABI sceSharePlayGetCurrentConnectionInfoA(OrbisSharePlayConnectionInfoA* pInfo);
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

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::SharePlay