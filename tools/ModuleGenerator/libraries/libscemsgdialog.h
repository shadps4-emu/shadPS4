// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "library_common.h" 

namespace Libraries::MsgDialog{

int PS4_SYSV_ABI sceMsgDialogClose();
int PS4_SYSV_ABI sceMsgDialogGetResult();
int PS4_SYSV_ABI sceMsgDialogGetStatus();
int PS4_SYSV_ABI sceMsgDialogInitialize();
int PS4_SYSV_ABI sceMsgDialogOpen();
int PS4_SYSV_ABI sceMsgDialogProgressBarInc();
int PS4_SYSV_ABI sceMsgDialogProgressBarSetMsg();
int PS4_SYSV_ABI sceMsgDialogProgressBarSetValue();
int PS4_SYSV_ABI sceMsgDialogTerminate();
int PS4_SYSV_ABI sceMsgDialogUpdateStatus();

void RegisterlibSceMsgDialog(Core::Loader::SymbolsResolver * sym);
}