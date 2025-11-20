// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}
namespace Libraries::DiscMap {

int PS4_SYSV_ABI sceDiscMapGetPackageSize(s64 fflags, int* ret1, int* ret2);
int PS4_SYSV_ABI sceDiscMapIsRequestOnHDD(char* path, s64 offset, s64 nbytes, int* ret);
int PS4_SYSV_ABI Func_7C980FFB0AA27E7A(char* path, s64 offset, s64 nbytes, int* flags, int* ret1,
                                       int* ret2);
int PS4_SYSV_ABI Func_8A828CAEE7EDD5E9(char* path, s64 offset, s64 nbytes, int* flags, int* ret1,
                                       int* ret2);
int PS4_SYSV_ABI Func_E7EBCE96E92F91F8();

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::DiscMap