#include "Linker.h"
#include "../Memory.h"

Linker::Linker()
{
}

Linker::~Linker()
{
}

static u64 get_aligned_size(const elf_program_header* phdr)
{
	return (phdr->p_align != 0 ? (phdr->p_memsz + (phdr->p_align - 1)) & ~(phdr->p_align - 1) : phdr->p_memsz);
}

static u64 calculate_base_size(const elf_header* ehdr, const elf_program_header* phdr)
{
	u64 base_size = 0;
	for (u16 i = 0; i < ehdr->e_phnum; i++)
	{
		if (phdr[i].p_memsz != 0 && (phdr[i].p_type == PT_LOAD || phdr[i].p_type == PT_SCE_RELRO))
		{
			auto phdrh = phdr + i;
			u64 last_addr = phdr[i].p_vaddr + get_aligned_size(phdrh);
			if (last_addr > base_size)
			{
				base_size = last_addr;
			}
		}
	}
	return base_size;
}

Module* Linker::LoadModule(const std::string& elf_name)
{
	auto* m = new Module;
	m->elf = new Elf;
	m->elf->Open(elf_name);//load elf
	
	if (m->elf->isElfFile())
	{
		LoadModuleToMemory(m);
	}
	else
	{
		return nullptr;//it is not a valid elf file //TODO check it why!
	}
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

void Linker::LoadModuleToMemory(Module* m)
{
	//get elf header, program header
	auto* elf_header = m->elf->GetElfHeader();
	auto* elf_pheader = m->elf->GetProgramHeader();

	u64 base_size = calculate_base_size(elf_header,elf_pheader);
	m->aligned_base_size = (base_size & ~(static_cast<u64>(0x1000) - 1)) + 0x1000;//align base size to 0x1000 block size (TODO is that the default block size or it can be changed?


}