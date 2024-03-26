// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <Zydis/Zydis.h>
#include <common/assert.h>
#include "common/config.h"
#include "common/logging/log.h"
#include "common/path_util.h"
#include "common/string_util.h"
#include "core/aerolib/aerolib.h"
#include "core/aerolib/stubs.h"
#include "core/hle/libraries/libkernel/thread_management.h"
#include "core/linker.h"
#include "core/tls.h"
#include "core/virtual_memory.h"

namespace Core {

static u64 LoadAddress = SYSTEM_RESERVED + CODE_BASE_OFFSET;
static constexpr u64 CODE_BASE_INCR = 0x010000000u;

static u64 GetAlignedSize(const elf_program_header& phdr) {
    return (phdr.p_align != 0 ? (phdr.p_memsz + (phdr.p_align - 1)) & ~(phdr.p_align - 1)
                              : phdr.p_memsz);
}

static u64 CalculateBaseSize(const elf_header& ehdr, std::span<const elf_program_header> phdr) {
    u64 base_size = 0;
    for (u16 i = 0; i < ehdr.e_phnum; i++) {
        if (phdr[i].p_memsz != 0 && (phdr[i].p_type == PT_LOAD || phdr[i].p_type == PT_SCE_RELRO)) {
            u64 last_addr = phdr[i].p_vaddr + GetAlignedSize(phdr[i]);
            if (last_addr > base_size) {
                base_size = last_addr;
            }
        }
    }
    return base_size;
}

static std::string EncodeId(u64 nVal) {
    std::string enc;
    const char pCodes[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+-";
    if (nVal < 0x40u) {
        enc += pCodes[nVal];
    } else {
        if (nVal < 0x1000u) {
            enc += pCodes[static_cast<u16>(nVal >> 6u) & 0x3fu];
            enc += pCodes[nVal & 0x3fu];
        } else {
            enc += pCodes[static_cast<u16>(nVal >> 12u) & 0x3fu];
            enc += pCodes[static_cast<u16>(nVal >> 6u) & 0x3fu];
            enc += pCodes[nVal & 0x3fu];
        }
    }
    return enc;
}

Linker::Linker() = default;

Linker::~Linker() = default;

Module* Linker::LoadModule(const std::filesystem::path& elf_name) {
    std::scoped_lock lock{m_mutex};

    if (!std::filesystem::exists(elf_name)) {
        LOG_ERROR(Core_Linker, "Provided module {} does not exist", elf_name.string());
        return nullptr;
    }

    auto& m = m_modules.emplace_back();
    m = std::make_unique<Module>();
    m->elf.Open(elf_name);
    m->file_name = std::filesystem::path(elf_name).filename().string();

    if (m->elf.IsElfFile()) {
        LoadModuleToMemory(m.get());
        LoadDynamicInfo(m.get());
        LoadSymbols(m.get());
    } else {
        m_modules.pop_back();
        return nullptr; // It is not a valid elf file //TODO check it why!
    }

    return m.get();
}

Module* Linker::FindModule(u32 id) {
    // TODO atm we only have 1 module so we don't need to iterate on vector
    if (m_modules.empty()) [[unlikely]] {
        return nullptr;
    }
    return m_modules[0].get();
}

void Linker::LoadModuleToMemory(Module* m) {
    // get elf header, program header
    const auto elf_header = m->elf.GetElfHeader();
    const auto elf_pheader = m->elf.GetProgramHeader();

    u64 base_size = CalculateBaseSize(elf_header, elf_pheader);
    m->aligned_base_size = (base_size & ~(static_cast<u64>(0x1000) - 1)) +
                           0x1000; // align base size to 0x1000 block size (TODO is that the default
                                   // block size or it can be changed?

    m->base_virtual_addr = VirtualMemory::memory_alloc(LoadAddress, m->aligned_base_size,
                                                       VirtualMemory::MemoryMode::ExecuteReadWrite);

    LoadAddress += CODE_BASE_INCR * (1 + m->aligned_base_size / CODE_BASE_INCR);

    LOG_INFO(Core_Linker, "====Load Module to Memory ========");
    LOG_INFO(Core_Linker, "base_virtual_addr ......: {:#018x}", m->base_virtual_addr);
    LOG_INFO(Core_Linker, "base_size ..............: {:#018x}", base_size);
    LOG_INFO(Core_Linker, "aligned_base_size ......: {:#018x}", m->aligned_base_size);

    for (u16 i = 0; i < elf_header.e_phnum; i++) {
        switch (elf_pheader[i].p_type) {
        case PT_LOAD:
        case PT_SCE_RELRO:
            if (elf_pheader[i].p_memsz != 0) {
                u64 segment_addr = elf_pheader[i].p_vaddr + m->base_virtual_addr;
                u64 segment_file_size = elf_pheader[i].p_filesz;
                u64 segment_memory_size = GetAlignedSize(elf_pheader[i]);
                auto segment_mode = m->elf.ElfPheaderFlagsStr(elf_pheader[i].p_flags);
                LOG_INFO(Core_Linker, "program header = [{}] type = {}", i,
                         m->elf.ElfPheaderTypeStr(elf_pheader[i].p_type));
                LOG_INFO(Core_Linker, "segment_addr ..........: {:#018x}", segment_addr);
                LOG_INFO(Core_Linker, "segment_file_size .....: {}", segment_file_size);
                LOG_INFO(Core_Linker, "segment_memory_size ...: {}", segment_memory_size);
                LOG_INFO(Core_Linker, "segment_mode ..........: {}", segment_mode);

                m->elf.LoadSegment(segment_addr, elf_pheader[i].p_offset, segment_file_size);

                if (elf_pheader[i].p_flags & PF_EXEC) {
                    PatchTLS(segment_addr, segment_file_size);
                }
            } else {
                LOG_ERROR(Core_Linker, "p_memsz==0 in type {}",
                          m->elf.ElfPheaderTypeStr(elf_pheader[i].p_type));
            }
            break;
        case PT_DYNAMIC:
            if (elf_pheader[i].p_filesz != 0) {
                m->m_dynamic.resize(elf_pheader[i].p_filesz);
                m->elf.LoadSegment(reinterpret_cast<u64>(m->m_dynamic.data()),
                                   elf_pheader[i].p_offset, elf_pheader[i].p_filesz);
            } else {
                LOG_ERROR(Core_Linker, "p_filesz==0 in type {}",
                          m->elf.ElfPheaderTypeStr(elf_pheader[i].p_type));
            }
            break;
        case PT_SCE_DYNLIBDATA:
            if (elf_pheader[i].p_filesz != 0) {
                m->m_dynamic_data.resize(elf_pheader[i].p_filesz);
                m->elf.LoadSegment(reinterpret_cast<u64>(m->m_dynamic_data.data()),
                                   elf_pheader[i].p_offset, elf_pheader[i].p_filesz);
            } else {
                LOG_ERROR(Core_Linker, "p_filesz==0 in type {}",
                          m->elf.ElfPheaderTypeStr(elf_pheader[i].p_type));
            }
            break;
        case PT_TLS:
            m->tls.image_virtual_addr = elf_pheader[i].p_vaddr + m->base_virtual_addr;
            m->tls.image_size = GetAlignedSize(elf_pheader[i]);
            LOG_INFO(Core_Linker, "tls virtual address ={:#x}", m->tls.image_virtual_addr);
            LOG_INFO(Core_Linker, "tls image size      ={}", m->tls.image_size);
            break;
        case PT_SCE_PROCPARAM:
            m->proc_param_virtual_addr = elf_pheader[i].p_vaddr + m->base_virtual_addr;
            break;
        default:
            LOG_ERROR(Core_Linker, "Unimplemented type {}",
                      m->elf.ElfPheaderTypeStr(elf_pheader[i].p_type));
        }
    }
    LOG_INFO(Core_Linker, "program entry addr ..........: {:#018x}",
             m->elf.GetElfEntry() + m->base_virtual_addr);
}

void Linker::LoadDynamicInfo(Module* m) {
    for (const auto* dyn = reinterpret_cast<elf_dynamic*>(m->m_dynamic.data());
         dyn->d_tag != DT_NULL; dyn++) {
        switch (dyn->d_tag) {
        case DT_SCE_HASH: // Offset of the hash table.
            m->dynamic_info.hash_table =
                reinterpret_cast<void*>(m->m_dynamic_data.data() + dyn->d_un.d_ptr);
            break;
        case DT_SCE_HASHSZ: // Size of the hash table
            m->dynamic_info.hash_table_size = dyn->d_un.d_val;
            break;
        case DT_SCE_STRTAB: // Offset of the string table.
            m->dynamic_info.str_table =
                reinterpret_cast<char*>(m->m_dynamic_data.data() + dyn->d_un.d_ptr);
            break;
        case DT_SCE_STRSZ: // Size of the string table.
            m->dynamic_info.str_table_size = dyn->d_un.d_val;
            break;
        case DT_SCE_SYMTAB: // Offset of the symbol table.
            m->dynamic_info.symbol_table =
                reinterpret_cast<elf_symbol*>(m->m_dynamic_data.data() + dyn->d_un.d_ptr);
            break;
        case DT_SCE_SYMTABSZ: // Size of the symbol table.
            m->dynamic_info.symbol_table_total_size = dyn->d_un.d_val;
            break;
        case DT_INIT:
            m->dynamic_info.init_virtual_addr = dyn->d_un.d_ptr;
            break;
        case DT_FINI:
            m->dynamic_info.fini_virtual_addr = dyn->d_un.d_ptr;
            break;
        case DT_SCE_PLTGOT: // Offset of the global offset table.
            m->dynamic_info.pltgot_virtual_addr = dyn->d_un.d_ptr;
            break;
        case DT_SCE_JMPREL: // Offset of the table containing jump slots.
            m->dynamic_info.jmp_relocation_table =
                reinterpret_cast<elf_relocation*>(m->m_dynamic_data.data() + dyn->d_un.d_ptr);
            break;
        case DT_SCE_PLTRELSZ: // Size of the global offset table.
            m->dynamic_info.jmp_relocation_table_size = dyn->d_un.d_val;
            break;
        case DT_SCE_PLTREL: // The type of relocations in the relocation table. Should be DT_RELA
            m->dynamic_info.jmp_relocation_type = dyn->d_un.d_val;
            if (m->dynamic_info.jmp_relocation_type != DT_RELA) {
                LOG_WARNING(Core_Linker, "DT_SCE_PLTREL is NOT DT_RELA should check!");
            }
            break;
        case DT_SCE_RELA: // Offset of the relocation table.
            m->dynamic_info.relocation_table =
                reinterpret_cast<elf_relocation*>(m->m_dynamic_data.data() + dyn->d_un.d_ptr);
            break;
        case DT_SCE_RELASZ: // Size of the relocation table.
            m->dynamic_info.relocation_table_size = dyn->d_un.d_val;
            break;
        case DT_SCE_RELAENT: // The size of relocation table entries.
            m->dynamic_info.relocation_table_entries_size = dyn->d_un.d_val;
            if (m->dynamic_info.relocation_table_entries_size !=
                0x18) // this value should always be 0x18
            {
                LOG_WARNING(Core_Linker, "DT_SCE_RELAENT is NOT 0x18 should check!");
            }
            break;
        case DT_INIT_ARRAY: // Address of the array of pointers to initialization functions
            m->dynamic_info.init_array_virtual_addr = dyn->d_un.d_ptr;
            break;
        case DT_FINI_ARRAY: // Address of the array of pointers to termination functions
            m->dynamic_info.fini_array_virtual_addr = dyn->d_un.d_ptr;
            break;
        case DT_INIT_ARRAYSZ: // Size in bytes of the array of initialization functions
            m->dynamic_info.init_array_size = dyn->d_un.d_val;
            break;
        case DT_FINI_ARRAYSZ: // Size in bytes of the array of terminationfunctions
            m->dynamic_info.fini_array_size = dyn->d_un.d_val;
            break;
        case DT_PREINIT_ARRAY: // Address of the array of pointers to pre - initialization functions
            m->dynamic_info.preinit_array_virtual_addr = dyn->d_un.d_ptr;
            break;
        case DT_PREINIT_ARRAYSZ: // Size in bytes of the array of pre - initialization functions
            m->dynamic_info.preinit_array_size = dyn->d_un.d_val;
            break;
        case DT_SCE_SYMENT: // The size of symbol table entries
            m->dynamic_info.symbol_table_entries_size = dyn->d_un.d_val;
            if (m->dynamic_info.symbol_table_entries_size !=
                0x18) // this value should always be 0x18
            {
                LOG_WARNING(Core_Linker, "DT_SCE_SYMENT is NOT 0x18 should check!");
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
            if (m->dynamic_info.flags != 0x04) // this value should always be DF_TEXTREL (0x04)
            {
                LOG_WARNING(Core_Linker, "DT_FLAGS is NOT 0x04 should check!");
            }
            break;
        case DT_NEEDED: // Offset of the library string in the string table to be linked in.
            if (m->dynamic_info.str_table !=
                nullptr) // in theory this should already be filled from about just make a test case
            {
                m->dynamic_info.needed.push_back(m->dynamic_info.str_table + dyn->d_un.d_val);
            } else {
                LOG_ERROR(Core_Linker, "DT_NEEDED str table is not loaded should check!");
            }
            break;
        case DT_SCE_NEEDED_MODULE: {
            ModuleInfo info{};
            info.value = dyn->d_un.d_val;
            info.name = m->dynamic_info.str_table + info.name_offset;
            info.enc_id = EncodeId(info.id);
            m->dynamic_info.import_modules.push_back(info);
        } break;
        case DT_SCE_IMPORT_LIB: {
            LibraryInfo info{};
            info.value = dyn->d_un.d_val;
            info.name = m->dynamic_info.str_table + info.name_offset;
            info.enc_id = EncodeId(info.id);
            m->dynamic_info.import_libs.push_back(info);
        } break;
        case DT_SCE_FINGERPRINT:
            // The fingerprint is a 24 byte (0x18) size buffer that contains a unique identifier for
            // the given app. How exactly this is generated isn't known, however it is not necessary
            // to have a valid fingerprint. While an invalid fingerprint will cause a warning to be
            // printed to the kernel log, the ELF will still load and run.
            LOG_INFO(Core_Linker, "unsupported DT_SCE_FINGERPRINT value = ..........: {:#018x}",
                     dyn->d_un.d_val);
            break;
        case DT_SCE_IMPORT_LIB_ATTR:
            // The upper 32-bits should contain the module index multiplied by 0x10000. The lower
            // 32-bits should be a constant 0x9.
            LOG_INFO(Core_Linker, "unsupported DT_SCE_IMPORT_LIB_ATTR value = ......: {:#018x}",
                     dyn->d_un.d_val);
            break;
        case DT_SCE_ORIGINAL_FILENAME:
            m->dynamic_info.filename = m->dynamic_info.str_table + dyn->d_un.d_val;
            break;
        case DT_SCE_MODULE_INFO: // probably only useable in shared modules
        {
            ModuleInfo info{};
            info.value = dyn->d_un.d_val;
            info.name = m->dynamic_info.str_table + info.name_offset;
            info.enc_id = EncodeId(info.id);
            m->dynamic_info.export_modules.push_back(info);
        } break;
        case DT_SCE_MODULE_ATTR:
            // TODO?
            LOG_INFO(Core_Linker, "unsupported DT_SCE_MODULE_ATTR value = ..........: {:#018x}",
                     dyn->d_un.d_val);
            break;
        case DT_SCE_EXPORT_LIB: {
            LibraryInfo info{};
            info.value = dyn->d_un.d_val;
            info.name = m->dynamic_info.str_table + info.name_offset;
            info.enc_id = EncodeId(info.id);
            m->dynamic_info.export_libs.push_back(info);
        } break;
        default:
            LOG_INFO(Core_Linker, "unsupported dynamic tag ..........: {:#018x}", dyn->d_tag);
        }
    }
}

const ModuleInfo* Linker::FindModule(const Module& m, const std::string& id) {
    const auto& import_modules = m.dynamic_info.import_modules;
    int index = 0;
    for (const auto& mod : import_modules) {
        if (mod.enc_id.compare(id) == 0) {
            return &import_modules.at(index);
        }
        index++;
    }
    const auto& export_modules = m.dynamic_info.export_modules;
    index = 0;
    for (const auto& mod : export_modules) {
        if (mod.enc_id.compare(id) == 0) {
            return &export_modules.at(index);
        }
        index++;
    }
    return nullptr;
}

const LibraryInfo* Linker::FindLibrary(const Module& m, const std::string& id) {
    const auto& import_libs = m.dynamic_info.import_libs;
    int index = 0;
    for (const auto& lib : import_libs) {
        if (lib.enc_id.compare(id) == 0) {
            return &import_libs.at(index);
        }
        index++;
    }
    const auto& export_libs = m.dynamic_info.export_libs;
    index = 0;
    for (const auto& lib : export_libs) {
        if (lib.enc_id.compare(id) == 0) {
            return &export_libs.at(index);
        }
        index++;
    }
    return nullptr;
}

void Linker::LoadSymbols(Module* m) {

    const auto symbol_database = [this](Module* m, Loader::SymbolsResolver* symbol,
                                        bool export_func) {
        if (m->dynamic_info.symbol_table == nullptr || m->dynamic_info.str_table == nullptr ||
            m->dynamic_info.symbol_table_total_size == 0) {
            LOG_INFO(Core_Linker, "Symbol table not found!");
            return;
        }
        for (auto* sym = m->dynamic_info.symbol_table;
             reinterpret_cast<u8*>(sym) < reinterpret_cast<u8*>(m->dynamic_info.symbol_table) +
                                              m->dynamic_info.symbol_table_total_size;
             sym++) {
            std::string id = std::string(m->dynamic_info.str_table + sym->st_name);
            auto bind = sym->GetBind();
            auto type = sym->GetType();
            auto visibility = sym->GetVisibility();
            const auto ids = Common::SplitString(id, '#');
            if (ids.size() == 3) {
                const auto* library = FindLibrary(*m, ids.at(1));
                const auto* module = FindModule(*m, ids.at(2));
                ASSERT_MSG(library && module, "Unable to find library and module");
                if ((bind == STB_GLOBAL || bind == STB_WEAK) &&
                    (type == STT_FUN || type == STT_OBJECT) &&
                    export_func == (sym->st_value != 0)) {
                    std::string nidName = "";
                    auto aeronid = AeroLib::FindByNid(ids.at(0).c_str());
                    if (aeronid != nullptr) {
                        nidName = aeronid->name;
                    } else {
                        nidName = "UNK";
                    }
                    Loader::SymbolResolver sym_r{};
                    sym_r.name = ids.at(0);
                    sym_r.nidName = nidName;
                    sym_r.library = library->name;
                    sym_r.library_version = library->version;
                    sym_r.module = module->name;
                    sym_r.module_version_major = module->version_major;
                    sym_r.module_version_minor = module->version_minor;
                    switch (type) {
                    case STT_NOTYPE:
                        sym_r.type = Loader::SymbolType::NoType;
                        break;
                    case STT_FUN:
                        sym_r.type = Loader::SymbolType::Function;
                        break;
                    case STT_OBJECT:
                        sym_r.type = Loader::SymbolType::Object;
                        break;
                    default:
                        sym_r.type = Loader::SymbolType::Unknown;
                        break;
                    }
                    symbol->AddSymbol(sym_r,
                                      (export_func ? sym->st_value + m->base_virtual_addr : 0));
                }
            }
        }
    };
    symbol_database(m, &m->export_sym, true);
    symbol_database(m, &m->import_sym, false);
}

void Linker::Relocate(Module* m) {
    const auto relocate = [this](u32 idx, elf_relocation* rel, Module* m, bool isJmpRel) {
        auto type = rel->GetType();
        auto symbol = rel->GetSymbol();
        auto addend = rel->rel_addend;
        auto* symbolsTlb = m->dynamic_info.symbol_table;
        auto* namesTlb = m->dynamic_info.str_table;

        u64 rel_value = 0;
        u64 rel_base_virtual_addr = m->base_virtual_addr;
        u64 rel_virtual_addr = m->base_virtual_addr + rel->rel_offset;
        bool rel_isResolved = false;
        Loader::SymbolType rel_sym_type = Loader::SymbolType::Unknown;
        std::string rel_name;

        switch (type) {
        case R_X86_64_RELATIVE:
            rel_value = rel_base_virtual_addr + addend;
            rel_isResolved = true;
            break;
        case R_X86_64_DTPMOD64:
            rel_value = reinterpret_cast<uint64_t>(m);
            rel_isResolved = true;
            rel_sym_type = Loader::SymbolType::Tls;
            break;
        case R_X86_64_GLOB_DAT:
        case R_X86_64_JUMP_SLOT:
            addend = 0;
        case R_X86_64_64: {
            auto sym = symbolsTlb[symbol];
            auto sym_bind = sym.GetBind();
            auto sym_type = sym.GetType();
            auto sym_visibility = sym.GetVisibility();
            u64 symbol_vitrual_addr = 0;
            Loader::SymbolRecord symrec{};
            switch (sym_type) {
            case STT_FUN:
                rel_sym_type = Loader::SymbolType::Function;
                break;
            case STT_OBJECT:
                rel_sym_type = Loader::SymbolType::Object;
                break;
            case STT_NOTYPE:
                rel_sym_type = Loader::SymbolType::NoType;
                break;
            default:
                ASSERT_MSG(0, "unknown symbol type {}", sym_type);
            }
            if (sym_visibility != 0) // should be zero log if else
            {
                LOG_INFO(Core_Linker, "symbol visilibity !=0");
            }
            switch (sym_bind) {
            case STB_LOCAL:
                symbol_vitrual_addr = rel_base_virtual_addr + sym.st_value;
                break;
            case STB_GLOBAL:
            case STB_WEAK: {
                rel_name = namesTlb + sym.st_name;
                Resolve(rel_name, rel_sym_type, m, &symrec);
                symbol_vitrual_addr = symrec.virtual_address;
            } break;
            default:
                ASSERT_MSG(0, "unknown bind type {}", sym_bind);
            }
            rel_isResolved = (symbol_vitrual_addr != 0);
            rel_value = (rel_isResolved ? symbol_vitrual_addr + addend : 0);
            rel_name = symrec.name;
        } break;
        default:
            LOG_INFO(Core_Linker, "UNK type {:#010x} rel symbol : {:#010x}", type, symbol);
        }

        if (rel_isResolved) {
            VirtualMemory::memory_patch(rel_virtual_addr, rel_value);
        } else {
            LOG_INFO(Core_Linker, "function not patched! {}", rel_name);
        }
    };

    u32 idx = 0;
    for (auto* rel = m->dynamic_info.relocation_table;
         reinterpret_cast<u8*>(rel) < reinterpret_cast<u8*>(m->dynamic_info.relocation_table) +
                                          m->dynamic_info.relocation_table_size;
         rel++, idx++) {
        relocate(idx, rel, m, false);
    }
    idx = 0;
    for (auto* rel = m->dynamic_info.jmp_relocation_table;
         reinterpret_cast<u8*>(rel) < reinterpret_cast<u8*>(m->dynamic_info.jmp_relocation_table) +
                                          m->dynamic_info.jmp_relocation_table_size;
         rel++, idx++) {
        relocate(idx, rel, m, true);
    }
}

template <typename T>
bool contains(const std::vector<T>& vecObj, const T& element) {
    auto it = std::find(vecObj.begin(), vecObj.end(), element);
    return it != vecObj.end();
}

Module* Linker::FindExportedModule(const ModuleInfo& module, const LibraryInfo& library) {
    // std::scoped_lock lock{m_mutex};

    for (auto& m : m_modules) {
        const auto& export_libs = m->dynamic_info.export_libs;
        const auto& export_modules = m->dynamic_info.export_modules;

        if (contains(export_libs, library) && contains(export_modules, module)) {
            return m.get();
        }
    }
    return nullptr;
}

void Linker::Resolve(const std::string& name, Loader::SymbolType sym_type, Module* m,
                     Loader::SymbolRecord* return_info) {
    // std::scoped_lock lock{m_mutex};
    const auto ids = Common::SplitString(name, '#');
    ASSERT_MSG(ids.size() == 3, "Symbols must be 3 parts name, library, module");

    const auto* library = FindLibrary(*m, ids.at(1));
    const auto* module = FindModule(*m, ids.at(2));
    ASSERT_MSG(library && module, "Unable to find library and module");

    Loader::SymbolResolver sr{};
    sr.name = ids.at(0);
    sr.library = library->name;
    sr.library_version = library->version;
    sr.module = module->name;
    sr.module_version_major = module->version_major;
    sr.module_version_minor = module->version_minor;
    sr.type = sym_type;

    const Loader::SymbolRecord* rec = nullptr;

    rec = m_hle_symbols.FindSymbol(sr);
    if (rec == nullptr) {
        // check if it an export function
        if (auto* p = FindExportedModule(*module, *library);
            p != nullptr && p->export_sym.GetSize() > 0) {
            rec = p->export_sym.FindSymbol(sr);
        }
    }
    if (rec != nullptr) {
        *return_info = *rec;
    } else {
        auto aeronid = AeroLib::FindByNid(sr.name.c_str());
        if (aeronid) {
            return_info->name = aeronid->name;
            return_info->virtual_address = AeroLib::GetStub(aeronid->nid);
        } else {
            return_info->virtual_address = AeroLib::GetStub(sr.name.c_str());
            return_info->name = "Unknown !!!";
        }
        LOG_ERROR(Core_Linker, "Linker: Stub resolved {} as {} (lib: {}, mod: {})", sr.name,
                  return_info->name, library->name, module->name);
    }
}

u64 Linker::GetProcParam() {
    // std::scoped_lock lock{m_mutex};

    for (auto& m : m_modules) {
        if (!m->elf.IsSharedLib()) {
            return m->proc_param_virtual_addr;
        }
    }
    return 0;
}
using exit_func_t = PS4_SYSV_ABI void (*)();
using entry_func_t = PS4_SYSV_ABI void (*)(EntryParams* params, exit_func_t atexit_func);
using module_ini_func_t = PS4_SYSV_ABI int (*)(size_t args, const void* argp, module_func_t func);

static PS4_SYSV_ABI int run_module(uint64_t addr, size_t args, const void* argp,
                                   module_func_t func) {
    return reinterpret_cast<module_ini_func_t>(addr)(args, argp, func);
}

int Linker::StartModule(Module* m, size_t args, const void* argp, module_func_t func) {
    LOG_INFO(Core_Linker, "Module started : {}\n", m->file_name);
    return run_module(m->dynamic_info.init_virtual_addr + m->base_virtual_addr, args, argp, func);
}

void Linker::StartAllModules() {
    std::scoped_lock lock{m_mutex};

    for (auto& m : m_modules) {
        if (m->elf.IsSharedLib()) {
            StartModule(m.get(), 0, nullptr, nullptr);
        }
    }
}

static PS4_SYSV_ABI void ProgramExitFunc() {
    fmt::print("exit function called\n");
}

static void RunMainEntry(u64 addr, EntryParams* params, exit_func_t exit_func) {
    // reinterpret_cast<entry_func_t>(addr)(params, exit_func); // can't be used, stack has to have
    // a specific layout

    asm volatile("andq $-16, %%rsp\n" // Align to 16 bytes
                 "subq $8, %%rsp\n"   // videoout_basic expects the stack to be misaligned

                 // Kernel also pushes some more things here during process init
                 // at least: environment, auxv, possibly other things

                 "pushq 8(%1)\n" // copy EntryParams to top of stack like the kernel does
                 "pushq 0(%1)\n" // OpenOrbis expects to find it there

                 "movq %1, %%rdi\n" // also pass params and exit func
                 "movq %2, %%rsi\n" // as before

                 "jmp *%0\n" // can't use call here, as that would mangle the prepared stack.
                             // there's no coming back
                 :
                 : "r"(addr), "r"(params), "r"(exit_func)
                 : "rax", "rsi", "rdi", "rsp", "rbp");
}

void Linker::Execute() {
    if (Config::debugDump()) {
        DebugDump();
    }

    Core::Libraries::LibKernel::pthreadInitSelfMainThread();
    // relocate all modules
    for (const auto& m : m_modules) {
        Relocate(m.get());
    }
    StartAllModules();
    EntryParams p{};
    p.argc = 1;
    p.argv[0] = "eboot.bin"; // hmm should be ok?

    for (auto& m : m_modules) {
        if (!m->elf.IsSharedLib()) {
            RunMainEntry(m->elf.GetElfEntry() + m->base_virtual_addr, &p, ProgramExitFunc);
        }
    }
}

void Linker::DebugDump() {
    std::scoped_lock lock{m_mutex};
    const auto& log_dir = Common::FS::GetUserPath(Common::FS::PathType::LogDir);
    const std::filesystem::path debug(log_dir / "debugdump");
    std::filesystem::create_directory(debug);
    for (const auto& m : m_modules) {
        // TODO make a folder with game id for being more unique?
        const std::filesystem::path filepath(debug / m.get()->file_name);
        std::filesystem::create_directory(filepath);
        m.get()->import_sym.DebugDump(filepath / "imports.txt");
        m.get()->export_sym.DebugDump(filepath / "exports.txt");
    }
}

} // namespace Core
