// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/rtc/rtc.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Np::NpTus {

using OrbisNpTssSlotId = s32;
using OrbisNpTusSlotId = s32;

struct OrbisNpTusVariable {
    OrbisNpId npId;
    int set;
    Libraries::Rtc::OrbisRtcTick lastChanged;
    u8 pad1[4];
    OrbisNpId lastChangedAuthor;
    s64 variable;
    s64 oldVariable;
    OrbisNpAccountId owner;
    OrbisNpAccountId lastChangedAuthorId;
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
    OrbisNpTusDataInfo info;
};

static_assert(sizeof(OrbisNpTusDataStatus) == 0x1F0);

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

static_assert(sizeof(OrbisNpTusDataStatusA) == 0x210);

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

struct OrbisNpTssGetDataOptParam {
    u64 size;
    u64* offset;
    u64* last;
    void* param;
};

s32 PS4_SYSV_ABI sceNpTusWaitAsync(int reqId, int* result);

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Np::NpTus