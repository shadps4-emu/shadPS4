#include "Linker.h"
#include "../Memory.h"
#include "../../Util/Log.h"
#include "../../Util/Disassembler.h"
#include "../../Util/StringUtil.h"

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

static std::string encodeId(u64 nVal)
{
	std::string enc;
	const char pCodes[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+-";
	if (nVal < 0x40u)
	{
		enc += pCodes[nVal];
	}
	else
	{
		if (nVal < 0x1000u)
		{
			enc += pCodes[static_cast<u16>(nVal >> 6u) & 0x3fu];
			enc += pCodes[nVal & 0x3fu];
		}
		else
		{
			enc += pCodes[static_cast<u16>(nVal >> 12u) & 0x3fu];
			enc += pCodes[static_cast<u16>(nVal >> 6u) & 0x3fu];
			enc += pCodes[nVal & 0x3fu];
		}
	}
	return enc;
}
Module* Linker::LoadModule(const std::string& elf_name)
{
	auto* m = new Module;
	m->elf = new Elf;
	m->elf->Open(elf_name);//load elf

	if (m->elf->isElfFile())
	{
		LoadModuleToMemory(m);
		LoadDynamicInfo(m);
		LoadSymbols(m);
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

void Linker::LoadDynamicInfo(Module* m)
{
	m->dynamic_info = new DynamicModuleInfo;

	for (const auto* dyn = static_cast<elf_dynamic*>(m->m_dynamic); dyn->d_tag != DT_NULL; dyn++)
	{
		switch (dyn->d_tag)
		{
		case DT_SCE_HASH: //Offset of the hash table.
			m->dynamic_info->hash_table = reinterpret_cast<void*>(static_cast<uint8_t*>(m->m_dynamic_data) + dyn->d_un.d_ptr);
			break;
		case DT_SCE_HASHSZ: //Size of the hash table
			m->dynamic_info->hash_table_size = dyn->d_un.d_val;
			break;
		case DT_SCE_STRTAB://Offset of the string table. 
			m->dynamic_info->str_table = reinterpret_cast<char*>(static_cast<uint8_t*>(m->m_dynamic_data) + dyn->d_un.d_ptr);
			break;
		case DT_SCE_STRSZ: //Size of the string table.
			m->dynamic_info->str_table_size = dyn->d_un.d_val;
			break;
		case DT_SCE_SYMTAB://Offset of the symbol table.
			m->dynamic_info->symbol_table = reinterpret_cast<elf_symbol*>(static_cast<uint8_t*>(m->m_dynamic_data) + dyn->d_un.d_ptr);
			break;
		case DT_SCE_SYMTABSZ://Size of the symbol table.
			m->dynamic_info->symbol_table_total_size = dyn->d_un.d_val;
			break;
		case DT_INIT:
			m->dynamic_info->init_virtual_addr = dyn->d_un.d_ptr;
			break;
		case DT_FINI:
			m->dynamic_info->fini_virtual_addr = dyn->d_un.d_ptr;
			break;
		case DT_SCE_PLTGOT: //Offset of the global offset table.
			m->dynamic_info->pltgot_virtual_addr = dyn->d_un.d_ptr;
			break;
		case DT_SCE_JMPREL: //Offset of the table containing jump slots.
			m->dynamic_info->jmp_relocation_table = reinterpret_cast<elf_relocation*>(static_cast<uint8_t*>(m->m_dynamic_data) + dyn->d_un.d_ptr);
			break;
		case DT_SCE_PLTRELSZ: //Size of the global offset table.
			m->dynamic_info->jmp_relocation_table_size = dyn->d_un.d_val;
			break;
		case DT_SCE_PLTREL: //The type of relocations in the relocation table. Should be DT_RELA
			m->dynamic_info->jmp_relocation_type = dyn->d_un.d_val;
			if (m->dynamic_info->jmp_relocation_type != DT_RELA)
			{
				LOG_WARN_IF(debug_loader, "DT_SCE_PLTREL is NOT DT_RELA should check!");
			}
			break;
		case DT_SCE_RELA: //Offset of the relocation table.
			m->dynamic_info->relocation_table = reinterpret_cast<elf_relocation*>(static_cast<uint8_t*>(m->m_dynamic_data) + dyn->d_un.d_ptr);
			break;
		case DT_SCE_RELASZ: //Size of the relocation table.
			m->dynamic_info->relocation_table_size = dyn->d_un.d_val;
			break;
		case DT_SCE_RELAENT : //The size of relocation table entries.
			m->dynamic_info->relocation_table_entries_size = dyn->d_un.d_val;
			if (m->dynamic_info->relocation_table_entries_size != 0x18) //this value should always be 0x18
			{
				LOG_WARN_IF(debug_loader, "DT_SCE_RELAENT is NOT 0x18 should check!");
			}
			break;
		case DT_INIT_ARRAY:// Address of the array of pointers to initialization functions 
			m->dynamic_info->init_array_virtual_addr = dyn->d_un.d_ptr;
			break;
		case DT_FINI_ARRAY: // Address of the array of pointers to termination functions
			m->dynamic_info->fini_array_virtual_addr = dyn->d_un.d_ptr;
			break;
		case DT_INIT_ARRAYSZ://Size in bytes of the array of initialization functions
			m->dynamic_info->init_array_size = dyn->d_un.d_val;
			break;
		case DT_FINI_ARRAYSZ://Size in bytes of the array of terminationfunctions
			m->dynamic_info->fini_array_size = dyn->d_un.d_val;
			break;
		case DT_PREINIT_ARRAY://Address of the array of pointers to pre - initialization functions
			m->dynamic_info->preinit_array_virtual_addr = dyn->d_un.d_ptr;
			break;
		case DT_PREINIT_ARRAYSZ://Size in bytes of the array of pre - initialization functions
			m->dynamic_info->preinit_array_size = dyn->d_un.d_val;
			break;
		case DT_SCE_SYMENT: //The size of symbol table entries
			m->dynamic_info->symbol_table_entries_size = dyn->d_un.d_val;
			if (m->dynamic_info->symbol_table_entries_size != 0x18) //this value should always be 0x18
			{
				LOG_WARN_IF(debug_loader, "DT_SCE_SYMENT is NOT 0x18 should check!");
			}
			break;
		case DT_DEBUG:
			m->dynamic_info->debug = dyn->d_un.d_val;
			break;
		case DT_TEXTREL:
			m->dynamic_info->textrel = dyn->d_un.d_val;
			break;
		case DT_FLAGS:
			m->dynamic_info->flags = dyn->d_un.d_val;
			if (m->dynamic_info->flags != 0x04) //this value should always be DF_TEXTREL (0x04)
			{
				LOG_WARN_IF(debug_loader, "DT_FLAGS is NOT 0x04 should check!");
			}
			break;
		case DT_NEEDED://Offset of the library string in the string table to be linked in.
			if (m->dynamic_info->str_table != nullptr)//in theory this should already be filled from about just make a test case
			{
				m->dynamic_info->needed.push_back(m->dynamic_info->str_table + dyn->d_un.d_val);
			}
			else
			{
				LOG_ERROR_IF(debug_loader, "DT_NEEDED str table is not loaded should check!");
			}
			break;
		case DT_SCE_NEEDED_MODULE:
			{
				ModuleInfo info{};
				info.value = dyn->d_un.d_val;
				info.name = m->dynamic_info->str_table + info.name_offset;
				info.enc_id = encodeId(info.id);
				m->dynamic_info->import_modules.push_back(info);
			}
			break;
		case DT_SCE_IMPORT_LIB:
			{
				LibraryInfo info{};
				info.value = dyn->d_un.d_val;
				info.name = m->dynamic_info->str_table + info.name_offset;
				info.enc_id = encodeId(info.id);
				m->dynamic_info->import_libs.push_back(info);
			}
			break;
		case DT_SCE_FINGERPRINT:
			//The fingerprint is a 24 byte (0x18) size buffer that contains a unique identifier for the given app. 
			//How exactly this is generated isn't known, however it is not necessary to have a valid fingerprint. 
			//While an invalid fingerprint will cause a warning to be printed to the kernel log, the ELF will still load and run.
			LOG_INFO_IF(debug_loader, "unsupported DT_SCE_FINGERPRINT value = ..........: {:#018x}\n", dyn->d_un.d_val);
			break;
		case DT_SCE_IMPORT_LIB_ATTR:
			//The upper 32-bits should contain the module index multiplied by 0x10000. The lower 32-bits should be a constant 0x9.
			LOG_INFO_IF(debug_loader, "unsupported DT_SCE_IMPORT_LIB_ATTR value = ..........: {:#018x}\n", dyn->d_un.d_val);
			break;
		case DT_SCE_ORIGINAL_FILENAME:
			m->dynamic_info->filename = m->dynamic_info->str_table + dyn->d_un.d_val;
			break;
		case DT_SCE_MODULE_INFO://probably only useable in shared modules
			{
				ModuleInfo info{};
				info.value = dyn->d_un.d_val;
				info.name = m->dynamic_info->str_table + info.name_offset;
				info.enc_id = encodeId(info.id);
				m->dynamic_info->export_modules.push_back(info);
			}
			break;
		case DT_SCE_MODULE_ATTR:
			//TODO?
			LOG_INFO_IF(debug_loader, "unsupported DT_SCE_MODULE_ATTR value = ..........: {:#018x}\n", dyn->d_un.d_val);
			break;
		case DT_SCE_EXPORT_LIB:
			{
				LibraryInfo info{};
				info.value = dyn->d_un.d_val;
				info.name = m->dynamic_info->str_table + info.name_offset;
				info.enc_id = encodeId(info.id);
				m->dynamic_info->export_libs.push_back(info);
			}
			break;
		default:
			LOG_INFO_IF(debug_loader, "unsupported dynamic tag ..........: {:#018x}\n", dyn->d_tag);
		}
		
	}
}

const ModuleInfo* Linker::FindModule(const Module& m, const std::string& id)
{
	const auto& import_modules = m.dynamic_info->import_modules;
	int index = 0;
	for (auto mod : import_modules)
	{
		if (mod.enc_id.compare(id) == 0)
		{
			return &import_modules.at(index);
		}
		index++;
	}
	const auto& export_modules = m.dynamic_info->export_modules;
	index = 0;
	for (auto mod : export_modules)
	{
		if (mod.enc_id.compare(id) == 0)
		{
			return &export_modules.at(index);
		}
		index++;
	}
	return nullptr;
}

const LibraryInfo* Linker::FindLibrary(const Module& m, const std::string& id)
{
	const auto& import_libs = m.dynamic_info->import_libs;
	int index = 0;
	for (auto lib : import_libs)
	{
		if (lib.enc_id.compare(id) == 0)
		{
			return &import_libs.at(index);
		}
		index++;
	}
	const auto& export_libs = m.dynamic_info->export_libs;
	index = 0;
	for (auto lib : export_libs)
	{
		if (lib.enc_id.compare(id) == 0)
		{
			return &export_libs.at(index);
		}
		index++;
	}
	return nullptr;
}

void Linker::LoadSymbols(Module* m)
{
	if (m->dynamic_info->symbol_table == nullptr || m->dynamic_info->str_table == nullptr || m->dynamic_info->symbol_table_total_size==0)
	{
		LOG_INFO_IF(debug_loader, "Symbol table not found!\n");
		return;
	}
	for (auto* sym = m->dynamic_info->symbol_table;
		reinterpret_cast<uint8_t*>(sym) < reinterpret_cast<uint8_t*>(m->dynamic_info->symbol_table) + m->dynamic_info->symbol_table_total_size;
		sym++)
	{
		std::string id = std::string(m->dynamic_info->str_table + sym->st_name);
		auto ids = StringUtil::split(id, '#');
		if (ids.size() == 3)//symbols are 3 parts name , library , module 
		{
			const auto* library = FindLibrary(*m, ids.at(1));
			const auto* module = FindModule(*m, ids.at(2));
			auto bind = sym->GetBind();
			auto type = sym->GetType();
			auto visibility = sym->GetVisibility();
			if (library != nullptr || module != nullptr)
			{
				LOG_INFO_IF(debug_loader, "name {} library {} module {} bind {} type {} visibility {}\n", ids.at(0),library->name,module->name,bind,type,visibility);
			}
		}
	}
}