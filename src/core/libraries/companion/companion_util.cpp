// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/companion/companion_util.h"

namespace Libraries::CompanionUtil {

s32 PS4_SYSV_ABI sceCompanionUtilGetEvent() {
    return 0x8000000;
}

s32 PS4_SYSV_ABI sceCompanionUtilGetRemoteOskEvent() {
    LOG_ERROR(Lib_CompanionUtil, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCompanionUtilInitialize() {
    LOG_ERROR(Lib_CompanionUtil, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCompanionUtilOptParamInitialize() {
    LOG_ERROR(Lib_CompanionUtil, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceCompanionUtilTerminate() {
    LOG_ERROR(Lib_CompanionUtil, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterlibSceCompanionUtil(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("cE5Msy11WhU", "libSceCompanionUtil", 1, "libSceCompanionUtil", 1, 1,
                 sceCompanionUtilGetEvent);
    LIB_FUNCTION("MaVrz79mT5o", "libSceCompanionUtil", 1, "libSceCompanionUtil", 1, 1,
                 sceCompanionUtilGetRemoteOskEvent);
    LIB_FUNCTION("xb1xlIhf0QY", "libSceCompanionUtil", 1, "libSceCompanionUtil", 1, 1,
                 sceCompanionUtilInitialize);
    LIB_FUNCTION("IPN-FRSrafk", "libSceCompanionUtil", 1, "libSceCompanionUtil", 1, 1,
                 sceCompanionUtilOptParamInitialize);
    LIB_FUNCTION("H1fYQd5lFAI", "libSceCompanionUtil", 1, "libSceCompanionUtil", 1, 1,
                 sceCompanionUtilTerminate);
};

} // namespace Libraries::CompanionUtil