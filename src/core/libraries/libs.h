// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/loader/elf.h"
#include "core/loader/symbols_resolver.h"
#include "core/tls.h"

#define LIB_FUNCTION(nid, lib, libversion, mod, function)                                          \
    do {                                                                                           \
        Core::Loader::SymbolResolver sr{};                                                         \
        sr.name = nid;                                                                             \
        sr.library = lib;                                                                          \
        sr.library_version = libversion;                                                           \
        sr.module = mod;                                                                           \
        sr.type = Core::Loader::SymbolType::Function;                                              \
        auto func = reinterpret_cast<u64>(HOST_CALL(function));                                    \
        sym->AddSymbol(sr, func);                                                                  \
    } while (0)

#define LIB_OBJ(nid, lib, libversion, mod, obj)                                                    \
    do {                                                                                           \
        Core::Loader::SymbolResolver sr{};                                                         \
        sr.name = nid;                                                                             \
        sr.library = lib;                                                                          \
        sr.library_version = libversion;                                                           \
        sr.module = mod;                                                                           \
        sr.type = Core::Loader::SymbolType::Object;                                                \
        sym->AddSymbol(sr, reinterpret_cast<u64>(obj));                                            \
    } while (0)

namespace Libraries {

void InitHLELibs(Core::Loader::SymbolsResolver* sym);

} // namespace Libraries
