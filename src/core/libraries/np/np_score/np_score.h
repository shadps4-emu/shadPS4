// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/np/np_types.h"
#include "core/libraries/rtc/rtc.h"
#include "core/libraries/system/userservice.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Np::NpScore {

using OrbisNpScoreBoardId = u32;
using OrbisNpScorePcId = s32;
using OrbisNpScoreRankNumber = u32;
using OrbisNpScoreValue = s64;

constexpr int ORBIS_NP_SCORE_COMMENT_MAXLEN = 63;
constexpr int ORBIS_NP_SCORE_GAMEINFO_MAXSIZE = 189;

struct OrbisNpScoreBoardInfo {
    u32 rankLimit;
    u32 updateMode;
    u32 sortMode;
    OrbisNpScoreRankNumber uploadNumLimit;
    u64 uploadSizeLimit;
};

struct OrbisNpScoreComment {
    char utf8Comment[ORBIS_NP_SCORE_COMMENT_MAXLEN + 1];
};

struct OrbisNpScoreGameInfo {
    u64 infoSize;
    u8 data[ORBIS_NP_SCORE_GAMEINFO_MAXSIZE];
    u8 pad2[3];
};

struct OrbisNpScoreGetFriendRankingOptParam {
    u64 size;
    u32* startSerialRank;
    u32* hits;
};

struct OrbisNpScoreAccountIdPcId {
    OrbisNpAccountId accountId;
    OrbisNpScorePcId pcId;
    u8 pad[4];
};

struct OrbisNpScoreNpIdPcId {
    OrbisNpId npId;
    OrbisNpScorePcId pcId;
    u8 pad[4];
};

struct OrbisNpScoreRankData {
    OrbisNpId npId;
    u8 reserved[49];
    u8 pad0[3];
    OrbisNpScorePcId pcId;
    OrbisNpScoreRankNumber serialRank;
    OrbisNpScoreRankNumber rank;
    OrbisNpScoreRankNumber highestRank;
    s32 hasGameData;
    u8 pad1[4];
    OrbisNpScoreValue scoreValue;
    Rtc::OrbisRtcTick recordDate;
};

struct OrbisNpScoreRankDataA {
    OrbisNpOnlineId onlineId;
    u8 reserved0[16];
    u8 reserved[49];
    u8 pad0[3];
    OrbisNpScorePcId pcId;
    OrbisNpScoreRankNumber serialRank;
    OrbisNpScoreRankNumber rank;
    OrbisNpScoreRankNumber highestRank;
    s32 hasGameData;
    u8 pad1[4];
    OrbisNpScoreValue scoreValue;
    Rtc::OrbisRtcTick recordDate;
    OrbisNpAccountId accountId;
    u8 pad2[8];
};

struct OrbisNpScorePlayerRankDataA {
    s32 hasData;
    u8 pad0[4];
    OrbisNpScoreRankDataA rankData;
};

struct OrbisNpScoreRankDataForCrossSave {
    OrbisNpId npId;
    u8 reserved[49];
    u8 pad0[3];
    OrbisNpScorePcId pcId;
    OrbisNpScoreRankNumber serialRank;
    OrbisNpScoreRankNumber rank;
    OrbisNpScoreRankNumber highestRank;
    s32 hasGameData;
    u8 pad1[4];
    OrbisNpScoreValue scoreValue;
    Rtc::OrbisRtcTick recordDate;
    OrbisNpAccountId accountId;
    u8 pad2[8];
};

struct OrbisNpScorePlayerRankDataForCrossSave {
    s32 hasData;
    u8 pad0[4];
    OrbisNpScoreRankDataForCrossSave rankData;
};
int PS4_SYSV_ABI sceNpScoreCreateNpTitleCtx(OrbisNpServiceLabel serviceLabel, OrbisNpId* npId);
// stubbed
int PS4_SYSV_ABI sceNpScoreAbortRequest(s32 reqId);
int PS4_SYSV_ABI sceNpScoreCensorComment(s32 reqId, const char* comment, void* option);
int PS4_SYSV_ABI sceNpScoreCensorCommentAsync(s32 reqId, const char* comment, void* option);
int PS4_SYSV_ABI sceNpScoreChangeModeForOtherSaveDataOwners();
int PS4_SYSV_ABI sceNpScoreCreateNpTitleCtxA(OrbisNpServiceLabel npServiceLabel,
                                             UserService::OrbisUserServiceUserId selfId);
