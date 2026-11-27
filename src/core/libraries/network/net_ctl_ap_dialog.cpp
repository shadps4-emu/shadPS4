// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/libs.h"
#include "core/libraries/network/net_ctl_ap_dialog.h"
#include "core/libraries/network/netctl.h"

namespace Libraries::NetCtlApDialog {

using CommonDialog::Error;
using CommonDialog::Status;

static Status g_status = Status::NONE;
static CommonDialog::Result g_result = CommonDialog::Result::OK;

Error PS4_SYSV_ABI sceNetCtlApDialogInitialize() {
    LOG_DEBUG(Lib_NetCtl, "called");
    if (g_status != Status::NONE) {
        return Error::ALREADY_INITIALIZED;
    }
    g_status = Status::INITIALIZED;
    return Error::OK;
}

Error PS4_SYSV_ABI sceNetCtlApDialogOpen(const OrbisNetCtlApDialogParam* param) {
    LOG_DEBUG(Lib_NetCtl, "called");
    if (param == nullptr) {
        return Error::ARG_NULL;
    }
    if (g_status == Status::NONE) {
        return Error::NOT_INITIALIZED;
    }
    if (g_status == Status::RUNNING) {
        return Error::BUSY;
    }
    g_result =
        NetCtl::StartAccessPoint() ? CommonDialog::Result::OK : CommonDialog::Result::USER_CANCELED;
    g_status = Status::FINISHED;
    return Error::OK;
}

Status PS4_SYSV_ABI sceNetCtlApDialogUpdateStatus() {
    return g_status;
}

Error PS4_SYSV_ABI sceNetCtlApDialogGetResult(OrbisNetCtlApDialogResult* result) {
    if (result == nullptr) {
        return Error::ARG_NULL;
    }
    if (g_status != Status::FINISHED) {
        return Error::NOT_FINISHED;
    }
    result->result = g_result;
    return Error::OK;
}

Error PS4_SYSV_ABI sceNetCtlApDialogTerminate() {
    LOG_DEBUG(Lib_NetCtl, "called");
    if (g_status == Status::NONE) {
        return Error::NOT_INITIALIZED;
    }
    g_status = Status::NONE;
    return Error::OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("py8-vzIY-RY", "libSceNetCtlApDialog", 1, "libSceNetCtlApDialog",
                 sceNetCtlApDialogInitialize);
    LIB_FUNCTION("SKJQwgudGFg", "libSceNetCtlApDialog", 1, "libSceNetCtlApDialog",
                 sceNetCtlApDialogOpen);
    LIB_FUNCTION("ys1JAOGzNG0", "libSceNetCtlApDialog", 1, "libSceNetCtlApDialog",
                 sceNetCtlApDialogUpdateStatus);
    LIB_FUNCTION("ZaEVF6WqBP8", "libSceNetCtlApDialog", 1, "libSceNetCtlApDialog",
                 sceNetCtlApDialogGetResult);
    LIB_FUNCTION("5X+QFaVtiKg", "libSceNetCtlApDialog", 1, "libSceNetCtlApDialog",
                 sceNetCtlApDialogTerminate);
}

} // namespace Libraries::NetCtlApDialog
