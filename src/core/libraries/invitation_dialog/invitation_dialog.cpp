// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/invitationdialog/invitation_dialog.h"

namespace Libraries::InvitationDialog {

s32 PS4_SYSV_ABI sceInvitationDialogClose() {
    LOG_ERROR(Lib_InvitationDialog, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceInvitationDialogGetResult() {
    LOG_ERROR(Lib_InvitationDialog, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceInvitationDialogGetResultA() {
    LOG_ERROR(Lib_InvitationDialog, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceInvitationDialogGetStatus() {
    LOG_ERROR(Lib_InvitationDialog, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceInvitationDialogInitialize() {
    LOG_ERROR(Lib_InvitationDialog, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceInvitationDialogOpen() {
    LOG_ERROR(Lib_InvitationDialog, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceInvitationDialogOpenA() {
    LOG_ERROR(Lib_InvitationDialog, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceInvitationDialogTerminate() {
    LOG_ERROR(Lib_InvitationDialog, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceInvitationDialogUpdateStatus() {
    LOG_ERROR(Lib_InvitationDialog, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("WWtCL5lzi7Y", "libSceInvitationDialog", 1, "libSceInvitationDialog",
                 sceInvitationDialogClose);
    LIB_FUNCTION("8XKR6wa64iQ", "libSceInvitationDialog", 1, "libSceInvitationDialog",
                 sceInvitationDialogGetResult);
    LIB_FUNCTION("WuuUhuKOxwQ", "libSceInvitationDialog", 1, "libSceInvitationDialog",
                 sceInvitationDialogGetResultA);
    LIB_FUNCTION("EiF92YDNHRA", "libSceInvitationDialog", 1, "libSceInvitationDialog",
                 sceInvitationDialogGetStatus);
    LIB_FUNCTION("XvA5KS56wcs", "libSceInvitationDialog", 1, "libSceInvitationDialog",
                 sceInvitationDialogInitialize);
    LIB_FUNCTION("0zU0G+wiVLA", "libSceInvitationDialog", 1, "libSceInvitationDialog",
                 sceInvitationDialogOpen);
    LIB_FUNCTION("sAxbHhAWMXM", "libSceInvitationDialog", 1, "libSceInvitationDialog",
                 sceInvitationDialogOpenA);
    LIB_FUNCTION("B6HVJtDYxEE", "libSceInvitationDialog", 1, "libSceInvitationDialog",
                 sceInvitationDialogTerminate);
    LIB_FUNCTION("9+g9iOq+7kg", "libSceInvitationDialog", 1, "libSceInvitationDialog",
                 sceInvitationDialogUpdateStatus);
    LIB_FUNCTION("8XKR6wa64iQ", "libSceInvitationDialogCompat", 1, "libSceInvitationDialog",
                 sceInvitationDialogGetResult);
    LIB_FUNCTION("0zU0G+wiVLA", "libSceInvitationDialogCompat", 1, "libSceInvitationDialog",
                 sceInvitationDialogOpen);
};

} // namespace Libraries::InvitationDialog