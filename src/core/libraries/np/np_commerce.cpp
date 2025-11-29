// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/system/commondialog.h"

namespace Libraries::Np::NpCommerce {

using CommonDialog::Error;
using CommonDialog::Result;
using CommonDialog::Status;

static Status g_dialog_status = Status::NONE;
static Result g_dialog_result = Result::OK;

s32 PS4_SYSV_ABI sceNpCommerceDialogClose() {
    LOG_INFO(Lib_NpCommerce, "called");
    if (g_dialog_status == Status::NONE) {
        return static_cast<s32>(Error::NOT_INITIALIZED);
    }
    if (g_dialog_status != Status::FINISHED) {
        return static_cast<s32>(Error::NOT_FINISHED);
    }
    g_dialog_status = Status::INITIALIZED;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpCommerceDialogGetResult(s32* result) {
    LOG_INFO(Lib_NpCommerce, "called");
    if (result == nullptr) {
        return static_cast<s32>(Error::ARG_NULL);
    }
    if (g_dialog_status != Status::FINISHED) {
        return static_cast<s32>(Error::NOT_FINISHED);
    }
    *result = static_cast<s32>(g_dialog_result);
    return ORBIS_OK;
}

s8 PS4_SYSV_ABI sceNpCommerceDialogGetStatus() {
    LOG_DEBUG(Lib_NpCommerce, "called, status = {}", static_cast<u32>(g_dialog_status));
    return static_cast<s8>(g_dialog_status);
}

s32 PS4_SYSV_ABI sceNpCommerceDialogInitialize() {
    LOG_INFO(Lib_NpCommerce, "called");
    if (g_dialog_status != Status::NONE) {
        return static_cast<s32>(Error::ALREADY_INITIALIZED);
    }
    g_dialog_status = Status::INITIALIZED;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpCommerceDialogInitializeInternal() {
    LOG_INFO(Lib_NpCommerce, "called");
    return sceNpCommerceDialogInitialize();
}

s16 PS4_SYSV_ABI sceNpCommerceDialogOpen(s64 check) {
    LOG_INFO(Lib_NpCommerce, "called, check = {}", check);
    if (g_dialog_status != Status::INITIALIZED) {
        LOG_WARNING(Lib_NpCommerce, "Dialog not initialized");
        return ORBIS_OK;
    }

    g_dialog_status = Status::FINISHED;
    g_dialog_result = Result::USER_CANCELED;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpCommerceDialogTerminate() {
    LOG_INFO(Lib_NpCommerce, "called");
    if (g_dialog_status == Status::NONE) {
        return static_cast<s32>(Error::NOT_INITIALIZED);
    }
    if (g_dialog_status == Status::RUNNING) {
        return static_cast<s32>(Error::NOT_FINISHED);
    }
    g_dialog_status = Status::NONE;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpCommerceDialogUpdateStatus() {
    LOG_DEBUG(Lib_NpCommerce, "called, status = {}", static_cast<u32>(g_dialog_status));
    return static_cast<s32>(g_dialog_status);
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
