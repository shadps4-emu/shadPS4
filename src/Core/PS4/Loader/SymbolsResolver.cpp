#include "../../../types.h"
#include "SymbolsResolver.h"
#include <Util/log.h>


void SymbolsResolver::AddSymbol(const SymbolRes& s, u64 virtual_addr)
{
	SymbolRecord r{};
    r.name = GenerateName(s);
	r.virtual_address = virtual_addr;
	m_symbols.push_back(r);
}

std::string SymbolsResolver::GenerateName(const SymbolRes& s) {
    char str[256];
    sprintf(str, "%s lib[%s_v%d]mod[%s_v%d.%d]", s.name.c_str(),s.library.c_str(), s.library_version, s.module.c_str(),
            s.module_version_major, s.module_version_minor);
    return std::string(str);
}

const SymbolRecord* SymbolsResolver::FindSymbol(const SymbolRes& s) const { 
	std::string name = GenerateName(s);
    int index = 0;
    for (auto symbol : m_symbols) {
        if (symbol.name.compare(name) == 0) {
            return &m_symbols.at(index);
        }
        index++;
    }
    LOG_INFO_IF(true, "unresolved! {}\n", name);
	return nullptr; 
}