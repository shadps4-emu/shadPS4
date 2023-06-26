#include "../../../types.h"
#include "SymbolsResolver.h"


void SymbolsResolver::AddSymbol(const SymbolRes& s, u64 virtual_addr)
{
	SymbolRecord r{};
	char str[256];
	sprintf(str, "%s (%s)[%s_v%d][%s_v%d.%d]", s.name.c_str(),s.nidName.c_str(), s.library.c_str(), s.library_version, s.module.c_str(),s.module_version_major, s.module_version_minor);
	r.name = std::string(str);
	r.virtual_address = virtual_addr;
	m_symbols.push_back(r);
}