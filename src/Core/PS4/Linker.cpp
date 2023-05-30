#include "Linker.h"
#include "../Memory.h"
#include "../../Util/Log.h"
#include "../../Util/Disassembler.h"

constexpr bool debug_loader = true;

static u64 g_load_addr = SYSTEM_RESERVED + CODE_BASE_OFFSET;

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

	u64 base_size = calculate_base_size(elf_header, elf_pheader);
	m->aligned_base_size = (base_size & ~(static_cast<u64>(0x1000) - 1)) + 0x1000;//align base size to 0x1000 block size (TODO is that the default block size or it can be changed?

	m->base_virtual_addr = Memory::VirtualMemory::memory_alloc(g_load_addr, m->aligned_base_size);

	LOG_INFO_IF(debug_loader, "====Load Module to Memory ========\n");
	LOG_INFO_IF(debug_loader, "base_virtual_addr ......: {:#018x}\n", m->base_virtual_addr);
	LOG_INFO_IF(debug_loader, "base_size ..............: {:#018x}\n", base_size);
	LOG_INFO_IF(debug_loader, "aligned_base_size ......: {:#018x}\n", m->aligned_base_size);

	for (u16 i = 0; i < elf_header->e_phnum; i++)
	{
		switch(elf_pheader[i].p_type)
		{
			case PT_LOAD:
			case PT_SCE_RELRO:
				if (elf_pheader[i].p_memsz != 0)
				{
					u64 segment_addr = elf_pheader[i].p_vaddr + m->base_virtual_addr;
					u64 segment_file_size = elf_pheader[i].p_filesz;
					u64 segment_memory_size = get_aligned_size(elf_pheader + i);
					auto segment_mode = m->elf->ElfPheaderFlagsStr((elf_pheader + i)->p_flags);
					LOG_INFO_IF(debug_loader, "program header = [{}] type = {}\n",i,m->elf->ElfPheaderTypeStr(elf_pheader[i].p_type));
					LOG_INFO_IF(debug_loader, "segment_addr ..........: {:#018x}\n", segment_addr);
					LOG_INFO_IF(debug_loader, "segment_file_size .....: {}\n", segment_file_size);
					LOG_INFO_IF(debug_loader, "segment_memory_size ...: {}\n", segment_memory_size);
					LOG_INFO_IF(debug_loader, "segment_mode ..........: {}\n", segment_mode);
					
					m->elf->LoadSegment(segment_addr, elf_pheader[i].p_offset, segment_file_size);
				}
				else
				{
					LOG_ERROR_IF(debug_loader, "p_memsz==0 in type {}\n", m->elf->ElfPheaderTypeStr(elf_pheader[i].p_type));
				}
				break;
			case PT_DYNAMIC:
				if (elf_pheader[i].p_filesz != 0)
				{
					void* dynamic = new u08[elf_pheader[i].p_filesz];
					m->elf->LoadSegment(reinterpret_cast<u64>(dynamic), elf_pheader[i].p_offset, elf_pheader[i].p_filesz);
					m->m_dynamic = dynamic;
				}
				else
				{
					LOG_ERROR_IF(debug_loader, "p_filesz==0 in type {}\n", m->elf->ElfPheaderTypeStr(elf_pheader[i].p_type));
				}
				break;
			case PT_SCE_DYNLIBDATA:
				if (elf_pheader[i].p_filesz != 0)
				{
					void* dynamic = new u08[elf_pheader[i].p_filesz];
					m->elf->LoadSegment(reinterpret_cast<u64>(dynamic), elf_pheader[i].p_offset, elf_pheader[i].p_filesz);
					m->m_dynamic_data = dynamic;
				}
				else
				{
					LOG_ERROR_IF(debug_loader, "p_filesz==0 in type {}\n", m->elf->ElfPheaderTypeStr(elf_pheader[i].p_type));
				}
				break;
			default:
				LOG_ERROR_IF(debug_loader, "Unimplemented type {}\n", m->elf->ElfPheaderTypeStr(elf_pheader[i].p_type));
		}
	}
	LOG_INFO_IF(debug_loader, "program entry addr ..........: {:#018x}\n", m->elf->GetElfEntry() + m->base_virtual_addr);

	auto* rt1 = reinterpret_cast<uint8_t*>(m->elf->GetElfEntry() + m->base_virtual_addr);
	ZyanU64 runtime_address = m->elf->GetElfEntry() + m->base_virtual_addr;

	// Loop over the instructions in our buffer.
	ZyanUSize offset = 0;
	ZydisDisassembledInstruction instruction;
	while (ZYAN_SUCCESS(ZydisDisassembleIntel(
		/* machine_mode:    */ ZYDIS_MACHINE_MODE_LONG_64,
		/* runtime_address: */ runtime_address,
		/* buffer:          */ rt1 + offset,
		/* length:          */ sizeof(rt1) - offset,
		/* instruction:     */ &instruction
	))) {
		printf("%016" PRIX64 "  %s\n", runtime_address, instruction.text);
		offset += instruction.info.length;
		runtime_address += instruction.info.length;
	}
}