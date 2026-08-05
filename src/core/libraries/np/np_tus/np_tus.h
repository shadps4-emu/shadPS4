// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <core/libraries/rtc/rtc.h>
#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Np::NpTus {

enum OrbisNpTusOpeType : s32 {
    ORBIS_NP_TUS_OPETYPE_EQUAL = 1,
    ORBIS_NP_TUS_OPETYPE_NOT_EQUAL = 2,
    ORBIS_NP_TUS_OPETYPE_GREATER_THAN = 3,
    ORBIS_NP_TUS_OPETYPE_GREATER_OR_EQUAL = 4,
    ORBIS_NP_TUS_OPETYPE_LESS_THAN = 5,
    ORBIS_NP_TUS_OPETYPE_LESS_OR_EQUAL = 6,
};

enum OrbisNpTusDataStatusSortType : s32 {
    ORBIS_NP_TUS_DATASTATUS_SORTTYPE_DESCENDING_DATE = 1,
    ORBIS_NP_TUS_DATASTATUS_SORTTYPE_ASCENDING_DATE = 2,
};

enum OrbisNpTusVariableSortType : s32 {
    ORBIS_NP_TUS_VARIABLE_SORTTYPE_DESCENDING_DATE = 1,
    ORBIS_NP_TUS_VARIABLE_SORTTYPE_ASCENDING_DATE = 2,
    ORBIS_NP_TUS_VARIABLE_SORTTYPE_DESCENDING_VALUE = 3,
    ORBIS_NP_TUS_VARIABLE_SORTTYPE_ASCENDING_VALUE = 4,
};

struct OrbisNpTusVirtualUserId {
    char data[ORBIS_NP_ONLINEID_MAX_LENGTH];
    char term;
    char dummy[3];
};

enum class OrbisNpTssStatus : int {
    Ok = 0,
    Partial = 1,
    NotModified = 2,
};

struct OrbisNpTssDataStatus {
    Libraries::Rtc::OrbisRtcTick modified;
    OrbisNpTssStatus status;
    u64 contentLength;
};

enum class OrbisNpTssIfType : int {
    IfModifiedSince = 0,
    IfRange = 1,
};

struct OrbisNpTssIfModifiedSinceParam {
    OrbisNpTssIfType ifType;
    s8 padding[4];
    Libraries::Rtc::OrbisRtcTick lastModified;
};

struct OrbisNpTssGetDataOptParam {
    u64 size;
    u64* offset;
    u64* lastByte;
    OrbisNpTssIfModifiedSinceParam* ifParam;
};

struct OrbisNpTusDataInfo {
    u64 size;
    u8 data[384];
};

struct OrbisNpTusDataStatus {
    OrbisNpId npId;
    int set;
    Libraries::Rtc::OrbisRtcTick lastChanged;
    OrbisNpId lastChangedAuthor;
    u8 pad2[4];
    void* data;
    u64 dataSize;
    OrbisNpTusDataInfo info; // recheck
};

struct OrbisNpTusDataStatusA {
    OrbisNpOnlineId onlineId;
    u8 pad[16];
    int set;
    Libraries::Rtc::OrbisRtcTick lastChanged;
    OrbisNpOnlineId lastChangedAuthor;
    u8 pad2[20];
    void* data;
    u64 dataSize;
    OrbisNpTusDataInfo info;
    OrbisNpAccountId owner;
    OrbisNpAccountId lastChangedAuthorId;
    u8 pad3[16];
};

#pragma pack(push, 4)
struct OrbisNpTusVariable {
    OrbisNpId ownerId;
    s32 hasData;
    u8 pad[4];
    Libraries::Rtc::OrbisRtcTick lastChangedDate;
    OrbisNpId lastChangedAuthorId;
    s64 variable;
    s64 oldVariable;
    OrbisNpAccountId ownerAccountId;
    OrbisNpAccountId lastChangedAuthorAccountId;
};
#pragma pack(pop)

struct OrbisNpTusVariableA {
    OrbisNpOnlineId ownerId;
    u8 reserved1[16];
    s32 hasData;
    Libraries::Rtc::OrbisRtcTick lastChangedDate;
    u8 pad[4];
    OrbisNpOnlineId lastChangedAuthorId;
    u8 reserved2[16];
    s64 variable;
    s64 oldVariable;
    OrbisNpAccountId ownerAccountId;
    OrbisNpAccountId lastChangedAuthorAccountId;
};

