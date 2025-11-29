// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"

namespace Libraries::Np::NpCommerce {
s32 PS4_SYSV_ABI sceNpCommerceDialogClose() {
    LOG_ERROR(Lib_NpCommerce, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpCommerceDialogGetResult(s32* result) {
    LOG_ERROR(Lib_NpCommerce, "(STUBBED) called");
    return ORBIS_OK;
}

s8 PS4_SYSV_ABI sceNpCommerceDialogGetStatus() {
    LOG_ERROR(Lib_NpCommerce, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpCommerceDialogInitialize() {
    LOG_ERROR(Lib_NpCommerce, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpCommerceDialogInitializeInternal() {
    LOG_ERROR(Lib_NpCommerce, "(STUBBED) called");
    return ORBIS_OK;
}

s16 PS4_SYSV_ABI sceNpCommerceDialogOpen(s64 check) {
    LOG_ERROR(Lib_NpCommerce, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpCommerceDialogTerminate() {
    LOG_ERROR(Lib_NpCommerce, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpCommerceDialogUpdateStatus() {
    LOG_ERROR(Lib_NpCommerce, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpCommerceHidePsStoreIcon() {
    LOG_ERROR(Lib_NpCommerce, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpCommerceSetPsStoreIconLayout(s32 layout) {
    LOG_ERROR(Lib_NpCommerce, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpCommerceShowPsStoreIcon(s16 icon) {
    LOG_ERROR(Lib_NpCommerce, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("NU3ckGHMFXo", "libSceNpCommerce", 1, "libSceNpCommerce",
                 sceNpCommerceDialogClose);
    LIB_FUNCTION("r42bWcQbtZY", "libSceNpCommerce", 1, "libSceNpCommerce",
                 sceNpCommerceDialogGetResult);
    LIB_FUNCTION("CCbC+lqqvF0", "libSceNpCommerce", 1, "libSceNpCommerce",
                 sceNpCommerceDialogGetStatus);
    LIB_FUNCTION("0aR2aWmQal4", "libSceNpCommerce", 1, "libSceNpCommerce",
                 sceNpCommerceDialogInitialize);
    LIB_FUNCTION("9ZiLXAGG5rg", "libSceNpCommerce", 1, "libSceNpCommerce",
                 sceNpCommerceDialogInitializeInternal);
    LIB_FUNCTION("DfSCDRA3EjY", "libSceNpCommerce", 1, "libSceNpCommerce", sceNpCommerceDialogOpen);
    LIB_FUNCTION("m-I92Ab50W8", "libSceNpCommerce", 1, "libSceNpCommerce",
                 sceNpCommerceDialogTerminate);
    LIB_FUNCTION("LR5cwFMMCVE", "libSceNpCommerce", 1, "libSceNpCommerce",
                 sceNpCommerceDialogUpdateStatus);
    LIB_FUNCTION("dsqCVsNM0Zg", "libSceNpCommerce", 1, "libSceNpCommerce",
                 sceNpCommerceHidePsStoreIcon);
    LIB_FUNCTION("uKTDW8hk-ts", "libSceNpCommerce", 1, "libSceNpCommerce",
                 sceNpCommerceSetPsStoreIconLayout);
    LIB_FUNCTION("DHmwsa6S8Tc", "libSceNpCommerce", 1, "libSceNpCommerce",
                 sceNpCommerceShowPsStoreIcon);
};

} // namespace Libraries::Np::NpCommerce
