// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::NpScore {

int PS4_SYSV_ABI sceNpScoreAbortRequest();
int PS4_SYSV_ABI sceNpScoreCensorComment();
int PS4_SYSV_ABI sceNpScoreCensorCommentAsync();
int PS4_SYSV_ABI sceNpScoreChangeModeForOtherSaveDataOwners();
int PS4_SYSV_ABI sceNpScoreCreateNpTitleCtx();
int PS4_SYSV_ABI sceNpScoreCreateNpTitleCtxA();
int PS4_SYSV_ABI sceNpScoreCreateRequest();
int PS4_SYSV_ABI sceNpScoreCreateTitleCtx();
int PS4_SYSV_ABI sceNpScoreDeleteNpTitleCtx();
int PS4_SYSV_ABI sceNpScoreDeleteRequest();
int PS4_SYSV_ABI sceNpScoreGetBoardInfo();
int PS4_SYSV_ABI sceNpScoreGetBoardInfoAsync();
int PS4_SYSV_ABI sceNpScoreGetFriendsRanking();
int PS4_SYSV_ABI sceNpScoreGetFriendsRankingA();
int PS4_SYSV_ABI sceNpScoreGetFriendsRankingAAsync();
int PS4_SYSV_ABI sceNpScoreGetFriendsRankingAsync();
int PS4_SYSV_ABI sceNpScoreGetFriendsRankingForCrossSave();
int PS4_SYSV_ABI sceNpScoreGetFriendsRankingForCrossSaveAsync();
int PS4_SYSV_ABI sceNpScoreGetGameData();
int PS4_SYSV_ABI sceNpScoreGetGameDataAsync();
int PS4_SYSV_ABI sceNpScoreGetGameDataByAccountId();
int PS4_SYSV_ABI sceNpScoreGetGameDataByAccountIdAsync();
int PS4_SYSV_ABI sceNpScoreGetRankingByAccountId();
int PS4_SYSV_ABI sceNpScoreGetRankingByAccountIdAsync();
int PS4_SYSV_ABI sceNpScoreGetRankingByAccountIdForCrossSave();
int PS4_SYSV_ABI sceNpScoreGetRankingByAccountIdForCrossSaveAsync();
int PS4_SYSV_ABI sceNpScoreGetRankingByAccountIdPcId();
int PS4_SYSV_ABI sceNpScoreGetRankingByAccountIdPcIdAsync();
int PS4_SYSV_ABI sceNpScoreGetRankingByAccountIdPcIdForCrossSave();
int PS4_SYSV_ABI sceNpScoreGetRankingByAccountIdPcIdForCrossSaveAsync();
int PS4_SYSV_ABI sceNpScoreGetRankingByNpId();
int PS4_SYSV_ABI sceNpScoreGetRankingByNpIdAsync();
int PS4_SYSV_ABI sceNpScoreGetRankingByNpIdPcId();
int PS4_SYSV_ABI sceNpScoreGetRankingByNpIdPcIdAsync();
int PS4_SYSV_ABI sceNpScoreGetRankingByRange();
int PS4_SYSV_ABI sceNpScoreGetRankingByRangeA();
int PS4_SYSV_ABI sceNpScoreGetRankingByRangeAAsync();
int PS4_SYSV_ABI sceNpScoreGetRankingByRangeAsync();
int PS4_SYSV_ABI sceNpScoreGetRankingByRangeForCrossSave();
int PS4_SYSV_ABI sceNpScoreGetRankingByRangeForCrossSaveAsync();
int PS4_SYSV_ABI sceNpScorePollAsync();
int PS4_SYSV_ABI sceNpScoreRecordGameData();
int PS4_SYSV_ABI sceNpScoreRecordGameDataAsync();
int PS4_SYSV_ABI sceNpScoreRecordScore();
int PS4_SYSV_ABI sceNpScoreRecordScoreAsync();
int PS4_SYSV_ABI sceNpScoreSanitizeComment();
int PS4_SYSV_ABI sceNpScoreSanitizeCommentAsync();
int PS4_SYSV_ABI sceNpScoreSetPlayerCharacterId();
int PS4_SYSV_ABI sceNpScoreSetThreadParam();
int PS4_SYSV_ABI sceNpScoreSetTimeout();
int PS4_SYSV_ABI sceNpScoreWaitAsync();

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::NpScore