struct OrbisNpTusVariableForCrossSave {
    OrbisNpId ownerId;
    s32 hasData;
    Libraries::Rtc::OrbisRtcTick lastChangedDate;
    u8 pad[4];
    OrbisNpId lastChangedAuthorId;
    s64 variable;
    s64 oldVariable;
    OrbisNpAccountId ownerAccountId;
    OrbisNpAccountId lastChangedAuthorAccountId;
};

struct OrbisNpTusDataStatusForCrossSave {
    OrbisNpId ownerId;
    s32 hasData;
    Libraries::Rtc::OrbisRtcTick lastChangedDate;
    OrbisNpId lastChangedAuthorId;
    u8 pad[4];
    void* data;
    u64 dataSize;
    OrbisNpTusDataInfo info;
    OrbisNpAccountId ownerAccountId;
    OrbisNpAccountId lastChangedAuthorAccountId;
    u8 reserved[16];
};

s32 PS4_SYSV_ABI sceNpTssCreateNpTitleCtx(OrbisNpServiceLabel serviceLabel, OrbisNpId* npId);
s32 PS4_SYSV_ABI sceNpTssCreateNpTitleCtxA(OrbisNpServiceLabel serviceLabel,
                                           Libraries::UserService::OrbisUserServiceUserId userId);
s32 PS4_SYSV_ABI sceNpTssGetData();
s32 PS4_SYSV_ABI sceNpTssGetDataAsync(int reqId, s32 slotId, OrbisNpTssDataStatus* dataStatus,
                                      u64 dataStatusSize, void* data, u64 dataSize,
                                      OrbisNpTssGetDataOptParam* option);
s32 PS4_SYSV_ABI sceNpTssGetSmallStorage();
s32 PS4_SYSV_ABI sceNpTssGetSmallStorageAsync(int reqId);
s32 PS4_SYSV_ABI sceNpTssGetStorage();
s32 PS4_SYSV_ABI sceNpTssGetStorageAsync(int reqId);
s32 PS4_SYSV_ABI sceNpTusAbortRequest(int reqId);
s32 PS4_SYSV_ABI sceNpTusAddAndGetVariable();
s32 PS4_SYSV_ABI sceNpTusAddAndGetVariableA();
s32 PS4_SYSV_ABI sceNpTusAddAndGetVariableAAsync(int reqId);
s32 PS4_SYSV_ABI sceNpTusAddAndGetVariableAsync(int reqId);
s32 PS4_SYSV_ABI sceNpTusAddAndGetVariableAVUser();
s32 PS4_SYSV_ABI sceNpTusAddAndGetVariableAVUserAsync(int reqId);
s32 PS4_SYSV_ABI sceNpTusAddAndGetVariableForCrossSave();
s32 PS4_SYSV_ABI sceNpTusAddAndGetVariableForCrossSaveAsync(int reqId);
s32 PS4_SYSV_ABI sceNpTusAddAndGetVariableForCrossSaveVUser();
s32 PS4_SYSV_ABI sceNpTusAddAndGetVariableForCrossSaveVUserAsync(int reqId);
s32 PS4_SYSV_ABI sceNpTusAddAndGetVariableVUser();
s32 PS4_SYSV_ABI sceNpTusAddAndGetVariableVUserAsync(int reqId);
s32 PS4_SYSV_ABI sceNpTusChangeModeForOtherSaveDataOwners();
s32 PS4_SYSV_ABI sceNpTusCreateNpTitleCtx(OrbisNpServiceLabel serviceLabel, OrbisNpId* npId);
s32 PS4_SYSV_ABI sceNpTusCreateNpTitleCtxA(OrbisNpServiceLabel serviceLabel,
                                           Libraries::UserService::OrbisUserServiceUserId userId);
