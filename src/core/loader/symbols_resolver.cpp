// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <fmt/format.h>
#include <array>
#include "common/io_file.h"
#include "common/string_util.h"
#include "common/types.h"
#include "core/aerolib/aerolib.h"
#include "core/loader/symbols_resolver.h"

namespace Core::Loader {

void SymbolsResolver::AddSymbol(const SymbolResolver& s, u64 virtual_addr) {
    m_symbols.emplace_back(GenerateName(s), s.nidName, virtual_addr);
}

std::string SymbolsResolver::GenerateName(const SymbolResolver& s) {
    return fmt::format("{}#{}#{}#{}#{}", s.name, s.library, s.library_version, s.module,
                       SymbolTypeToS(s.type));
}

const SymbolRecord* SymbolsResolver::FindSymbol(const SymbolResolver& s) const {
    const std::string name = GenerateName(s);
    for (u32 i = 0; i < m_symbols.size(); i++) {
        if (m_symbols[i].name == name) {
            return &m_symbols[i];
        }
    }

    // LOG_INFO(Core_Linker, "Unresolved! {}", name);
    return nullptr;
}

const SymbolRecord* SymbolsResolver::FindSymbolByNid(std::string_view nid,
                                                      SymbolType preferred_type) const {
    if (nid.empty()) {
        return nullptr;
    }

    const auto find_by_type = [&](SymbolType type) -> const SymbolRecord* {
        for (const auto& symbol : m_symbols) {
            const std::string_view full_name{symbol.name};
            const size_t first_sep = full_name.find('#');
            if (first_sep == std::string_view::npos || full_name.substr(0, first_sep) != nid) {
                continue;
            }
            if (type == SymbolType::Unknown) {
                return &symbol;
            }
            const size_t last_sep = full_name.rfind('#');
            if (last_sep == std::string_view::npos) {
                continue;
            }
            if (full_name.substr(last_sep + 1) == SymbolTypeToS(type)) {
                return &symbol;
            }
        }
        return nullptr;
    };

    if (const auto* record = find_by_type(preferred_type)) {
        return record;
    }

    static constexpr std::array<SymbolType, 4> FallbackTypes = {
        SymbolType::Function,
        SymbolType::Object,
        SymbolType::NoType,
        SymbolType::Unknown,
    };
    for (const auto type : FallbackTypes) {
        if (type == preferred_type) {
            continue;
        }
        if (const auto* record = find_by_type(type)) {
            return record;
        }
    }
    return nullptr;
}

void SymbolsResolver::DebugDump(const std::filesystem::path& file_name) {
    Common::FS::IOFile f{file_name, Common::FS::FileAccessMode::Create,
                         Common::FS::FileType::TextFile};
    for (const auto& symbol : m_symbols) {
        const auto ids = Common::SplitString(symbol.name, '#');
        const auto aeronid = AeroLib::FindByNid(ids.at(0).c_str());
        const auto nid_name = aeronid ? aeronid->name : "UNK";
        f.WriteString(fmt::format("0x{:<20x} {:<16} {:<60} {:<30} {:<2} {:<30} {:<10}\n",
                                  symbol.virtual_address, ids.at(0), nid_name, ids.at(1), ids.at(2),
                                  ids.at(3), ids.at(4)));
    }
}

} // namespace Core::Loader
