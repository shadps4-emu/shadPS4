// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/system/commondialog.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::SaveData::Dialog {

struct OrbisSaveDataDialogParam;
struct OrbisSaveDataDialogResult;
enum class OrbisSaveDataDialogProgressBarTarget : u32;

CommonDialog::Error PS4_SYSV_ABI sceSaveDataDialogClose();
CommonDialog::Error PS4_SYSV_ABI sceSaveDataDialogGetResult(OrbisSaveDataDialogResult* result);
CommonDialog::Status PS4_SYSV_ABI sceSaveDataDialogGetStatus();
CommonDialog::Error PS4_SYSV_ABI sceSaveDataDialogInitialize();
s32 PS4_SYSV_ABI sceSaveDataDialogIsReadyToDisplay();
CommonDialog::Error PS4_SYSV_ABI sceSaveDataDialogOpen(const OrbisSaveDataDialogParam* param);
CommonDialog::Error PS4_SYSV_ABI
sceSaveDataDialogProgressBarInc(OrbisSaveDataDialogProgressBarTarget target, u32 delta);
CommonDialog::Error PS4_SYSV_ABI
sceSaveDataDialogProgressBarSetValue(OrbisSaveDataDialogProgressBarTarget target, u32 rate);
CommonDialog::Error PS4_SYSV_ABI sceSaveDataDialogTerminate();
CommonDialog::Status PS4_SYSV_ABI sceSaveDataDialogUpdateStatus();

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::SaveData::Dialog
