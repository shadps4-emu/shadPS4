// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <core/libraries/system/commondialog.h>
#include <core/libraries/system/userservice.h>
#include "common/types.h"
#include "core/libraries/np/np_types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Np::NpProfileDialog {

enum class OrbisNpProfileDialogMode : u32 {
    ORBIS_NP_PROFILE_DIALOG_MODE_INVALID = 0,
    ORBIS_NP_PROFILE_DIALOG_MODE_NORMAL = 1,
    ORBIS_NP_PROFILE_DIALOG_MODE_FRIEND_REQUEST = 2,
    ORBIS_NP_PROFILE_DIALOG_MODE_ADD_TO_BLOCK_LIST = 3,
    ORBIS_NP_PROFILE_DIALOG_MODE_GRIEF_REPORT = 4,
};

static constexpr s32 ORBIS_NP_PROFILE_DIALOG_MENU_GRIEF_REPORT_ITEM_INVALID = 0x00000000;
static constexpr s32 ORBIS_NP_PROFILE_DIALOG_MENU_GRIEF_REPORT_ITEM_ONLINE_ID = 0x00000001;
static constexpr s32 ORBIS_NP_PROFILE_DIALOG_MENU_GRIEF_REPORT_ITEM_NAME = 0x00000002;
static constexpr s32 ORBIS_NP_PROFILE_DIALOG_MENU_GRIEF_REPORT_ITEM_PICTURE = 0x00000004;
static constexpr s32 ORBIS_NP_PROFILE_DIALOG_MENU_GRIEF_REPORT_ITEM_ABOUT_ME = 0x00000008;

struct OrbisNpProfileDialogParam {
    CommonDialog::BaseParam baseParam;
    u64 size;
    OrbisNpProfileDialogMode mode;
    Libraries::UserService::OrbisUserServiceUserId userId;
    Libraries::Np::OrbisNpOnlineId targetOnlineId;
    void* userData;
    u8 reserved[32];
};

struct OrbisNpProfileGriefReportParam {
    s32 reportItem;
    u8 reserved[28];
};

struct OrbisNpProfileDialogParamA {
    CommonDialog::BaseParam baseParam;
    u64 size;
    OrbisNpProfileDialogMode mode;
    Libraries::UserService::OrbisUserServiceUserId userId;
    Libraries::Np::OrbisNpAccountId targetAccountId;
    int : 32;
    void* userData;
    union {
        uint8_t reserved[32];
        OrbisNpProfileGriefReportParam griefReportParam;
    };
};

struct OrbisNpProfileDialogResult {
    s32 result;
    CommonDialog::Result userAction;
    void* userData;
    u8 reserved[32];
};

Libraries::CommonDialog::Error PS4_SYSV_ABI
sceNpProfileDialogOpen(OrbisNpProfileDialogParam* param);
Libraries::CommonDialog::Error PS4_SYSV_ABI sceNpProfileDialogClose();
Libraries::CommonDialog::Error PS4_SYSV_ABI
sceNpProfileDialogGetResult(OrbisNpProfileDialogResult* result);
Libraries::CommonDialog::Status PS4_SYSV_ABI sceNpProfileDialogGetStatus();
Libraries::CommonDialog::Error PS4_SYSV_ABI sceNpProfileDialogInitialize();
Libraries::CommonDialog::Error PS4_SYSV_ABI
sceNpProfileDialogOpenA(OrbisNpProfileDialogParamA* param);
Libraries::CommonDialog::Error PS4_SYSV_ABI sceNpProfileDialogTerminate();
Libraries::CommonDialog::Status PS4_SYSV_ABI sceNpProfileDialogUpdateStatus();

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Np::NpProfileDialog