int PS4_SYSV_ABI sceNpScoreCreateRequest(s32 titleCtxId);
int PS4_SYSV_ABI sceNpScoreCreateTitleCtx();
int PS4_SYSV_ABI sceNpScoreDeleteNpTitleCtx(s32 titleCtxId);
int PS4_SYSV_ABI sceNpScoreDeleteRequest(s32 reqId);
int PS4_SYSV_ABI sceNpScoreGetBoardInfo(s32 reqId, OrbisNpScoreBoardId boardId,
                                        OrbisNpScoreBoardInfo* boardInfo, void* option);
int PS4_SYSV_ABI sceNpScoreGetBoardInfoAsync(s32 reqId, OrbisNpScoreBoardId boardId,
                                             OrbisNpScoreBoardInfo* boardInfo, void* option);
int PS4_SYSV_ABI sceNpScoreGetFriendsRanking(s32 reqId, OrbisNpScoreBoardId boardId,
                                             s32 includeSelf, OrbisNpScoreRankData* rankArray,
                                             u64 rankArraySize, OrbisNpScoreComment* commentArray,
                                             u64 commentArraySize, OrbisNpScoreGameInfo* infoArray,
                                             u64 infoArraySize, u64 arrayNum,
                                             Rtc::OrbisRtcTick* lastSortDate,
                                             OrbisNpScoreRankNumber* totalRecord, void* option);
int PS4_SYSV_ABI sceNpScoreGetFriendsRankingA(s32 reqId, OrbisNpScoreBoardId boardId,
                                              s32 includeSelf, OrbisNpScoreRankDataA* rankArray,
                                              u64 rankArraySize, OrbisNpScoreComment* commentArray,
                                              u64 commentArraySize, OrbisNpScoreGameInfo* infoArray,
                                              u64 infoArraySize, u64 arrayNum,
                                              Rtc::OrbisRtcTick* lastSortDate,
                                              OrbisNpScoreRankNumber* totalRecord,
                                              OrbisNpScoreGetFriendRankingOptParam* option);
int PS4_SYSV_ABI sceNpScoreGetFriendsRankingAAsync(
    s32 reqId, OrbisNpScoreBoardId boardId, s32 includeSelf, OrbisNpScoreRankDataA* rankArray,
    u64 rankArraySize, OrbisNpScoreComment* commentArray, u64 commentArraySize,
    OrbisNpScoreGameInfo* infoArray, u64 infoArraySize, u64 arrayNum,
    Rtc::OrbisRtcTick* lastSortDate, OrbisNpScoreRankNumber* totalRecord,
    OrbisNpScoreGetFriendRankingOptParam* option);
int PS4_SYSV_ABI sceNpScoreGetFriendsRankingAsync(
    s32 reqId, OrbisNpScoreBoardId boardId, s32 includeSelf, OrbisNpScoreRankData* rankArray,
    u64 rankArraySize, OrbisNpScoreComment* commentArray, u64 commentArraySize,
    OrbisNpScoreGameInfo* infoArray, u64 infoArraySize, u64 arrayNum,
    Rtc::OrbisRtcTick* lastSortDate, OrbisNpScoreRankNumber* totalRecord,
    OrbisNpScoreGetFriendRankingOptParam* option);
int PS4_SYSV_ABI sceNpScoreGetFriendsRankingForCrossSave(
    s32 reqId, OrbisNpScoreBoardId boardId, s32 includeSelf,
    OrbisNpScoreRankDataForCrossSave* rankArray, u64 rankArraySize,
    OrbisNpScoreComment* commentArray, u64 commentArraySize, OrbisNpScoreGameInfo* infoArray,
    u64 infoArraySize, u64 arrayNum, Rtc::OrbisRtcTick* lastSortDate,
    OrbisNpScoreRankNumber* totalRecord, OrbisNpScoreGetFriendRankingOptParam* option);
