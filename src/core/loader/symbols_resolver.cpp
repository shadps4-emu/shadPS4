#include "common/log.h"
#include "common/types.h"
#include "core/loader/symbols_resolver.h"

namespace Core::Loader {

void SymbolsResolver::AddSymbol(const SymbolRes& s, u64 virtual_addr) {
    SymbolRecord r{};
    r.name = GenerateName(s);
    r.virtual_address = virtual_addr;
    m_symbols.push_back(r);
}

std::string SymbolsResolver::GenerateName(const SymbolRes& s) {
    return fmt::format("{} lib[{}_v{}]mod[{}_v{}.{}]", s.name, s.library, s.library_version,
                       s.module, s.module_version_major, s.module_version_minor);
}

const SymbolRecord* SymbolsResolver::FindSymbol(const SymbolRes& s) const {
    const std::string name = GenerateName(s);
    for (u32 i = 0; i < m_symbols.size(); i++) {
        if (m_symbols[i].name.compare(name) == 0) {
            return &m_symbols[i];
        }
    }

    LOG_INFO("Unresolved! {}\n", name);
    return nullptr;
}

} // namespace Core::Loader
