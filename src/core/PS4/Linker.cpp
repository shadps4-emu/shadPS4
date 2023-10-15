#include "Linker.h"
#include "../virtual_memory.h"
#include <Util/log.h>
#include <fmt/core.h>
#include "Zydis.h"
#include <Util/string_util.h>
#include "Util/aerolib.h"
#include "Loader/SymbolsResolver.h"
#include "HLE/Kernel/ThreadManagement.h"
#include "Stubs.h"

#include "../third-party/xbyak/xbyak/xbyak.h"

#include <set>

constexpr bool debug_loader = true;

static u64 g_load_addr = SYSTEM_RESERVED + CODE_BASE_OFFSET;

static u64 get_aligned_size(const elf_program_header& phdr)
{
    return (phdr.p_align != 0 ? (phdr.p_memsz + (phdr.p_align - 1)) & ~(phdr.p_align - 1) : phdr.p_memsz);
}

static u64 calculate_base_size(const elf_header& ehdr, std::span<const elf_program_header> phdr)
{
    u64 base_size = 0;
    for (u16 i = 0; i < ehdr.e_phnum; i++)
    {
        if (phdr[i].p_memsz != 0 && (phdr[i].p_type == PT_LOAD || phdr[i].p_type == PT_SCE_RELRO))
        {
            u64 last_addr = phdr[i].p_vaddr + get_aligned_size(phdr[i]);
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

Linker::Linker() = default;

Linker::~Linker() = default;

Module* Linker::LoadModule(const std::string& elf_name)
{
    std::scoped_lock lock{m_mutex};

    auto& m = m_modules.emplace_back();
    m.linker = this;
    m.elf.Open(elf_name);

    if (m.elf.isElfFile()) {
        LoadModuleToMemory(&m);
        LoadDynamicInfo(&m);
        LoadSymbols(&m);
        Relocate(&m);
    } else {
        m_modules.pop_back();
        return nullptr; // It is not a valid elf file //TODO check it why!
    }

    return &m;
}

Module* Linker::FindModule(/*u32 id*/)
{
    // TODO atm we only have 1 module so we don't need to iterate on vector
    if (m_modules.empty()) [[unlikely]] {
        return nullptr;
    }
    return &m_modules[0];
}

void Linker::LoadModuleToMemory(Module* m)
{
	//get elf header, program header
    const auto elf_header = m->elf.GetElfHeader();
    const auto elf_pheader = m->elf.GetProgramHeader();

	u64 base_size = calculate_base_size(elf_header, elf_pheader);
	m->aligned_base_size = (base_size & ~(static_cast<u64>(0x1000) - 1)) + 0x1000;//align base size to 0x1000 block size (TODO is that the default block size or it can be changed?

	m->base_virtual_addr = VirtualMemory::memory_alloc(g_load_addr, m->aligned_base_size, VirtualMemory::MemoryMode::ExecuteReadWrite);

	LOG_INFO_IF(debug_loader, "====Load Module to Memory ========\n");
	LOG_INFO_IF(debug_loader, "base_virtual_addr ......: {:#018x}\n", m->base_virtual_addr);
	LOG_INFO_IF(debug_loader, "base_size ..............: {:#018x}\n", base_size);
	LOG_INFO_IF(debug_loader, "aligned_base_size ......: {:#018x}\n", m->aligned_base_size);

    for (u16 i = 0; i < elf_header.e_phnum; i++)
	{
        switch (elf_pheader[i].p_type)
		{
			case PT_LOAD:
			case PT_SCE_RELRO:
				if (elf_pheader[i].p_memsz != 0)
				{
					u64 segment_addr = elf_pheader[i].p_vaddr + m->base_virtual_addr;
					u64 segment_file_size = elf_pheader[i].p_filesz;
                    u64 segment_memory_size = get_aligned_size(elf_pheader[i]);
                    auto segment_mode = m->elf.ElfPheaderFlagsStr(elf_pheader[i].p_flags);
                    LOG_INFO_IF(debug_loader, "program header = [{}] type = {}\n",i,m->elf.ElfPheaderTypeStr(elf_pheader[i].p_type));
					LOG_INFO_IF(debug_loader, "segment_addr ..........: {:#018x}\n", segment_addr);
					LOG_INFO_IF(debug_loader, "segment_file_size .....: {}\n", segment_file_size);
					LOG_INFO_IF(debug_loader, "segment_memory_size ...: {}\n", segment_memory_size);
					LOG_INFO_IF(debug_loader, "segment_mode ..........: {}\n", segment_mode);
					
                    m->elf.LoadSegment(segment_addr, elf_pheader[i].p_offset, segment_file_size);
				}
				else
				{
                    LOG_ERROR_IF(debug_loader, "p_memsz==0 in type {}\n", m->elf.ElfPheaderTypeStr(elf_pheader[i].p_type));
				}
				break;
			case PT_DYNAMIC:
				if (elf_pheader[i].p_filesz != 0)
				{
                    m->m_dynamic.resize(elf_pheader[i].p_filesz);
                    m->elf.LoadSegment(reinterpret_cast<u64>(m->m_dynamic.data()), elf_pheader[i].p_offset, elf_pheader[i].p_filesz);
				}
				else
				{
                    LOG_ERROR_IF(debug_loader, "p_filesz==0 in type {}\n", m->elf.ElfPheaderTypeStr(elf_pheader[i].p_type));
				}
				break;
			case PT_SCE_DYNLIBDATA:
				if (elf_pheader[i].p_filesz != 0)
				{
                    m->m_dynamic_data.resize(elf_pheader[i].p_filesz);
                    m->elf.LoadSegment(reinterpret_cast<u64>(m->m_dynamic_data.data()), elf_pheader[i].p_offset, elf_pheader[i].p_filesz);
				}
				else
				{
                    LOG_ERROR_IF(debug_loader, "p_filesz==0 in type {}\n", m->elf.ElfPheaderTypeStr(elf_pheader[i].p_type));
				}
				break;
			default:
                LOG_ERROR_IF(debug_loader, "Unimplemented type {}\n", m->elf.ElfPheaderTypeStr(elf_pheader[i].p_type));
		}
	}
    LOG_INFO_IF(debug_loader, "program entry addr ..........: {:#018x}\n", m->elf.GetElfEntry() + m->base_virtual_addr);

    auto* rt1 = reinterpret_cast<uint8_t*>(m->elf.GetElfEntry() + m->base_virtual_addr);
    ZyanU64 runtime_address = m->elf.GetElfEntry() + m->base_virtual_addr;

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
		fmt::print("{:#x}" PRIX64 "  {}\n", runtime_address, instruction.text);
		offset += instruction.info.length;
		runtime_address += instruction.info.length;
	}
}

void Linker::LoadDynamicInfo(Module* m)
{
    for (const auto* dyn = reinterpret_cast<elf_dynamic*>(m->m_dynamic.data()); dyn->d_tag != DT_NULL; dyn++)
	{
		switch (dyn->d_tag)
		{
		case DT_SCE_HASH: //Offset of the hash table.
            m->dynamic_info.hash_table = reinterpret_cast<void*>(m->m_dynamic_data.data() + dyn->d_un.d_ptr);
			break;
		case DT_SCE_HASHSZ: //Size of the hash table
            m->dynamic_info.hash_table_size = dyn->d_un.d_val;
			break;
		case DT_SCE_STRTAB://Offset of the string table. 
            m->dynamic_info.str_table = reinterpret_cast<char*>(m->m_dynamic_data.data() + dyn->d_un.d_ptr);
			break;
		case DT_SCE_STRSZ: //Size of the string table.
            m->dynamic_info.str_table_size = dyn->d_un.d_val;
			break;
		case DT_SCE_SYMTAB://Offset of the symbol table.
            m->dynamic_info.symbol_table = reinterpret_cast<elf_symbol*>(m->m_dynamic_data.data() + dyn->d_un.d_ptr);
			break;
		case DT_SCE_SYMTABSZ://Size of the symbol table.
            m->dynamic_info.symbol_table_total_size = dyn->d_un.d_val;
			break;
		case DT_INIT:
            m->dynamic_info.init_virtual_addr = dyn->d_un.d_ptr;
			break;
		case DT_FINI:
            m->dynamic_info.fini_virtual_addr = dyn->d_un.d_ptr;
			break;
		case DT_SCE_PLTGOT: //Offset of the global offset table.
            m->dynamic_info.pltgot_virtual_addr = dyn->d_un.d_ptr;
			break;
		case DT_SCE_JMPREL: //Offset of the table containing jump slots.
            m->dynamic_info.jmp_relocation_table = reinterpret_cast<elf_relocation*>(m->m_dynamic_data.data() + dyn->d_un.d_ptr);
			break;
		case DT_SCE_PLTRELSZ: //Size of the global offset table.
            m->dynamic_info.jmp_relocation_table_size = dyn->d_un.d_val;
			break;
		case DT_SCE_PLTREL: //The type of relocations in the relocation table. Should be DT_RELA
            m->dynamic_info.jmp_relocation_type = dyn->d_un.d_val;
            if (m->dynamic_info.jmp_relocation_type != DT_RELA)
			{
				LOG_WARN_IF(debug_loader, "DT_SCE_PLTREL is NOT DT_RELA should check!");
			}
			break;
		case DT_SCE_RELA: //Offset of the relocation table.
            m->dynamic_info.relocation_table = reinterpret_cast<elf_relocation*>(m->m_dynamic_data.data() + dyn->d_un.d_ptr);
			break;
		case DT_SCE_RELASZ: //Size of the relocation table.
            m->dynamic_info.relocation_table_size = dyn->d_un.d_val;
			break;
		case DT_SCE_RELAENT : //The size of relocation table entries.
            m->dynamic_info.relocation_table_entries_size = dyn->d_un.d_val;
            if (m->dynamic_info.relocation_table_entries_size != 0x18) //this value should always be 0x18
			{
				LOG_WARN_IF(debug_loader, "DT_SCE_RELAENT is NOT 0x18 should check!");
			}
			break;
		case DT_INIT_ARRAY:// Address of the array of pointers to initialization functions 
            m->dynamic_info.init_array_virtual_addr = dyn->d_un.d_ptr;
			break;
		case DT_FINI_ARRAY: // Address of the array of pointers to termination functions
            m->dynamic_info.fini_array_virtual_addr = dyn->d_un.d_ptr;
			break;
		case DT_INIT_ARRAYSZ://Size in bytes of the array of initialization functions
            m->dynamic_info.init_array_size = dyn->d_un.d_val;
			break;
		case DT_FINI_ARRAYSZ://Size in bytes of the array of terminationfunctions
            m->dynamic_info.fini_array_size = dyn->d_un.d_val;
			break;
		case DT_PREINIT_ARRAY://Address of the array of pointers to pre - initialization functions
            m->dynamic_info.preinit_array_virtual_addr = dyn->d_un.d_ptr;
			break;
		case DT_PREINIT_ARRAYSZ://Size in bytes of the array of pre - initialization functions
            m->dynamic_info.preinit_array_size = dyn->d_un.d_val;
			break;
		case DT_SCE_SYMENT: //The size of symbol table entries
            m->dynamic_info.symbol_table_entries_size = dyn->d_un.d_val;
            if (m->dynamic_info.symbol_table_entries_size != 0x18) //this value should always be 0x18
			{
				LOG_WARN_IF(debug_loader, "DT_SCE_SYMENT is NOT 0x18 should check!");
			}
			break;
		case DT_DEBUG:
            m->dynamic_info.debug = dyn->d_un.d_val;
			break;
		case DT_TEXTREL:
            m->dynamic_info.textrel = dyn->d_un.d_val;
			break;
		case DT_FLAGS:
            m->dynamic_info.flags = dyn->d_un.d_val;
            if (m->dynamic_info.flags != 0x04) //this value should always be DF_TEXTREL (0x04)
			{
				LOG_WARN_IF(debug_loader, "DT_FLAGS is NOT 0x04 should check!");
			}
			break;
		case DT_NEEDED://Offset of the library string in the string table to be linked in.
            if (m->dynamic_info.str_table != nullptr)//in theory this should already be filled from about just make a test case
			{
                m->dynamic_info.needed.push_back(m->dynamic_info.str_table + dyn->d_un.d_val);
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
                info.name = m->dynamic_info.str_table + info.name_offset;
				info.enc_id = encodeId(info.id);
                m->dynamic_info.import_modules.push_back(info);
			}
			break;
		case DT_SCE_IMPORT_LIB:
			{
				LibraryInfo info{};
				info.value = dyn->d_un.d_val;
                info.name = m->dynamic_info.str_table + info.name_offset;
				info.enc_id = encodeId(info.id);
                m->dynamic_info.import_libs.push_back(info);
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
            m->dynamic_info.filename = m->dynamic_info.str_table + dyn->d_un.d_val;
			break;
		case DT_SCE_MODULE_INFO://probably only useable in shared modules
			{
				ModuleInfo info{};
				info.value = dyn->d_un.d_val;
                info.name = m->dynamic_info.str_table + info.name_offset;
				info.enc_id = encodeId(info.id);
                m->dynamic_info.export_modules.push_back(info);
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
                info.name = m->dynamic_info.str_table + info.name_offset;
				info.enc_id = encodeId(info.id);
                m->dynamic_info.export_libs.push_back(info);
			}
			break;
		default:
			LOG_INFO_IF(debug_loader, "unsupported dynamic tag ..........: {:#018x}\n", dyn->d_tag);
		}
		
	}
}

const ModuleInfo* Linker::FindModule(const Module& m, const std::string& id)
{
    const auto& import_modules = m.dynamic_info.import_modules;
	int index = 0;
    for (const auto& mod : import_modules)
	{
		if (mod.enc_id.compare(id) == 0)
		{
			return &import_modules.at(index);
		}
		index++;
	}
    const auto& export_modules = m.dynamic_info.export_modules;
	index = 0;
    for (const auto& mod : export_modules)
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
    const auto& import_libs = m.dynamic_info.import_libs;
	int index = 0;
    for (const auto& lib : import_libs)
	{
		if (lib.enc_id.compare(id) == 0)
		{
			return &import_libs.at(index);
		}
		index++;
	}
    const auto& export_libs = m.dynamic_info.export_libs;
	index = 0;
    for (const auto& lib : export_libs)
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
    if (m->dynamic_info.symbol_table == nullptr || m->dynamic_info.str_table == nullptr || m->dynamic_info.symbol_table_total_size==0)
	{
		LOG_INFO_IF(debug_loader, "Symbol table not found!\n");
		return;
	}

    for (auto* sym = m->dynamic_info.symbol_table;
        reinterpret_cast<uint8_t*>(sym) < reinterpret_cast<uint8_t*>(m->dynamic_info.symbol_table) + m->dynamic_info.symbol_table_total_size;
		sym++)
	{
        std::string id = std::string(m->dynamic_info.str_table + sym->st_name);
        auto ids = StringUtil::split_string(id, '#');
		if (ids.size() == 3)//symbols are 3 parts name , library , module 
		{
			const auto* library = FindLibrary(*m, ids.at(1));
			const auto* module = FindModule(*m, ids.at(2));
			auto bind = sym->GetBind();
			auto type = sym->GetType();
			auto visibility = sym->GetVisibility();
			if (library != nullptr || module != nullptr)
			{
				switch (bind)
				{
					case STB_GLOBAL:
					case STB_WEAK:
						break;
					default:
						LOG_INFO_IF(debug_loader, "Unsupported bind {} for name symbol {} \n", bind,ids.at(0));
						continue;
				}
				switch (type)
				{
					case STT_OBJECT:
					case STT_FUN:
						break;
					default:
						LOG_INFO_IF(debug_loader, "Unsupported type {} for name symbol {} \n", type, ids.at(0));
						continue;
				}
				switch (visibility)
				{
					case STV_DEFAULT:
						break;
					default:
						LOG_INFO_IF(debug_loader, "Unsupported visibility {} for name symbol {} \n", visibility, ids.at(0));
						continue;
				}
				//if st_value!=0 then it's export symbol
				bool is_sym_export = sym->st_value != 0;
				std::string nidName = "";

				auto aeronid = aerolib::find_by_nid(ids.at(0).c_str());

				if (aeronid != nullptr)
				{
                    nidName = aeronid->name;
				}
				else
				{
					nidName = "UNK";
				}

				SymbolRes sym_r{};
				sym_r.name = ids.at(0);
				sym_r.nidName = nidName;
				sym_r.library = library->name;
				sym_r.library_version = library->version;
				sym_r.module = module->name;
				sym_r.module_version_major = module->version_major;
				sym_r.module_version_minor = module->version_minor;
				sym_r.type = type;

				if (is_sym_export)
				{
                    m->export_sym.AddSymbol(sym_r, sym->st_value + m->base_virtual_addr);
				}
				else
				{
                    m->import_sym.AddSymbol(sym_r,0);
				}
				

				LOG_INFO_IF(debug_loader, "name {} function {} library {} module {} bind {} type {} visibility {}\n", ids.at(0),nidName,library->name, module->name, bind, type, visibility);
			}
		}
	}
}
static void relocate(u32 idx, elf_relocation* rel, Module* m, bool isJmpRel) {
    auto type = rel->GetType();
    auto symbol = rel->GetSymbol();
    auto addend = rel->rel_addend;
    auto* symbolsTlb = m->dynamic_info.symbol_table;
    auto* namesTlb = m->dynamic_info.str_table;

    u64 rel_value = 0;
    u64 rel_base_virtual_addr = m->base_virtual_addr;
    u64 rel_virtual_addr = m->base_virtual_addr + rel->rel_offset;
    bool rel_isResolved = false;
    u08 rel_sym_type = 0;
    std::string rel_name;

    switch (type) {
        case R_X86_64_RELATIVE:
            if (symbol != 0)  // should be always zero
            {
                LOG_INFO_IF(debug_loader, "R_X86_64_RELATIVE symbol not zero = {:#010x}\n", type, symbol);
            }
            rel_value = rel_base_virtual_addr + addend;
            rel_isResolved = true;
            break;
        case R_X86_64_64:
        case R_X86_64_JUMP_SLOT:  // similar but addend is not take into account
        {
            auto sym = symbolsTlb[symbol];
            auto sym_bind = sym.GetBind();
            auto sym_type = sym.GetType();
            auto sym_visibility = sym.GetVisibility();
            u64 symbol_vitrual_addr = 0;
            SymbolRecord symrec{};
            switch (sym_type) {
                case STT_FUN: rel_sym_type = 2; break;
                case STT_OBJECT: rel_sym_type = 1; break;
                default: LOG_INFO_IF(debug_loader, "unknown symbol type {}\n", sym_type);
            }
            if (sym_visibility != 0)  // should be zero log if else
            {
                LOG_INFO_IF(debug_loader, "symbol visilibity !=0\n");
            }
            switch (sym_bind) {
                case STB_GLOBAL:
                    rel_name = namesTlb + sym.st_name;
                    m->linker->Resolve(rel_name, rel_sym_type, m, &symrec);
                    symbol_vitrual_addr = symrec.virtual_address;
                    rel_isResolved = (symbol_vitrual_addr != 0);

                    rel_name = symrec.name;
                    if (type == R_X86_64_JUMP_SLOT) {
                        addend = 0;
                    }
                    rel_value = (rel_isResolved ? symbol_vitrual_addr + addend : 0);
                    if (!rel_isResolved) {
                        LOG_INFO_IF(debug_loader, "R_X86_64_64-R_X86_64_JUMP_SLOT sym_type {} bind STB_GLOBAL symbol : {:#010x}\n", sym_type, symbol);
                    }
                    break;
                default: LOG_INFO_IF(debug_loader, "UNK bind {}\n", sym_bind);
            }

        } break;
        default: LOG_INFO_IF(debug_loader, "UNK type {:#010x} rel symbol : {:#010x}\n", type, symbol);
    }

    if (rel_isResolved) {
        VirtualMemory::memory_patch(rel_virtual_addr, rel_value);
	}
	else
	{
        LOG_INFO_IF(debug_loader, "function not patched! {}\n",rel_name);
	}
}

void Linker::Relocate(Module* m)
{
	u32 idx = 0;
    for (auto* rel = m->dynamic_info.relocation_table; reinterpret_cast<u08*>(rel) < reinterpret_cast<u08*>(m->dynamic_info.relocation_table) + m->dynamic_info.relocation_table_size; rel++, idx++)
	{
		relocate(idx, rel, m, false);
	}
	idx = 0;
    for (auto* rel = m->dynamic_info.jmp_relocation_table; reinterpret_cast<u08*>(rel) < reinterpret_cast<u08*>(m->dynamic_info.jmp_relocation_table) + m->dynamic_info.jmp_relocation_table_size; rel++, idx++)
	{
		relocate(idx, rel, m, true);
	}
}

void GenerateTrampoline(u64 hle_handler, u64 context_base);

__declspec(align(32)) struct Context {
    u64 gpr[16];
    u64 ymm[16 * 4];
    u64 rip;
    u64 rflags;

	// misc
    u64 host_rsp;
    u64 trampoline_ret;
};
Context thread_context;


void Linker::Resolve(const std::string& name, int Symtype, Module* m, SymbolRecord* return_info) { 
	auto ids = StringUtil::split_string(name, '#');

	if (ids.size() == 3)  // symbols are 3 parts name , library , module
    {
        const auto* library = FindLibrary(*m, ids.at(1));
        const auto* module = FindModule(*m, ids.at(2));

		if (library != nullptr && module != nullptr) {
            SymbolRes sr{};
            sr.name = ids.at(0);
            sr.library = library->name;
            sr.library_version = library->version;
            sr.module = module->name;
            sr.module_version_major = module->version_major;
            sr.module_version_minor = module->version_minor;
            sr.type = Symtype;

			const SymbolRecord* rec = nullptr;
            rec = m_hle_symbols.FindSymbol(sr);

            if (rec != nullptr) {
                *return_info = *rec;
            } else {
                auto aeronid = aerolib::find_by_nid(sr.name.c_str());
                if (aeronid) {
                    return_info->name = aeronid->name;
                    return_info->virtual_address = GetStub(aeronid->nid);
                } else {
                    return_info->virtual_address = GetStub(sr.name.c_str());
                    return_info->name = "Unknown !!!";
				}
                LOG_ERROR_IF(debug_loader, "Linker: Stub resolved {} as {} (lib: {}, mod: {}) \n", sr.name, return_info->name, library->name, module->name);
            }
		}
		else
		{
            __debugbreak();//den tha prepei na ftasoume edo
		}
	}
	else
	{
        __debugbreak();//oute edo mallon
	}
    GenerateTrampoline(return_info->virtual_address, (u64)&thread_context);
}

using exit_func_t          = PS4_SYSV_ABI void (*)();
using entry_func_t           = PS4_SYSV_ABI void (*)(EntryParams* params, exit_func_t atexit_func);

static PS4_SYSV_ABI void ProgramExitFunc() {
    fmt::print("exit function called\n");
}

auto gen = new Xbyak::CodeGenerator(64 * 1024 * 1024);



constexpr auto rip_offs = offsetof(Context, rip);
constexpr auto ymm_offs = offsetof(Context, ymm[0]);
constexpr auto ymm_size = 32;
constexpr auto rflags_offs = offsetof(Context, rflags);
constexpr auto host_rsp_offs = offsetof(Context, host_rsp);
constexpr auto trampoline_ret_offs = offsetof(Context, trampoline_ret);

std::unordered_map<u64, PS4_SYSV_ABI u64 (*)()> translated_entries;

void push_abi_regs() {
	using namespace Xbyak;
    for (int i = 3; i < 16; i++) {
        if (i == 4) continue;
        gen->push(Reg64(i));
    }
}

void pop_abi_regs() {
	using namespace Xbyak;
    for (int i = 15; i >= 3; i--) {
        if (i == 4) continue;
        gen->pop(Reg64(i));
    }
}
auto TranslateCode(u08* runtime_address, u64 context_base) -> PS4_SYSV_ABI u64 (*)() {
    printf("TranslateCode: %p, ctx: %llX\n", runtime_address, context_base);

	using namespace Xbyak;
	using namespace Xbyak::util;

	auto Entry = (PS4_SYSV_ABI u64(*)())gen->getCurr();

	const Reg* reg_z2x[ZYDIS_REGISTER_MAX_VALUE] = {
        [ZYDIS_REGISTER_AL] = &gen->rax,
        [ZYDIS_REGISTER_CL] = &gen->rcx,
        [ZYDIS_REGISTER_DL] = &gen->rdx,
        [ZYDIS_REGISTER_BL] = &gen->rbx,
        [ZYDIS_REGISTER_AH] = &gen->rax,
        [ZYDIS_REGISTER_CH] = &gen->rcx,
        [ZYDIS_REGISTER_DH] = &gen->rdx,
        [ZYDIS_REGISTER_BH] = &gen->rbx,
        [ZYDIS_REGISTER_SPL] = &gen->rsp,
        [ZYDIS_REGISTER_BPL] = &gen->rbp,
        [ZYDIS_REGISTER_SIL] = &gen->rsi,
        [ZYDIS_REGISTER_DIL] = &gen->rdi,
        [ZYDIS_REGISTER_R8B] = &gen->r8,
        [ZYDIS_REGISTER_R9B] = &gen->r9,
        [ZYDIS_REGISTER_R10B] = &gen->r10,
        [ZYDIS_REGISTER_R11B] = &gen->r11,
        [ZYDIS_REGISTER_R12B] = &gen->r12,
        [ZYDIS_REGISTER_R13B] = &gen->r13,
        [ZYDIS_REGISTER_R14B] = &gen->r14,
        [ZYDIS_REGISTER_R15B] = &gen->r15,

        [ZYDIS_REGISTER_AX] = &gen->rax,
        [ZYDIS_REGISTER_CX] = &gen->rcx,
        [ZYDIS_REGISTER_DX] = &gen->rdx,
        [ZYDIS_REGISTER_BX] = &gen->rbx,
        [ZYDIS_REGISTER_SP] = &gen->rsp,
        [ZYDIS_REGISTER_BP] = &gen->rbp,
        [ZYDIS_REGISTER_SI] = &gen->rsi,
        [ZYDIS_REGISTER_DI] = &gen->rdi,
        [ZYDIS_REGISTER_R8W] = &gen->r8,
        [ZYDIS_REGISTER_R9W] = &gen->r9,
        [ZYDIS_REGISTER_R10W] = &gen->r10,
        [ZYDIS_REGISTER_R11W] = &gen->r11,
        [ZYDIS_REGISTER_R12W] = &gen->r12,
        [ZYDIS_REGISTER_R13W] = &gen->r13,
        [ZYDIS_REGISTER_R14W] = &gen->r14,
        [ZYDIS_REGISTER_R15W] = &gen->r15,

		[ZYDIS_REGISTER_EAX] = &gen->rax,
        [ZYDIS_REGISTER_ECX] = &gen->rcx,
        [ZYDIS_REGISTER_EDX] = &gen->rdx,
        [ZYDIS_REGISTER_EBX] = &gen->rbx,
        [ZYDIS_REGISTER_ESP] = &gen->rsp,
        [ZYDIS_REGISTER_EBP] = &gen->rbp,
        [ZYDIS_REGISTER_ESI] = &gen->rsi,
        [ZYDIS_REGISTER_EDI] = &gen->rdi,
        [ZYDIS_REGISTER_R8D] = &gen->r8,
        [ZYDIS_REGISTER_R9D] = &gen->r9,
        [ZYDIS_REGISTER_R10D] = &gen->r10,
        [ZYDIS_REGISTER_R11D] = &gen->r11,
        [ZYDIS_REGISTER_R12D] = &gen->r12,
        [ZYDIS_REGISTER_R13D] = &gen->r13,
        [ZYDIS_REGISTER_R14D] = &gen->r14,
        [ZYDIS_REGISTER_R15D] = &gen->r15,

		[ZYDIS_REGISTER_RAX] = &gen->rax,
        [ZYDIS_REGISTER_RCX] = &gen->rcx,
        [ZYDIS_REGISTER_RDX] = &gen->rdx,
        [ZYDIS_REGISTER_RBX] = &gen->rbx,
        [ZYDIS_REGISTER_RSP] = &gen->rsp,
        [ZYDIS_REGISTER_RBP] = &gen->rbp,
        [ZYDIS_REGISTER_RSI] = &gen->rsi,
        [ZYDIS_REGISTER_RDI] = &gen->rdi,
        [ZYDIS_REGISTER_R8] = &gen->r8,
        [ZYDIS_REGISTER_R9] = &gen->r9,
        [ZYDIS_REGISTER_R10] =  &gen->r10,
        [ZYDIS_REGISTER_R11] =  &gen->r11,
        [ZYDIS_REGISTER_R12] =  &gen->r12,
        [ZYDIS_REGISTER_R13] =  &gen->r13,
        [ZYDIS_REGISTER_R14] =  &gen->r14,
        [ZYDIS_REGISTER_R15] =  &gen->r15,
		
		[ZYDIS_REGISTER_XMM0] =  &gen->ymm0,
        [ZYDIS_REGISTER_XMM1] =  &gen->ymm1,
        [ZYDIS_REGISTER_XMM2] =  &gen->ymm2,
        [ZYDIS_REGISTER_XMM3] =  &gen->ymm3,
        [ZYDIS_REGISTER_XMM4] =  &gen->ymm4,
        [ZYDIS_REGISTER_XMM5] =  &gen->ymm5,
        [ZYDIS_REGISTER_XMM6] =  &gen->ymm6,
        [ZYDIS_REGISTER_XMM7] =  &gen->ymm7,
        [ZYDIS_REGISTER_XMM8] =  &gen->ymm8,
        [ZYDIS_REGISTER_XMM9] =  &gen->ymm9,
        [ZYDIS_REGISTER_XMM10] = &gen->ymm10,
        [ZYDIS_REGISTER_XMM11] = &gen->ymm11,
        [ZYDIS_REGISTER_XMM12] = &gen->ymm12,
        [ZYDIS_REGISTER_XMM13] = &gen->ymm13,
        [ZYDIS_REGISTER_XMM14] = &gen->ymm14,
        [ZYDIS_REGISTER_XMM15] = &gen->ymm15,

		[ZYDIS_REGISTER_YMM0] =  &gen->ymm0,
		[ZYDIS_REGISTER_YMM1] =  &gen->ymm1,
		[ZYDIS_REGISTER_YMM2] =  &gen->ymm2,
		[ZYDIS_REGISTER_YMM3] =  &gen->ymm3,
		[ZYDIS_REGISTER_YMM4] =  &gen->ymm4,
		[ZYDIS_REGISTER_YMM5] =  &gen->ymm5,
		[ZYDIS_REGISTER_YMM6] =  &gen->ymm6,
		[ZYDIS_REGISTER_YMM7] =  &gen->ymm7,
		[ZYDIS_REGISTER_YMM8] =  &gen->ymm8,
		[ZYDIS_REGISTER_YMM9] =  &gen->ymm9,
		[ZYDIS_REGISTER_YMM10] = &gen->ymm10,
		[ZYDIS_REGISTER_YMM11] = &gen->ymm11,
		[ZYDIS_REGISTER_YMM12] = &gen->ymm12,
		[ZYDIS_REGISTER_YMM13] = &gen->ymm13,
		[ZYDIS_REGISTER_YMM14] = &gen->ymm14,
		[ZYDIS_REGISTER_YMM15] = &gen->ymm15,
    };

	push_abi_regs();

	gen->mov(gen->rax, context_base + host_rsp_offs);
    gen->mov(gen->qword[gen->rax], gen->rsp);

	std::set<ZydisRegister> UsedRegs;
    
    ZydisDisassembledInstruction instruction;
    while (ZYAN_SUCCESS(ZydisDisassembleIntel(
        /* machine_mode:    */ ZYDIS_MACHINE_MODE_LONG_64,
        /* runtime_address: */ (uint64_t)runtime_address,
        /* buffer:          */ runtime_address,
        /* length:          */ 15,
        /* instruction:     */ &instruction))) {
        printf("%016" PRIX64 "  %s\n", (u64)runtime_address, instruction.text);

		auto next_address = runtime_address + instruction.info.length;
        
		UsedRegs.clear();

		bool UsesFlags = false;

		if (instruction.info.meta.branch_type != ZYDIS_BRANCH_TYPE_NONE) {
            if (instruction.info.mnemonic == ZYDIS_MNEMONIC_CALL || instruction.info.mnemonic == ZYDIS_MNEMONIC_JMP) {

                if (instruction.info.mnemonic == ZYDIS_MNEMONIC_CALL) {
                    gen->mov(gen->rax, context_base + 4 * 8);
                    gen->mov(gen->rcx, gen->rax);
                    gen->mov(gen->rsp, gen->qword[gen->rax]);
                    gen->mov(gen->rax, (u64)next_address);
                    gen->push(gen->rax);
                    gen->mov(gen->qword[gen->rcx], gen->rsp);
                }

                gen->mov(gen->rax, context_base);
                gen->mov(gen->rsp, gen->qword[gen->rax + host_rsp_offs]);
                pop_abi_regs();

                if (instruction.operands[0].type == ZYDIS_OPERAND_TYPE_IMMEDIATE) {
                    gen->mov(gen->rax, (u64)next_address + instruction.operands[0].imm.value.s);
                } else if (instruction.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER) {
                    gen->mov(gen->rax, ptr[gen->rax + reg_z2x[instruction.operands[0].reg.value]->getIdx() * 8]);
                } else if (instruction.operands[0].type == ZYDIS_OPERAND_TYPE_MEMORY) {
                    if (instruction.operands[0].mem.base == ZYDIS_REGISTER_RIP) {
                        assert(instruction.operands[0].mem.index == ZYDIS_REGISTER_NONE);
                        assert(instruction.operands[0].mem.disp.has_displacement);
                        gen->mov(gen->rax, (u64)next_address + instruction.operands[0].mem.disp.value);
                        gen->mov(rax, ptr[rax]);
                    } else {
                        assert(instruction.operands[0].mem.index == ZYDIS_REGISTER_NONE);

                        gen->mov(gen->rax, context_base);
                        gen->mov(rcx, ptr[rax + reg_z2x[instruction.operands[0].mem.base]->getIdx() * 8]);
                        if (instruction.operands[0].mem.disp.has_displacement) {
                            gen->mov(rax, ptr[rcx + instruction.operands[0].mem.disp.value]);
                        } else {
                            gen->mov(rax, ptr[rcx]);
                        }
                    }
                } else {
                    assert(false);
                }
                gen->ret();
            } else if (instruction.info.mnemonic == ZYDIS_MNEMONIC_RET) {
                gen->mov(gen->rax, context_base + 4 * 8);
                gen->mov(gen->rsp, gen->qword[gen->rax]);
                gen->pop(gen->rcx);
                gen->mov(gen->qword[gen->rax], gen->rsp);

                gen->mov(gen->rax, context_base);
                gen->mov(gen->rsp, gen->qword[gen->rax + host_rsp_offs]);
                pop_abi_regs();

                gen->mov(gen->rax, gen->rcx);
                gen->ret();
            } else if (instruction.info.mnemonic == ZYDIS_MNEMONIC_JB || instruction.info.mnemonic == ZYDIS_MNEMONIC_JBE ||
                       instruction.info.mnemonic == ZYDIS_MNEMONIC_JL || instruction.info.mnemonic == ZYDIS_MNEMONIC_JLE ||
                       instruction.info.mnemonic == ZYDIS_MNEMONIC_JNB || instruction.info.mnemonic == ZYDIS_MNEMONIC_JNBE ||
                       instruction.info.mnemonic == ZYDIS_MNEMONIC_JNL || instruction.info.mnemonic == ZYDIS_MNEMONIC_JNLE ||
                       instruction.info.mnemonic == ZYDIS_MNEMONIC_JNO || instruction.info.mnemonic == ZYDIS_MNEMONIC_JNP ||
                       instruction.info.mnemonic == ZYDIS_MNEMONIC_JNS || instruction.info.mnemonic == ZYDIS_MNEMONIC_JNZ ||
                       instruction.info.mnemonic == ZYDIS_MNEMONIC_JO || instruction.info.mnemonic == ZYDIS_MNEMONIC_JP ||
                       instruction.info.mnemonic == ZYDIS_MNEMONIC_JS || instruction.info.mnemonic == ZYDIS_MNEMONIC_JZ) {
                void (CodeGenerator::*jump_map[])(const Label& label, CodeGenerator::LabelType type) = {
                    [ZYDIS_MNEMONIC_JB] = &CodeGenerator::jb,   [ZYDIS_MNEMONIC_JBE] = &CodeGenerator::jbe,
                    [ZYDIS_MNEMONIC_JL] = &CodeGenerator::jl,   [ZYDIS_MNEMONIC_JLE] = &CodeGenerator::jle,
                    [ZYDIS_MNEMONIC_JNB] = &CodeGenerator::jnb, [ZYDIS_MNEMONIC_JNBE] = &CodeGenerator::jnbe,
                    [ZYDIS_MNEMONIC_JNL] = &CodeGenerator::jnl, [ZYDIS_MNEMONIC_JNLE] = &CodeGenerator::jnle,
                    [ZYDIS_MNEMONIC_JNO] = &CodeGenerator::jno, [ZYDIS_MNEMONIC_JNP] = &CodeGenerator::jnp,
                    [ZYDIS_MNEMONIC_JNS] = &CodeGenerator::jns, [ZYDIS_MNEMONIC_JNZ] = &CodeGenerator::jnz,
                    [ZYDIS_MNEMONIC_JO] = &CodeGenerator::jo,   [ZYDIS_MNEMONIC_JP] = &CodeGenerator::jp,
                    [ZYDIS_MNEMONIC_JS] = &CodeGenerator::js,   [ZYDIS_MNEMONIC_JZ] = &CodeGenerator::jz,
                };

				assert(instruction.operands[0].type == ZYDIS_OPERAND_TYPE_IMMEDIATE);

                // restore regs, rsp
                gen->mov(gen->rax, context_base + host_rsp_offs);
                gen->mov(gen->rsp, gen->qword[gen->rax]);
                pop_abi_regs();

                // load flags
                gen->mov(gen->rax, context_base);
                gen->mov(rax, ptr[gen->rax + rflags_offs]);
                gen->push(rax);
                gen->popf();

                u64 dest_addr = (u64)next_address + instruction.operands[0].imm.value.s;

                gen->mov(rax, (u64)dest_addr);  // if jump is taken, go to dest_addr
                Label dest_jump;
                (gen->*jump_map[instruction.info.mnemonic])(dest_jump, CodeGenerator::T_NEAR);
                gen->mov(rax, (u64)next_address);  // else fall through to next_addr
                gen->L(dest_jump);

                // all done, return
                gen->ret();

            } else if (instruction.info.mnemonic == ZYDIS_MNEMONIC_LOOP || 
				instruction.info.mnemonic == ZYDIS_MNEMONIC_LOOPNE || 
				instruction.info.mnemonic == ZYDIS_MNEMONIC_LOOPE) {
                void (CodeGenerator::*jump_map[])(const Label& label) = {
                    [ZYDIS_MNEMONIC_LOOP] = &CodeGenerator::loop,
                    [ZYDIS_MNEMONIC_LOOPNE] = &CodeGenerator::loopne,
                    [ZYDIS_MNEMONIC_LOOPE] = &CodeGenerator::loope
                };

				// restore regs, rsp
                gen->mov(gen->rax, context_base + host_rsp_offs);
                gen->mov(gen->rsp, gen->qword[gen->rax]);
                pop_abi_regs();

				// load flags
                gen->mov(gen->rax, context_base);
                // backup ctx
                gen->mov(rdx, rax);

                gen->mov(rax, ptr[gen->rax + rflags_offs]);
                gen->push(rax);
                gen->popf();

				// load rcx
                gen->mov(rcx, ptr[rdx + rcx.getIdx() * 8]);


				u64 dest_addr = (u64)next_address + instruction.operands[0].imm.value.s;

                gen->mov(rax, (u64)dest_addr);  // if jump is taken, go to dest_addr
                Label dest_jump;
                (gen->*jump_map[instruction.info.mnemonic])(dest_jump);
                gen->mov(rax, (u64)next_address);  // else fall through to next_addr
                gen->L(dest_jump);

				// store rcx

				gen->mov(ptr[gen->rdx + rcx.getIdx() * 8], rcx);

				gen->ret();

			} else {
				// Handle more Branches
				assert(false);
			}
            break;
        } else {
            for (int i = 0; i < instruction.info.operand_count; i++) {
                auto operand = &instruction.operands[i];
                if (operand->type == ZYDIS_OPERAND_TYPE_REGISTER) {
                    if (operand->reg.value == ZYDIS_REGISTER_RFLAGS) {
                        UsesFlags = true;
                    } else {
						UsedRegs.insert(operand->reg.value);
					}
                } else if (operand->type == ZYDIS_OPERAND_TYPE_MEMORY) {
                    if (operand->mem.base && operand->mem.base != ZYDIS_REGISTER_RIP) UsedRegs.insert(operand->mem.base);
                    if (operand->mem.index) UsedRegs.insert(operand->mem.index);
                }
            }

			if (UsesFlags) {
                gen->mov(gen->rax, context_base);
                gen->mov(rsp, ptr[gen->rax + host_rsp_offs]);
                gen->mov(rax, ptr[gen->rax + rflags_offs]);
                gen->push(rax);
                gen->popf();
			}

            u16 used_mask = 0;

            for (auto used_reg : UsedRegs) {
                if (reg_z2x[used_reg] && reg_z2x[used_reg]->isREG()) {
                    used_mask |= 1 << reg_z2x[used_reg]->getIdx();
                }
            }

            u16 unused_mask = ~used_mask;

            // not really possible?
            assert(unused_mask != 0);

            Reg64 context_reg;
            Reg64 rip_reg;

			int free_reg = 0;

            for (; free_reg < 16; free_reg++) {
                if (unused_mask & (1 << free_reg)) {
                    context_reg = Reg64(free_reg);
                    break;
                }
            }

			free_reg++;

			for (; free_reg < 16; free_reg++) {
                if (unused_mask & (1 << free_reg)) {
                    rip_reg = Reg64(free_reg);
                    break;
                }
            }

			if (instruction.info.attributes & ZYDIS_ATTRIB_IS_RELATIVE) {
                gen->mov(gen->rax, (u64)next_address);
                gen->mov(rip_reg, gen->rax);
            }

            gen->mov(gen->rax, context_base);

            if (context_reg != gen->rax) {
				gen->mov(context_reg, gen->rax);
			}

            for (auto used_reg : UsedRegs) {
                if (!reg_z2x[used_reg]) {
                    assert(false);
                    //if (used_reg == ZYDIS_REGISTER_RIP) {
					//	// handled
                    ////} else if (used_reg == ZYDIS_REGISTER_EFLAGS) {
					//} else {
					//}
				} else if (reg_z2x[used_reg]->isREG()) {
                    gen->mov(*reg_z2x[used_reg], gen->ptr[context_reg + reg_z2x[used_reg]->getIdx() * 8]);
                } else if (reg_z2x[used_reg]->isYMM()) {
                    gen->vmovaps(*(Ymm*)reg_z2x[used_reg], gen->ptr[context_reg + ymm_offs + reg_z2x[used_reg]->getIdx() * ymm_size]);
                } else {
                    assert(false);
                }
            }

			if (instruction.info.attributes & ZYDIS_ATTRIB_IS_RELATIVE) {
                assert(rip_reg.getIdx() < 8);
                assert(instruction.info.raw.modrm.offset != 0);
                for (int op_byte = 0; op_byte < instruction.info.length; op_byte++) {
                    if (op_byte == instruction.info.raw.modrm.offset) {
                        assert(instruction.info.raw.modrm.mod == 0 && instruction.info.raw.modrm.rm == 5);
                        gen->db((0x2 << 6) | (instruction.info.raw.modrm.reg << 3) | (rip_reg.getIdx() & 7));
                    } else {
                        gen->db(runtime_address[op_byte]);
					}
                }
            } else {
                for (int op_byte = 0; op_byte < instruction.info.length; op_byte++) gen->db(runtime_address[op_byte]);
			}

            for (auto used_reg : UsedRegs) {
                if (reg_z2x[used_reg]->isREG()) {
                    gen->mov(gen->ptr[context_reg + reg_z2x[used_reg]->getIdx() * 8], *reg_z2x[used_reg]);
                } else if (reg_z2x[used_reg]->isYMM()) {
                    gen->vmovaps(gen->ptr[context_reg + ymm_offs + reg_z2x[used_reg]->getIdx() * ymm_size], *(Ymm*)reg_z2x[used_reg]);
                } else {
                    assert(false);
                }
            }

			if (UsesFlags) {
                gen->mov(gen->rax, context_base);
                gen->mov(rsp, ptr[gen->rax + host_rsp_offs]);
                gen->pushf();
                gen->pop(rcx);
                gen->mov(ptr[gen->rax + rflags_offs], rcx);
            }
        }

		runtime_address = next_address;
    }

	gen->int3();
    gen->int3();
    gen->int3();
    gen->int3();

	gen->ready();
	return Entry;
}

void GenerateTrampoline(u64 hle_handler, u64 context_base) {
    printf("Generating trampoline %llX\n", hle_handler);

	using namespace Xbyak;
	using namespace Xbyak::util;

    auto entry = translated_entries.find(hle_handler);
    if (entry == translated_entries.end()) {

		translated_entries[hle_handler] = gen->getCurr<PS4_SYSV_ABI u64 (*)()>();

        push_abi_regs();

        gen->mov(gen->rax, context_base);
        gen->mov(gen->qword[gen->rax + host_rsp_offs], gen->rsp);

        gen->mov(rax, context_base);

		// RSP
		gen->mov(rsp, ptr[rax + rsp.getIdx() * 8]);
        
		// pop & store original return address
        gen->pop(rdx);
        gen->mov(ptr[rax + trampoline_ret_offs], rdx);

        // args: RDI, RSI, RDX, RCX, R8, R9
        Reg64 args_64[] = {rdi, rsi, rdx, rcx, r8, r9};
        for (const auto& reg : args_64) {
            gen->mov(reg, ptr[rax + reg.getIdx() * 8]);
		}
        // args: XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6 and XMM7
        for (int i = 0; i < 8; i++) {
            gen->vmovaps(Ymm(i), ptr[rax + ymm_offs + i * ymm_size]);
		}

		gen->mov(rax, hle_handler);
        gen->call(rax) ;// this replaces the original return address

        // rets: RAX, RDX
        gen->mov(rcx, rax);
        Reg64 rets_64[] = {rcx /* rax is used as temp */, rdx};

        gen->mov(rax, context_base);
        gen->mov(ptr[rax + rax.getIdx() * 8], rets_64[0]);
        gen->mov(ptr[rax + rets_64[1].getIdx() * 8], rets_64[1]);
        
        // rets: XMM0
        gen->vmovaps(ptr[rax + ymm_offs + 0 * ymm_size], Ymm(0));

		// faux ret
		gen->mov(gen->rax, context_base + 4 * 8);
        gen->mov(gen->rsp, gen->qword[gen->rax]);
        gen->pop(gen->rcx);
        gen->mov(gen->qword[gen->rax], gen->rsp);

        gen->mov(gen->rax, context_base);
        gen->mov(gen->rsp, gen->qword[gen->rax + host_rsp_offs]);
        pop_abi_regs();

        gen->mov(gen->rax, ptr[gen->rax + trampoline_ret_offs]);
        gen->ret();
    }
}

static void run_main_entry(u64 addr, EntryParams* params, exit_func_t exit_func) {
    // reinterpret_cast<entry_func_t>(addr)(params, exit_func); // can't be used, stack has to have a specific layout

    // Allocate stack for guest thread
    auto stack_top = 8 * 1024 * 1024 + (u64)VirtualAlloc(0, 8 * 1024 * 1024, MEM_COMMIT, PAGE_READWRITE);

	{
        auto& rsp = thread_context.gpr[4];
        auto& rsi = thread_context.gpr[6];
        auto& rdi = thread_context.gpr[7];

        rsp = stack_top;

        rsp = rsp & ~16;
        rsp = rsp - 8;

        rsp = rsp - 8;
        *(void**)rsp = params->argv;

        rsp = rsp - 8;
        *(u64*)rsp = params->argc;

        rsi = (u64)params;
        rdi = (u64)exit_func;
    }

    thread_context.rip = addr;

    for (;;) {
        auto entry = translated_entries.find(thread_context.rip);
        if (entry == translated_entries.end()) {
            auto Entry = TranslateCode((u08*)thread_context.rip, (u64)&thread_context);
            translated_entries[thread_context.rip] = Entry;
            thread_context.rip = Entry();
        } else {
            thread_context.rip = entry->second();
        }
    }
}


static void run_main_entry_native(u64 addr, EntryParams* params, exit_func_t exit_func) {
    // reinterpret_cast<entry_func_t>(addr)(params, exit_func); // can't be used, stack has to have a specific layout

    asm volatile(
        "andq $-16, %%rsp\n"  // Align to 16 bytes
        "subq $8, %%rsp\n"    // videoout_basic expects the stack to be misaligned

        // Kernel also pushes some more things here during process init
        // at least: environment, auxv, possibly other things

        "pushq 8(%1)\n"  // copy EntryParams to top of stack like the kernel does
        "pushq 0(%1)\n"  // OpenOrbis expects to find it there

        "movq %1, %%rdi\n"  // also pass params and exit func
        "movq %2, %%rsi\n"  // as before

        "jmp *%0\n"  // can't use call here, as that would mangle the prepared stack.
                     // there's no coming back
        :
        : "r"(addr), "r"(params), "r"(exit_func)
        : "rax", "rsi", "rdi", "rsp", "rbp");
}


void Linker::Execute() {
    HLE::Libs::LibKernel::ThreadManagement::Pthread_Init_Self_MainThread();
    EntryParams p{};
    p.argc = 1;
    p.argv[0] = "eboot.bin"; //hmm should be ok?

    const auto& module = m_modules.at(0);
    run_main_entry(module.elf.GetElfEntry() + module.base_virtual_addr, &p, ProgramExitFunc);
}
