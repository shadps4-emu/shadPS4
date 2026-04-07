// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/web_browser_dialog/webbrowserdialog.h"
#include "magic_enum/magic_enum.hpp"

namespace Libraries::WebBrowserDialog {

static auto g_status = Libraries::CommonDialog::Status::NONE;

Libraries::CommonDialog::Error PS4_SYSV_ABI sceWebBrowserDialogClose() {
    LOG_ERROR(Lib_WebBrowserDialog, "(STUBBED) called");
    if (g_status != Libraries::CommonDialog::Status::RUNNING) {
        return Libraries::CommonDialog::Error::NOT_RUNNING;
    }
    LOG_INFO(Lib_NpProfileDialog, "TODO: close npprofile ui dialog"); // TODO close Ui dialog
    return Libraries::CommonDialog::Error::OK;
}

s32 PS4_SYSV_ABI sceWebBrowserDialogGetEvent() {
    LOG_ERROR(Lib_WebBrowserDialog, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceWebBrowserDialogGetResult() {
    LOG_ERROR(Lib_WebBrowserDialog, "(STUBBED) called");
    return ORBIS_OK;
}

Libraries::CommonDialog::Status PS4_SYSV_ABI sceWebBrowserDialogGetStatus() {
    LOG_TRACE(Lib_WebBrowserDialog, "called status={}", magic_enum::enum_name(g_status));
    return g_status;
}

Libraries::CommonDialog::Error PS4_SYSV_ABI sceWebBrowserDialogInitialize() {
    if (!CommonDialog::g_isInitialized) {
        return Libraries::CommonDialog::Error::NOT_SYSTEM_INITIALIZED;
    }
    if (g_status != Libraries::CommonDialog::Status::NONE) {
        LOG_ERROR(Lib_WebBrowserDialog, "already initialized");
        return Libraries::CommonDialog::Error::ALREADY_INITIALIZED;
    }
    if (CommonDialog::g_isUsed) {
        return Libraries::CommonDialog::Error::BUSY;
    }
    g_status = Libraries::CommonDialog::Status::INITIALIZED;
    CommonDialog::g_isUsed = true;
    return Libraries::CommonDialog::Error::OK;
}

s32 PS4_SYSV_ABI sceWebBrowserDialogNavigate() {
    LOG_ERROR(Lib_WebBrowserDialog, "(STUBBED) called");
    return ORBIS_OK;
}

Libraries::CommonDialog::Error PS4_SYSV_ABI sceWebBrowserDialogOpen() {
    if (g_status != Libraries::CommonDialog::Status::INITIALIZED &&
        g_status != Libraries::CommonDialog::Status::FINISHED) {
        LOG_INFO(Lib_WebBrowserDialog, "called without initialize");
        return Libraries::CommonDialog::Error::INVALID_STATE;
    }
    LOG_ERROR(Lib_WebBrowserDialog, "(STUBBED) called"); // TODO open ui dialog
    g_status = Libraries::CommonDialog::Status::RUNNING;
    return Libraries::CommonDialog::Error::OK;
}

s32 PS4_SYSV_ABI sceWebBrowserDialogOpenForPredeterminedContent() {
    LOG_ERROR(Lib_WebBrowserDialog, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceWebBrowserDialogResetCookie() {
    LOG_ERROR(Lib_WebBrowserDialog, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceWebBrowserDialogSetCookie() {
    LOG_ERROR(Lib_WebBrowserDialog, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceWebBrowserDialogSetZoom() {
    LOG_ERROR(Lib_WebBrowserDialog, "(STUBBED) called");
    return ORBIS_OK;
}

Libraries::CommonDialog::Error PS4_SYSV_ABI sceWebBrowserDialogTerminate() {
    if (g_status == Libraries::CommonDialog::Status::RUNNING) {
        sceWebBrowserDialogClose();
    }
    if (g_status == Libraries::CommonDialog::Status::NONE) {
        return Libraries::CommonDialog::Error::NOT_INITIALIZED;
    }
    g_status = Libraries::CommonDialog::Status::NONE;
    CommonDialog::g_isUsed = false;
    return Libraries::CommonDialog::Error::OK;
}

Libraries::CommonDialog::Status PS4_SYSV_ABI sceWebBrowserDialogUpdateStatus() {
    LOG_TRACE(Lib_WebBrowserDialog, "called status={}", magic_enum::enum_name(g_status));
    if (g_status == Libraries::CommonDialog::Status::RUNNING) {
        g_status = Libraries::CommonDialog::Status::FINISHED; // TODO removed it when implementing
                                                              // real dialog
    }
    return g_status;
}

s32 PS4_SYSV_ABI Func_F2BE042771625F8C() {
    LOG_ERROR(Lib_WebBrowserDialog, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("PSK+Eik919Q", "libSceWebBrowserDialog", 1, "libSceWebBrowserDialog",
                 sceWebBrowserDialogClose);
    LIB_FUNCTION("Wit4LjeoeX4", "libSceWebBrowserDialog", 1, "libSceWebBrowserDialog",
                 sceWebBrowserDialogGetEvent);
    LIB_FUNCTION("vCaW0fgVQmc", "libSceWebBrowserDialog", 1, "libSceWebBrowserDialog",
                 sceWebBrowserDialogGetResult);
    LIB_FUNCTION("CFTG6a8TjOU", "libSceWebBrowserDialog", 1, "libSceWebBrowserDialog",
                 sceWebBrowserDialogGetStatus);
    LIB_FUNCTION("jqb7HntFQFc", "libSceWebBrowserDialog", 1, "libSceWebBrowserDialog",
                 sceWebBrowserDialogInitialize);
    LIB_FUNCTION("uYELOMVnmNQ", "libSceWebBrowserDialog", 1, "libSceWebBrowserDialog",
                 sceWebBrowserDialogNavigate);
    LIB_FUNCTION("FraP7debcdg", "libSceWebBrowserDialog", 1, "libSceWebBrowserDialog",
                 sceWebBrowserDialogOpen);
    LIB_FUNCTION("O7dIZQrwVFY", "libSceWebBrowserDialog", 1, "libSceWebBrowserDialog",
                 sceWebBrowserDialogOpenForPredeterminedContent);
    LIB_FUNCTION("Cya+jvTtPqg", "libSceWebBrowserDialog", 1, "libSceWebBrowserDialog",
                 sceWebBrowserDialogResetCookie);
    LIB_FUNCTION("TZnDVkP91Rg", "libSceWebBrowserDialog", 1, "libSceWebBrowserDialog",
                 sceWebBrowserDialogSetCookie);
    LIB_FUNCTION("RLhKBOoNyXY", "libSceWebBrowserDialog", 1, "libSceWebBrowserDialog",
                 sceWebBrowserDialogSetZoom);
    LIB_FUNCTION("ocHtyBwHfys", "libSceWebBrowserDialog", 1, "libSceWebBrowserDialog",
                 sceWebBrowserDialogTerminate);
    LIB_FUNCTION("h1dR-t5ISgg", "libSceWebBrowserDialog", 1, "libSceWebBrowserDialog",
                 sceWebBrowserDialogUpdateStatus);
    LIB_FUNCTION("8r4EJ3FiX4w", "libSceWebBrowserDialogLimited", 1, "libSceWebBrowserDialog",
                 Func_F2BE042771625F8C);
};

} // namespace Libraries::WebBrowserDialog