// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <functional>

#include "common/logging/log.h"
#include "core/loader/elf.h"
#include "core/loader/symbols_resolver.h"

#define W(foo) foo

#define LIB_FUNCTION(nid, lib, libversion, mod, moduleVersionMajor, moduleVersionMinor, function)  \
    {                                                                                              \
        Core::Loader::SymbolResolver sr{};                                                         \
        sr.name = nid;                                                                             \
        sr.library = lib;                                                                          \
        sr.library_version = libversion;                                                           \
        sr.module = mod;                                                                           \
        sr.module_version_major = moduleVersionMajor;                                              \
        sr.module_version_minor = moduleVersionMinor;                                              \
        sr.type = Core::Loader::SymbolType::Function;                                              \
        auto func = reinterpret_cast<u64>(function);                                               \
        sym->AddSymbol(sr, func);                                                                  \
    }

#define LIB_OBJ(nid, lib, libversion, mod, moduleVersionMajor, moduleVersionMinor, function)       \
    {                                                                                              \
        Core::Loader::SymbolResolver sr{};                                                         \
        sr.name = nid;                                                                             \
        sr.library = lib;                                                                          \
        sr.library_version = libversion;                                                           \
        sr.module = mod;                                                                           \
        sr.module_version_major = moduleVersionMajor;                                              \
        sr.module_version_minor = moduleVersionMinor;                                              \
        sr.type = Core::Loader::SymbolType::Object;                                                \
        auto func = reinterpret_cast<u64>(function);                                               \
        sym->AddSymbol(sr, func);                                                                  \
    }

namespace Libraries {

void InitHLELibs(Core::Loader::SymbolsResolver* sym);

} // namespace Libraries