s32 PS4_SYSV_ABI sceNpTusCreateRequest(int libCtxId);
s32 PS4_SYSV_ABI sceNpTusCreateTitleCtx();
s32 PS4_SYSV_ABI sceNpTusDeleteMultiSlotData();
s32 PS4_SYSV_ABI sceNpTusDeleteMultiSlotDataA();
s32 PS4_SYSV_ABI sceNpTusDeleteMultiSlotDataAAsync(int reqId);
s32 PS4_SYSV_ABI sceNpTusDeleteMultiSlotDataAsync(int reqId);
s32 PS4_SYSV_ABI sceNpTusDeleteMultiSlotDataVUser();
s32 PS4_SYSV_ABI sceNpTusDeleteMultiSlotDataVUserAsync(int reqId);
s32 PS4_SYSV_ABI sceNpTusDeleteMultiSlotVariable();
s32 PS4_SYSV_ABI sceNpTusDeleteMultiSlotVariableA();
s32 PS4_SYSV_ABI sceNpTusDeleteMultiSlotVariableAAsync(int reqId);
s32 PS4_SYSV_ABI sceNpTusDeleteMultiSlotVariableAsync(int reqId);
s32 PS4_SYSV_ABI sceNpTusDeleteMultiSlotVariableVUser();
s32 PS4_SYSV_ABI sceNpTusDeleteMultiSlotVariableVUserAsync(int reqId);
s32 PS4_SYSV_ABI sceNpTusDeleteNpTitleCtx(int ctxId);
s32 PS4_SYSV_ABI sceNpTusDeleteRequest(int requestId);
s32 PS4_SYSV_ABI sceNpTusGetData();
s32 PS4_SYSV_ABI sceNpTusGetDataA();
s32 PS4_SYSV_ABI sceNpTusGetDataAAsync(int reqId);
s32 PS4_SYSV_ABI sceNpTusGetDataAsync(int reqId);
s32 PS4_SYSV_ABI sceNpTusGetDataAVUser();
s32 PS4_SYSV_ABI sceNpTusGetDataAVUserAsync(int reqId);
s32 PS4_SYSV_ABI sceNpTusGetDataForCrossSave();
s32 PS4_SYSV_ABI sceNpTusGetDataForCrossSaveAsync(int reqId);
s32 PS4_SYSV_ABI sceNpTusGetDataForCrossSaveVUser();
s32 PS4_SYSV_ABI sceNpTusGetDataForCrossSaveVUserAsync(int reqId);
s32 PS4_SYSV_ABI sceNpTusGetDataVUser();
s32 PS4_SYSV_ABI sceNpTusGetDataVUserAsync(int reqId);
s32 PS4_SYSV_ABI sceNpTusGetFriendsDataStatus();
s32 PS4_SYSV_ABI sceNpTusGetFriendsDataStatusA();
s32 PS4_SYSV_ABI sceNpTusGetFriendsDataStatusAAsync(int reqId);
s32 PS4_SYSV_ABI sceNpTusGetFriendsDataStatusAsync(int reqId);
s32 PS4_SYSV_ABI sceNpTusGetFriendsDataStatusForCrossSave();
s32 PS4_SYSV_ABI sceNpTusGetFriendsDataStatusForCrossSaveAsync(int reqId);
s32 PS4_SYSV_ABI sceNpTusGetFriendsVariable();
s32 PS4_SYSV_ABI sceNpTusGetFriendsVariableA();
s32 PS4_SYSV_ABI sceNpTusGetFriendsVariableAAsync(int reqId);
s32 PS4_SYSV_ABI sceNpTusGetFriendsVariableAsync(int reqId);
s32 PS4_SYSV_ABI sceNpTusGetFriendsVariableForCrossSave();
s32 PS4_SYSV_ABI sceNpTusGetFriendsVariableForCrossSaveAsync(int reqId);
s32 PS4_SYSV_ABI sceNpTusGetMultiSlotDataStatus();
s32 PS4_SYSV_ABI sceNpTusGetMultiSlotDataStatusA();
s32 PS4_SYSV_ABI sceNpTusGetMultiSlotDataStatusAAsync(int reqId);
s32 PS4_SYSV_ABI sceNpTusGetMultiSlotDataStatusAsync(int reqId);
s32 PS4_SYSV_ABI sceNpTusGetMultiSlotDataStatusAVUser();
s32 PS4_SYSV_ABI sceNpTusGetMultiSlotDataStatusAVUserAsync(int reqId);
s32 PS4_SYSV_ABI sceNpTusGetMultiSlotDataStatusForCrossSave();
s32 PS4_SYSV_ABI sceNpTusGetMultiSlotDataStatusForCrossSaveAsync(int reqId);
s32 PS4_SYSV_ABI sceNpTusGetMultiSlotDataStatusForCrossSaveVUser();
s32 PS4_SYSV_ABI sceNpTusGetMultiSlotDataStatusForCrossSaveVUserAsync(int reqId);
s32 PS4_SYSV_ABI sceNpTusGetMultiSlotDataStatusVUser();
s32 PS4_SYSV_ABI sceNpTusGetMultiSlotDataStatusVUserAsync(int reqId);
s32 PS4_SYSV_ABI sceNpTusGetMultiSlotVariable(int reqId, OrbisNpId* npId, s32* slotIds,
                                              OrbisNpTusVariable* variableArray, u64 variablesSize,
                                              int arrayLen, void* option);
