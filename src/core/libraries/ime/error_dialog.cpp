// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <utility>
#include <imgui.h>
#include <magic_enum/magic_enum.hpp>

#include "common/assert.h"
#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/macro.h"
#include "core/libraries/system/commondialog.h"
#include "error_dialog.h"
#include "imgui/imgui_std.h"
#include "shadps4_app.h"

namespace Libraries::ErrorDialog {

struct Param {
    s32 size;
    s32 errorCode;
    OrbisUserServiceUserId userId;
    s32 _reserved;
};

Error PS4_SYSV_ABI sceErrorDialogClose() {
    LOG_DEBUG(Lib_ErrorDialog, "called");
    if (ShadPs4App::GetInstance()->m_emulator.m_hle_layer->m_error_dialog.g_status !=
        Status::RUNNING) {
        return Error::NOT_RUNNING;
    }
    ShadPs4App::GetInstance()->m_emulator.m_hle_layer->m_error_dialog.g_dialog_ui.Finish();
    return Error::OK;
}

Status PS4_SYSV_ABI sceErrorDialogGetStatus() {
    LOG_TRACE(Lib_ErrorDialog, "called status={}", magic_enum::enum_name(g_status));
    return ShadPs4App::GetInstance()->m_emulator.m_hle_layer->m_error_dialog.g_status;
}

Error PS4_SYSV_ABI sceErrorDialogInitialize() {
    LOG_DEBUG(Lib_ErrorDialog, "called");
    if (ShadPs4App::GetInstance()->m_emulator.m_hle_layer->m_error_dialog.g_status !=
        Status::NONE) {
        return Error::ALREADY_INITIALIZED;
    }
    ShadPs4App::GetInstance()->m_emulator.m_hle_layer->m_error_dialog.g_status =
        Status::INITIALIZED;
    return Error::OK;
}

Error PS4_SYSV_ABI sceErrorDialogOpen(const Param* param) {
    if (ShadPs4App::GetInstance()->m_emulator.m_hle_layer->m_error_dialog.g_status !=
            Status::INITIALIZED &&
        ShadPs4App::GetInstance()->m_emulator.m_hle_layer->m_error_dialog.g_status !=
            Status::FINISHED) {
        LOG_INFO(Lib_ErrorDialog, "called without initialize");
        return Error::INVALID_STATE;
    }
    if (param == nullptr) {
        LOG_DEBUG(Lib_ErrorDialog, "called param:(NULL)");
        return Error::ARG_NULL;
    }
    const auto err = static_cast<u32>(param->errorCode);
    LOG_DEBUG(Lib_ErrorDialog, "called param->errorCode = {:#x}", err);
    ASSERT(param->size == sizeof(Param));

    const std::string err_message = fmt::format("An error has occurred. \nCode: {:#X}", err);
    ShadPs4App::GetInstance()->m_emulator.m_hle_layer->m_error_dialog.g_status = Status::RUNNING;
    ShadPs4App::GetInstance()->m_emulator.m_hle_layer->m_error_dialog.g_dialog_ui =
        ErrorDialogUi{&ShadPs4App::GetInstance()->m_emulator.m_hle_layer->m_error_dialog.g_status, err_message};
    return Error::OK;
}

int PS4_SYSV_ABI sceErrorDialogOpenDetail() {
    LOG_ERROR(Lib_ErrorDialog, "(STUBBED) called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceErrorDialogOpenWithReport() {
    LOG_ERROR(Lib_ErrorDialog, "(STUBBED) called");
    return ORBIS_OK;
}

Error PS4_SYSV_ABI sceErrorDialogTerminate() {
    LOG_DEBUG(Lib_ErrorDialog, "called");
    if (ShadPs4App::GetInstance()->m_emulator.m_hle_layer->m_error_dialog.g_status ==
        Status::RUNNING) {
        sceErrorDialogClose();
    }
    if (ShadPs4App::GetInstance()->m_emulator.m_hle_layer->m_error_dialog.g_status ==
        Status::NONE) {
        return Error::NOT_INITIALIZED;
    }
    ShadPs4App::GetInstance()->m_emulator.m_hle_layer->m_error_dialog.g_status = Status::NONE;
    return Error::OK;
}

Status PS4_SYSV_ABI sceErrorDialogUpdateStatus() {
    LOG_TRACE(Lib_ErrorDialog, "called status={}", magic_enum::enum_name(g_status));
    return ShadPs4App::GetInstance()->m_emulator.m_hle_layer->m_error_dialog.g_status;
}

Library::Library(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("ekXHb1kDBl0", "libSceErrorDialog", 1, "libSceErrorDialog", sceErrorDialogClose);
    LIB_FUNCTION("t2FvHRXzgqk", "libSceErrorDialog", 1, "libSceErrorDialog",
                 sceErrorDialogGetStatus);
    LIB_FUNCTION("I88KChlynSs", "libSceErrorDialog", 1, "libSceErrorDialog",
                 sceErrorDialogInitialize);
    LIB_FUNCTION("M2ZF-ClLhgY", "libSceErrorDialog", 1, "libSceErrorDialog", sceErrorDialogOpen);
    LIB_FUNCTION("jrpnVQfJYgQ", "libSceErrorDialog", 1, "libSceErrorDialog",
                 sceErrorDialogOpenDetail);
    LIB_FUNCTION("wktCiyWoDTI", "libSceErrorDialog", 1, "libSceErrorDialog",
                 sceErrorDialogOpenWithReport);
    LIB_FUNCTION("9XAxK2PMwk8", "libSceErrorDialog", 1, "libSceErrorDialog",
                 sceErrorDialogTerminate);
    LIB_FUNCTION("WWiGuh9XfgQ", "libSceErrorDialog", 1, "libSceErrorDialog",
                 sceErrorDialogUpdateStatus);
};

} // namespace Libraries::ErrorDialog