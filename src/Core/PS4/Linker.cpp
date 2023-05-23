#include "Linker.h"

Linker::Linker()
{
}

Linker::~Linker()
{
}

Module* Linker::LoadModule(const std::string& elf_name)
{
	auto* m = new Module;
	m->elf = new Elf;
	m->elf->Open(elf_name);//load elf

	m_modules.push_back(m);//added it to load modules

	return m;
}

Module* Linker::FindModule(/*u32 id*/)
{
	//find module . TODO atm we only have 1 module so we don't need to iterate on vector
	Module* m = m_modules.at(0);

	if (m)
	{
		return m;
	}
	return nullptr;
}