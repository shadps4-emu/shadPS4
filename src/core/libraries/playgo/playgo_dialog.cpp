// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "common/logging/log.h"
#include "core/libraries/libs.h"
#include "core/libraries/playgo/playgo_dialog.h"
#include "core/libraries/system/commondialog.h"

namespace Libraries::PlayGo::Dialog {

using CommonDialog::Error;
using CommonDialog::Result;
using CommonDialog::Status;

Error PS4_SYSV_ABI scePlayGoDialogClose() {
    LOG_ERROR(Lib_PlayGoDialog, "(DUMMY) called");
    return Error::OK;
}

Error PS4_SYSV_ABI scePlayGoDialogGetResult(OrbisPlayGoDialogResult* result) {
    LOG_ERROR(Lib_PlayGoDialog, "(DUMMY) called");
    if (result == nullptr) {
        return Error::ARG_NULL;
    }
    // Result value 3 allows games to proceed.
    result->result = static_cast<Result>(3);
    return Error::OK;
}

Status PS4_SYSV_ABI scePlayGoDialogGetStatus() {
    LOG_ERROR(Lib_PlayGoDialog, "(DUMMY) called");
    return Status::FINISHED;
}

Error PS4_SYSV_ABI scePlayGoDialogInitialize() {
    LOG_ERROR(Lib_PlayGoDialog, "(DUMMY) called");
    return Error::OK;
}

Error PS4_SYSV_ABI scePlayGoDialogOpen(const OrbisPlayGoDialogParam* param) {
    LOG_ERROR(Lib_PlayGoDialog, "(DUMMY) called");
    if (param == nullptr) {
        return Error::ARG_NULL;
    }
    ASSERT(param->size == sizeof(OrbisPlayGoDialogParam));
    ASSERT(param->baseParam.size == sizeof(CommonDialog::BaseParam));
    return Error::OK;
}

Error PS4_SYSV_ABI scePlayGoDialogTerminate() {
    LOG_ERROR(Lib_PlayGoDialog, "(DUMMY) called");
    return Error::OK;
}

Status PS4_SYSV_ABI scePlayGoDialogUpdateStatus() {
    LOG_ERROR(Lib_PlayGoDialog, "(DUMMY) called");
    return Status::FINISHED;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("fbigNQiZpm0", "libScePlayGoDialog", 1, "libScePlayGoDialog", 1, 1,
                 scePlayGoDialogClose);
    LIB_FUNCTION("wx9TDplJKB4", "libScePlayGoDialog", 1, "libScePlayGoDialog", 1, 1,
                 scePlayGoDialogGetResult);
    LIB_FUNCTION("NOAMxY2EGS0", "libScePlayGoDialog", 1, "libScePlayGoDialog", 1, 1,
                 scePlayGoDialogGetStatus);
    LIB_FUNCTION("fECamTJKpsM", "libScePlayGoDialog", 1, "libScePlayGoDialog", 1, 1,
                 scePlayGoDialogInitialize);
    LIB_FUNCTION("kHd72ukqbxw", "libScePlayGoDialog", 1, "libScePlayGoDialog", 1, 1,
                 scePlayGoDialogOpen);
    LIB_FUNCTION("okgIGdr5Iz0", "libScePlayGoDialog", 1, "libScePlayGoDialog", 1, 1,
                 scePlayGoDialogTerminate);
    LIB_FUNCTION("Yb60K7BST48", "libScePlayGoDialog", 1, "libScePlayGoDialog", 1, 1,
                 scePlayGoDialogUpdateStatus);
};

} // namespace Libraries::PlayGo::Dialog
