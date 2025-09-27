// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "core/libraries/np/np_sns_facebook_dialog.h"
#include "core/libraries/system/commondialog.h"

namespace Libraries::Np::NpSnsFacebookDialog {

Libraries::CommonDialog::Status PS4_SYSV_ABI sceNpSnsFacebookDialogUpdateStatus() {
    LOG_ERROR(Lib_NpSnsFacebookDialog, "(STUBBED) called");
    return Libraries::CommonDialog::Status::NONE;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("fjV7C8H0Y8k", "libSceNpSnsFacebookDialog", 1, "libSceNpSnsFacebookDialog",
                 sceNpSnsFacebookDialogUpdateStatus);
};

} // namespace Libraries::Np::NpSnsFacebookDialog