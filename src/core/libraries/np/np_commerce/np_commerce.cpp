// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstddef>
#include <cstring>
#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/np/np_commerce/np_commerce_ui.h"
#include "core/libraries/system/commondialog.h"
#include "core/libraries/system/userservice.h"
#include "np_commerce.h"

namespace Libraries::Np::NpCommerce {

static Status g_dialog_status = Status::NONE;
static s32 g_result_code = ORBIS_COMMON_DIALOG_RESULT_USER_CANCELED;
static CommerceDialogState g_state;
static CommerceDialogUi g_dialog;

s32 PS4_SYSV_ABI sceNpCommerceDialogInitialize() {
    LOG_INFO(Lib_NpCommerce, "called");
    if (g_dialog_status != Status::NONE) {
        return static_cast<s32>(Error::ALREADY_INITIALIZED);
    }
    g_dialog_status = Status::INITIALIZED;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpCommerceDialogTerminate() {
    LOG_INFO(Lib_NpCommerce, "called");
    if (g_dialog_status == Status::NONE) {
        return static_cast<s32>(Error::NOT_INITIALIZED);
    }
    g_dialog.Finish(CommerceResult::OK);
    g_dialog_status = Status::NONE;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpCommerceDialogClose() {
    LOG_INFO(Lib_NpCommerce, "called");
    switch (g_dialog_status) {
    case Status::NONE:
        return static_cast<s32>(Error::NOT_INITIALIZED);
    case Status::INITIALIZED:
        return static_cast<s32>(Error::NOT_RUNNING);
    case Status::FINISHED:
        return static_cast<s32>(Error::ALREADY_CLOSE);
    default:
        break;
    }
    g_dialog.Finish(CommerceResult::OK);
    return ORBIS_OK;
}

s8 PS4_SYSV_ABI sceNpCommerceDialogGetStatus() {
    LOG_DEBUG(Lib_NpCommerce, "called, status = {}", static_cast<u32>(g_dialog_status));
    return static_cast<s8>(g_dialog_status);
}

s32 PS4_SYSV_ABI sceNpCommerceDialogInitializeInternal() {
    LOG_INFO(Lib_NpCommerce, "called");
    return sceNpCommerceDialogInitialize();
}

s32 PS4_SYSV_ABI sceNpCommerceDialogUpdateStatus() {
    LOG_DEBUG(Lib_NpCommerce, "called, status = {}", static_cast<u32>(g_dialog_status));
    return static_cast<s32>(g_dialog_status);
}

s32 PS4_SYSV_ABI sceNpCommerceDialogGetResult(OrbisNpCommerceDialogResult* result) {
    if (result == nullptr) {
        return static_cast<s32>(Error::ARG_NULL);
    }
    if (g_dialog_status != Status::FINISHED) {
        return static_cast<s32>(Error::NOT_FINISHED);
    }
    result->result = g_result_code;
    // authorized is only meaningful in PLUS mode
    result->authorized = g_state.mode == CommerceMode::PLUS &&
                         g_result_code == ORBIS_NP_COMMERCE_DIALOG_RESULT_PURCHASED;
    result->userData = g_state.user_data;
    LOG_INFO(Lib_NpCommerce, "result={} authorized={}", result->result, result->authorized);
    return g_result_code;
}

s32 PS4_SYSV_ABI sceNpCommerceDialogOpen(const OrbisNpCommerceDialogParam* param) {
    LOG_INFO(Lib_NpCommerce, "called, param = {}", static_cast<const void*>(param));
    if (g_dialog_status != Status::INITIALIZED && g_dialog_status != Status::FINISHED) {
        LOG_WARNING(Lib_NpCommerce, "Open in bad state {}", static_cast<u32>(g_dialog_status));
        return static_cast<s32>(Error::NOT_INITIALIZED);
    }
    if (param == nullptr) {
        return static_cast<s32>(Error::ARG_NULL);
    }
    if (param->mode < ORBIS_NP_COMMERCE_DIALOG_MODE_CATEGORY ||
        param->mode > ORBIS_NP_COMMERCE_DIALOG_MODE_PLUS) {
        LOG_ERROR(Lib_NpCommerce, "SceNpCommerceDialog Parameter Error (mode={})", param->mode);
        g_result_code = static_cast<s32>(Error::PARAM_INVALID);
        g_dialog_status = Status::FINISHED;
        return static_cast<s32>(Error::PARAM_INVALID);
    }

    g_state = CommerceDialogState{};
    g_state.mode = static_cast<CommerceMode>(param->mode);
    g_state.user_id = param->userId;
    g_state.service_label = param->serviceLabel;
    g_state.user_data = param->userData;
    // features is only meaningful in PLUS mode
    g_state.features = g_state.mode == CommerceMode::PLUS ? param->features : 0;

    // Resolve the calling user's name for display
    char name_buf[Libraries::UserService::ORBIS_USER_SERVICE_MAX_USER_NAME_LENGTH + 1] = {};
    if (Libraries::UserService::sceUserServiceGetUserName(param->userId, name_buf,
                                                          sizeof(name_buf)) >= 0) {
        g_state.username = name_buf;
    }

    if (param->numTargets > ORBIS_NP_COMMERCE_DIALOG_NUM_TARGETS_MAX) {
        LOG_WARNING(Lib_NpCommerce, "numTargets {} exceeds max {}, clamping", param->numTargets,
                    ORBIS_NP_COMMERCE_DIALOG_NUM_TARGETS_MAX);
    }
    const u32 count = std::min(param->numTargets, ORBIS_NP_COMMERCE_DIALOG_NUM_TARGETS_MAX);
    if (count > 0 && param->targets != nullptr) {
        g_state.targets.reserve(count);
        for (u32 i = 0; i < count; ++i) {
            const char* label = param->targets[i];
            if (label != nullptr) {
                g_state.targets.emplace_back(label, strnlen(label, MAX_LABEL_LEN));
            }
        }
    }
    LOG_INFO(Lib_NpCommerce,
             "open: mode={} user_id={} serviceLabel={:#x} numTargets={} features={:#x} first='{}'",
             param->mode, g_state.user_id, g_state.service_label, param->numTargets,
             g_state.features, g_state.targets.empty() ? "" : g_state.targets.front());

    g_dialog_status = Status::RUNNING;
    g_result_code = ORBIS_COMMON_DIALOG_RESULT_USER_CANCELED;
    g_dialog = CommerceDialogUi(&g_state, &g_dialog_status, &g_result_code);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpCommerceHidePsStoreIcon() {
    LOG_INFO(Lib_NpCommerce, "called");
    PsStoreIconLayer::Instance().Hide();
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpCommerceSetPsStoreIconLayout(s32 layout) {
    LOG_INFO(Lib_NpCommerce, "called, layout = {}", layout);
    if (layout < 0 || layout > 2) {
        LOG_WARNING(Lib_NpCommerce, "invalid layout {}, ignoring", layout);
        return ORBIS_OK;
    }
    PsStoreIconLayer::Instance().SetLayout(static_cast<PsStoreIconLayout>(layout));
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceNpCommerceShowPsStoreIcon(s32 pos) {
    LOG_INFO(Lib_NpCommerce, "called, pos = {}", pos);
    const auto p =
        (pos >= 0 && pos <= 2) ? static_cast<PsStoreIconPos>(pos) : PsStoreIconPos::CENTER;
    PsStoreIconLayer::Instance().Show(p);
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
