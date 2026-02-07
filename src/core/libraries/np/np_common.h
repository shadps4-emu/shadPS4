// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Np::NpCommon {

struct OrbisNpFreeKernelMemoryArgs {
    u64 length;
    u64 unk;
    void* addr;
};

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Np::NpCommon