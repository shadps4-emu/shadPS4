// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <fmt/format.h>
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

void SymbolsResolver::DebugDump(const std::filesystem::path& file_name) {
    Common::FS::IOFile f{file_name, Common::FS::FileAccessMode::Write,
                         Common::FS::FileType::TextFile};
    for (const auto& symbol : m_symbols) {
        const auto ids = Common::SplitString(symbol.name, '#');
        const auto aeronid = AeroLib::FindByNid(ids.at(0).c_str());
        const auto nid_name = aeronid ? aeronid->name : "UNK";
        f.WriteString(
            fmt::format("0x{:<20x} {:<16} {:<60} {:<30} {:<2} {:<30} {:<2} {:<2} {:<10}\n",
                        symbol.virtual_address, ids.at(0), nid_name, ids.at(1), ids.at(2),
                        ids.at(3), ids.at(4), ids.at(5), ids.at(6)));
    }
}

} // namespace Core::Loader
