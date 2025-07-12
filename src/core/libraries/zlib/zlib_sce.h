// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}
namespace Libraries::Zlib {

s32 PS4_SYSV_ABI sceZlibInitialize(const void* buffer, u32 length);
s32 PS4_SYSV_ABI sceZlibInflate(const void* src, u32 src_len, void* dst, u32 dst_len,
                                u64* request_id);
s32 PS4_SYSV_ABI sceZlibWaitForDone(u64* request_id, const u32* timeout);
s32 PS4_SYSV_ABI sceZlibGetResult(u64 request_id, u32* dst_length, s32* status);
s32 PS4_SYSV_ABI sceZlibFinalize();

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Zlib