s32 PS4_SYSV_ABI sceNpTusGetMultiSlotVariableA();
s32 PS4_SYSV_ABI sceNpTusGetMultiSlotVariableAAsync(int reqId, OrbisNpAccountId targetAccountId,
                                                    s32* slotIds,
                                                    OrbisNpTusVariableA* variableArray,
                                                    u64 variablesSize, int arrayLen, void* option);
s32 PS4_SYSV_ABI sceNpTusGetMultiSlotVariableAsync(int reqId, OrbisNpId* npId, s32* slotIds,
                                                   OrbisNpTusVariable* variableArray,
                                                   u64 variablesSize, int arrayLen, void* option);
s32 PS4_SYSV_ABI sceNpTusGetMultiSlotVariableAVUser(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId, s32* slotIds,
    OrbisNpTusVariableA* variables, u64 variablesSize, int arrayLen, void* option);
s32 PS4_SYSV_ABI sceNpTusGetMultiSlotVariableAVUserAsync(
    int reqId, const OrbisNpTusVirtualUserId* virtualUserId, s32* slotIds,
    OrbisNpTusVariableA* variableArray, u64 variablesSize, int arrayLen, void* option);
s32 PS4_SYSV_ABI sceNpTusGetMultiSlotVariableForCrossSave();
s32 PS4_SYSV_ABI sceNpTusGetMultiSlotVariableForCrossSaveAsync(int reqId);
s32 PS4_SYSV_ABI sceNpTusGetMultiSlotVariableForCrossSaveVUser();
s32 PS4_SYSV_ABI sceNpTusGetMultiSlotVariableForCrossSaveVUserAsync(int reqId);
s32 PS4_SYSV_ABI sceNpTusGetMultiSlotVariableVUser();
s32 PS4_SYSV_ABI sceNpTusGetMultiSlotVariableVUserAsync(int reqId);
s32 PS4_SYSV_ABI sceNpTusGetMultiUserDataStatus();
s32 PS4_SYSV_ABI sceNpTusGetMultiUserDataStatusA();
s32 PS4_SYSV_ABI sceNpTusGetMultiUserDataStatusAAsync(int reqId);
s32 PS4_SYSV_ABI sceNpTusGetMultiUserDataStatusAsync(int reqId);
s32 PS4_SYSV_ABI sceNpTusGetMultiUserDataStatusAVUser();
s32 PS4_SYSV_ABI sceNpTusGetMultiUserDataStatusAVUserAsync(int reqId);
s32 PS4_SYSV_ABI sceNpTusGetMultiUserDataStatusForCrossSave();
s32 PS4_SYSV_ABI sceNpTusGetMultiUserDataStatusForCrossSaveAsync(int reqId);
s32 PS4_SYSV_ABI sceNpTusGetMultiUserDataStatusForCrossSaveVUser();
s32 PS4_SYSV_ABI sceNpTusGetMultiUserDataStatusForCrossSaveVUserAsync(int reqId);
s32 PS4_SYSV_ABI sceNpTusGetMultiUserDataStatusVUser();
s32 PS4_SYSV_ABI sceNpTusGetMultiUserDataStatusVUserAsync(int reqId);
s32 PS4_SYSV_ABI sceNpTusGetMultiUserVariable();
s32 PS4_SYSV_ABI sceNpTusGetMultiUserVariableA();
s32 PS4_SYSV_ABI sceNpTusGetMultiUserVariableAAsync(int reqId);
s32 PS4_SYSV_ABI sceNpTusGetMultiUserVariableAsync(int reqId);
s32 PS4_SYSV_ABI sceNpTusGetMultiUserVariableAVUser();
s32 PS4_SYSV_ABI sceNpTusGetMultiUserVariableAVUserAsync(int reqId);
s32 PS4_SYSV_ABI sceNpTusGetMultiUserVariableForCrossSave();
s32 PS4_SYSV_ABI sceNpTusGetMultiUserVariableForCrossSaveAsync(int reqId);
s32 PS4_SYSV_ABI sceNpTusGetMultiUserVariableForCrossSaveVUser();
s32 PS4_SYSV_ABI sceNpTusGetMultiUserVariableForCrossSaveVUserAsync(int reqId);
s32 PS4_SYSV_ABI sceNpTusGetMultiUserVariableVUser();
s32 PS4_SYSV_ABI sceNpTusGetMultiUserVariableVUserAsync(int reqId);
s32 PS4_SYSV_ABI sceNpTusPollAsync(int reqId, int* result);
s32 PS4_SYSV_ABI sceNpTusSetData();
s32 PS4_SYSV_ABI sceNpTusSetDataA();
s32 PS4_SYSV_ABI sceNpTusSetDataAAsync(int reqId);
s32 PS4_SYSV_ABI sceNpTusSetDataAsync(int reqId);
s32 PS4_SYSV_ABI sceNpTusSetDataAVUser();
s32 PS4_SYSV_ABI sceNpTusSetDataAVUserAsync(int reqId);
s32 PS4_SYSV_ABI sceNpTusSetDataVUser();
s32 PS4_SYSV_ABI sceNpTusSetDataVUserAsync(int reqId);
s32 PS4_SYSV_ABI sceNpTusSetMultiSlotVariable(int reqId, OrbisNpId* npId, s32* slotIds,
                                              s64* variables, int arrayLen, void* option);
