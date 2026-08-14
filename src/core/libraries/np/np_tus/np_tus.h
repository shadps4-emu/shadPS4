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

struct OrbisNpTusGetFriendsVariableOptParam {
    u64 size;
    u32* startSerialRank;
    u32* hits;
};

struct OrbisNpTusGetFriendsDataStatusOptParam {
    u64 size;
    u32* startSerialRank;
    u32* hits;
};

struct OrbisNpTusDataInfo {
    u64 size;
    u8 data[384];
};

struct OrbisNpTusDataStatus {
    OrbisNpId ownerId;
    s32 hasData;
    Libraries::Rtc::OrbisRtcTick lastChangedDate;
    OrbisNpId lastChangedAuthorId;
    u8 pad[4];
    void* data;
    u64 dataSize;
    OrbisNpTusDataInfo info;
};

struct OrbisNpTusDataStatusA {
    OrbisNpOnlineId ownerId;
    u8 reserved1[16];
    s32 hasData;
    Libraries::Rtc::OrbisRtcTick lastChangedDate;
    OrbisNpOnlineId lastChangedAuthorId;
    u8 reserved2[20];
    void* data;
    u64 dataSize;
    OrbisNpTusDataInfo info;
    OrbisNpAccountId ownerAccountId;
    OrbisNpAccountId lastChangedAuthorAccountId;
    u8 reserved[16];
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
#pragma pack(pop)

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
s32 PS4_SYSV_ABI sceNpTusAddAndGetVariable(int reqId, const OrbisNpId* targetNpId, s32 slotId,
                                           s64 inVariable, const OrbisNpId* isLastChangedAuthor,
                                           const Libraries::Rtc::OrbisRtcTick* isLastChangedDate,
                                           OrbisNpTusVariable* outVariable, u64 outVariableSize,
                                           void* option);
s32 PS4_SYSV_ABI sceNpTusAddAndGetVariableA(int reqId, OrbisNpAccountId targetAccountId, s32 slotId,
                                            s64 inVariable,
                                            const OrbisNpAccountId* isLastChangedAuthor,
                                            const Libraries::Rtc::OrbisRtcTick* isLastChangedDate,
                                            OrbisNpTusVariableA* outVariable, u64 outVariableSize,
                                            void* option);
s32 PS4_SYSV_ABI sceNpTusAddAndGetVariableAAsync(
    int reqId, OrbisNpAccountId targetAccountId, s32 slotId, s64 inVariable,
    const OrbisNpAccountId* isLastChangedAuthor,
    const Libraries::Rtc::OrbisRtcTick* isLastChangedDate, OrbisNpTusVariableA* outVariable,
    u64 outVariableSize, void* option);
s32 PS4_SYSV_ABI sceNpTusAddAndGetVariableAsync(
    int reqId, const OrbisNpId* targetNpId, s32 slotId, s64 inVariable,
    const OrbisNpId* isLastChangedAuthor, const Libraries::Rtc::OrbisRtcTick* isLastChangedDate,
    OrbisNpTusVariable* outVariable, u64 outVariableSize, void* option);
s32 PS4_SYSV_ABI sceNpTusAddAndGetVariableAVUser(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId, s32 slotId, s64 inVariable,
    const OrbisNpAccountId* isLastChangedAuthor,
    const Libraries::Rtc::OrbisRtcTick* isLastChangedDate, OrbisNpTusVariableA* outVariable,
    u64 outVariableSize, void* option);
s32 PS4_SYSV_ABI sceNpTusAddAndGetVariableAVUserAsync(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId, s32 slotId, s64 inVariable,
    const OrbisNpAccountId* isLastChangedAuthor,
    const Libraries::Rtc::OrbisRtcTick* isLastChangedDate, OrbisNpTusVariableA* outVariable,
    u64 outVariableSize, void* option);
s32 PS4_SYSV_ABI sceNpTusAddAndGetVariableForCrossSave(
    int reqId, OrbisNpAccountId targetAccountId, s32 slotId, s64 inVariable,
    const OrbisNpAccountId* isLastChangedAuthor,
    const Libraries::Rtc::OrbisRtcTick* isLastChangedDate,
    OrbisNpTusVariableForCrossSave* outVariable, u64 outVariableSize, void* option);
s32 PS4_SYSV_ABI sceNpTusAddAndGetVariableForCrossSaveAsync(
    int reqId, OrbisNpAccountId targetAccountId, s32 slotId, s64 inVariable,
    const OrbisNpAccountId* isLastChangedAuthor,
    const Libraries::Rtc::OrbisRtcTick* isLastChangedDate,
    OrbisNpTusVariableForCrossSave* outVariable, u64 outVariableSize, void* option);
s32 PS4_SYSV_ABI sceNpTusAddAndGetVariableForCrossSaveVUser(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId, s32 slotId, s64 inVariable,
    const OrbisNpAccountId* isLastChangedAuthor,
    const Libraries::Rtc::OrbisRtcTick* isLastChangedDate,
    OrbisNpTusVariableForCrossSave* outVariable, u64 outVariableSize, void* option);
s32 PS4_SYSV_ABI sceNpTusAddAndGetVariableForCrossSaveVUserAsync(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId, s32 slotId, s64 inVariable,
    const OrbisNpAccountId* isLastChangedAuthor,
    const Libraries::Rtc::OrbisRtcTick* isLastChangedDate,
    OrbisNpTusVariableForCrossSave* outVariable, u64 outVariableSize, void* option);
s32 PS4_SYSV_ABI sceNpTusAddAndGetVariableVUser(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId, s32 slotId, s64 inVariable,
    const OrbisNpId* isLastChangedAuthor, const Libraries::Rtc::OrbisRtcTick* isLastChangedDate,
    OrbisNpTusVariable* outVariable, u64 outVariableSize, void* option);
s32 PS4_SYSV_ABI sceNpTusAddAndGetVariableVUserAsync(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId, s32 slotId, s64 inVariable,
    const OrbisNpId* isLastChangedAuthor, const Libraries::Rtc::OrbisRtcTick* isLastChangedDate,
    OrbisNpTusVariable* outVariable, u64 outVariableSize, void* option);
s32 PS4_SYSV_ABI sceNpTusChangeModeForOtherSaveDataOwners();
s32 PS4_SYSV_ABI sceNpTusCreateNpTitleCtx(OrbisNpServiceLabel serviceLabel, OrbisNpId* npId);
s32 PS4_SYSV_ABI sceNpTusCreateNpTitleCtxA(OrbisNpServiceLabel serviceLabel,
                                           Libraries::UserService::OrbisUserServiceUserId userId);
s32 PS4_SYSV_ABI sceNpTusCreateRequest(int libCtxId);
s32 PS4_SYSV_ABI sceNpTusCreateTitleCtx();
s32 PS4_SYSV_ABI sceNpTusDeleteMultiSlotData(int reqId, const OrbisNpId* targetNpId, s32* slotIds,
                                             int arrayLen, void* option);
s32 PS4_SYSV_ABI sceNpTusDeleteMultiSlotDataA(int reqId, OrbisNpAccountId targetAccountId,
                                              s32* slotIds, int arrayLen, void* option);
s32 PS4_SYSV_ABI sceNpTusDeleteMultiSlotDataAAsync(int reqId, OrbisNpAccountId targetAccountId,
                                                   s32* slotIds, int arrayLen, void* option);
s32 PS4_SYSV_ABI sceNpTusDeleteMultiSlotDataAsync(int reqId, const OrbisNpId* targetNpId,
                                                  s32* slotIds, int arrayLen, void* option);
s32 PS4_SYSV_ABI
sceNpTusDeleteMultiSlotDataVUser(int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId,
                                 s32* slotIds, int arrayLen, void* option);
s32 PS4_SYSV_ABI
sceNpTusDeleteMultiSlotDataVUserAsync(int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId,
                                      s32* slotIds, int arrayLen, void* option);
s32 PS4_SYSV_ABI sceNpTusDeleteMultiSlotVariable(int reqId, const OrbisNpId* targetNpId,
                                                 s32* slotIds, int arrayLen, void* option);
s32 PS4_SYSV_ABI sceNpTusDeleteMultiSlotVariableA(int reqId, OrbisNpAccountId targetAccountId,
                                                  s32* slotIds, int arrayLen, void* option);
s32 PS4_SYSV_ABI sceNpTusDeleteMultiSlotVariableAAsync(int reqId, OrbisNpAccountId targetAccountId,
                                                       s32* slotIds, int arrayLen, void* option);
s32 PS4_SYSV_ABI sceNpTusDeleteMultiSlotVariableAsync(int reqId, const OrbisNpId* targetNpId,
                                                      s32* slotIds, int arrayLen, void* option);
s32 PS4_SYSV_ABI
sceNpTusDeleteMultiSlotVariableVUser(int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId,
                                     s32* slotIds, int arrayLen, void* option);
s32 PS4_SYSV_ABI sceNpTusDeleteMultiSlotVariableVUserAsync(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId, s32* slotIds, int arrayLen,
    void* option);
s32 PS4_SYSV_ABI sceNpTusDeleteNpTitleCtx(int ctxId);
s32 PS4_SYSV_ABI sceNpTusDeleteRequest(int requestId);
s32 PS4_SYSV_ABI sceNpTusGetData(int reqId, const OrbisNpId* targetNpId, s32 slotId,
                                 OrbisNpTusDataStatus* dataStatus, u64 dataStatusSize, void* data,
                                 u64 recvSize, void* option);
s32 PS4_SYSV_ABI sceNpTusGetDataA(int reqId, OrbisNpAccountId targetAccountId, s32 slotId,
                                  OrbisNpTusDataStatusA* dataStatus, u64 dataStatusSize, void* data,
                                  u64 recvSize, void* option);
s32 PS4_SYSV_ABI sceNpTusGetDataAAsync(int reqId, OrbisNpAccountId targetAccountId, s32 slotId,
                                       OrbisNpTusDataStatusA* dataStatus, u64 dataStatusSize,
                                       void* data, u64 recvSize, void* option);
s32 PS4_SYSV_ABI sceNpTusGetDataAsync(int reqId, const OrbisNpId* targetNpId, s32 slotId,
                                      OrbisNpTusDataStatus* dataStatus, u64 dataStatusSize,
                                      void* data, u64 recvSize, void* option);
s32 PS4_SYSV_ABI sceNpTusGetDataAVUser(int reqId,
                                       const OrbisNpTusVirtualUserId* targetVirtualUserId,
                                       s32 slotId, OrbisNpTusDataStatusA* dataStatus,
                                       u64 dataStatusSize, void* data, u64 recvSize, void* option);
s32 PS4_SYSV_ABI sceNpTusGetDataAVUserAsync(int reqId,
                                            const OrbisNpTusVirtualUserId* targetVirtualUserId,
                                            s32 slotId, OrbisNpTusDataStatusA* dataStatus,
                                            u64 dataStatusSize, void* data, u64 recvSize,
                                            void* option);
s32 PS4_SYSV_ABI sceNpTusGetDataForCrossSave(int reqId, OrbisNpAccountId targetAccountId,
                                             s32 slotId,
                                             OrbisNpTusDataStatusForCrossSave* dataStatus,
                                             u64 dataStatusSize, void* data, u64 recvSize,
                                             void* option);
s32 PS4_SYSV_ABI sceNpTusGetDataForCrossSaveAsync(int reqId, OrbisNpAccountId targetAccountId,
                                                  s32 slotId,
                                                  OrbisNpTusDataStatusForCrossSave* dataStatus,
                                                  u64 dataStatusSize, void* data, u64 recvSize,
                                                  void* option);
s32 PS4_SYSV_ABI
sceNpTusGetDataForCrossSaveVUser(int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId,
                                 s32 slotId, OrbisNpTusDataStatusForCrossSave* dataStatus,
                                 u64 dataStatusSize, void* data, u64 recvSize, void* option);
s32 PS4_SYSV_ABI
sceNpTusGetDataForCrossSaveVUserAsync(int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId,
                                      s32 slotId, OrbisNpTusDataStatusForCrossSave* dataStatus,
                                      u64 dataStatusSize, void* data, u64 recvSize, void* option);
s32 PS4_SYSV_ABI sceNpTusGetDataVUser(int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId,
                                      s32 slotId, OrbisNpTusDataStatus* dataStatus,
                                      u64 dataStatusSize, void* data, u64 recvSize, void* option);
s32 PS4_SYSV_ABI sceNpTusGetDataVUserAsync(int reqId,
                                           const OrbisNpTusVirtualUserId* targetVirtualUserId,
                                           s32 slotId, OrbisNpTusDataStatus* dataStatus,
                                           u64 dataStatusSize, void* data, u64 recvSize,
                                           void* option);
s32 PS4_SYSV_ABI sceNpTusGetFriendsDataStatus(int reqId, s32 slotId, s32 includeSelf, s32 sortType,
                                              OrbisNpTusDataStatus* statusArray,
                                              u64 statusArraySize, int arrayLen, void* option);
s32 PS4_SYSV_ABI sceNpTusGetFriendsDataStatusA(int reqId, s32 slotId, s32 includeSelf, s32 sortType,
                                               OrbisNpTusDataStatusA* statusArray,
                                               u64 statusArraySize, int arrayLen,
                                               OrbisNpTusGetFriendsDataStatusOptParam* option);
s32 PS4_SYSV_ABI sceNpTusGetFriendsDataStatusAAsync(int reqId, s32 slotId, s32 includeSelf,
                                                    s32 sortType,
                                                    OrbisNpTusDataStatusA* statusArray,
                                                    u64 statusArraySize, int arrayLen,
                                                    OrbisNpTusGetFriendsDataStatusOptParam* option);
s32 PS4_SYSV_ABI sceNpTusGetFriendsDataStatusAsync(int reqId, s32 slotId, s32 includeSelf,
                                                   s32 sortType, OrbisNpTusDataStatus* statusArray,
                                                   u64 statusArraySize, int arrayLen, void* option);
s32 PS4_SYSV_ABI sceNpTusGetFriendsDataStatusForCrossSave(
    int reqId, s32 slotId, s32 includeSelf, s32 sortType,
    OrbisNpTusDataStatusForCrossSave* statusArray, u64 statusArraySize, int arrayLen,
    OrbisNpTusGetFriendsDataStatusOptParam* option);
s32 PS4_SYSV_ABI sceNpTusGetFriendsDataStatusForCrossSaveAsync(
    int reqId, s32 slotId, s32 includeSelf, s32 sortType,
    OrbisNpTusDataStatusForCrossSave* statusArray, u64 statusArraySize, int arrayLen,
    OrbisNpTusGetFriendsDataStatusOptParam* option);
s32 PS4_SYSV_ABI sceNpTusGetFriendsVariable(int reqId, s32 slotId, s32 includeSelf, s32 sortType,
                                            OrbisNpTusVariable* variableArray,
                                            u64 variableArraySize, int arrayLen, void* option);
s32 PS4_SYSV_ABI sceNpTusGetFriendsVariableA(int reqId, s32 slotId, s32 includeSelf, s32 sortType,
                                             OrbisNpTusVariableA* variableArray,
                                             u64 variableArraySize, int arrayLen,
                                             OrbisNpTusGetFriendsVariableOptParam* option);
s32 PS4_SYSV_ABI sceNpTusGetFriendsVariableAAsync(int reqId, s32 slotId, s32 includeSelf,
                                                  s32 sortType, OrbisNpTusVariableA* variableArray,
                                                  u64 variableArraySize, int arrayLen,
                                                  OrbisNpTusGetFriendsVariableOptParam* option);
s32 PS4_SYSV_ABI sceNpTusGetFriendsVariableAsync(int reqId, s32 slotId, s32 includeSelf,
                                                 s32 sortType, OrbisNpTusVariable* variableArray,
                                                 u64 variableArraySize, int arrayLen, void* option);
s32 PS4_SYSV_ABI sceNpTusGetFriendsVariableForCrossSave(
    int reqId, s32 slotId, s32 includeSelf, s32 sortType,
    OrbisNpTusVariableForCrossSave* variableArray, u64 variableArraySize, int arrayLen,
    OrbisNpTusGetFriendsVariableOptParam* option);
s32 PS4_SYSV_ABI sceNpTusGetFriendsVariableForCrossSaveAsync(
    int reqId, s32 slotId, s32 includeSelf, s32 sortType,
    OrbisNpTusVariableForCrossSave* variableArray, u64 variableArraySize, int arrayLen,
    OrbisNpTusGetFriendsVariableOptParam* option);
s32 PS4_SYSV_ABI sceNpTusGetMultiSlotDataStatus(int reqId, const OrbisNpId* targetNpId,
                                                s32* slotIds, OrbisNpTusDataStatus* statusArray,
                                                u64 statusArraySize, int arrayLen, void* option);
s32 PS4_SYSV_ABI sceNpTusGetMultiSlotDataStatusA(int reqId, OrbisNpAccountId targetAccountId,
                                                 s32* slotIds, OrbisNpTusDataStatusA* statusArray,
                                                 u64 statusArraySize, int arrayLen, void* option);
s32 PS4_SYSV_ABI sceNpTusGetMultiSlotDataStatusAAsync(int reqId, OrbisNpAccountId targetAccountId,
                                                      s32* slotIds,
                                                      OrbisNpTusDataStatusA* statusArray,
                                                      u64 statusArraySize, int arrayLen,
                                                      void* option);
s32 PS4_SYSV_ABI sceNpTusGetMultiSlotDataStatusAsync(int reqId, const OrbisNpId* targetNpId,
                                                     s32* slotIds,
                                                     OrbisNpTusDataStatus* statusArray,
                                                     u64 statusArraySize, int arrayLen,
                                                     void* option);
s32 PS4_SYSV_ABI sceNpTusGetMultiSlotDataStatusAVUser(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId, s32* slotIds,
    OrbisNpTusDataStatusA* statusArray, u64 statusArraySize, int arrayLen, void* option);
s32 PS4_SYSV_ABI sceNpTusGetMultiSlotDataStatusAVUserAsync(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId, s32* slotIds,
    OrbisNpTusDataStatusA* statusArray, u64 statusArraySize, int arrayLen, void* option);
s32 PS4_SYSV_ABI sceNpTusGetMultiSlotDataStatusForCrossSave(
    int reqId, OrbisNpAccountId targetAccountId, s32* slotIds,
    OrbisNpTusDataStatusForCrossSave* statusArray, u64 statusArraySize, int arrayLen, void* option);
s32 PS4_SYSV_ABI sceNpTusGetMultiSlotDataStatusForCrossSaveAsync(
    int reqId, OrbisNpAccountId targetAccountId, s32* slotIds,
    OrbisNpTusDataStatusForCrossSave* statusArray, u64 statusArraySize, int arrayLen, void* option);
s32 PS4_SYSV_ABI sceNpTusGetMultiSlotDataStatusForCrossSaveVUser(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId, s32* slotIds,
    OrbisNpTusDataStatusForCrossSave* statusArray, u64 statusArraySize, int arrayLen, void* option);
s32 PS4_SYSV_ABI sceNpTusGetMultiSlotDataStatusForCrossSaveVUserAsync(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId, s32* slotIds,
    OrbisNpTusDataStatusForCrossSave* statusArray, u64 statusArraySize, int arrayLen, void* option);
s32 PS4_SYSV_ABI sceNpTusGetMultiSlotDataStatusVUser(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId, s32* slotIds,
    OrbisNpTusDataStatus* statusArray, u64 statusArraySize, int arrayLen, void* option);
s32 PS4_SYSV_ABI sceNpTusGetMultiSlotDataStatusVUserAsync(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId, s32* slotIds,
    OrbisNpTusDataStatus* statusArray, u64 statusArraySize, int arrayLen, void* option);
s32 PS4_SYSV_ABI sceNpTusGetMultiSlotVariable(int reqId, OrbisNpId* npId, s32* slotIds,
                                              OrbisNpTusVariable* variableArray, u64 variablesSize,
                                              int arrayLen, void* option);
s32 PS4_SYSV_ABI sceNpTusGetMultiSlotVariableA(int reqId, OrbisNpAccountId targetAccountId,
                                               s32* slotIds, OrbisNpTusVariableA* variableArray,
                                               u64 variablesSize, int arrayLen, void* option);
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
s32 PS4_SYSV_ABI sceNpTusGetMultiSlotVariableForCrossSave(
    int reqId, OrbisNpAccountId targetAccountId, s32* slotIds,
    OrbisNpTusVariableForCrossSave* variableArray, u64 variablesSize, int arrayLen, void* option);
s32 PS4_SYSV_ABI sceNpTusGetMultiSlotVariableForCrossSaveAsync(
    int reqId, OrbisNpAccountId targetAccountId, s32* slotIds,
    OrbisNpTusVariableForCrossSave* variableArray, u64 variablesSize, int arrayLen, void* option);
s32 PS4_SYSV_ABI sceNpTusGetMultiSlotVariableForCrossSaveVUser(
    int reqId, const OrbisNpTusVirtualUserId* virtualUserId, s32* slotIds,
    OrbisNpTusVariableForCrossSave* variableArray, u64 variablesSize, int arrayLen, void* option);
s32 PS4_SYSV_ABI sceNpTusGetMultiSlotVariableForCrossSaveVUserAsync(
    int reqId, const OrbisNpTusVirtualUserId* virtualUserId, s32* slotIds,
    OrbisNpTusVariableForCrossSave* variableArray, u64 variablesSize, int arrayLen, void* option);
s32 PS4_SYSV_ABI sceNpTusGetMultiSlotVariableVUser(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId, s32* slotIds,
    OrbisNpTusVariable* variableArray, u64 variableArraySize, int arrayLen, void* option);
s32 PS4_SYSV_ABI sceNpTusGetMultiSlotVariableVUserAsync(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId, s32* slotIds,
    OrbisNpTusVariable* variableArray, u64 variableArraySize, int arrayLen, void* option);
s32 PS4_SYSV_ABI sceNpTusGetMultiUserDataStatus(int reqId, const OrbisNpId* targetNpIdArray,
                                                s32 slotId, OrbisNpTusDataStatus* statusArray,
                                                u64 statusArraySize, int arrayLen, void* option);
s32 PS4_SYSV_ABI sceNpTusGetMultiUserDataStatusA(int reqId,
                                                 const OrbisNpAccountId* targetAccountIdArray,
                                                 s32 slotId, OrbisNpTusDataStatusA* statusArray,
                                                 u64 statusArraySize, int arrayLen, void* option);
s32 PS4_SYSV_ABI sceNpTusGetMultiUserDataStatusAAsync(
    int reqId, const OrbisNpAccountId* targetAccountIdArray, s32 slotId,
    OrbisNpTusDataStatusA* statusArray, u64 statusArraySize, int arrayLen, void* option);
s32 PS4_SYSV_ABI sceNpTusGetMultiUserDataStatusAsync(int reqId, const OrbisNpId* targetNpIdArray,
                                                     s32 slotId, OrbisNpTusDataStatus* statusArray,
                                                     u64 statusArraySize, int arrayLen,
                                                     void* option);
s32 PS4_SYSV_ABI sceNpTusGetMultiUserDataStatusAVUser(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserIdArray, s32 slotId,
    OrbisNpTusDataStatusA* statusArray, u64 statusArraySize, int arrayLen, void* option);
s32 PS4_SYSV_ABI sceNpTusGetMultiUserDataStatusAVUserAsync(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserIdArray, s32 slotId,
    OrbisNpTusDataStatusA* statusArray, u64 statusArraySize, int arrayLen, void* option);
s32 PS4_SYSV_ABI sceNpTusGetMultiUserDataStatusForCrossSave(
    int reqId, const OrbisNpAccountId* targetAccountIdArray, s32 slotId,
    OrbisNpTusDataStatusForCrossSave* statusArray, u64 statusArraySize, int arrayLen, void* option);
s32 PS4_SYSV_ABI sceNpTusGetMultiUserDataStatusForCrossSaveAsync(
    int reqId, const OrbisNpAccountId* targetAccountIdArray, s32 slotId,
    OrbisNpTusDataStatusForCrossSave* statusArray, u64 statusArraySize, int arrayLen, void* option);
s32 PS4_SYSV_ABI sceNpTusGetMultiUserDataStatusForCrossSaveVUser(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserIdArray, s32 slotId,
    OrbisNpTusDataStatusForCrossSave* statusArray, u64 statusArraySize, int arrayLen, void* option);
s32 PS4_SYSV_ABI sceNpTusGetMultiUserDataStatusForCrossSaveVUserAsync(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserIdArray, s32 slotId,
    OrbisNpTusDataStatusForCrossSave* statusArray, u64 statusArraySize, int arrayLen, void* option);
s32 PS4_SYSV_ABI sceNpTusGetMultiUserDataStatusVUser(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserIdArray, s32 slotId,
    OrbisNpTusDataStatus* statusArray, u64 statusArraySize, int arrayLen, void* option);
s32 PS4_SYSV_ABI sceNpTusGetMultiUserDataStatusVUserAsync(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserIdArray, s32 slotId,
    OrbisNpTusDataStatus* statusArray, u64 statusArraySize, int arrayLen, void* option);
s32 PS4_SYSV_ABI sceNpTusGetMultiUserVariable(int reqId, const OrbisNpId* targetNpIdArray,
                                              s32 slotId, OrbisNpTusVariable* variableArray,
                                              u64 variableArraySize, int arrayLen, void* option);
s32 PS4_SYSV_ABI sceNpTusGetMultiUserVariableA(int reqId,
                                               const OrbisNpAccountId* targetAccountIdArray,
                                               s32 slotId, OrbisNpTusVariableA* variableArray,
                                               u64 variableArraySize, int arrayLen, void* option);
s32 PS4_SYSV_ABI sceNpTusGetMultiUserVariableAAsync(int reqId,
                                                    const OrbisNpAccountId* targetAccountIdArray,
                                                    s32 slotId, OrbisNpTusVariableA* variableArray,
                                                    u64 variableArraySize, int arrayLen,
                                                    void* option);
s32 PS4_SYSV_ABI sceNpTusGetMultiUserVariableAsync(int reqId, const OrbisNpId* targetNpIdArray,
                                                   s32 slotId, OrbisNpTusVariable* variableArray,
                                                   u64 variableArraySize, int arrayLen,
                                                   void* option);
s32 PS4_SYSV_ABI sceNpTusGetMultiUserVariableAVUser(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserIdArray, s32 slotId,
    OrbisNpTusVariableA* variableArray, u64 variableArraySize, int arrayLen, void* option);
s32 PS4_SYSV_ABI sceNpTusGetMultiUserVariableAVUserAsync(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserIdArray, s32 slotId,
    OrbisNpTusVariableA* variableArray, u64 variableArraySize, int arrayLen, void* option);
s32 PS4_SYSV_ABI
sceNpTusGetMultiUserVariableForCrossSave(int reqId, const OrbisNpAccountId* targetAccountIdArray,
                                         s32 slotId, OrbisNpTusVariableForCrossSave* variableArray,
                                         u64 variableArraySize, int arrayLen, void* option);
s32 PS4_SYSV_ABI sceNpTusGetMultiUserVariableForCrossSaveAsync(
    int reqId, const OrbisNpAccountId* targetAccountIdArray, s32 slotId,
    OrbisNpTusVariableForCrossSave* variableArray, u64 variableArraySize, int arrayLen,
    void* option);
s32 PS4_SYSV_ABI sceNpTusGetMultiUserVariableForCrossSaveVUser(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserIdArray, s32 slotId,
    OrbisNpTusVariableForCrossSave* variableArray, u64 variableArraySize, int arrayLen,
    void* option);
s32 PS4_SYSV_ABI sceNpTusGetMultiUserVariableForCrossSaveVUserAsync(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserIdArray, s32 slotId,
    OrbisNpTusVariableForCrossSave* variableArray, u64 variableArraySize, int arrayLen,
    void* option);
s32 PS4_SYSV_ABI sceNpTusGetMultiUserVariableVUser(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserIdArray, s32 slotId,
    OrbisNpTusVariable* variableArray, u64 variableArraySize, int arrayLen, void* option);
s32 PS4_SYSV_ABI sceNpTusGetMultiUserVariableVUserAsync(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserIdArray, s32 slotId,
    OrbisNpTusVariable* variableArray, u64 variableArraySize, int arrayLen, void* option);
s32 PS4_SYSV_ABI sceNpTusPollAsync(int reqId, int* result);
s32 PS4_SYSV_ABI sceNpTusSetData(int reqId, const OrbisNpId* targetNpId, s32 slotId, u64 totalSize,
                                 u64 sendSize, const void* data, const OrbisNpTusDataInfo* info,
                                 u64 infoStructSize, const OrbisNpId* isLastChangedAuthor,
                                 const Libraries::Rtc::OrbisRtcTick* isLastChangedDate,
                                 void* option);
s32 PS4_SYSV_ABI sceNpTusSetDataA(int reqId, OrbisNpAccountId targetAccountId, s32 slotId,
                                  u64 totalSize, u64 sendSize, const void* data,
                                  const OrbisNpTusDataInfo* info, u64 infoStructSize,
                                  const OrbisNpAccountId* isLastChangedAuthor,
                                  const Libraries::Rtc::OrbisRtcTick* isLastChangedDate,
                                  void* option);
s32 PS4_SYSV_ABI sceNpTusSetDataAAsync(int reqId, OrbisNpAccountId targetAccountId, s32 slotId,
                                       u64 totalSize, u64 sendSize, const void* data,
                                       const OrbisNpTusDataInfo* info, u64 infoStructSize,
                                       const OrbisNpAccountId* isLastChangedAuthor,
                                       const Libraries::Rtc::OrbisRtcTick* isLastChangedDate,
                                       void* option);
s32 PS4_SYSV_ABI sceNpTusSetDataAsync(int reqId, const OrbisNpId* targetNpId, s32 slotId,
                                      u64 totalSize, u64 sendSize, const void* data,
                                      const OrbisNpTusDataInfo* info, u64 infoStructSize,
                                      const OrbisNpId* isLastChangedAuthor,
                                      const Libraries::Rtc::OrbisRtcTick* isLastChangedDate,
                                      void* option);
s32 PS4_SYSV_ABI sceNpTusSetDataAVUser(int reqId,
                                       const OrbisNpTusVirtualUserId* targetVirtualUserId,
                                       s32 slotId, u64 totalSize, u64 sendSize, const void* data,
                                       const OrbisNpTusDataInfo* info, u64 infoStructSize,
                                       const OrbisNpAccountId* isLastChangedAuthor,
                                       const Libraries::Rtc::OrbisRtcTick* isLastChangedDate,
                                       void* option);
s32 PS4_SYSV_ABI sceNpTusSetDataAVUserAsync(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId, s32 slotId, u64 totalSize,
    u64 sendSize, const void* data, const OrbisNpTusDataInfo* info, u64 infoStructSize,
    const OrbisNpAccountId* isLastChangedAuthor,
    const Libraries::Rtc::OrbisRtcTick* isLastChangedDate, void* option);
s32 PS4_SYSV_ABI sceNpTusSetDataVUser(int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId,
                                      s32 slotId, u64 totalSize, u64 sendSize, const void* data,
                                      const OrbisNpTusDataInfo* info, u64 infoStructSize,
                                      const OrbisNpId* isLastChangedAuthor,
                                      const Libraries::Rtc::OrbisRtcTick* isLastChangedDate,
                                      void* option);
s32 PS4_SYSV_ABI sceNpTusSetDataVUserAsync(int reqId,
                                           const OrbisNpTusVirtualUserId* targetVirtualUserId,
                                           s32 slotId, u64 totalSize, u64 sendSize,
                                           const void* data, const OrbisNpTusDataInfo* info,
                                           u64 infoStructSize, const OrbisNpId* isLastChangedAuthor,
                                           const Libraries::Rtc::OrbisRtcTick* isLastChangedDate,
                                           void* option);
s32 PS4_SYSV_ABI sceNpTusSetMultiSlotVariable(int reqId, OrbisNpId* npId, s32* slotIds,
                                              s64* variables, int arrayLen, void* option);
s32 PS4_SYSV_ABI sceNpTusSetMultiSlotVariableA(int reqId, OrbisNpAccountId targetAccountId,
                                               s32* slotIds, const s64* variables, int arrayLen,
                                               void* option);
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
s32 PS4_SYSV_ABI sceNpTusTryAndSetVariable(int reqId, const OrbisNpId* targetNpId, s32 slotId,
                                           s32 opeType, s64 variable, const s64* compareValue,
                                           const OrbisNpId* isLastChangedAuthor,
                                           const Libraries::Rtc::OrbisRtcTick* isLastChangedDate,
                                           OrbisNpTusVariable* resultVariable,
                                           u64 resultVariableSize, void* option);
s32 PS4_SYSV_ABI sceNpTusTryAndSetVariableA(int reqId, OrbisNpAccountId targetAccountId, s32 slotId,
                                            s32 opeType, s64 variable, const s64* compareValue,
                                            const OrbisNpAccountId* isLastChangedAuthor,
                                            const Libraries::Rtc::OrbisRtcTick* isLastChangedDate,
                                            OrbisNpTusVariableA* resultVariable,
                                            u64 resultVariableSize, void* option);
s32 PS4_SYSV_ABI sceNpTusTryAndSetVariableAAsync(
    int reqId, OrbisNpAccountId targetAccountId, s32 slotId, s32 opeType, s64 variable,
    const s64* compareValue, const OrbisNpAccountId* isLastChangedAuthor,
    const Libraries::Rtc::OrbisRtcTick* isLastChangedDate, OrbisNpTusVariableA* resultVariable,
    u64 resultVariableSize, void* option);
s32 PS4_SYSV_ABI sceNpTusTryAndSetVariableAsync(
    int reqId, const OrbisNpId* targetNpId, s32 slotId, s32 opeType, s64 variable,
    const s64* compareValue, const OrbisNpId* isLastChangedAuthor,
    const Libraries::Rtc::OrbisRtcTick* isLastChangedDate, OrbisNpTusVariable* resultVariable,
    u64 resultVariableSize, void* option);
s32 PS4_SYSV_ABI sceNpTusTryAndSetVariableAVUser(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId, s32 slotId, s32 opeType,
    s64 variable, const s64* compareValue, const OrbisNpAccountId* isLastChangedAuthor,
    const Libraries::Rtc::OrbisRtcTick* isLastChangedDate, OrbisNpTusVariableA* resultVariable,
    u64 resultVariableSize, void* option);
s32 PS4_SYSV_ABI sceNpTusTryAndSetVariableAVUserAsync(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId, s32 slotId, s32 opeType,
    s64 variable, const s64* compareValue, const OrbisNpAccountId* isLastChangedAuthor,
    const Libraries::Rtc::OrbisRtcTick* isLastChangedDate, OrbisNpTusVariableA* resultVariable,
    u64 resultVariableSize, void* option);
s32 PS4_SYSV_ABI sceNpTusTryAndSetVariableForCrossSave(
    int reqId, OrbisNpAccountId targetAccountId, s32 slotId, s32 opeType, s64 variable,
    const s64* compareValue, const OrbisNpAccountId* isLastChangedAuthor,
    const Libraries::Rtc::OrbisRtcTick* isLastChangedDate,
    OrbisNpTusVariableForCrossSave* resultVariable, u64 resultVariableSize, void* option);
s32 PS4_SYSV_ABI sceNpTusTryAndSetVariableForCrossSaveAsync(
    int reqId, OrbisNpAccountId targetAccountId, s32 slotId, s32 opeType, s64 variable,
    const s64* compareValue, const OrbisNpAccountId* isLastChangedAuthor,
    const Libraries::Rtc::OrbisRtcTick* isLastChangedDate,
    OrbisNpTusVariableForCrossSave* resultVariable, u64 resultVariableSize, void* option);
s32 PS4_SYSV_ABI sceNpTusTryAndSetVariableForCrossSaveVUser(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId, s32 slotId, s32 opeType,
    s64 variable, const s64* compareValue, const OrbisNpAccountId* isLastChangedAuthor,
    const Libraries::Rtc::OrbisRtcTick* isLastChangedDate,
    OrbisNpTusVariableForCrossSave* resultVariable, u64 resultVariableSize, void* option);
s32 PS4_SYSV_ABI sceNpTusTryAndSetVariableForCrossSaveVUserAsync(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId, s32 slotId, s32 opeType,
    s64 variable, const s64* compareValue, const OrbisNpAccountId* isLastChangedAuthor,
    const Libraries::Rtc::OrbisRtcTick* isLastChangedDate,
    OrbisNpTusVariableForCrossSave* resultVariable, u64 resultVariableSize, void* option);
s32 PS4_SYSV_ABI sceNpTusTryAndSetVariableVUser(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId, s32 slotId, s32 opeType,
    s64 variable, const s64* compareValue, const OrbisNpId* isLastChangedAuthor,
    const Libraries::Rtc::OrbisRtcTick* isLastChangedDate, OrbisNpTusVariable* resultVariable,
    u64 resultVariableSize, void* option);
s32 PS4_SYSV_ABI sceNpTusTryAndSetVariableVUserAsync(
    int reqId, const OrbisNpTusVirtualUserId* targetVirtualUserId, s32 slotId, s32 opeType,
    s64 variable, const s64* compareValue, const OrbisNpId* isLastChangedAuthor,
    const Libraries::Rtc::OrbisRtcTick* isLastChangedDate, OrbisNpTusVariable* resultVariable,
    u64 resultVariableSize, void* option);
s32 PS4_SYSV_ABI sceNpTusWaitAsync(int reqId, int* result);

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Np::NpTus