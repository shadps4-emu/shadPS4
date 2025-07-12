// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "companion_error.h"
#include "core/libraries/companion/companion_util.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"

namespace Libraries::CompanionUtil {

u32 PS4_SYSV_ABI getEvent(sceCompanionUtilContext* ctx, sceCompanionUtilEvent* outEvent,
                          s32 param_3) {
    if (outEvent == 0) {
        return ORBIS_COMPANION_UTIL_INVALID_ARGUMENT;
    }

    if (ctx == nullptr) {
        return ORBIS_COMPANION_UTIL_INVALID_POINTER;
    }

    uint8_t* base = ctx->blob;
    int flag = *reinterpret_cast<int*>(base + 0x178);
    if (flag == 0) {
        return ORBIS_COMPANION_UTIL_NO_EVENT;
    }

    return ORBIS_COMPANION_UTIL_OK;
}

s32 PS4_SYSV_ABI sceCompanionUtilGetEvent(sceCompanionUtilEvent* outEvent) {
    sceCompanionUtilContext* ctx = nullptr;
    u32 ret = getEvent(ctx, outEvent, 1);

    LOG_DEBUG(Lib_CompanionUtil, "(STUBBED) called ret: {}", ret);
    return ret;
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

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
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