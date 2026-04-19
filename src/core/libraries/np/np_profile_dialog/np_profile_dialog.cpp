// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>
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
static OrbisNpProfileDialogResult g_result{};
static NpProfileDialogUi g_profile_dialog_ui;

Libraries::CommonDialog::Error sceNpProfileDialogOpen(OrbisNpProfileDialogParam* param) {
    if (g_status != Libraries::CommonDialog::Status::INITIALIZED &&
        g_status != Libraries::CommonDialog::Status::FINISHED) {
        LOG_ERROR(Lib_NpProfileDialog, "called without initialize");
        return Libraries::CommonDialog::Error::INVALID_STATE;
    }
    if (param == nullptr) {
        LOG_ERROR(Lib_NpProfileDialog, "param is nullptr");
        return Libraries::CommonDialog::Error::ARG_NULL;
    }

    LOG_DEBUG(Lib_NpProfileDialog,
              "param details: mode={}, userId={}, targetOnlineId='{}', userData={}",
              static_cast<int>(param->mode), param->userId, param->targetOnlineId.data,
              fmt::ptr(param->userData));

    NpProfileDialogState state{};
    state.onlineId = std::string(param->targetOnlineId.data);
    state.hasAccountId = false;
    state.mode = param->mode;
    state.userId = param->userId;
    g_state = state;
    g_result = {};
    g_result.userData = param->userData;
    g_status = Libraries::CommonDialog::Status::RUNNING;

    LOG_DEBUG(Lib_NpProfileDialog, "creating UI with onlineId='{}', mode={}, userId={}",
              state.onlineId, static_cast<int>(state.mode), state.userId);

    g_profile_dialog_ui = NpProfileDialogUi(&g_state, &g_status, &g_result);

    LOG_INFO(Lib_NpProfileDialog, "dialog opened successfully for user {}", param->userId);
    return Libraries::CommonDialog::Error::OK;
}

Libraries::CommonDialog::Error PS4_SYSV_ABI sceNpProfileDialogClose() {
    LOG_DEBUG(Lib_NpProfileDialog, "called");
    if (g_status == Libraries::CommonDialog::Status::NONE) {
        return Libraries::CommonDialog::Error::NOT_INITIALIZED;
    }
    if (g_status != Libraries::CommonDialog::Status::RUNNING) {
        return Libraries::CommonDialog::Error::NOT_RUNNING;
    }
    g_profile_dialog_ui.Finish(Libraries::CommonDialog::Result::OK);
    return Libraries::CommonDialog::Error::OK;
}

Libraries::CommonDialog::Error PS4_SYSV_ABI
sceNpProfileDialogGetResult(OrbisNpProfileDialogResult* result) {
    LOG_DEBUG(Lib_NpProfileDialog, "called");

    if (g_status == Libraries::CommonDialog::Status::NONE) {
        LOG_ERROR(Lib_NpProfileDialog, "failed: not initialized (g_status=NONE)");
        return Libraries::CommonDialog::Error::NOT_INITIALIZED;
    }
    if (result == nullptr) {
        LOG_ERROR(Lib_NpProfileDialog, "failed: result pointer is nullptr");
        return Libraries::CommonDialog::Error::ARG_NULL;
    }
    if (g_status != Libraries::CommonDialog::Status::FINISHED) {
        LOG_ERROR(Lib_NpProfileDialog, "failed: dialog not finished (g_status={})",
                  static_cast<int>(g_status));
        return Libraries::CommonDialog::Error::NOT_FINISHED;
    }

    *result = g_result;

    LOG_DEBUG(Lib_NpProfileDialog, "result details: resultCode={}, userAction={}, userData={}",
              g_result.result, static_cast<int>(g_result.userAction), fmt::ptr(g_result.userData));
    return Libraries::CommonDialog::Error::OK;
}

Libraries::CommonDialog::Status PS4_SYSV_ABI sceNpProfileDialogGetStatus() {
    LOG_TRACE(Lib_NpProfileDialog, "called status={}", magic_enum::enum_name(g_status));
    return g_status;
}

