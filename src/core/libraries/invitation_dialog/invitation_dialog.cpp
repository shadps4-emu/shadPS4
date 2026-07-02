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
    LOG_DEBUG(Lib_InvitationDialog, "sceInvitationDialogClose() called");
    LOG_INFO(Lib_InvitationDialog, "Closing invitation dialog, current status={}",
             magic_enum::enum_name(g_status));

    if (g_status != Libraries::CommonDialog::Status::RUNNING) {
        LOG_WARNING(Lib_InvitationDialog, "Cannot close dialog: not running (status={})",
                    magic_enum::enum_name(g_status));
        return Libraries::CommonDialog::Error::NOT_RUNNING;
    }

    LOG_INFO(Lib_InvitationDialog, "Closing invitation UI dialog");
    g_status = Libraries::CommonDialog::Status::FINISHED;
    return Libraries::CommonDialog::Error::OK;
}

s32 PS4_SYSV_ABI sceInvitationDialogGetResult(OrbisInvitationDialogResult* result) {
    LOG_DEBUG(Lib_InvitationDialog, "sceInvitationDialogGetResult() called");
    LOG_INFO(Lib_InvitationDialog, "Getting invitation dialog result, result ptr={}",
             fmt::ptr(result));

    if (!result) {
        LOG_ERROR(Lib_InvitationDialog, "Result parameter is NULL");
        return ORBIS_INVITATION_DIALOG_ERROR_BLOCKED_ALL_PLAYERS;
    }

    // Log result fields if they contain data
    if (result->callbackArg) {
        LOG_TRACE(Lib_InvitationDialog, "Result callbackArg={}", fmt::ptr(result->callbackArg));
    }
    LOG_TRACE(Lib_InvitationDialog, "Result errorCode={:#x}", result->errorCode);
    LOG_TRACE(Lib_InvitationDialog, "Result status={}", magic_enum::enum_name(result->result));

    if (result->sentOnlineIds) {
        LOG_TRACE(Lib_InvitationDialog, "Result sentOnlineIds count={}",
                  result->sentOnlineIds->count);
        for (u32 i = 0; i < result->sentOnlineIds->count &&
                        i < ORBIS_INVITATION_DIALOG_ADDRESS_USER_LIST_MAX_SIZE;
             i++) {
            LOG_TRACE(Lib_InvitationDialog, "  sentOnlineIds[{}]: {}", i,
                      result->sentOnlineIds->onlineId[i].data);
        }
    }

    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceInvitationDialogGetResultA(OrbisInvitationDialogResultA* result) {
    LOG_DEBUG(Lib_InvitationDialog, "sceInvitationDialogGetResultA() called (async version)");
    LOG_INFO(Lib_InvitationDialog, "Getting invitation dialog result (async), result ptr={}",
             fmt::ptr(result));

    if (!result) {
        LOG_ERROR(Lib_InvitationDialog, "Result parameter is NULL");
        return ORBIS_INVITATION_DIALOG_ERROR_BLOCKED_ALL_PLAYERS;
    }

    // Log result fields
    if (result->callbackArg) {
        LOG_TRACE(Lib_InvitationDialog, "ResultA callbackArg={}", fmt::ptr(result->callbackArg));
    }
    LOG_TRACE(Lib_InvitationDialog, "ResultA errorCode={:#x}", result->errorCode);
    LOG_TRACE(Lib_InvitationDialog, "ResultA status={}", magic_enum::enum_name(result->result));

    if (result->sentUsers) {
        LOG_TRACE(Lib_InvitationDialog, "ResultA sentUsers count={}", result->sentUsers->count);
        for (u32 i = 0;
             i < result->sentUsers->count && i < ORBIS_INVITATION_DIALOG_ADDRESS_USER_LIST_MAX_SIZE;
             i++) {
            LOG_TRACE(Lib_InvitationDialog, "  sentUsers[{}]: onlineId={}, accountId={}", i,
                      result->sentUsers->users[i].onlineId.data,
                      result->sentUsers->users[i].accountId);
        }
    }

    return ORBIS_OK;
}

Libraries::CommonDialog::Status PS4_SYSV_ABI sceInvitationDialogGetStatus() {
    LOG_DEBUG(Lib_InvitationDialog, "sceInvitationDialogGetStatus() called");
    LOG_INFO(Lib_InvitationDialog, "Getting invitation dialog status: {}",
             magic_enum::enum_name(g_status));
    return g_status;
}

Libraries::CommonDialog::Error PS4_SYSV_ABI sceInvitationDialogInitialize() {
    LOG_DEBUG(Lib_InvitationDialog, "sceInvitationDialogInitialize() called");
    LOG_INFO(Lib_InvitationDialog, "Initializing invitation dialog system");

    LOG_TRACE(Lib_InvitationDialog, "CommonDialog::g_isInitialized={}",
              CommonDialog::g_isInitialized);
    LOG_TRACE(Lib_InvitationDialog, "Current g_status={}", magic_enum::enum_name(g_status));
    LOG_TRACE(Lib_InvitationDialog, "CommonDialog::g_isUsed={}", CommonDialog::g_isUsed);

    if (!CommonDialog::g_isInitialized) {
        LOG_ERROR(Lib_InvitationDialog, "CommonDialog system not initialized");
        return Libraries::CommonDialog::Error::NOT_SYSTEM_INITIALIZED;
    }

    if (g_status != Libraries::CommonDialog::Status::NONE) {
        LOG_ERROR(Lib_InvitationDialog, "Invitation dialog already initialized (status={})",
                  magic_enum::enum_name(g_status));
        return Libraries::CommonDialog::Error::ALREADY_INITIALIZED;
    }

    if (CommonDialog::g_isUsed) {
        LOG_ERROR(Lib_InvitationDialog, "CommonDialog already in use");
        return Libraries::CommonDialog::Error::BUSY;
    }

    g_status = Libraries::CommonDialog::Status::INITIALIZED;
    CommonDialog::g_isUsed = true;
    LOG_INFO(Lib_InvitationDialog, "Invitation dialog initialized successfully");
    return Libraries::CommonDialog::Error::OK;
}

Libraries::CommonDialog::Error PS4_SYSV_ABI
sceInvitationDialogOpen(const OrbisInvitationDialogParam* param) {
    LOG_DEBUG(Lib_InvitationDialog, "sceInvitationDialogOpen() called");
    LOG_INFO(Lib_InvitationDialog, "Opening invitation dialog, param ptr={}", fmt::ptr(param));

    if (!param) {
        LOG_ERROR(Lib_InvitationDialog, "Parameter is NULL");
        return Libraries::CommonDialog::Error::ARG_NULL;
    }

    LOG_TRACE(Lib_InvitationDialog, "Param size={:#x}", param->size);
    LOG_TRACE(Lib_InvitationDialog, "Param mode={}", param->mode);

    // Validate mode
    if (param->mode != ORBIS_INVITATION_DIALOG_MODE_SEND &&
        param->mode != ORBIS_INVITATION_DIALOG_MODE_RECV) {
        LOG_ERROR(Lib_InvitationDialog, "Invalid mode: {} (expected SEND={} or RECV={})",
                  param->mode, ORBIS_INVITATION_DIALOG_MODE_SEND,
                  ORBIS_INVITATION_DIALOG_MODE_RECV);
        return Libraries::CommonDialog::Error::PARAM_INVALID;
    }

    LOG_TRACE(Lib_InvitationDialog, "Param userId={:#x}", param->userId);
    LOG_TRACE(Lib_InvitationDialog, "Param callbackArg={}", fmt::ptr(param->callbackArg));
    LOG_TRACE(Lib_InvitationDialog, "Param dataParam={}", fmt::ptr(param->dataParam));

    // Log base parameters
    LOG_TRACE(Lib_InvitationDialog, "BaseParam mode={}",
              magic_enum::enum_name(param->baseParam.mode));
    LOG_TRACE(Lib_InvitationDialog, "BaseParam callback={}", fmt::ptr(param->baseParam.callback));

    // Log data parameters if present
    if (param->dataParam) {
        LOG_TRACE(Lib_InvitationDialog, "DataParam pointer: {}", fmt::ptr(param->dataParam));
        if (param->mode == ORBIS_INVITATION_DIALOG_MODE_SEND) {
            LOG_TRACE(Lib_InvitationDialog, "SendInfo:");
            if (param->dataParam->SendInfo.userMessage) {
                LOG_TRACE(Lib_InvitationDialog, "  userMessage: '{}'",
                          param->dataParam->SendInfo.userMessage);
            }
            if (param->dataParam->SendInfo.sessionId) {
                LOG_TRACE(Lib_InvitationDialog, "  sessionId: {}",
                          param->dataParam->SendInfo.sessionId->data);
            }

            // Log address parameters
            const auto& addressParam = param->dataParam->SendInfo.addressParam;
            LOG_TRACE(Lib_InvitationDialog, "  addressType={}", addressParam.addressType);
            if (addressParam.addressType == ORBIS_INVITATION_DIALOG_ADDRESS_TYPE_USERDISABL) {
                LOG_TRACE(Lib_InvitationDialog, "  UserSelectDisableAddress:");
                LOG_TRACE(Lib_InvitationDialog, "    onlineIds={}",
                          fmt::ptr(addressParam.addressInfo.UserSelectDisableAddress.onlineIds));
                LOG_TRACE(Lib_InvitationDialog, "    onlineIdsCount={}",
                          addressParam.addressInfo.UserSelectDisableAddress.onlineIdsCount);
            } else if (addressParam.addressType ==
                       ORBIS_INVITATION_DIALOG_ADDRESS_TYPE_USERENABLE) {
                LOG_TRACE(Lib_InvitationDialog, "  UserSelectEnableAddress:");
                LOG_TRACE(Lib_InvitationDialog, "    onlineIdsMaxCount={}",
                          addressParam.addressInfo.UserSelectEnableAddress.onlineIdsMaxCount);
            } else {
                LOG_WARNING(Lib_InvitationDialog, "  Invalid addressType: {}",
                            addressParam.addressType);
            }
        } else if (param->mode == ORBIS_INVITATION_DIALOG_MODE_RECV) {
            LOG_TRACE(Lib_InvitationDialog, "RecvInfo (receive mode - no additional data)");
        }
    }

    // Check initialization state
    if (g_status != Libraries::CommonDialog::Status::INITIALIZED &&
        g_status != Libraries::CommonDialog::Status::FINISHED) {
        LOG_INFO(Lib_InvitationDialog, "Called without proper initialization (status={})",
                 magic_enum::enum_name(g_status));
        return Libraries::CommonDialog::Error::INVALID_STATE;
    }

    LOG_ERROR(Lib_InvitationDialog,
              "(STUBBED) Opening invitation dialog UI - TODO implement actual UI");
    g_status = Libraries::CommonDialog::Status::RUNNING;
    LOG_INFO(Lib_InvitationDialog, "Invitation dialog opened successfully (stubbed)");
    return Libraries::CommonDialog::Error::OK;
}

Libraries::CommonDialog::Error PS4_SYSV_ABI
sceInvitationDialogOpenA(const OrbisInvitationDialogParamA* param) {
    LOG_DEBUG(Lib_InvitationDialog, "sceInvitationDialogOpenA() called (async version)");
    LOG_INFO(Lib_InvitationDialog, "Opening invitation dialog asynchronously, param ptr={}",
             fmt::ptr(param));

    if (!param) {
        LOG_ERROR(Lib_InvitationDialog, "Parameter is NULL");
        return Libraries::CommonDialog::Error::ARG_NULL;
    }

    LOG_TRACE(Lib_InvitationDialog, "ParamA size={:#x}", param->size);
    LOG_TRACE(Lib_InvitationDialog, "ParamA mode={}", param->mode);

    // Validate mode
    if (param->mode != ORBIS_INVITATION_DIALOG_MODE_SEND &&
        param->mode != ORBIS_INVITATION_DIALOG_MODE_RECV) {
        LOG_ERROR(Lib_InvitationDialog, "Invalid mode: {} (expected SEND={} or RECV={})",
                  param->mode, ORBIS_INVITATION_DIALOG_MODE_SEND,
                  ORBIS_INVITATION_DIALOG_MODE_RECV);
        return Libraries::CommonDialog::Error::PARAM_INVALID;
    }

    LOG_TRACE(Lib_InvitationDialog, "ParamA userId={:#x}", param->userId);
    LOG_TRACE(Lib_InvitationDialog, "ParamA callbackArg={}", fmt::ptr(param->callbackArg));
    LOG_TRACE(Lib_InvitationDialog, "ParamA dataParam={}", fmt::ptr(param->dataParam));

    // Log base parameters
    LOG_TRACE(Lib_InvitationDialog, "BaseParam mode={}",
              magic_enum::enum_name(param->baseParam.mode));
    LOG_TRACE(Lib_InvitationDialog, "BaseParam callback={}", fmt::ptr(param->baseParam.callback));

    // Log data parameters if present
    if (param->dataParam) {
        LOG_TRACE(Lib_InvitationDialog, "DataParamA pointer: {}", fmt::ptr(param->dataParam));
        if (param->mode == ORBIS_INVITATION_DIALOG_MODE_SEND) {
            LOG_TRACE(Lib_InvitationDialog, "SendInfo:");
            if (param->dataParam->SendInfo.userMessage) {
                LOG_TRACE(Lib_InvitationDialog, "  userMessage: '{}'",
                          param->dataParam->SendInfo.userMessage);
            }
            if (param->dataParam->SendInfo.sessionId) {
                LOG_TRACE(Lib_InvitationDialog, "  sessionId: {}",
                          param->dataParam->SendInfo.sessionId->data);
            }

            // Log address parameters
            const auto& addressParam = param->dataParam->SendInfo.addressParam;
            LOG_TRACE(Lib_InvitationDialog, "  addressType={}", addressParam.addressType);
            if (addressParam.addressType == ORBIS_INVITATION_DIALOG_ADDRESS_TYPE_USERDISABL) {
                LOG_TRACE(Lib_InvitationDialog, "  UserSelectDisableAddress:");
                LOG_TRACE(Lib_InvitationDialog, "    accountIds={}",
                          fmt::ptr(addressParam.addressInfo.UserSelectDisableAddress.accountIds));
                LOG_TRACE(Lib_InvitationDialog, "    accountIdsCount={}",
                          addressParam.addressInfo.UserSelectDisableAddress.accountIdsCount);
            } else if (addressParam.addressType ==
                       ORBIS_INVITATION_DIALOG_ADDRESS_TYPE_USERENABLE) {
                LOG_TRACE(Lib_InvitationDialog, "  UserSelectEnableAddress:");
                LOG_TRACE(Lib_InvitationDialog, "    userMaxCount={}",
                          addressParam.addressInfo.UserSelectEnableAddress.userMaxCount);
            } else {
                LOG_WARNING(Lib_InvitationDialog, "  Invalid addressType: {}",
                            addressParam.addressType);
            }
        } else if (param->mode == ORBIS_INVITATION_DIALOG_MODE_RECV) {
            LOG_TRACE(Lib_InvitationDialog, "RecvInfo (receive mode - no additional data)");
        }
    }

    // Check initialization state
    if (g_status != Libraries::CommonDialog::Status::INITIALIZED &&
        g_status != Libraries::CommonDialog::Status::FINISHED) {
        LOG_INFO(Lib_InvitationDialog, "Called without proper initialization (status={})",
                 magic_enum::enum_name(g_status));
        return Libraries::CommonDialog::Error::INVALID_STATE;
    }

    LOG_ERROR(Lib_InvitationDialog,
              "(STUBBED) Opening invitation dialog UI asynchronously - TODO implement actual UI");
    g_status = Libraries::CommonDialog::Status::RUNNING;
    LOG_INFO(Lib_InvitationDialog, "Invitation dialog opened successfully (async, stubbed)");
    return Libraries::CommonDialog::Error::OK;
}

Libraries::CommonDialog::Error PS4_SYSV_ABI sceInvitationDialogTerminate() {
    LOG_DEBUG(Lib_InvitationDialog, "sceInvitationDialogTerminate() called");
    LOG_INFO(Lib_InvitationDialog, "Terminating invitation dialog system, current status={}",
             magic_enum::enum_name(g_status));

    if (g_status == Libraries::CommonDialog::Status::RUNNING) {
        LOG_INFO(Lib_InvitationDialog, "Dialog is running, closing before termination");
        sceInvitationDialogClose();
    }

    if (g_status == Libraries::CommonDialog::Status::NONE) {
        LOG_ERROR(Lib_InvitationDialog, "Cannot terminate: not initialized");
        return Libraries::CommonDialog::Error::NOT_INITIALIZED;
    }

    g_status = Libraries::CommonDialog::Status::NONE;
    CommonDialog::g_isUsed = false;
    LOG_INFO(Lib_InvitationDialog, "Invitation dialog terminated successfully");
    return Libraries::CommonDialog::Error::OK;
}

Libraries::CommonDialog::Status PS4_SYSV_ABI sceInvitationDialogUpdateStatus() {
    LOG_DEBUG(Lib_InvitationDialog, "sceInvitationDialogUpdateStatus() called");
    LOG_TRACE(Lib_InvitationDialog, "UpdateStatus called, current status={}",
              magic_enum::enum_name(g_status));

    if (g_status == Libraries::CommonDialog::Status::RUNNING) {
        // TODO: Remove this when implementing real dialog
        // This is a stub that automatically transitions from RUNNING to FINISHED
        LOG_WARNING(Lib_InvitationDialog,
                    "Auto-transitioning from RUNNING to FINISHED (stub behavior)");
        g_status = Libraries::CommonDialog::Status::FINISHED;
    }

    LOG_INFO(Lib_InvitationDialog, "UpdateStatus returned: {}", magic_enum::enum_name(g_status));
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