// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/web_browser_dialog/webbrowserdialog.h"

namespace Libraries::WebBrowserDialog {

s32 PS4_SYSV_ABI sceWebBrowserDialogClose() {
    LOG_ERROR(Lib_WebBrowserDialog, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceWebBrowserDialogGetEvent() {
    LOG_ERROR(Lib_WebBrowserDialog, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceWebBrowserDialogGetResult() {
    LOG_ERROR(Lib_WebBrowserDialog, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceWebBrowserDialogGetStatus() {
    LOG_ERROR(Lib_WebBrowserDialog, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceWebBrowserDialogInitialize() {
    LOG_ERROR(Lib_WebBrowserDialog, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceWebBrowserDialogNavigate() {
    LOG_ERROR(Lib_WebBrowserDialog, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceWebBrowserDialogOpen() {
    LOG_ERROR(Lib_WebBrowserDialog, "(STUBBED) called");
    return ORBIS_OK;
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

s32 PS4_SYSV_ABI sceWebBrowserDialogTerminate() {
    LOG_ERROR(Lib_WebBrowserDialog, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceWebBrowserDialogUpdateStatus() {
    LOG_ERROR(Lib_WebBrowserDialog, "(STUBBED) called");
    return ORBIS_OK;
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