// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <imgui.h>
#include <magic_enum.hpp>

#include "common/assert.h"
#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/system/msgdialog.h"
#include "imgui_internal.h"

namespace Libraries::MsgDialog {

using CommonDialog::Error;
using CommonDialog::Result;
using CommonDialog::Status;

static auto g_status = Status::NONE;
static Param g_param{};
static MsgDialogResult g_result{};
static MsgDialogUi g_msg_dialog_ui;

Error PS4_SYSV_ABI sceMsgDialogClose() {
    if (g_status != Status::RUNNING) {
        return Error::NOT_RUNNING;
    }
    g_msg_dialog_ui.Finish(ButtonId::INVALID);
    return Error::OK;
}

Error PS4_SYSV_ABI sceMsgDialogGetResult(MsgDialogResult* result) {
    if (g_status != Status::FINISHED) {
        return Error::NOT_FINISHED;
    }
    if (result == nullptr) {
        return Error::ARG_NULL;
    }
    for (const auto v : result->reserved) {
        if (v != 0) {
            return Error::PARAM_INVALID;
        }
    }
    *result = g_result;
    return Error::OK;
}

Status PS4_SYSV_ABI sceMsgDialogGetStatus() {
    return g_status;
}

Error PS4_SYSV_ABI sceMsgDialogInitialize() {
    if (!CommonDialog::g_isInitialized) {
        return Error::NOT_SYSTEM_INITIALIZED;
    }
    if (g_status != Status::NONE) {
        return Error::ALREADY_INITIALIZED;
    }
    if (CommonDialog::g_isUsed) {
        return Error::BUSY;
    }
    g_status = Status::INITIALIZED;
    CommonDialog::g_isUsed = true;

    return Error::OK;
}

Error PS4_SYSV_ABI sceMsgDialogOpen(const Param* param) {
    if (g_status != Status::INITIALIZED && g_status != Status::FINISHED) {
        return Error::INVALID_STATE;
    }
    if (param == nullptr) {
        return Error::ARG_NULL;
    }
    ASSERT(param->size == sizeof(Param));
    ASSERT(param->baseParam.size == sizeof(CommonDialog::BaseParam));
    g_status = Status::RUNNING;
    g_param = *param;
    g_result = {};
    g_msg_dialog_ui = MsgDialogUi(&g_param, &g_status, &g_result);
    return Error::OK;
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

Error PS4_SYSV_ABI sceMsgDialogTerminate() {
    if (g_status == Status::RUNNING) {
        sceMsgDialogClose();
    }
    if (g_status == Status::NONE) {
        return Error::NOT_INITIALIZED;
    }
    g_status = Status::NONE;
    CommonDialog::g_isUsed = false;
    return Error::OK;
}

Status PS4_SYSV_ABI sceMsgDialogUpdateStatus() {
    return g_status;
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
