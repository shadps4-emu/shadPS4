// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Np::NpCommon {

using OrbisNpServiceLabel = u32;
using OrbisNpAccountId = u64;
using OrbisNpPlatformType = s32; // 0=none , 1=ps3 , 2=vita , 3=ps4

struct OrbisNpPeerAddressA {
    OrbisNpAccountId accountId;
    OrbisNpPlatformType platform;
    char padding[4];
};

struct OrbisNpFreeKernelMemoryArgs {
    u64 length;
    u64 unk;
    void* addr;
};

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Np::NpCommon