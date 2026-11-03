// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/types.h"
#include "core/loader/symbols_resolver.h"

void LinkSymbolImpl(Core::Loader::SymbolsResolver* sym, char const* nid, char const* lib,
                    u16 libversion, char const* mod, u64 symbol,
                    Core::Loader::SymbolType sym_type) {}

namespace Core::Loader {

void SymbolsResolver::AddSymbol(const SymbolResolver& /*sym*/, u64 /*addr*/) {}

} // namespace Core::Loader
