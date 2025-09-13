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

struct SymbolRecord {
    std::string name;
    std::string nid_name;
    u64 virtual_address;
};

struct SymbolResolver {
    std::string name;
    std::string nidName;
    std::string library;
    u16 library_version;
    std::string module;
    SymbolType type;
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

    static std::string GenerateName(const SymbolResolver& s);

    static std::string_view SymbolTypeToS(SymbolType sym_type) {
        switch (sym_type) {
        case SymbolType::Unknown:
            return "Unknown";
        case SymbolType::Function:
            return "Function";
        case SymbolType::Object:
            return "Object";
        case SymbolType::Tls:
            return "Tls";
        case SymbolType::NoType:
            return "NoType";
        default:
            UNREACHABLE();
        }
    }

private:
    std::vector<SymbolRecord> m_symbols;
};

} // namespace Core::Loader
