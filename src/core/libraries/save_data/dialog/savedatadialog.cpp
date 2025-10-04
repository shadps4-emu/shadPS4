// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <magic_enum/magic_enum.hpp>

#include "common/elf_info.h"
#include "common/logging/log.h"
#include "core/libraries/libs.h"
#include "core/libraries/system/commondialog.h"
#include "savedatadialog.h"
#include "savedatadialog_ui.h"

namespace Libraries::SaveData::Dialog {

using CommonDialog::Error;
using CommonDialog::Result;
using CommonDialog::Status;

static auto g_status = Status::NONE;
static SaveDialogState g_state{};
static SaveDialogResult g_result{};
static SaveDialogUi g_save_dialog_ui;

Error PS4_SYSV_ABI sceSaveDataDialogClose() {
    LOG_DEBUG(Lib_SaveDataDialog, "called");
    if (g_status != Status::RUNNING) {
        return Error::NOT_RUNNING;
    }
    g_save_dialog_ui.Finish(ButtonId::INVALID);
    g_save_dialog_ui = SaveDialogUi{};
    return Error::OK;
}

Error PS4_SYSV_ABI sceSaveDataDialogGetResult(OrbisSaveDataDialogResult* result) {
    LOG_DEBUG(Lib_SaveDataDialog, "called");
    if (g_status != Status::FINISHED) {
        return Error::NOT_FINISHED;
    }
    if (result == nullptr) {
        return Error::ARG_NULL;
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
    return 1;
}

Error PS4_SYSV_ABI sceSaveDataDialogOpen(const OrbisSaveDataDialogParam* param) {
    if (g_status != Status::INITIALIZED && g_status != Status::FINISHED) {
        LOG_INFO(Lib_SaveDataDialog, "called without initialize");
        return Error::INVALID_STATE;
    }
    if (param == nullptr) {
        LOG_DEBUG(Lib_SaveDataDialog, "called param:(NULL)");
        return Error::ARG_NULL;
    }
    LOG_DEBUG(Lib_SaveDataDialog, "called param->mode: {}", magic_enum::enum_name(param->mode));
    ASSERT(param->size == sizeof(OrbisSaveDataDialogParam));
    ASSERT(param->baseParam.size == sizeof(CommonDialog::BaseParam));
    g_result = {};
    g_state = SaveDialogState{*param};
    g_status = Status::RUNNING;
    g_save_dialog_ui = SaveDialogUi(&g_state, &g_status, &g_result);
    return Error::OK;
}

Error PS4_SYSV_ABI sceSaveDataDialogProgressBarInc(OrbisSaveDataDialogProgressBarTarget target,
                                                   u32 delta) {
    LOG_DEBUG(Lib_SaveDataDialog, "called");
    if (g_status != Status::RUNNING) {
        return Error::NOT_RUNNING;
    }
    if (g_state.GetMode() != SaveDataDialogMode::PROGRESS_BAR) {
        return Error::NOT_SUPPORTED;
    }
    if (target != OrbisSaveDataDialogProgressBarTarget::DEFAULT) {
        return Error::PARAM_INVALID;
    }
    g_state.GetState<SaveDialogState::ProgressBarState>().progress += delta;
    return Error::OK;
}

Error PS4_SYSV_ABI sceSaveDataDialogProgressBarSetValue(OrbisSaveDataDialogProgressBarTarget target,
                                                        u32 rate) {
    LOG_DEBUG(Lib_SaveDataDialog, "called");
    if (g_status != Status::RUNNING) {
        return Error::NOT_RUNNING;
    }
    if (g_state.GetMode() != SaveDataDialogMode::PROGRESS_BAR) {
        return Error::NOT_SUPPORTED;
    }
    if (target != OrbisSaveDataDialogProgressBarTarget::DEFAULT) {
        return Error::PARAM_INVALID;
    }
    g_state.GetState<SaveDialogState::ProgressBarState>().progress = rate;
    return Error::OK;
}

Error PS4_SYSV_ABI sceSaveDataDialogTerminate() {
    LOG_DEBUG(Lib_SaveDataDialog, "called");
    if (g_status == Status::RUNNING) {
        sceSaveDataDialogClose();
    }
    if (g_status == Status::NONE) {
        return Error::NOT_INITIALIZED;
    }
    g_save_dialog_ui = SaveDialogUi{};
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
};

} // namespace Libraries::SaveData::Dialog