int PS4_SYSV_ABI sceNpScoreGetFriendsRankingForCrossSaveAsync(
    s32 reqId, OrbisNpScoreBoardId boardId, s32 includeSelf,
    OrbisNpScoreRankDataForCrossSave* rankArray, u64 rankArraySize,
    OrbisNpScoreComment* commentArray, u64 commentArraySize, OrbisNpScoreGameInfo* infoArray,
    u64 infoArraySize, u64 arrayNum, Rtc::OrbisRtcTick* lastSortDate,
    OrbisNpScoreRankNumber* totalRecord, OrbisNpScoreGetFriendRankingOptParam* option);
int PS4_SYSV_ABI sceNpScoreGetGameData();
int PS4_SYSV_ABI sceNpScoreGetGameDataAsync();
int PS4_SYSV_ABI sceNpScoreGetGameDataByAccountId(s32 reqId, OrbisNpScoreBoardId boardId,
                                                  OrbisNpAccountId accountId, u64* totalSize,
                                                  u64 recvSize, void* data, void* option);
int PS4_SYSV_ABI sceNpScoreGetGameDataByAccountIdAsync(s32 reqId, OrbisNpScoreBoardId boardId,
                                                       OrbisNpAccountId accountId, u64* totalSize,
                                                       u64 recvSize, void* data, void* option);
int PS4_SYSV_ABI sceNpScoreGetRankingByAccountId(
    s32 reqId, OrbisNpScoreBoardId boardId, const OrbisNpAccountId* accountIdArray,
    u64 accountIdArraySize, OrbisNpScorePlayerRankDataA* rankArray, u64 rankArraySize,
    OrbisNpScoreComment* commentArray, u64 commentArraySize, OrbisNpScoreGameInfo* infoArray,
    u64 infoArraySize, u64 arrayNum, Rtc::OrbisRtcTick* lastSortDate,
    OrbisNpScoreRankNumber* totalRecord, void* option);
int PS4_SYSV_ABI sceNpScoreGetRankingByAccountIdAsync(
    s32 reqId, OrbisNpScoreBoardId boardId, const OrbisNpAccountId* accountIdArray,
    u64 accountIdArraySize, OrbisNpScorePlayerRankDataA* rankArray, u64 rankArraySize,
    OrbisNpScoreComment* commentArray, u64 commentArraySize, OrbisNpScoreGameInfo* infoArray,
    u64 infoArraySize, u64 arrayNum, Rtc::OrbisRtcTick* lastSortDate,
    OrbisNpScoreRankNumber* totalRecord, void* option);
int PS4_SYSV_ABI sceNpScoreGetRankingByAccountIdForCrossSave(
    s32 reqId, OrbisNpScoreBoardId boardId, const OrbisNpAccountId* accountIdArray,
    u64 accountIdArraySize, OrbisNpScorePlayerRankDataForCrossSave* rankArray, u64 rankArraySize,
    OrbisNpScoreComment* commentArray, u64 commentArraySize, OrbisNpScoreGameInfo* infoArray,
    u64 infoArraySize, u64 arrayNum, Rtc::OrbisRtcTick* lastSortDate,
    OrbisNpScoreRankNumber* totalRecord, void* option);
int PS4_SYSV_ABI sceNpScoreGetRankingByAccountIdForCrossSaveAsync(
    s32 reqId, OrbisNpScoreBoardId boardId, const OrbisNpAccountId* accountIdArray,
    u64 accountIdArraySize, OrbisNpScorePlayerRankDataForCrossSave* rankArray, u64 rankArraySize,
    OrbisNpScoreComment* commentArray, u64 commentArraySize, OrbisNpScoreGameInfo* infoArray,
    u64 infoArraySize, u64 arrayNum, Rtc::OrbisRtcTick* lastSortDate,
    OrbisNpScoreRankNumber* totalRecord, void* option);
int PS4_SYSV_ABI sceNpScoreGetRankingByAccountIdPcId(
    s32 reqId, OrbisNpScoreBoardId boardId, const OrbisNpScoreAccountIdPcId* idArray,
    u64 idArraySize, OrbisNpScorePlayerRankDataA* rankArray, u64 rankArraySize,
    OrbisNpScoreComment* commentArray, u64 commentArraySize, OrbisNpScoreGameInfo* infoArray,
    u64 infoArraySize, u64 arrayNum, Rtc::OrbisRtcTick* lastSortDate,
    OrbisNpScoreRankNumber* totalRecord, void* option);
