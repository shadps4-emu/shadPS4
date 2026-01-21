// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <core/libraries/system/commondialog.h>
#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::WebBrowserDialog {

s32 PS4_SYSV_ABI sceWebBrowserDialogClose();
s32 PS4_SYSV_ABI sceWebBrowserDialogGetEvent();
s32 PS4_SYSV_ABI sceWebBrowserDialogGetResult();
Libraries::CommonDialog::Status PS4_SYSV_ABI sceWebBrowserDialogGetStatus();
Libraries::CommonDialog::Error PS4_SYSV_ABI sceWebBrowserDialogInitialize();
s32 PS4_SYSV_ABI sceWebBrowserDialogNavigate();
s32 PS4_SYSV_ABI sceWebBrowserDialogOpen();
s32 PS4_SYSV_ABI sceWebBrowserDialogOpenForPredeterminedContent();
s32 PS4_SYSV_ABI sceWebBrowserDialogResetCookie();
s32 PS4_SYSV_ABI sceWebBrowserDialogSetCookie();
s32 PS4_SYSV_ABI sceWebBrowserDialogSetZoom();
Libraries::CommonDialog::Error PS4_SYSV_ABI sceWebBrowserDialogTerminate();
Libraries::CommonDialog::Status PS4_SYSV_ABI sceWebBrowserDialogUpdateStatus();
s32 PS4_SYSV_ABI Func_F2BE042771625F8C();

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::WebBrowserDialog