Libraries::CommonDialog::Error PS4_SYSV_ABI sceNpProfileDialogInitialize() {
    if (!CommonDialog::g_isInitialized) {
        LOG_ERROR(Lib_NpProfileDialog, "failed: system not initialized");
        return Libraries::CommonDialog::Error::NOT_SYSTEM_INITIALIZED;
    }
    if (g_status != Libraries::CommonDialog::Status::NONE) {
        LOG_ERROR(Lib_NpProfileDialog, "failed: already initialized (g_status={})",
                  static_cast<int>(g_status));
        return Libraries::CommonDialog::Error::ALREADY_INITIALIZED;
    }
    if (CommonDialog::g_isUsed) {
        LOG_ERROR(Lib_NpProfileDialog, "failed: dialog system busy (g_isUsed=true)");
        return Libraries::CommonDialog::Error::BUSY;
    }

    g_status = Libraries::CommonDialog::Status::INITIALIZED;
    CommonDialog::g_isUsed = true;
    LOG_INFO(Lib_NpProfileDialog, "initialized successfully");
    return Libraries::CommonDialog::Error::OK;
}

Libraries::CommonDialog::Error PS4_SYSV_ABI
sceNpProfileDialogOpenA(OrbisNpProfileDialogParamA* param) {
    if (g_status != Libraries::CommonDialog::Status::INITIALIZED &&
        g_status != Libraries::CommonDialog::Status::FINISHED) {
        LOG_ERROR(Lib_NpProfileDialog, "called without initialize");
        return Libraries::CommonDialog::Error::INVALID_STATE;
    }
    if (param == nullptr) {
        LOG_ERROR(Lib_NpProfileDialog, "failed: param is nullptr");
        return Libraries::CommonDialog::Error::ARG_NULL;
    }

    LOG_DEBUG(Lib_NpProfileDialog,
              "param details: mode={}, userId={}, targetAccountId='{}', userData={}",
              static_cast<int>(param->mode), param->userId, param->targetAccountId,
              fmt::ptr(param->userData));
    LOG_ERROR(Lib_NpProfileDialog, "(STUBBED) called");

    NpProfileDialogState state{};
    state.accountId = param->targetAccountId;
    state.hasAccountId = true;
    state.mode = param->mode;
    state.userId = param->userId;
    g_state = state;
    g_result = {};
    g_result.userData = param->userData;
    g_status = Libraries::CommonDialog::Status::RUNNING;

    LOG_DEBUG(Lib_NpProfileDialog, "creating UI with accountId='{}', mode={}, userId={}",
              state.accountId, static_cast<int>(state.mode), state.userId);

    g_profile_dialog_ui = NpProfileDialogUi(&g_state, &g_status, &g_result);

    LOG_INFO(Lib_NpProfileDialog, "dialog opened successfully for user {} (account ID variant)",
             param->userId);
    return Libraries::CommonDialog::Error::OK;
}

Libraries::CommonDialog::Error PS4_SYSV_ABI sceNpProfileDialogTerminate() {
    LOG_DEBUG(Lib_NpProfileDialog, "called (g_status={})", static_cast<int>(g_status));

    if (g_status == Libraries::CommonDialog::Status::RUNNING) {
        LOG_ERROR(Lib_NpProfileDialog, "dialog is running, closing it first");
        sceNpProfileDialogClose();
    }
    if (g_status == Libraries::CommonDialog::Status::NONE) {
        LOG_ERROR(Lib_NpProfileDialog, "failed: not initialized (g_status=NONE)");
        return Libraries::CommonDialog::Error::NOT_INITIALIZED;
    }

    g_status = Libraries::CommonDialog::Status::NONE;
    CommonDialog::g_isUsed = false;

    LOG_INFO(Lib_NpProfileDialog, "terminated successfully");
    return Libraries::CommonDialog::Error::OK;
}

Libraries::CommonDialog::Status PS4_SYSV_ABI sceNpProfileDialogUpdateStatus() {
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
