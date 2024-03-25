// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/io_file.h"
#include "common/logging/log.h"
#include "common/string_util.h"
#include "common/types.h"
#include "core/aerolib/aerolib.h"
#include "core/loader/symbols_resolver.h"

namespace Core::Loader {

void SymbolsResolver::AddSymbol(const SymbolResolver& s, u64 virtual_addr) {
    SymbolRecord& r = m_symbols.emplace_back();
    r.name = GenerateName(s);
    r.virtual_address = virtual_addr;
}

std::string SymbolsResolver::GenerateName(const SymbolResolver& s) {
    return fmt::format("{}#{}#{}#{}#{}#{}#{}", s.name, s.library, s.library_version, s.module,
                       s.module_version_major, s.module_version_minor, SymbolTypeToS(s.type));
}

const SymbolRecord* SymbolsResolver::FindSymbol(const SymbolResolver& s) const {
    const std::string name = GenerateName(s);
    for (u32 i = 0; i < m_symbols.size(); i++) {
        if (m_symbols[i].name == name) {
            return &m_symbols[i];
        }
    }

    LOG_INFO(Core_Linker, "Unresolved! {}", name);
    return nullptr;
}

void SymbolsResolver::DebugDump(const std::filesystem::path& file_name) {
    Common::FS::IOFile f{file_name, Common::FS::FileAccessMode::Write,
                         Common::FS::FileType::TextFile};
    for (const auto& symbol : m_symbols) {
        const auto ids = Common::SplitString(symbol.name, '#');
        std::string nidName = "";
        auto aeronid = AeroLib::FindByNid(ids.at(0).c_str());
        if (aeronid != nullptr) {
            nidName = aeronid->name;
        } else {
            nidName = "UNK";
        }
        f.WriteString(fmt::format("0x{:<20x} {:<16} {:<60} {:<30} {:<2} {:<30} {:<2} {:<2} {:<10}\n",
                                  symbol.virtual_address, ids.at(0), nidName, ids.at(1), ids.at(2),
                                  ids.at(3), ids.at(4), ids.at(5), ids.at(6)));
    }
}

} // namespace Core::Loader