int PS4_SYSV_ABI sceNpScoreGetRankingByAccountIdPcIdAsync(
    s32 reqId, OrbisNpScoreBoardId boardId, const OrbisNpScoreAccountIdPcId* idArray,
    u64 idArraySize, OrbisNpScorePlayerRankDataA* rankArray, u64 rankArraySize,
    OrbisNpScoreComment* commentArray, u64 commentArraySize, OrbisNpScoreGameInfo* infoArray,
    u64 infoArraySize, u64 arrayNum, Rtc::OrbisRtcTick* lastSortDate,
    OrbisNpScoreRankNumber* totalRecord, void* option);
int PS4_SYSV_ABI sceNpScoreGetRankingByAccountIdPcIdForCrossSave(
    s32 reqId, OrbisNpScoreBoardId boardId, const OrbisNpScoreAccountIdPcId* idArray,
    u64 idArraySize, OrbisNpScorePlayerRankDataForCrossSave* rankArray, u64 rankArraySize,
    OrbisNpScoreComment* commentArray, u64 commentArraySize, OrbisNpScoreGameInfo* infoArray,
    u64 infoArraySize, u64 arrayNum, Rtc::OrbisRtcTick* lastSortDate,
    OrbisNpScoreRankNumber* totalRecord, void* option);
int PS4_SYSV_ABI sceNpScoreGetRankingByAccountIdPcIdForCrossSaveAsync(
    s32 reqId, OrbisNpScoreBoardId boardId, const OrbisNpScoreAccountIdPcId* idArray,
    u64 idArraySize, OrbisNpScorePlayerRankDataForCrossSave* rankArray, u64 rankArraySize,
    OrbisNpScoreComment* commentArray, u64 commentArraySize, OrbisNpScoreGameInfo* infoArray,
    u64 infoArraySize, u64 arrayNum, Rtc::OrbisRtcTick* lastSortDate,
    OrbisNpScoreRankNumber* totalRecord, void* option);
int PS4_SYSV_ABI sceNpScoreGetRankingByNpId();
int PS4_SYSV_ABI sceNpScoreGetRankingByNpIdAsync();
int PS4_SYSV_ABI sceNpScoreGetRankingByNpIdPcId();
int PS4_SYSV_ABI sceNpScoreGetRankingByNpIdPcIdAsync();
int PS4_SYSV_ABI sceNpScoreGetRankingByRange(
    s32 reqId, OrbisNpScoreBoardId boardId, OrbisNpScoreRankNumber startSerialRank,
    OrbisNpScoreRankData* rankArray, u64 rankArraySize, OrbisNpScoreComment* commentArray,
    u64 commentArraySize, OrbisNpScoreGameInfo* infoArray, u64 infoArraySize, u64 arrayNum,
    Rtc::OrbisRtcTick* lastSortDate, OrbisNpScoreRankNumber* totalRecord, void* option);
int PS4_SYSV_ABI sceNpScoreGetRankingByRangeA(
    s32 reqId, OrbisNpScoreBoardId boardId, OrbisNpScoreRankNumber startSerialRank,
    OrbisNpScoreRankDataA* rankArray, u64 rankArraySize, OrbisNpScoreComment* commentArray,
    u64 commentArraySize, OrbisNpScoreGameInfo* infoArray, u64 infoArraySize, u64 arrayNum,
    Rtc::OrbisRtcTick* lastSortDate, OrbisNpScoreRankNumber* totalRecord, void* option);
int PS4_SYSV_ABI sceNpScoreGetRankingByRangeAAsync(
    s32 reqId, OrbisNpScoreBoardId boardId, OrbisNpScoreRankNumber startSerialRank,
    OrbisNpScoreRankDataA* rankArray, u64 rankArraySize, OrbisNpScoreComment* commentArray,
    u64 commentArraySize, OrbisNpScoreGameInfo* infoArray, u64 infoArraySize, u64 arrayNum,
    Rtc::OrbisRtcTick* lastSortDate, OrbisNpScoreRankNumber* totalRecord, void* option);
