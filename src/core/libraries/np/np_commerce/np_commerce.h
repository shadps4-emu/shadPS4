// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Np::NpCommerce {

using CommonDialog::BaseParam;
using CommonDialog::Error;
using CommonDialog::Status;

static constexpr u64 MAX_LABEL_LEN = 128;
static constexpr u32 ORBIS_NP_COMMERCE_DIALOG_NUM_TARGETS_MAX = 10;

struct OrbisNpCommerceDialogParam {
    BaseParam baseParam;
    s32 size;
    Libraries::UserService::OrbisUserServiceUserId userId;
    s32 mode;
    u32 serviceLabel;
    const char* const* targets;
    u32 numTargets;
    s32 : 32;
    u64 features;
    void* userData;
    u8 reserved[32];
};
static_assert(sizeof(OrbisNpCommerceDialogParam) == 0x80);

struct OrbisNpCommerceDialogResult {
    s32 result;
    bool authorized;
    char : 8;
    short : 16;
    void* userData;
    u8 reserved[32];
};
static_assert(sizeof(OrbisNpCommerceDialogResult) == 0x30);

// OrbisNpCommerceDialogResult return codes
static constexpr s32 ORBIS_COMMON_DIALOG_RESULT_OK = 0;
static constexpr s32 ORBIS_COMMON_DIALOG_RESULT_USER_CANCELED = 1;
static constexpr s32 ORBIS_NP_COMMERCE_DIALOG_RESULT_PURCHASED = 2;

// OrbisNpCommerceDialogMode
static constexpr s32 ORBIS_NP_COMMERCE_DIALOG_MODE_CATEGORY = 0;     // Browse category mode
static constexpr s32 ORBIS_NP_COMMERCE_DIALOG_MODE_PRODUCT = 1;      // Browse product mode
static constexpr s32 ORBIS_NP_COMMERCE_DIALOG_MODE_PRODUCT_CODE = 2; // promotion code mode
static constexpr s32 ORBIS_NP_COMMERCE_DIALOG_MODE_CHECKOUT = 3;     // Checkout mode
static constexpr s32 ORBIS_NP_COMMERCE_DIALOG_MODE_DOWNLOADLIST = 4; // Download mode
static constexpr s32 ORBIS_NP_COMMERCE_DIALOG_MODE_PLUS = 5;         // PlayStation Plus mode

void RegisterLib(Core::Loader::SymbolsResolver* sym);

} // namespace Libraries::Np::NpCommerce