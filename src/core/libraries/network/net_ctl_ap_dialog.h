// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>

#include "common/types.h"
#include "core/libraries/system/commondialog.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::NetCtlApDialog {

struct OrbisNetCtlApDialogParam {
    CommonDialog::BaseParam base_param;
    u64 size;
    std::array<u8, 0x30> reserved;
};
static_assert(sizeof(OrbisNetCtlApDialogParam) == 0x68);

struct OrbisNetCtlApDialogResult {
    CommonDialog::Result result;
    std::array<u8, 0x24> reserved;
};
static_assert(sizeof(OrbisNetCtlApDialogResult) == 0x28);

CommonDialog::Error PS4_SYSV_ABI sceNetCtlApDialogInitialize();
CommonDialog::Error PS4_SYSV_ABI sceNetCtlApDialogOpen(const OrbisNetCtlApDialogParam* param);
CommonDialog::Status PS4_SYSV_ABI sceNetCtlApDialogUpdateStatus();
CommonDialog::Error PS4_SYSV_ABI sceNetCtlApDialogGetResult(OrbisNetCtlApDialogResult* result);
CommonDialog::Error PS4_SYSV_ABI sceNetCtlApDialogTerminate();

void RegisterLib(Core::Loader::SymbolsResolver* sym);

} // namespace Libraries::NetCtlApDialog