int PS4_SYSV_ABI sceNpScoreGetRankingByRangeAsync(
    s32 reqId, OrbisNpScoreBoardId boardId, OrbisNpScoreRankNumber startSerialRank,
    OrbisNpScoreRankData* rankArray, u64 rankArraySize, OrbisNpScoreComment* commentArray,
    u64 commentArraySize, OrbisNpScoreGameInfo* infoArray, u64 infoArraySize, u64 arrayNum,
    Rtc::OrbisRtcTick* lastSortDate, OrbisNpScoreRankNumber* totalRecord, void* option);
int PS4_SYSV_ABI sceNpScoreGetRankingByRangeForCrossSave(
    s32 reqId, OrbisNpScoreBoardId boardId, OrbisNpScoreRankNumber startSerialRank,
    OrbisNpScoreRankDataForCrossSave* rankArray, u64 rankArraySize,
    OrbisNpScoreComment* commentArray, u64 commentArraySize, OrbisNpScoreGameInfo* infoArray,
    u64 infoArraySize, u64 arrayNum, Rtc::OrbisRtcTick* lastSortDate,
    OrbisNpScoreRankNumber* totalRecord, void* option);
int PS4_SYSV_ABI sceNpScoreGetRankingByRangeForCrossSaveAsync(
    s32 reqId, OrbisNpScoreBoardId boardId, OrbisNpScoreRankNumber startSerialRank,
    OrbisNpScoreRankDataForCrossSave* rankArray, u64 rankArraySize,
    OrbisNpScoreComment* commentArray, u64 commentArraySize, OrbisNpScoreGameInfo* infoArray,
    u64 infoArraySize, u64 arrayNum, Rtc::OrbisRtcTick* lastSortDate,
    OrbisNpScoreRankNumber* totalRecord, void* option);
int PS4_SYSV_ABI sceNpScorePollAsync(s32 reqId, s32* result);
int PS4_SYSV_ABI sceNpScoreRecordGameData(s32 reqId, OrbisNpScoreBoardId boardId,
                                          OrbisNpScoreValue score, u64 totalSize, u64 sendSize,
                                          const void* data, void* option);
int PS4_SYSV_ABI sceNpScoreRecordGameDataAsync(s32 reqId, OrbisNpScoreBoardId boardId,
                                               OrbisNpScoreValue score, u64 totalSize, u64 sendSize,
                                               const void* data, void* option);
int PS4_SYSV_ABI sceNpScoreRecordScore(s32 reqId, OrbisNpScoreBoardId boardId,
                                       OrbisNpScoreValue score,
                                       const OrbisNpScoreComment* scoreComment,
                                       const OrbisNpScoreGameInfo* gameInfo,
                                       OrbisNpScoreRankNumber* tmpRank,
                                       const Rtc::OrbisRtcTick* compareDate, void* option);
int PS4_SYSV_ABI sceNpScoreRecordScoreAsync(s32 reqId, OrbisNpScoreBoardId boardId,
                                            OrbisNpScoreValue score,
                                            const OrbisNpScoreComment* scoreComment,
                                            const OrbisNpScoreGameInfo* gameInfo,
                                            OrbisNpScoreRankNumber* tmpRank,
                                            const Rtc::OrbisRtcTick* compareDate, void* option);
int PS4_SYSV_ABI sceNpScoreSanitizeComment(s32 reqId, const char* comment, char* sanitizedComment,
                                           void* option);
int PS4_SYSV_ABI sceNpScoreSanitizeCommentAsync(s32 reqId, const char* comment,
                                                char* sanitizedComment, void* option);
int PS4_SYSV_ABI sceNpScoreSetPlayerCharacterId(s32 ctxId, OrbisNpScorePcId pcId);
int PS4_SYSV_ABI sceNpScoreSetThreadParam(s32 threadPriority, u64 cpuAffinityMask);
int PS4_SYSV_ABI sceNpScoreSetTimeout(s32 id, s32 resolveRetry, s32 resolveTimeout, s32 connTimeout,
                                      s32 sendTimeout, s32 recvTimeout);
int PS4_SYSV_ABI sceNpScoreWaitAsync(s32 reqId, s32* result);

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Np::NpScore
