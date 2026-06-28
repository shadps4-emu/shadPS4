// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Np::NpWebApi2 {

struct OrbisNpWebApi2IntInitializeArgs {
    s32 lib_http_ctx_id;
    s32 reserved;
    u64 pool_size;
    char* name;
    u64 struct_size;
};

struct OrbisNpWebApi2IntInitialize2Args {
    s32 lib_http_ctx_id;
    s32 reserved;
    u64 pool_size;
    char* name;
    u32 push_config_group;
    s32 reserved2;
    u64 struct_size;
};

struct OrbisNpWebApi2MemoryPoolStats {
    u64 pool_size;
    u64 max_inuse_size;
    u64 current_inuse_size;
    s32 reserved;
};

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Np::NpWebApi2