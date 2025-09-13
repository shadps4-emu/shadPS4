// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "core/libraries/error_codes.h"

// For structs and constants shared between multiple Np libraries.
namespace Libraries::Np {

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

}; // namespace Libraries::Np