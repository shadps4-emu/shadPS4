// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/libraries/system/commondialog.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::MsgDialog {

struct DialogResult;
struct OrbisParam;
enum class OrbisMsgDialogProgressBarTarget : u32;

CommonDialog::Error PS4_SYSV_ABI sceMsgDialogClose();
CommonDialog::Error PS4_SYSV_ABI sceMsgDialogGetResult(DialogResult* result);
CommonDialog::Status PS4_SYSV_ABI sceMsgDialogGetStatus();
CommonDialog::Error PS4_SYSV_ABI sceMsgDialogInitialize();
CommonDialog::Error PS4_SYSV_ABI sceMsgDialogOpen(const OrbisParam* param);
CommonDialog::Error PS4_SYSV_ABI sceMsgDialogProgressBarInc(OrbisMsgDialogProgressBarTarget,
                                                            u32 delta);
CommonDialog::Error PS4_SYSV_ABI sceMsgDialogProgressBarSetMsg(OrbisMsgDialogProgressBarTarget,
                                                               const char* msg);
CommonDialog::Error PS4_SYSV_ABI sceMsgDialogProgressBarSetValue(OrbisMsgDialogProgressBarTarget,
                                                                 u32 value);
CommonDialog::Error PS4_SYSV_ABI sceMsgDialogTerminate();
CommonDialog::Status PS4_SYSV_ABI sceMsgDialogUpdateStatus();

void RegisterlibSceMsgDialog(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::MsgDialog
