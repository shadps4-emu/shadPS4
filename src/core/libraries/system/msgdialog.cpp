// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/system/msgdialog.h"

#include <magic_enum.hpp>

namespace Libraries::MsgDialog {

int PS4_SYSV_ABI sceMsgDialogClose() {
    LOG_ERROR(Lib_MsgDlg, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceMsgDialogGetResult() {
    LOG_ERROR(Lib_MsgDlg, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceMsgDialogGetStatus() {
    LOG_ERROR(Lib_MsgDlg, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceMsgDialogInitialize() {
    LOG_ERROR(Lib_MsgDlg, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceMsgDialogOpen(const OrbisMsgDialogParam* param) {
    LOG_ERROR(Lib_MsgDlg, "(STUBBED) called");
    switch (param->mode) {
    case ORBIS_MSG_DIALOG_MODE_USER_MSG:
        LOG_INFO(Lib_MsgDlg, "sceMsgDialogOpen userMsg type = %s msg = %s",
                 magic_enum::enum_name(param->userMsgParam->buttonType), param->userMsgParam->msg);
        break;
    case ORBIS_MSG_DIALOG_MODE_PROGRESS_BAR:
        LOG_INFO(Lib_MsgDlg, "sceMsgDialogOpen progressBar type = %s msg = %s",
                 magic_enum::enum_name(param->progBarParam->barType), param->progBarParam->msg);
        break;
    case ORBIS_MSG_DIALOG_MODE_SYSTEM_MSG:
        LOG_INFO(Lib_MsgDlg, "sceMsgDialogOpen systemMsg type: %s",
                 magic_enum::enum_name(param->sysMsgParam->sysMsgType));
        break;
    default:
        break;
    }
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceMsgDialogProgressBarInc() {
    LOG_ERROR(Lib_MsgDlg, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceMsgDialogProgressBarSetMsg() {
    LOG_ERROR(Lib_MsgDlg, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceMsgDialogProgressBarSetValue() {
    LOG_ERROR(Lib_MsgDlg, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceMsgDialogTerminate() {
    LOG_ERROR(Lib_MsgDlg, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceMsgDialogUpdateStatus() {
    LOG_ERROR(Lib_MsgDlg, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterlibSceMsgDialog(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("HTrcDKlFKuM", "libSceMsgDialog", 1, "libSceMsgDialog", 1, 1, sceMsgDialogClose);
    LIB_FUNCTION("Lr8ovHH9l6A", "libSceMsgDialog", 1, "libSceMsgDialog", 1, 1,
                 sceMsgDialogGetResult);
    LIB_FUNCTION("CWVW78Qc3fI", "libSceMsgDialog", 1, "libSceMsgDialog", 1, 1,
                 sceMsgDialogGetStatus);
    LIB_FUNCTION("lDqxaY1UbEo", "libSceMsgDialog", 1, "libSceMsgDialog", 1, 1,
                 sceMsgDialogInitialize);
    LIB_FUNCTION("b06Hh0DPEaE", "libSceMsgDialog", 1, "libSceMsgDialog", 1, 1, sceMsgDialogOpen);
    LIB_FUNCTION("Gc5k1qcK4fs", "libSceMsgDialog", 1, "libSceMsgDialog", 1, 1,
                 sceMsgDialogProgressBarInc);
    LIB_FUNCTION("6H-71OdrpXM", "libSceMsgDialog", 1, "libSceMsgDialog", 1, 1,
                 sceMsgDialogProgressBarSetMsg);
    LIB_FUNCTION("wTpfglkmv34", "libSceMsgDialog", 1, "libSceMsgDialog", 1, 1,
                 sceMsgDialogProgressBarSetValue);
    LIB_FUNCTION("ePw-kqZmelo", "libSceMsgDialog", 1, "libSceMsgDialog", 1, 1,
                 sceMsgDialogTerminate);
    LIB_FUNCTION("6fIC3XKt2k0", "libSceMsgDialog", 1, "libSceMsgDialog", 1, 1,
                 sceMsgDialogUpdateStatus);
};

} // namespace Libraries::MsgDialog
