// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstddef>

#include "common/types.h"
#include "core/libraries/np/np_types.h"
#include "core/libraries/system/commondialog.h"
#include "core/libraries/system/userservice.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::GameCustomDataDialog {

constexpr u32 ORBIS_GAME_CUSTOM_DATA_DIALOG_ADDRESS_USER_LIST_MAX_SIZE = 16;
constexpr u32 ORBIS_GAME_CUSTOM_DATA_DIALOG_MAX_USER_MSG_SIZE = 512;
constexpr u32 ORBIS_GAME_CUSTOM_DATA_DIALOG_MAX_URL_LENGTH = 1024;

constexpr s32 ORBIS_GAME_CUSTOM_DATA_DIALOG_ERROR_BLOCKED_ALL_PLAYERS = 0x810B0001;

// SceGameCustomDataDialogMode
constexpr s32 ORBIS_GAME_CUSTOM_DATA_DIALOG_MODE_INVALID = 0;
constexpr s32 ORBIS_GAME_CUSTOM_DATA_DIALOG_MODE_SEND = 1;
constexpr s32 ORBIS_GAME_CUSTOM_DATA_DIALOG_MODE_RECV = 2;
constexpr s32 ORBIS_GAME_CUSTOM_DATA_DIALOG_MODE_SEND_URL = 3;

// SceGameCustomDataDialogAddressType
constexpr s32 ORBIS_GAME_CUSTOM_DATA_DIALOG_ADDRESS_TYPE_INVALID = 0;
constexpr s32 ORBIS_GAME_CUSTOM_DATA_DIALOG_ADDRESS_TYPE_USERDISABLE = 1; // app fixes recipients
constexpr s32 ORBIS_GAME_CUSTOM_DATA_DIALOG_ADDRESS_TYPE_USERENABLE = 2;  // user picks them

// Recipients by account ID (A variants).
union OrbisGameCustomDataDialogAddressInfoA {
    struct {
        const Libraries::Np::OrbisNpAccountId* accountIds;
        u32 accountIdsCount;
        int : 32;
    } UserSelectDisableAddress;
    struct {
        u32 userMaxCount;
    } UserSelectEnableAddress;
};

// Recipients by online ID (the older, non-A entry points).
union OrbisGameCustomDataDialogAddressInfo {
    struct {
        const Libraries::Np::OrbisNpOnlineId* onlineIds;
        u32 onlineIdsCount;
        int : 32;
    } UserSelectDisableAddress;
    struct {
        u32 userMaxCount;
    } UserSelectEnableAddress;
};

struct OrbisGameCustomDataDialogAddressParamA {
    s32 addressType;
    int : 32;
    OrbisGameCustomDataDialogAddressInfoA addressInfo;
};

struct OrbisGameCustomDataDialogAddressParam {
    s32 addressType;
    int : 32;
    OrbisGameCustomDataDialogAddressInfo addressInfo;
};

struct OrbisGameCustomDataDialogPlatformInfo {
    u32 count;
    int : 32;
    const char** platformName;
};

struct OrbisGameCustomDataMultiLanguageString {
    const char* language;
    const char* str;
};

template <typename AddressParam>
union OrbisGameCustomDataDialogDataParamT {
    struct {
        const char* userMessage;
        const char* dataName;
        const OrbisGameCustomDataMultiLanguageString* dataNameMultiLang;
        u32 dataNameMultiLangNum;
        int : 32;
        const char* dataDetail;
        const OrbisGameCustomDataMultiLanguageString* dataDetailMultiLang;
        u32 dataDetailMultiLangNum;
        int : 32;
        void* thumbnail;
        u32 thumbnailSize;
        int : 32;
        void* data;
        u32 dataSize;
        u8 padding[4];
        s32 expireMinutes;
        int : 32;
        OrbisGameCustomDataDialogPlatformInfo availablePlatform;
        AddressParam addressParam;
    } SendInfo;
    struct {
        const char* userMessage;
        const char* dataName;
        const OrbisGameCustomDataMultiLanguageString* dataNameMultiLang;
        u32 dataNameMultiLangNum;
        int : 32;
        const char* dataDetail;
        const OrbisGameCustomDataMultiLanguageString* dataDetailMultiLang;
        u32 dataDetailMultiLangNum;
        int : 32;
        void* thumbnail;
        u32 thumbnailSize;
        int : 32;
        const char* dataUrl;
        bool withItemId;
        u8 padding[7];
        s32 expireMinutes;
        int : 32;
        OrbisGameCustomDataDialogPlatformInfo availablePlatform;
        AddressParam addressParam;
    } SendUrlInfo;
    struct {
        u8 reserved[160];
    } RecvInfo;
};

