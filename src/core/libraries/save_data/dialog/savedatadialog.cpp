// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>
#include <magic_enum/magic_enum.hpp>

#include "common/logging/log.h"
#include "core/libraries/libs.h"
#include "core/libraries/save_data/save_instance.h"
#include "core/libraries/system/commondialog.h"
#include "savedatadialog.h"
#include "savedatadialog_ui.h"

namespace Libraries::SaveData::Dialog {

using CommonDialog::Error;
using CommonDialog::Status;

static auto g_status = Status::NONE;
static SaveDialogState g_state{};
static SaveDialogResult g_result{};
static SaveDialogUi g_save_dialog_ui;

namespace {

template <size_t N>
bool IsZero(const std::array<u8, N>& arr) {
    for (u8 b : arr) {
        if (b != 0) {
            return false;
        }
    }
    return true;
}

Error ValidateDialogParam(const OrbisSaveDataDialogParam* param) {
    if (param == nullptr) {
        return Error::ARG_NULL;
    }
    if (param->size != sizeof(OrbisSaveDataDialogParam) ||
        param->baseParam.size != sizeof(CommonDialog::BaseParam) || !IsZero(param->_reserved)) {
        return Error::PARAM_INVALID;
    }
    switch (param->dispType) {
    case OrbisSaveDataDialogType::SAVE:
    case OrbisSaveDataDialogType::LOAD:
    case OrbisSaveDataDialogType::DELETE:
        break;
    default:
        return Error::PARAM_INVALID;
    }
    if (param->animParam != nullptr) {
        if ((param->animParam->userOK != OrbisSaveDataDialogAnimation::ON &&
             param->animParam->userOK != OrbisSaveDataDialogAnimation::OFF) ||
            (param->animParam->userCancel != OrbisSaveDataDialogAnimation::ON &&
             param->animParam->userCancel != OrbisSaveDataDialogAnimation::OFF) ||
            !IsZero(param->animParam->_reserved)) {
            return Error::PARAM_INVALID;
        }
    }
    if (param->optionParam != nullptr) {
        if ((param->optionParam->back != OrbisSaveDataDialogOptionBack::ENABLE &&
             param->optionParam->back != OrbisSaveDataDialogOptionBack::DISABLE) ||
            !IsZero(param->optionParam->_reserved)) {
            return Error::PARAM_INVALID;
        }
    }
    if (param->items != nullptr) {
        const auto* items = param->items;
        if (items->userId <= 0 || !IsZero(items->_reserved)) {
            return Error::PARAM_INVALID;
        }
        if (static_cast<u32>(items->focusPos) >
                static_cast<u32>(OrbisSaveDataDialogFocusPos::DIRNAME) ||
            static_cast<u32>(items->itemStyle) >
                static_cast<u32>(OrbisSaveDataDialogItemStyle::TITLE_DATESIZE)) {
            return Error::PARAM_INVALID;
        }
        if (items->focusPos == OrbisSaveDataDialogFocusPos::DIRNAME &&
            (items->focusPosDirName == nullptr ||
             strnlen(items->focusPosDirName->data, OrbisSaveDataDirnameDataMaxsize) == 0)) {
            return Error::PARAM_INVALID;
        }
        if (items->newItem != nullptr) {
            if (param->dispType != OrbisSaveDataDialogType::SAVE ||
                !IsZero(items->newItem->_reserved) ||
                items->newItem->iconSize > SaveInstance::IconMaxBytes ||
                (items->newItem->iconSize > 0 && items->newItem->iconBuf == nullptr)) {
                return Error::PARAM_INVALID;
            }
        }
    }

    switch (param->mode) {
    case OrbisSaveDataDialogMode::LIST:
        if (param->items == nullptr) {
            return Error::PARAM_INVALID;
        }
        break;
    case OrbisSaveDataDialogMode::USER_MSG:
        if (param->userMsgParam == nullptr || !IsZero(param->userMsgParam->_reserved) ||
            static_cast<u32>(param->userMsgParam->buttonType) >
                static_cast<u32>(OrbisSaveDataDialogButtonType::OKCANCEL) ||
            static_cast<u32>(param->userMsgParam->msgType) >
                static_cast<u32>(OrbisSaveDataDialogUserMessageType::ERROR) ||
            param->userMsgParam->msg == nullptr) {
            return Error::PARAM_INVALID;
        }
        break;
    case OrbisSaveDataDialogMode::SYSTEM_MSG:
        if (param->sysMsgParam == nullptr || !IsZero(param->sysMsgParam->_reserved)) {
            return Error::PARAM_INVALID;
        }
        break;
    case OrbisSaveDataDialogMode::ERROR_CODE:
        if (param->errorCodeParam == nullptr || !IsZero(param->errorCodeParam->_reserved)) {
            return Error::PARAM_INVALID;
        }
        break;
    case OrbisSaveDataDialogMode::PROGRESS_BAR:
        if (param->progressBarParam == nullptr || !IsZero(param->progressBarParam->_reserved) ||
            param->progressBarParam->barType != OrbisSaveDataDialogProgressBarType::PERCENTAGE ||
            static_cast<u32>(param->progressBarParam->sysMsgType) >
                static_cast<u32>(OrbisSaveDataDialogProgressSystemMessageType::RESTORE)) {
            return Error::PARAM_INVALID;
        }
        break;
    default:
        return Error::PARAM_INVALID;
    }

    return Error::OK;
}

void CloseUiInternal() {
    g_save_dialog_ui.Finish(OrbisSaveDataDialogButtonId::INVALID);
    g_save_dialog_ui = SaveDialogUi{};
}

} // namespace

const SaveDialogState& GetDialogStateForTest() {
    return g_state;
}

void ResetDialogForTest() {
    if (g_status == Status::RUNNING) {
        CloseUiInternal();
    }
    g_save_dialog_ui = SaveDialogUi{};
    g_state = SaveDialogState{};
    g_result = SaveDialogResult{};
    g_status = Status::NONE;
    CommonDialog::g_isUsed = false;
}

Error PS4_SYSV_ABI sceSaveDataDialogClose(const OrbisSaveDataDialogCloseParam* closeParam) {
    LOG_DEBUG(Lib_SaveDataDialog, "called");
    if (g_status == Status::NONE) {
        return Error::NOT_INITIALIZED;
    }
    if (closeParam == nullptr) {
        return Error::ARG_NULL;
    }
    if (g_status != Status::RUNNING) {
        return Error::NOT_RUNNING;
    }
    if ((closeParam->anim != OrbisSaveDataDialogAnimation::ON &&
         closeParam->anim != OrbisSaveDataDialogAnimation::OFF) ||
        !IsZero(closeParam->_reserved)) {
        return Error::PARAM_INVALID;
    }
    CloseUiInternal();
    return Error::OK;
}

Error PS4_SYSV_ABI sceSaveDataDialogGetResult(OrbisSaveDataDialogResult* result) {
    LOG_DEBUG(Lib_SaveDataDialog, "called");
    if (g_status == Status::NONE) {
        return Error::NOT_INITIALIZED;
    }
    if (result == nullptr) {
        return Error::ARG_NULL;
    }
    if (g_status != Status::FINISHED) {
        return Error::NOT_FINISHED;
    }
    if (!IsZero(result->_reserved)) {
        return Error::PARAM_INVALID;
    }
    g_result.CopyTo(*result);
    return Error::OK;
}

Status PS4_SYSV_ABI sceSaveDataDialogGetStatus() {
    LOG_TRACE(Lib_SaveDataDialog, "called status={}", magic_enum::enum_name(g_status));
    return g_status;
}

Error PS4_SYSV_ABI sceSaveDataDialogInitialize() {
    LOG_DEBUG(Lib_SaveDataDialog, "called");
    if (!CommonDialog::g_isInitialized) {
        return Error::NOT_SYSTEM_INITIALIZED;
    }
    if (g_status != Status::NONE) {
        return Error::ALREADY_INITIALIZED;
    }
    if (CommonDialog::g_isUsed) {
        return Error::BUSY;
    }
    g_status = Status::INITIALIZED;
    CommonDialog::g_isUsed = true;
    return Error::OK;
}

s32 PS4_SYSV_ABI sceSaveDataDialogIsReadyToDisplay() {
    if (g_status == Status::NONE) {
        return static_cast<s32>(Error::NOT_INITIALIZED);
    }
    if (g_status != Status::RUNNING) {
        return static_cast<s32>(Error::INVALID_STATE);
    }
    return 1;
}

Error PS4_SYSV_ABI sceSaveDataDialogOpen(const OrbisSaveDataDialogParam* param) {
    if (g_status == Status::NONE) {
        return Error::NOT_INITIALIZED;
    }
    if (g_status != Status::INITIALIZED && g_status != Status::FINISHED) {
        return Error::INVALID_STATE;
    }
    if (const Error err = ValidateDialogParam(param); err != Error::OK) {
        return err;
    }
    LOG_DEBUG(Lib_SaveDataDialog, "called param->mode: {}", magic_enum::enum_name(param->mode));
    g_result = {};
    g_state = SaveDialogState{*param};
    g_status = Status::RUNNING;
    g_save_dialog_ui = SaveDialogUi(&g_state, &g_status, &g_result);
    return Error::OK;
}

Error PS4_SYSV_ABI sceSaveDataDialogProgressBarInc(OrbisSaveDataDialogProgressBarTarget target,
                                                   u32 delta) {
    LOG_DEBUG(Lib_SaveDataDialog, "called");
    if (g_status == Status::NONE) {
        return Error::NOT_INITIALIZED;
    }
    if (g_status != Status::RUNNING) {
        return Error::NOT_RUNNING;
    }
    if (g_state.GetMode() != OrbisSaveDataDialogMode::PROGRESS_BAR) {
        return Error::NOT_SUPPORTED;
    }
    if (target != OrbisSaveDataDialogProgressBarTarget::DEFAULT) {
        return Error::PARAM_INVALID;
    }
    auto& prog = g_state.GetState<SaveDialogState::ProgressBarState>().progress;
    prog = std::min<u32>(prog + delta, 100U);
    return Error::OK;
}

Error PS4_SYSV_ABI sceSaveDataDialogProgressBarSetValue(OrbisSaveDataDialogProgressBarTarget target,
                                                        u32 rate) {
    LOG_DEBUG(Lib_SaveDataDialog, "called");
    if (g_status == Status::NONE) {
        return Error::NOT_INITIALIZED;
    }
    if (g_status != Status::RUNNING) {
        return Error::NOT_RUNNING;
    }
    if (g_state.GetMode() != OrbisSaveDataDialogMode::PROGRESS_BAR) {
        return Error::NOT_SUPPORTED;
    }
    if (target != OrbisSaveDataDialogProgressBarTarget::DEFAULT) {
        return Error::PARAM_INVALID;
    }
    g_state.GetState<SaveDialogState::ProgressBarState>().progress = std::min<u32>(rate, 100U);
    return Error::OK;
}

Error PS4_SYSV_ABI sceSaveDataDialogTerminate() {
    LOG_DEBUG(Lib_SaveDataDialog, "called");
    if (g_status == Status::NONE) {
        return Error::NOT_INITIALIZED;
    }
    if (g_status == Status::RUNNING) {
        CloseUiInternal();
    }
    g_save_dialog_ui = SaveDialogUi{};
    g_state = SaveDialogState{};
    g_result = SaveDialogResult{};
    g_status = Status::NONE;
    CommonDialog::g_isUsed = false;
    return Error::OK;
}

Status PS4_SYSV_ABI sceSaveDataDialogUpdateStatus() {
    LOG_TRACE(Lib_SaveDataDialog, "called status={}", magic_enum::enum_name(g_status));
    return g_status;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("fH46Lag88XY", "libSceSaveDataDialog", 1, "libSceSaveDataDialog",
                 sceSaveDataDialogClose);
    LIB_FUNCTION("yEiJ-qqr6Cg", "libSceSaveDataDialog", 1, "libSceSaveDataDialog",
                 sceSaveDataDialogGetResult);
    LIB_FUNCTION("ERKzksauAJA", "libSceSaveDataDialog", 1, "libSceSaveDataDialog",
                 sceSaveDataDialogGetStatus);
    LIB_FUNCTION("s9e3+YpRnzw", "libSceSaveDataDialog", 1, "libSceSaveDataDialog",
                 sceSaveDataDialogInitialize);
    LIB_FUNCTION("en7gNVnh878", "libSceSaveDataDialog", 1, "libSceSaveDataDialog",
                 sceSaveDataDialogIsReadyToDisplay);
    LIB_FUNCTION("4tPhsP6FpDI", "libSceSaveDataDialog", 1, "libSceSaveDataDialog",
                 sceSaveDataDialogOpen);
    LIB_FUNCTION("V-uEeFKARJU", "libSceSaveDataDialog", 1, "libSceSaveDataDialog",
                 sceSaveDataDialogProgressBarInc);
    LIB_FUNCTION("hay1CfTmLyA", "libSceSaveDataDialog", 1, "libSceSaveDataDialog",
                 sceSaveDataDialogProgressBarSetValue);
    LIB_FUNCTION("YuH2FA7azqQ", "libSceSaveDataDialog", 1, "libSceSaveDataDialog",
                 sceSaveDataDialogTerminate);
    LIB_FUNCTION("KK3Bdg1RWK0", "libSceSaveDataDialog", 1, "libSceSaveDataDialog",
                 sceSaveDataDialogUpdateStatus);
}

} // namespace Libraries::SaveData::Dialog
