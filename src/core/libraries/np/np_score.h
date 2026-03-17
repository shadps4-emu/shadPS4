// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/np/np_types.h"
#include "core/libraries/rtc/rtc.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Np::NpScore {

using OrbisNpScoreBoardId = u32;
using OrbisNpScorePcId = u32;
using OrbisNpScoreRankNumber = u32;
using OrbisNpScoreValue = s64;

struct OrbisNpScoreNpIdPcId {
    Np::OrbisNpId npId;
    OrbisNpScorePcId pcId;
    u8 pad[4];
};

static_assert(sizeof(OrbisNpScoreNpIdPcId) == 0x2c);

struct OrbisNpScoreAccountIdPcId {
    OrbisNpAccountId accountId;
    OrbisNpScorePcId pcId;
    u8 pad[4];
};

struct OrbisNpScoreRankDataA {
    OrbisNpOnlineId onlineId;
    u8 reserved[68];
    OrbisNpScorePcId pcId;
    OrbisNpScoreRankNumber rank1;
    OrbisNpScoreRankNumber rank2;
    OrbisNpScoreRankNumber rank3;
    int hasGameData;
    u8 pad[4];
    OrbisNpScoreValue score;
    Rtc::OrbisRtcTick recordTime;
    OrbisNpAccountId accountId;
    u8 pad1[8];
};

static_assert(sizeof(OrbisNpScoreRankDataA) == 0x90);

struct OrbisNpScorePlayerRankDataA {
    int hasData;
    u8 pad[4];
    OrbisNpScoreRankDataA rankData;
};

static_assert(sizeof(OrbisNpScorePlayerRankDataA) == 0x98);

struct OrbisNpScoreRankData {
    OrbisNpOnlineId onlineId;
    u8 reserved[52];
    OrbisNpScorePcId pcId;
    OrbisNpScoreRankNumber rank1;
    OrbisNpScoreRankNumber rank2;
    OrbisNpScoreRankNumber rank3;
    int hasGameData;
    u8 pad[4];
    OrbisNpScoreValue score;
    Rtc::OrbisRtcTick recordTime;
    OrbisNpAccountId accountId;
    u8 pad1[8];
};

static_assert(sizeof(OrbisNpScoreRankData) == 0x80);

struct OrbisNpScorePlayerRankData {
    int hasData;
    u8 pad[4];
    OrbisNpScoreRankData rankData;
};

static_assert(sizeof(OrbisNpScorePlayerRankData) == 0x88);

struct OrbisNpScoreComment {
    char comment[64];
};

static_assert(sizeof(OrbisNpScoreComment) == 64);

struct OrbisNpScoreGameInfo {
    u64 dataSize;
    u8 data[189];
    u8 pad[3];
};

static_assert(sizeof(OrbisNpScoreGameInfo) == 200);

int PS4_SYSV_ABI sceNpScoreWaitAsync(int reqId, int* result);

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Np::NpScore