// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}
namespace Libraries::Zlib {

int PS4_SYSV_ABI sceZlibInitialize(const void* buffer, std::size_t length);
int PS4_SYSV_ABI sceZlibInflate(const void* source, u32 sourceLength, void* destination,
                                u32 destinationLength, u64* requestId);
int PS4_SYSV_ABI sceZlibWaitForDone(u64* requestId, u32* timeout);
int PS4_SYSV_ABI sceZlibGetResult(u64 requestId, u32* destinationLength, int* status);
int PS4_SYSV_ABI sceZlibFinalize();

void RegisterlibSceZlib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Zlib