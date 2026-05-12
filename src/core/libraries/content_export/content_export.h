// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::ContentExport {

typedef void* PS4_SYSV_ABI (*OrbisContentExportMalloc)(u64 datasize, void* userdata);
typedef void PS4_SYSV_ABI (*OrbisContentExportFree)(void* p, void* userdata);
typedef s32 PS4_SYSV_ABI (*OrbisContentExportDataProvideFunction)(void** data, u64* datasize,
                                                                  void* userdata);

struct OrbisContentExportInitParam {
    OrbisContentExportMalloc mallocfunc;
    OrbisContentExportFree freefunc;
    void* userdata;
    u64 bufsize;
    s64 reserved0;
    s64 reserved1;
};

struct OrbisContentExportParam {
    char title[257];
    char comment[257];
    char contenttype[65];
};

struct OrbisContentExportCallbackParam {
    u64 contentlength;
    OrbisContentExportDataProvideFunction func;
    void* userdata;
};

void RegisterLib(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::ContentExport