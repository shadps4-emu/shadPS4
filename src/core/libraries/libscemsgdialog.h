// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "library_common.h"
#include "libscecommondialog.h"

namespace Libraries::MsgDialog {

using OrbisUserServiceUserId = s32;

enum OrbisMsgDialogMode {
    ORBIS_MSG_DIALOG_MODE_USER_MSG = 1,
    ORBIS_MSG_DIALOG_MODE_PROGRESS_BAR = 2,
    ORBIS_MSG_DIALOG_MODE_SYSTEM_MSG = 3,
};

enum OrbisMsgDialogButtonType {
    ORBIS_MSG_DIALOG_BUTTON_TYPE_OK = 0,
    ORBIS_MSG_DIALOG_BUTTON_TYPE_YESNO = 1,
    ORBIS_MSG_DIALOG_BUTTON_TYPE_NONE = 2,
    ORBIS_MSG_DIALOG_BUTTON_TYPE_OK_CANCEL = 3,
    ORBIS_MSG_DIALOG_BUTTON_TYPE_WAIT = 5,
    ORBIS_MSG_DIALOG_BUTTON_TYPE_WAIT_CANCEL = 6,
    ORBIS_MSG_DIALOG_BUTTON_TYPE_YESNO_FOCUS_NO = 7,
    ORBIS_MSG_DIALOG_BUTTON_TYPE_OK_CANCEL_FOCUS_CANCEL = 8,
    ORBIS_MSG_DIALOG_BUTTON_TYPE_2BUTTONS = 9,
};

enum OrbisMsgDialogProgressBarType {
    ORBIS_MSG_DIALOG_PROGRESSBAR_TYPE_PERCENTAGE = 0,
    ORBIS_MSG_DIALOG_PROGRESSBAR_TYPE_PERCENTAGE_CANCEL = 1,
};

enum OrbisMsgDialogSystemMessageType {
    ORBIS_MSG_DIALOG_SYSMSG_TYPE_TRC_EMPTY_STORE = 0,
    ORBIS_MSG_DIALOG_SYSMSG_TYPE_TRC_PSN_CHAT_RESTRICTION = 1,
    ORBIS_MSG_DIALOG_SYSMSG_TYPE_TRC_PSN_UGC_RESTRICTION = 2,
    ORBIS_MSG_DIALOG_SYSMSG_TYPE_CAMERA_NOT_CONNECTED = 4,
    ORBIS_MSG_DIALOG_SYSMSG_TYPE_WARNING_PROFILE_PICTURE_AND_NAME_NOT_SHARED = 5,
};

struct OrbisMsgDialogButtonsParam {
    const char* msg1;
    const char* msg2;
    char reserved[32];
};

struct OrbisMsgDialogUserMessageParam {
    OrbisMsgDialogButtonType buttonType;
    s32 : 32;
    const char* msg;
    OrbisMsgDialogButtonsParam* buttonsParam;
    char reserved[24];
};

struct OrbisMsgDialogProgressBarParam {
    OrbisMsgDialogProgressBarType barType;
    int32_t : 32;
    const char* msg;
    char reserved[64];
};

struct OrbisMsgDialogSystemMessageParam {
    OrbisMsgDialogSystemMessageType sysMsgType;
    char reserved[32];
};

struct OrbisMsgDialogParam {
    CommonDialog::OrbisCommonDialogBaseParam baseParam;
    std::size_t size;
    OrbisMsgDialogMode mode;
    s32 : 32;
    OrbisMsgDialogUserMessageParam* userMsgParam;
    OrbisMsgDialogProgressBarParam* progBarParam;
    OrbisMsgDialogSystemMessageParam* sysMsgParam;
    OrbisUserServiceUserId userId;
    char reserved[40];
    s32 : 32;
};

int PS4_SYSV_ABI sceMsgDialogClose();
int PS4_SYSV_ABI sceMsgDialogGetResult();
int PS4_SYSV_ABI sceMsgDialogGetStatus();
int PS4_SYSV_ABI sceMsgDialogInitialize();
s32 PS4_SYSV_ABI sceMsgDialogOpen(const OrbisMsgDialogParam* param);
int PS4_SYSV_ABI sceMsgDialogProgressBarInc();
int PS4_SYSV_ABI sceMsgDialogProgressBarSetMsg();
int PS4_SYSV_ABI sceMsgDialogProgressBarSetValue();
int PS4_SYSV_ABI sceMsgDialogTerminate();
int PS4_SYSV_ABI sceMsgDialogUpdateStatus();

void RegisterlibSceMsgDialog(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::MsgDialog