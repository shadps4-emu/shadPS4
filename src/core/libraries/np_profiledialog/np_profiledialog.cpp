// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/np_profiledialog/np_profiledialog.h"

namespace Libraries::NpProfileDialog {

s32 PS4_SYSV_ABI sceNpProfileDialogOpen() {
    LOG_ERROR(Lib_NpProfileDialog, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpProfileDialogClose() {
    LOG_ERROR(Lib_NpProfileDialog, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpProfileDialogGetResult() {
    LOG_ERROR(Lib_NpProfileDialog, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpProfileDialogGetStatus() {
    LOG_ERROR(Lib_NpProfileDialog, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpProfileDialogInitialize() {
    LOG_ERROR(Lib_NpProfileDialog, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpProfileDialogOpenA() {
    LOG_ERROR(Lib_NpProfileDialog, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpProfileDialogTerminate() {
    LOG_ERROR(Lib_NpProfileDialog, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpProfileDialogUpdateStatus() {
    LOG_DEBUG(Lib_NpProfileDialog, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("uj9Cz7Tk0cc", "libSceNpProfileDialogCompat", 1, "libSceNpProfileDialog", 1, 1,
                 sceNpProfileDialogOpen);
    LIB_FUNCTION("wkwjz0Xdo2A", "libSceNpProfileDialog", 1, "libSceNpProfileDialog", 1, 1,
                 sceNpProfileDialogClose);
    LIB_FUNCTION("8rhLl1-0W-o", "libSceNpProfileDialog", 1, "libSceNpProfileDialog", 1, 1,
                 sceNpProfileDialogGetResult);
    LIB_FUNCTION("3BqoiFOjSsk", "libSceNpProfileDialog", 1, "libSceNpProfileDialog", 1, 1,
                 sceNpProfileDialogGetStatus);
    LIB_FUNCTION("Lg+NCE6pTwQ", "libSceNpProfileDialog", 1, "libSceNpProfileDialog", 1, 1,
                 sceNpProfileDialogInitialize);
    LIB_FUNCTION("uj9Cz7Tk0cc", "libSceNpProfileDialog", 1, "libSceNpProfileDialog", 1, 1,
                 sceNpProfileDialogOpen);
    LIB_FUNCTION("nrQRlLKzdwE", "libSceNpProfileDialog", 1, "libSceNpProfileDialog", 1, 1,
                 sceNpProfileDialogOpenA);
    LIB_FUNCTION("0Sp9vJcB1-w", "libSceNpProfileDialog", 1, "libSceNpProfileDialog", 1, 1,
                 sceNpProfileDialogTerminate);
    LIB_FUNCTION("haVZE9FgKqE", "libSceNpProfileDialog", 1, "libSceNpProfileDialog", 1, 1,
                 sceNpProfileDialogUpdateStatus);
};

} // namespace Libraries::NpProfileDialog