s32 PS4_SYSV_ABI sceNpTusSetMultiSlotVariableA();
s32 PS4_SYSV_ABI sceNpTusSetMultiSlotVariableAAsync(int reqId, OrbisNpAccountId targetAccountId,
                                                    s32* slotIds, const s64* variables,
                                                    int arrayLen, void* option);
s32 PS4_SYSV_ABI sceNpTusSetMultiSlotVariableAsync(int reqId, OrbisNpId* npId, s32* slotIds,
                                                   s64* variables, int arrayLen, void* option);
s32 PS4_SYSV_ABI
sceNpTusSetMultiSlotVariableVUser(int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId,
                                  s32* slotIds, s64* variables, int arrayLen, void* option);
s32 PS4_SYSV_ABI sceNpTusSetMultiSlotVariableVUserAsync(
    int reqId, const OrbisNpTusVirtualUserId* virtualUserId, s32* slotIds, const s64* variables,
    int arrayLen, void* option);
s32 PS4_SYSV_ABI sceNpTusSetThreadParam();
s32 PS4_SYSV_ABI sceNpTusSetTimeout();
s32 PS4_SYSV_ABI sceNpTusTryAndSetVariable();
s32 PS4_SYSV_ABI sceNpTusTryAndSetVariableA();
s32 PS4_SYSV_ABI sceNpTusTryAndSetVariableAAsync(int reqId);
s32 PS4_SYSV_ABI sceNpTusTryAndSetVariableAsync(int reqId);
s32 PS4_SYSV_ABI sceNpTusTryAndSetVariableAVUser();
s32 PS4_SYSV_ABI sceNpTusTryAndSetVariableAVUserAsync(int reqId);
s32 PS4_SYSV_ABI sceNpTusTryAndSetVariableForCrossSave();
s32 PS4_SYSV_ABI sceNpTusTryAndSetVariableForCrossSaveAsync(int reqId);
s32 PS4_SYSV_ABI sceNpTusTryAndSetVariableForCrossSaveVUser();
s32 PS4_SYSV_ABI sceNpTusTryAndSetVariableForCrossSaveVUserAsync(int reqId);
s32 PS4_SYSV_ABI sceNpTusTryAndSetVariableVUser();
s32 PS4_SYSV_ABI sceNpTusTryAndSetVariableVUserAsync(int reqId);
s32 PS4_SYSV_ABI sceNpTusWaitAsync(int reqId, int* result);

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Np::NpTus