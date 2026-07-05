// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstddef>

#include <core/libraries/np/np_types.h>
#include <core/libraries/system/commondialog.h>
#include <core/libraries/system/userservice.h>
#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::InvitationDialog {

constexpr int ORBIS_INVITATION_DIALOG_ADDRESS_USER_LIST_MAX_SIZE = 16;
constexpr int ORBIS_INVITATION_DIALOG_MAX_USER_MSG_SIZE = 512;
constexpr int ORBIS_NP_SESSION_ID_MAX_SIZE = 45;

struct OrbisInvitationDialogUserList {
    u32 count;
    struct {
        Libraries::Np::OrbisNpOnlineId onlineId;
        Libraries::Np::OrbisNpAccountId accountId;
    } users[ORBIS_INVITATION_DIALOG_ADDRESS_USER_LIST_MAX_SIZE];
};

struct SceInvitationDialogOnlineIdList {
    u32 count;
    Libraries::Np::OrbisNpOnlineId onlineId[ORBIS_INVITATION_DIALOG_ADDRESS_USER_LIST_MAX_SIZE];
};

constexpr int ORBIS_INVITATION_DIALOG_ERROR_BLOCKED_ALL_PLAYERS = 0x810A0001; // if errorCode !=0

struct OrbisInvitationDialogResultA {
    void* callbackArg;
    s32 errorCode;
    Libraries::CommonDialog::Result result;
    OrbisInvitationDialogUserList* sentUsers;
    u8 reserved[32];
};

struct OrbisInvitationDialogResult {
    void* callbackArg;
    s32 errorCode;
    Libraries::CommonDialog::Result result;
    SceInvitationDialogOnlineIdList* sentOnlineIds;
    u8 reserved[32];
};

// dialog mode
constexpr int ORBIS_INVITATION_DIALOG_MODE_INVALID = 0; // Invalid mode / initial value
constexpr int ORBIS_INVITATION_DIALOG_MODE_SEND = 1;    // Send mode
constexpr int ORBIS_INVITATION_DIALOG_MODE_RECV = 2;    // Receive mode

union OrbisInvitationDialogAddressInfoA {
    struct {
        const Libraries::Np::OrbisNpAccountId* accountIds;
        u32 accountIdsCount;
        int : 32;
    } UserSelectDisableAddress;
    struct {
        u32 userMaxCount;
    } UserSelectEnableAddress;
};

union OrbisInvitationDialogAddressInfo {
    struct {
        const Libraries::Np::OrbisNpOnlineId* onlineIds;
        u32 onlineIdsCount;
    } UserSelectDisableAddress;
    struct {
        u32 onlineIdsMaxCount;
    } UserSelectEnableAddress;
};

// addressType
constexpr int ORBIS_INVITATION_DIALOG_ADDRESS_TYPE_INVALID = 0;     // Invalid mode /initial value
constexpr int ORBIS_INVITATION_DIALOG_ADDRESS_TYPE_USERDISABLE = 1; // Recipient list editing
                                                                    // disabled
constexpr int ORBIS_INVITATION_DIALOG_ADDRESS_TYPE_USERENABLE = 2; // Recipient list editing enabled

struct OrbisInvitationDialogAddressParam {
    s32 addressType;
    OrbisInvitationDialogAddressInfo addressInfo;
};

struct OrbisInvitationDialogAddressParamA {
    s32 addressType;
    int : 32;
    OrbisInvitationDialogAddressInfoA addressInfo;
};

struct OrbisNpSessionId {
    char data[ORBIS_NP_SESSION_ID_MAX_SIZE];
    char term;
    char padding[2];
};

constexpr int ORBIS_NP_INVITATION_ID_SIZE = 60;

struct OrbisNpInvitationId {
    char data[ORBIS_NP_INVITATION_ID_SIZE];
    char term;
};

using OrbisNpSessionInvitationEventFlag = s32;
constexpr OrbisNpSessionInvitationEventFlag ORBIS_NP_SESSION_INVITATION_EVENT_FLAG_INVITATION =
    0x01;

// Delivered via SceSystemServiceEvent::data::param on ORBIS_SYSTEM_SERVICE_EVENT_SESSION_INVITATION
struct OrbisNpSessionInvitationEventParam {
    OrbisNpSessionId sessionId;
    OrbisNpInvitationId invitationId;
    OrbisNpSessionInvitationEventFlag flag;
    char padding[4];
    Libraries::Np::OrbisNpOnlineId onlineId;
};
static_assert(sizeof(OrbisNpSessionInvitationEventParam) == 0x8c,
              "SESSION_INVITATION param must be 140 bytes");
static_assert(offsetof(OrbisNpSessionInvitationEventParam, flag) == 112);
static_assert(offsetof(OrbisNpSessionInvitationEventParam, onlineId) == 120);

union OrbisInvitationDialogDataParam {
    struct {
        const char* userMessage;
        const OrbisNpSessionId* sessionId;
        OrbisInvitationDialogAddressParam addressParam;
    } SendInfo;
    struct {
        u8 reserved[64];
    } RecvInfo;
};

union OrbisInvitationDialogDataParamA {
    struct {
        const char* userMessage;
        const OrbisNpSessionId* sessionId;
        OrbisInvitationDialogAddressParamA addressParam;
    } SendInfo;
    struct {
        u8 reserved[64];
    } RecvInfo;
};

struct OrbisInvitationDialogParam {
    Libraries::CommonDialog::BaseParam baseParam;
    u32 size;
    s32 mode;
    Libraries::UserService::OrbisUserServiceUserId userId;
    void* callbackArg;
    OrbisInvitationDialogDataParam* dataParam;
    u8 reserved[64];
};

struct OrbisInvitationDialogParamA {
    Libraries::CommonDialog::BaseParam baseParam;
    u32 size;
    s32 mode;
    Libraries::UserService::OrbisUserServiceUserId userId;
    int : 32;
    void* callbackArg;
    const OrbisInvitationDialogDataParamA* dataParam;
    u8 reserved[64];
};

Libraries::CommonDialog::Error PS4_SYSV_ABI sceInvitationDialogClose();
s32 PS4_SYSV_ABI sceInvitationDialogGetResult(OrbisInvitationDialogResult* result);
s32 PS4_SYSV_ABI sceInvitationDialogGetResultA(OrbisInvitationDialogResultA* result);
Libraries::CommonDialog::Status PS4_SYSV_ABI sceInvitationDialogGetStatus();
Libraries::CommonDialog::Error PS4_SYSV_ABI sceInvitationDialogInitialize();
Libraries::CommonDialog::Error PS4_SYSV_ABI
sceInvitationDialogOpen(const OrbisInvitationDialogParam* param);
Libraries::CommonDialog::Error PS4_SYSV_ABI
sceInvitationDialogOpenA(const OrbisInvitationDialogParamA* param);
Libraries::CommonDialog::Error PS4_SYSV_ABI sceInvitationDialogTerminate();
Libraries::CommonDialog::Status PS4_SYSV_ABI sceInvitationDialogUpdateStatus();

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::InvitationDialog