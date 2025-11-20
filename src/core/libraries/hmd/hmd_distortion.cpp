// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/hmd/hmd.h"
#include "core/libraries/hmd/hmd_error.h"
#include "core/libraries/libs.h"

namespace Libraries::Hmd {

static bool g_library_initialized = false;

s32 PS4_SYSV_ABI sceHmdDistortionInitialize(void* reserved) {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    g_library_initialized = true;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdDistortionGet2dVrCommand() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdDistortionGetCompoundEyeCorrectionCommand() {
    // Stubbed on real hardware.
    return ORBIS_HMD_ERROR_PARAMETER_INVALID;
}

s32 PS4_SYSV_ABI sceHmdDistortionGetCorrectionCommand() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdDistortionGetWideNearCorrectionCommand() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdDistortionGetWorkMemoryAlign() {
    return 0x400;
}

s32 PS4_SYSV_ABI sceHmdDistortionGetWorkMemorySize() {
    return 0x20000;
}

s32 PS4_SYSV_ABI sceHmdDistortionSetOutputMinColor() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_B26430EA74FC3DC0() {
    // Stubbed on real hardware.
    return ORBIS_HMD_ERROR_PARAMETER_INVALID;
}

s32 PS4_SYSV_ABI sceHmdGetDistortionParams() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceHmdDistortionTerminate() {
    // Internal (non-exported) library function for terminating the distortion sub-library.
    if (!g_library_initialized) {
        return ORBIS_HMD_ERROR_NOT_INITIALIZED;
    }
    g_library_initialized = false;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_B614F290B67FB59B() {
    LOG_ERROR(Lib_Hmd, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterDistortion(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("gEokC+OGI8g", "libSceHmdDistortion", 1, "libSceHmd",
                 sceHmdDistortionGet2dVrCommand);
    LIB_FUNCTION("za4xJfzCBcM", "libSceHmd", 1, "libSceHmd", sceHmdDistortionGet2dVrCommand);
    LIB_FUNCTION("ER2ar8yUmbk", "libSceHmdDistortion", 1, "libSceHmd",
                 sceHmdDistortionGetCompoundEyeCorrectionCommand);
    LIB_FUNCTION("HT8qWOTOGmo", "libSceHmdDistortion", 1, "libSceHmd",
                 sceHmdDistortionGetCorrectionCommand);
    LIB_FUNCTION("grCYks4m8Jw", "libSceHmd", 1, "libSceHmd", sceHmdDistortionGetCorrectionCommand);
    LIB_FUNCTION("Vkkhy8RFIuk", "libSceHmdDistortion", 1, "libSceHmd",
                 sceHmdDistortionGetWideNearCorrectionCommand);
    LIB_FUNCTION("goi5ASvH-V8", "libSceHmd", 1, "libSceHmd",
                 sceHmdDistortionGetWideNearCorrectionCommand);
    LIB_FUNCTION("1cS7W5J-v3k", "libSceHmdDistortion", 1, "libSceHmd",
                 sceHmdDistortionGetWorkMemoryAlign);
    LIB_FUNCTION("8Ick-e6cDVY", "libSceHmd", 1, "libSceHmd", sceHmdDistortionGetWorkMemoryAlign);
    LIB_FUNCTION("36xDKk+Hw7o", "libSceHmdDistortion", 1, "libSceHmd",
                 sceHmdDistortionGetWorkMemorySize);
    LIB_FUNCTION("D5JfdpJKvXk", "libSceHmd", 1, "libSceHmd", sceHmdDistortionGetWorkMemorySize);
    LIB_FUNCTION("ao8NZ+FRYJE", "libSceHmdDistortion", 1, "libSceHmd", sceHmdDistortionInitialize);
    LIB_FUNCTION("8A4T5ahi790", "libSceHmdDistortion", 1, "libSceHmd",
                 sceHmdDistortionSetOutputMinColor);
    LIB_FUNCTION("mP2ZcYmDg-o", "libSceHmd", 1, "libSceHmd", sceHmdGetDistortionParams);
    LIB_FUNCTION("smQw6nT8PcA", "libSceHmdDistortion", 1, "libSceHmd", Func_B26430EA74FC3DC0);
    LIB_FUNCTION("thTykLZ-tZs", "libSceHmd", 1, "libSceHmd", Func_B614F290B67FB59B);
}
} // namespace Libraries::Hmd