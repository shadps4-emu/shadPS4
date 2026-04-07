// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <core/libraries/system/commondialog.h>
#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "magic_enum/magic_enum.hpp"
#include "np_profile_dialog.h"
#include "np_profile_dialog_ui.h"

namespace Libraries::Np::NpProfileDialog {

static auto g_status = Libraries::CommonDialog::Status::NONE;
static NpProfileDialogState g_state{};
// static DialogResult g_result{};
static int g_result = 0; // TODO use real result when implementing dialog
static NpProfileDialogUi g_profile_dialog_ui;

Libraries::CommonDialog::Error PS4_SYSV_ABI
sceNpProfileDialogOpen(OrbisNpProfileDialogParam* param) {
    if (g_status != Libraries::CommonDialog::Status::INITIALIZED &&
        g_status != Libraries::CommonDialog::Status::FINISHED) {
        LOG_INFO(Lib_NpProfileDialog, "called without initialize");
        return Libraries::CommonDialog::Error::INVALID_STATE;
    }
    LOG_ERROR(Lib_NpProfileDialog, "(STUBBED) called"); // TODO open ui dialog
    NpProfileDialogState state{};
    state.onlineId = std::string(param->targetOnlineId.data);
    state.userId = param->userId;
    g_state = state;
    g_status = Libraries::CommonDialog::Status::RUNNING;
    g_profile_dialog_ui = NpProfileDialogUi(&g_state, &g_result);
    return Libraries::CommonDialog::Error::OK;
}

Libraries::CommonDialog::Error PS4_SYSV_ABI sceNpProfileDialogClose() {
    LOG_DEBUG(Lib_NpProfileDialog, "called");
    if (g_status != Libraries::CommonDialog::Status::RUNNING) {
        return Libraries::CommonDialog::Error::NOT_RUNNING;
    }
    LOG_INFO(Lib_NpProfileDialog, "TODO: close npprofile ui dialog"); // TODO close Ui dialog
    return Libraries::CommonDialog::Error::OK;
}

s32 PS4_SYSV_ABI sceNpProfileDialogGetResult() {
    LOG_ERROR(Lib_NpProfileDialog, "(STUBBED) called");
    return ORBIS_OK;
}

Libraries::CommonDialog::Status PS4_SYSV_ABI sceNpProfileDialogGetStatus() {
    LOG_TRACE(Lib_NpProfileDialog, "called status={}", magic_enum::enum_name(g_status));
    return g_status;
}

Libraries::CommonDialog::Error PS4_SYSV_ABI sceNpProfileDialogInitialize() {
    if (!CommonDialog::g_isInitialized) {
        return Libraries::CommonDialog::Error::NOT_SYSTEM_INITIALIZED;
    }
    if (g_status != Libraries::CommonDialog::Status::NONE) {
        LOG_INFO(Lib_NpProfileDialog, "already initialized");
        return Libraries::CommonDialog::Error::ALREADY_INITIALIZED;
    }
    if (CommonDialog::g_isUsed) {
        return Libraries::CommonDialog::Error::BUSY;
    }
    g_status = Libraries::CommonDialog::Status::INITIALIZED;
    CommonDialog::g_isUsed = true;
    return Libraries::CommonDialog::Error::OK;
}

s32 PS4_SYSV_ABI sceNpProfileDialogOpenA(OrbisNpProfileDialogParamA* param) {
    LOG_ERROR(Lib_NpProfileDialog, "(STUBBED) called");
    return ORBIS_OK;
}

Libraries::CommonDialog::Error PS4_SYSV_ABI sceNpProfileDialogTerminate() {
    if (g_status == Libraries::CommonDialog::Status::RUNNING) {
        sceNpProfileDialogClose();
    }
    if (g_status == Libraries::CommonDialog::Status::NONE) {
        return Libraries::CommonDialog::Error::NOT_INITIALIZED;
    }
    g_status = Libraries::CommonDialog::Status::NONE;
    CommonDialog::g_isUsed = false;
    return Libraries::CommonDialog::Error::OK;
}

Libraries::CommonDialog::Status PS4_SYSV_ABI sceNpProfileDialogUpdateStatus() {
    if (g_status == Libraries::CommonDialog::Status::RUNNING) {
        g_status = Libraries::CommonDialog::Status::FINISHED; // TODO removed it when implementing
                                                              // real dialog
    }
    LOG_TRACE(Lib_NpProfileDialog, "called status={}", magic_enum::enum_name(g_status));
    return g_status;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("uj9Cz7Tk0cc", "libSceNpProfileDialogCompat", 1, "libSceNpProfileDialog",
                 sceNpProfileDialogOpen);
    LIB_FUNCTION("wkwjz0Xdo2A", "libSceNpProfileDialog", 1, "libSceNpProfileDialog",
                 sceNpProfileDialogClose);
    LIB_FUNCTION("8rhLl1-0W-o", "libSceNpProfileDialog", 1, "libSceNpProfileDialog",
                 sceNpProfileDialogGetResult);
    LIB_FUNCTION("3BqoiFOjSsk", "libSceNpProfileDialog", 1, "libSceNpProfileDialog",
                 sceNpProfileDialogGetStatus);
    LIB_FUNCTION("Lg+NCE6pTwQ", "libSceNpProfileDialog", 1, "libSceNpProfileDialog",
                 sceNpProfileDialogInitialize);
    LIB_FUNCTION("uj9Cz7Tk0cc", "libSceNpProfileDialog", 1, "libSceNpProfileDialog",
                 sceNpProfileDialogOpen);
    LIB_FUNCTION("nrQRlLKzdwE", "libSceNpProfileDialog", 1, "libSceNpProfileDialog",
                 sceNpProfileDialogOpenA);
    LIB_FUNCTION("0Sp9vJcB1-w", "libSceNpProfileDialog", 1, "libSceNpProfileDialog",
                 sceNpProfileDialogTerminate);
    LIB_FUNCTION("haVZE9FgKqE", "libSceNpProfileDialog", 1, "libSceNpProfileDialog",
                 sceNpProfileDialogUpdateStatus);
};

} // namespace Libraries::Np::NpProfileDialog