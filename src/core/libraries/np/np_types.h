// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/error_codes.h"

// For structs and constants shared between multiple Np libraries.
namespace Libraries::Np {

using OrbisNpAccountId = u64;

constexpr s32 ORBIS_NP_ONLINEID_MAX_LENGTH = 16;

struct OrbisNpOnlineId {
    char data[ORBIS_NP_ONLINEID_MAX_LENGTH];
    s8 term;
    s8 dummy[3];
};

struct OrbisNpId {
    OrbisNpOnlineId handle;
    u8 opt[8];
    u8 reserved[8];
};

struct OrbisNpClientId {
    char id[129];
    u8 padding[7];
};

struct OrbisNpAuthorizationCode {
    char code[129];
    u8 padding[7];
};

struct OrbisNpClientSecret {
    char secret[257];
    u8 padding[7];
};

struct OrbisNpIdToken {
    char token[4097];
    u8 padding[7];
};

using OrbisNpServiceLabel = u32;
constexpr OrbisNpServiceLabel ORBIS_NP_INVALID_SERVICE_LABEL = 0xFFFFFFFF;

enum class OrbisNpPlatformType : s32 {
    NONE = 0,
    PS3 = 1,
    VITA = 2,
    PS4 = 3,
};

struct OrbisNpPeerAddress {
    OrbisNpId* npId;
    OrbisNpPlatformType platformType;
    u8 padding[4];
};

struct OrbisNpPeerAddressA {
    OrbisNpAccountId accountId;
    OrbisNpPlatformType platform;
    char padding[4];
};

}; // namespace Libraries::Np