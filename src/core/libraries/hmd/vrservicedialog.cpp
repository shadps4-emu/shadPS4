// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/hmd/vrservicedialog.h"

namespace Libraries::VrServiceDialog {

s32 PS4_SYSV_ABI sceVrServiceDialogClose() {
    LOG_ERROR(Lib_VrServiceDialog, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVrServiceDialogGetResult() {
    LOG_ERROR(Lib_VrServiceDialog, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVrServiceDialogGetStatus() {
    LOG_ERROR(Lib_VrServiceDialog, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVrServiceDialogInitialize() {
    LOG_ERROR(Lib_VrServiceDialog, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVrServiceDialogOpen() {
    LOG_ERROR(Lib_VrServiceDialog, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVrServiceDialogTerminate() {
    LOG_ERROR(Lib_VrServiceDialog, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceVrServiceDialogUpdateStatus() {
    LOG_ERROR(Lib_VrServiceDialog, "(STUBBED) called");
    return 3; // finished
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("hBH2ABP7IeY", "libSceVrServiceDialog", 1, "libSceVrServiceDialog",
                 sceVrServiceDialogClose);
    LIB_FUNCTION("cYnBkgm8I0c", "libSceVrServiceDialog", 1, "libSceVrServiceDialog",
                 sceVrServiceDialogGetResult);
    LIB_FUNCTION("kUavKmsczkY", "libSceVrServiceDialog", 1, "libSceVrServiceDialog",
                 sceVrServiceDialogGetStatus);
    LIB_FUNCTION("hYFXG8FWThI", "libSceVrServiceDialog", 1, "libSceVrServiceDialog",
                 sceVrServiceDialogInitialize);
    LIB_FUNCTION("60-cjn5Dn0Q", "libSceVrServiceDialog", 1, "libSceVrServiceDialog",
                 sceVrServiceDialogOpen);
    LIB_FUNCTION("M4xKWUytNMo", "libSceVrServiceDialog", 1, "libSceVrServiceDialog",
                 sceVrServiceDialogTerminate);
    LIB_FUNCTION("RmRtBJpoHlA", "libSceVrServiceDialog", 1, "libSceVrServiceDialog",
                 sceVrServiceDialogUpdateStatus);
};

} // namespace Libraries::VrServiceDialog