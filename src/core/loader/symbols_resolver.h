// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <filesystem>
#include <span>
#include <string>
#include <vector>
#include "common/assert.h"
#include "common/types.h"

namespace Core::Loader {

enum class SymbolType {
    Unknown,
    Function,
    Object,
    Tls,
    NoType,
};

struct SymbolResolver {
    std::string name;
    std::string nidName;
    std::string library;
    u16 library_version;
    std::string module;
    SymbolType type;
    bool operator==(SymbolResolver const& o) const;
};

struct SymbolRecord {
    SymbolResolver symbol;
    u64 virtual_address;
};

class SymbolsResolver {
public:
    SymbolsResolver() = default;
    virtual ~SymbolsResolver() = default;

    void AddSymbol(const SymbolResolver& s, u64 virtual_addr);
    const SymbolRecord* FindSymbol(const SymbolResolver& s) const;

    void DebugDump(const std::filesystem::path& file_name);

    std::span<const SymbolRecord> GetSymbols() const {
        return m_symbols;
    }

    size_t GetSize() const noexcept {
        return m_symbols.size();
    }

    static std::string_view SymbolTypeToS(SymbolType sym_type);

private:
    std::vector<SymbolRecord> m_symbols;
};

} // namespace Core::Loader