using OrbisGameCustomDataDialogDataParamA =
    OrbisGameCustomDataDialogDataParamT<OrbisGameCustomDataDialogAddressParamA>;
using OrbisGameCustomDataDialogDataParam =
    OrbisGameCustomDataDialogDataParamT<OrbisGameCustomDataDialogAddressParam>;

template <typename DataParam>
struct OrbisGameCustomDataDialogParamT {
    Libraries::CommonDialog::BaseParam baseParam;
    u32 size;
    s32 mode;
    Libraries::UserService::OrbisUserServiceUserId userId;
    int : 32;
    void* callbackArg;
    const DataParam* dataParam;
    u8 reserved[64];
};

using OrbisGameCustomDataDialogParamA =
    OrbisGameCustomDataDialogParamT<OrbisGameCustomDataDialogDataParamA>;
using OrbisGameCustomDataDialogParam =
    OrbisGameCustomDataDialogParamT<OrbisGameCustomDataDialogDataParam>;

struct OrbisGameCustomDataDialogUserList {
    u32 count;
    struct {
        Libraries::Np::OrbisNpOnlineId onlineId;
        Libraries::Np::OrbisNpAccountId accountId;
    } users[ORBIS_GAME_CUSTOM_DATA_DIALOG_ADDRESS_USER_LIST_MAX_SIZE];
};

struct OrbisGameCustomDataDialogOnlineIdList {
    u32 count;
    Libraries::Np::OrbisNpOnlineId
        onlineId[ORBIS_GAME_CUSTOM_DATA_DIALOG_ADDRESS_USER_LIST_MAX_SIZE];
};

struct OrbisGameCustomDataDialogResultA {
    void* callbackArg;
    s32 errorCode;
    Libraries::CommonDialog::Result result;
    OrbisGameCustomDataDialogUserList* sentUsers;
    u8 reserved[32];
};

struct OrbisGameCustomDataDialogResult {
    void* callbackArg;
    s32 errorCode;
    Libraries::CommonDialog::Result result;
    OrbisGameCustomDataDialogOnlineIdList* sentOnlineIds;
    u8 reserved[32];
};

static_assert(offsetof(OrbisGameCustomDataDialogResultA, reserved) == 0x18);
static_assert(offsetof(OrbisGameCustomDataDialogResult, reserved) == 0x18);

// SceNpGameCustomDataId
using OrbisNpGameCustomDataId = u64;

// SceNpGameCustomDataEventParam: delivered in SceSystemServiceEvent::data::param with
// ORBIS_SYSTEM_SERVICE_EVENT_GAME_CUSTOM_DATA when the user chooses "Use game custom data". The
// game then reads the item through the Game Custom Data Web API by itemId.
struct OrbisNpGameCustomDataEventParam {
    OrbisNpGameCustomDataId itemId;                        // game custom data ID
    Libraries::Np::OrbisNpOnlineId onlineId;               // user who chose it
    Libraries::UserService::OrbisUserServiceUserId userId; // same user
};

Libraries::CommonDialog::Error PS4_SYSV_ABI sceGameCustomDataDialogInitialize();
Libraries::CommonDialog::Error PS4_SYSV_ABI sceGameCustomDataDialogTerminate();
Libraries::CommonDialog::Error PS4_SYSV_ABI
sceGameCustomDataDialogOpen(const OrbisGameCustomDataDialogParam* param);
Libraries::CommonDialog::Error PS4_SYSV_ABI
sceGameCustomDataDialogOpenA(const OrbisGameCustomDataDialogParamA* param);
Libraries::CommonDialog::Error PS4_SYSV_ABI sceGameCustomDataDialogClose();
Libraries::CommonDialog::Status PS4_SYSV_ABI sceGameCustomDataDialogUpdateStatus();
Libraries::CommonDialog::Status PS4_SYSV_ABI sceGameCustomDataDialogGetStatus();
s32 PS4_SYSV_ABI sceGameCustomDataDialogGetResult(OrbisGameCustomDataDialogResult* result);
s32 PS4_SYSV_ABI sceGameCustomDataDialogGetResultA(OrbisGameCustomDataDialogResultA* result);

void RegisterLib(Core::Loader::SymbolsResolver* sym);

} // namespace Libraries::GameCustomDataDialog
