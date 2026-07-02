// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/invitation_dialog/invitation_dialog.h"
#include "core/libraries/libs.h"
#include "magic_enum/magic_enum.hpp"

namespace Libraries::InvitationDialog {

static auto g_status = Libraries::CommonDialog::Status::NONE;

Libraries::CommonDialog::Error PS4_SYSV_ABI sceInvitationDialogClose() {
    LOG_ERROR(Lib_InvitationDialog, "(STUBBED) called");
    if (g_status != Libraries::CommonDialog::Status::RUNNING) {
        return Libraries::CommonDialog::Error::NOT_RUNNING;
    }
    LOG_INFO(Lib_InvitationDialog, "TODO: close invitation ui dialog");
    return Libraries::CommonDialog::Error::OK;
}

s32 PS4_SYSV_ABI sceInvitationDialogGetResult(OrbisInvitationDialogResult* result) {
    LOG_ERROR(Lib_InvitationDialog, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceInvitationDialogGetResultA(OrbisInvitationDialogResultA* result) {
    LOG_ERROR(Lib_InvitationDialog, "(STUBBED) called");
    return ORBIS_OK;
}

Libraries::CommonDialog::Status PS4_SYSV_ABI sceInvitationDialogGetStatus() {
    LOG_INFO(Lib_InvitationDialog, "called status={}", magic_enum::enum_name(g_status));
    return g_status;
}

Libraries::CommonDialog::Error PS4_SYSV_ABI sceInvitationDialogInitialize() {
    if (!CommonDialog::g_isInitialized) {
        return Libraries::CommonDialog::Error::NOT_SYSTEM_INITIALIZED;
    }
    if (g_status != Libraries::CommonDialog::Status::NONE) {
        LOG_ERROR(Lib_InvitationDialog, "already initialized");
        return Libraries::CommonDialog::Error::ALREADY_INITIALIZED;
    }
    if (CommonDialog::g_isUsed) {
        return Libraries::CommonDialog::Error::BUSY;
    }
    g_status = Libraries::CommonDialog::Status::INITIALIZED;
    CommonDialog::g_isUsed = true;
    return Libraries::CommonDialog::Error::OK;
}

Libraries::CommonDialog::Error PS4_SYSV_ABI
sceInvitationDialogOpen(const OrbisInvitationDialogParam* param) {
    if (g_status != Libraries::CommonDialog::Status::INITIALIZED &&
        g_status != Libraries::CommonDialog::Status::FINISHED) {
        LOG_INFO(Lib_InvitationDialog, "called without initialize");
        return Libraries::CommonDialog::Error::INVALID_STATE;
    }
    LOG_ERROR(Lib_InvitationDialog, "(STUBBED) called"); // TODO open ui dialog
    g_status = Libraries::CommonDialog::Status::RUNNING;
    return Libraries::CommonDialog::Error::OK;
}

Libraries::CommonDialog::Error PS4_SYSV_ABI
sceInvitationDialogOpenA(const OrbisInvitationDialogParamA* param) {
    if (g_status != Libraries::CommonDialog::Status::INITIALIZED &&
        g_status != Libraries::CommonDialog::Status::FINISHED) {
        LOG_INFO(Lib_InvitationDialog, "called without initialize");
        return Libraries::CommonDialog::Error::INVALID_STATE;
    }
    LOG_ERROR(Lib_InvitationDialog, "(STUBBED) called"); // TODO open ui dialog
    g_status = Libraries::CommonDialog::Status::RUNNING;
    return Libraries::CommonDialog::Error::OK;
}

Libraries::CommonDialog::Error PS4_SYSV_ABI sceInvitationDialogTerminate() {
    if (g_status == Libraries::CommonDialog::Status::RUNNING) {
        sceInvitationDialogClose();
    }
    if (g_status == Libraries::CommonDialog::Status::NONE) {
        return Libraries::CommonDialog::Error::NOT_INITIALIZED;
    }
    g_status = Libraries::CommonDialog::Status::NONE;
    CommonDialog::g_isUsed = false;
    return Libraries::CommonDialog::Error::OK;
}

Libraries::CommonDialog::Status PS4_SYSV_ABI sceInvitationDialogUpdateStatus() {
    LOG_TRACE(Lib_InvitationDialog, "called status={}", magic_enum::enum_name(g_status));
    if (g_status == Libraries::CommonDialog::Status::RUNNING) {
        g_status = Libraries::CommonDialog::Status::FINISHED; // TODO removed it when implementing
                                                              // real dialog
    }
    return g_status;
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