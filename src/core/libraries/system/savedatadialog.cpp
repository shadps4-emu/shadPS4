// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/system/savedatadialog.h"

namespace Libraries::SaveDataDialog {

int PS4_SYSV_ABI sceSaveDataDialogClose() {
    LOG_ERROR(Lib_SaveDataDialog, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataDialogGetResult() {
    LOG_ERROR(Lib_SaveDataDialog, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataDialogGetStatus() {
    LOG_ERROR(Lib_SaveDataDialog, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataDialogInitialize() {
    LOG_ERROR(Lib_SaveDataDialog, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataDialogIsReadyToDisplay() {
    LOG_ERROR(Lib_SaveDataDialog, "(STUBBED) called");
    return 1;
}

int PS4_SYSV_ABI sceSaveDataDialogOpen() {
    LOG_ERROR(Lib_SaveDataDialog, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataDialogProgressBarInc() {
    LOG_ERROR(Lib_SaveDataDialog, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataDialogProgressBarSetValue() {
    LOG_ERROR(Lib_SaveDataDialog, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataDialogTerminate() {
    LOG_ERROR(Lib_SaveDataDialog, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceSaveDataDialogUpdateStatus() {
    LOG_ERROR(Lib_SaveDataDialog, "(STUBBED) called");
    return 3; // SCE_COMMON_DIALOG_STATUS_FINISHED
}

void RegisterlibSceSaveDataDialog(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("fH46Lag88XY", "libSceSaveDataDialog", 1, "libSceSaveDataDialog", 1, 1,
                 sceSaveDataDialogClose);
    LIB_FUNCTION("yEiJ-qqr6Cg", "libSceSaveDataDialog", 1, "libSceSaveDataDialog", 1, 1,
                 sceSaveDataDialogGetResult);
    LIB_FUNCTION("ERKzksauAJA", "libSceSaveDataDialog", 1, "libSceSaveDataDialog", 1, 1,
                 sceSaveDataDialogGetStatus);
    LIB_FUNCTION("s9e3+YpRnzw", "libSceSaveDataDialog", 1, "libSceSaveDataDialog", 1, 1,
                 sceSaveDataDialogInitialize);
    LIB_FUNCTION("en7gNVnh878", "libSceSaveDataDialog", 1, "libSceSaveDataDialog", 1, 1,
                 sceSaveDataDialogIsReadyToDisplay);
    LIB_FUNCTION("4tPhsP6FpDI", "libSceSaveDataDialog", 1, "libSceSaveDataDialog", 1, 1,
                 sceSaveDataDialogOpen);
    LIB_FUNCTION("V-uEeFKARJU", "libSceSaveDataDialog", 1, "libSceSaveDataDialog", 1, 1,
                 sceSaveDataDialogProgressBarInc);
    LIB_FUNCTION("hay1CfTmLyA", "libSceSaveDataDialog", 1, "libSceSaveDataDialog", 1, 1,
                 sceSaveDataDialogProgressBarSetValue);
    LIB_FUNCTION("YuH2FA7azqQ", "libSceSaveDataDialog", 1, "libSceSaveDataDialog", 1, 1,
                 sceSaveDataDialogTerminate);
    LIB_FUNCTION("KK3Bdg1RWK0", "libSceSaveDataDialog", 1, "libSceSaveDataDialog", 1, 1,
                 sceSaveDataDialogUpdateStatus);
};

} // namespace Libraries::SaveDataDialog
