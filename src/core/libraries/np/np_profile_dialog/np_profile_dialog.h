// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <core/libraries/system/commondialog.h>
#include <core/libraries/system/userservice.h>
#include "common/types.h"
#include "core/libraries/np/np_profile_dialog/np_profile_dialog_mode.h"
#include "core/libraries/np/np_profile_dialog/np_profile_dialog_result.h"
#include "core/libraries/np/np_profile_dialog/np_profile_dialog_ui.h"
#include "core/libraries/np/np_types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Np::NpProfileDialog {

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

struct Library {
    Library(Core::Loader::SymbolsResolver* sym);

    Libraries::CommonDialog::Status g_status = Libraries::CommonDialog::Status::NONE;
    NpProfileDialogState g_state{};
    OrbisNpProfileDialogResult g_result{};
    NpProfileDialogUi g_profile_dialog_ui;
};
} // namespace Libraries::Np::NpProfileDialog
