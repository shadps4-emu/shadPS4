// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/system/commondialog.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::PlayGo::Dialog {

struct OrbisPlayGoDialogParam {
    CommonDialog::BaseParam baseParam;
    s32 size;
    u8 unk[0x30];
};
static_assert(sizeof(OrbisPlayGoDialogParam) == 0x68);

struct OrbisPlayGoDialogResult {
    u8 unk1[0x4];
    CommonDialog::Result result;
    u8 unk2[0x20];
};
static_assert(sizeof(OrbisPlayGoDialogResult) == 0x28);

CommonDialog::Error PS4_SYSV_ABI scePlayGoDialogClose();
CommonDialog::Error PS4_SYSV_ABI scePlayGoDialogGetResult(OrbisPlayGoDialogResult* result);
CommonDialog::Status PS4_SYSV_ABI scePlayGoDialogGetStatus();
CommonDialog::Error PS4_SYSV_ABI scePlayGoDialogInitialize();
CommonDialog::Error PS4_SYSV_ABI scePlayGoDialogOpen(const OrbisPlayGoDialogParam* param);
CommonDialog::Error PS4_SYSV_ABI scePlayGoDialogTerminate();
CommonDialog::Status PS4_SYSV_ABI scePlayGoDialogUpdateStatus();

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::PlayGo::Dialog
