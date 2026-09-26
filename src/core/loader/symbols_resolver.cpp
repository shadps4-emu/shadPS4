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
    m_symbols.emplace_back(s, virtual_addr);
}

std::string_view SymbolsResolver::SymbolTypeToS(SymbolType sym_type) {
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

const SymbolRecord* SymbolsResolver::FindSymbol(const SymbolResolver& s) const {
    for (u32 i = 0; i < m_symbols.size(); i++) {
        if (m_symbols[i].symbol == s) {
            return &m_symbols[i];
        }
    }

    // LOG_INFO(Core_Linker, "Unresolved! {}", name);
    return nullptr;
}

void SymbolsResolver::DebugDump(const std::filesystem::path& file_name) {
    Common::FS::IOFile f{file_name, Common::FS::FileAccessMode::Create,
                         Common::FS::FileType::TextFile};
    for (const auto& symbol : m_symbols) {
        const auto id = symbol.symbol;
        const auto aeronid = AeroLib::FindByNid(id.name.c_str());
        const auto nid_name = aeronid ? aeronid->name : "UNK";
        f.WriteString(fmt::format("0x{:<20x} {:<16} {:<60} {:<30} {:<2} {:<30} {:<10}\n",
                                  symbol.virtual_address, id.name, nid_name, id.library,
                                  id.library_version, id.module, SymbolTypeToS(id.type)));
    }
}

bool SymbolResolver::operator==(SymbolResolver const& o) const {
    // not checking the version and type as they're ignored on real hardware as well
    return this->name == o.name && this->library == o.library && this->module == o.module;
}

} // namespace Core::Loader
