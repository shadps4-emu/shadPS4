// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/loader/elf.h"
#include "core/loader/symbols_resolver.h"

#define LIB_FUNCTION(nid, lib, libversion, mod, moduleVersionMajor, moduleVersionMinor, function)  \
    {                                                                                              \
        Core::Loader::SymbolRes sr{};                                                              \
        sr.name = nid;                                                                             \
        sr.library = lib;                                                                          \
        sr.library_version = libversion;                                                           \
        sr.module = mod;                                                                           \
        sr.module_version_major = moduleVersionMajor;                                              \
        sr.module_version_minor = moduleVersionMinor;                                              \
        sr.type = STT_FUN;                                                                         \
        auto func = reinterpret_cast<u64>(function);                                               \
        sym->AddSymbol(sr, func);                                                                  \
    }

#define LIB_OBJ(nid, lib, libversion, mod, moduleVersionMajor, moduleVersionMinor, function)       \
    {                                                                                              \
        Core::Loader::SymbolRes sr{};                                                              \
        sr.name = nid;                                                                             \
        sr.library = lib;                                                                          \
        sr.library_version = libversion;                                                           \
        sr.module = mod;                                                                           \
        sr.module_version_major = moduleVersionMajor;                                              \
        sr.module_version_minor = moduleVersionMinor;                                              \
        sr.type = STT_OBJECT;                                                                      \
        auto func = reinterpret_cast<u64>(function);                                               \
        sym->AddSymbol(sr, func);                                                                  \
    }

#define PRINT_FUNCTION_NAME()                                                                      \
    { LOG_INFO_IF(true, "{}()\n", __func__); }

#define PRINT_DUMMY_FUNCTION_NAME()                                                                \
    { LOG_WARN_IF(true, "dummy {}()\n", __func__); }

#define PRINT_UNIMPLEMENTED_FUNCTION_NAME()                                                        \
    { LOG_ERROR_IF(true, "{}()\n", __func__); }

namespace Core::Libraries {

void InitHLELibs(Loader::SymbolsResolver* sym);

} // namespace Core::Libraries
