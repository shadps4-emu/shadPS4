// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/alignment.h"
#include "common/arch.h"
#include "common/assert.h"
#include "common/logging/log.h"
#include "common/memory_patcher.h"
#include "common/string_util.h"
#include "core/aerolib/aerolib.h"
#include "core/cpu_patches.h"
#include "core/loader/dwarf.h"
#include "core/memory.h"
#include "core/module.h"

namespace Core {

using EntryFunc = PS4_SYSV_ABI int (*)(size_t args, const void* argp, void* param);

static u64 LoadOffset = CODE_BASE_OFFSET;
static constexpr u64 CODE_BASE_INCR = 0x010000000u;

static u64 GetAlignedSize(const elf_program_header& phdr) {
    return (phdr.p_align != 0 ? (phdr.p_memsz + (phdr.p_align - 1)) & ~(phdr.p_align - 1)
                              : phdr.p_memsz);
}

static u64 CalculateBaseSize(const elf_header& ehdr, std::span<const elf_program_header> phdr) {
    u64 base_size = 0;
    for (u16 i = 0; i < ehdr.e_phnum; i++) {
        if (phdr[i].p_memsz != 0 && (phdr[i].p_type == PT_LOAD || phdr[i].p_type == PT_SCE_RELRO)) {
            const u64 last_addr = phdr[i].p_vaddr + GetAlignedSize(phdr[i]);
            base_size = std::max(last_addr, base_size);
        }
    }
    return base_size;
}

static std::string EncodeId(u64 nVal) {
    std::string enc;
    static constexpr std::string_view codes =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+-";
    if (nVal < 0x40u) {
        enc += codes[nVal];
    } else {
        if (nVal < 0x1000u) {
            enc += codes[static_cast<u16>(nVal >> 6u) & 0x3fu];
            enc += codes[nVal & 0x3fu];
        } else {
            enc += codes[static_cast<u16>(nVal >> 12u) & 0x3fu];
            enc += codes[static_cast<u16>(nVal >> 6u) & 0x3fu];
            enc += codes[nVal & 0x3fu];
        }
    }
    return enc;
}

Module::Module(Core::MemoryManager* memory_, const std::filesystem::path& file_, u32& max_tls_index)
    : memory{memory_}, file{file_}, name{file.stem().string()} {
    elf.Open(file);
    if (elf.IsElfFile()) {
        LoadModuleToMemory(max_tls_index);
        LoadDynamicInfo();
        LoadSymbols();
    }
}

Module::~Module() = default;

s32 Module::Start(size_t args, const void* argp, void* param) {
    LOG_INFO(Core_Linker, "Module started : {}", name);
    const VAddr addr = dynamic_info.init_virtual_addr + GetBaseAddress();
    return reinterpret_cast<EntryFunc>(addr)(args, argp, param);
}

void Module::LoadModuleToMemory(u32& max_tls_index) {
    static constexpr size_t BlockAlign = 0x1000;
    static constexpr u64 TrampolineSize = 8_MB;

    // Retrieve elf header and program header
    const auto elf_header = elf.GetElfHeader();
    const auto elf_pheader = elf.GetProgramHeader();
    const u64 base_size = CalculateBaseSize(elf_header, elf_pheader);
    aligned_base_size = Common::AlignUp(base_size, BlockAlign);

    // Map module segments (and possible TLS trampolines)
    void** out_addr = reinterpret_cast<void**>(&base_virtual_addr);
    memory->MapMemory(out_addr, memory->SystemReservedVirtualBase() + LoadOffset,
                      aligned_base_size + TrampolineSize, MemoryProt::CpuReadWrite,
                      MemoryMapFlags::Fixed, VMAType::Code, name, true);
    LoadOffset += CODE_BASE_INCR * (1 + aligned_base_size / CODE_BASE_INCR);
    LOG_INFO(Core_Linker, "Loading module {} to {}", name, fmt::ptr(*out_addr));

#ifdef ARCH_X86_64
    // Initialize trampoline generator.
    void* trampoline_addr = std::bit_cast<void*>(base_virtual_addr + aligned_base_size);
    RegisterPatchModule(*out_addr, aligned_base_size, trampoline_addr, TrampolineSize);
#endif

    LOG_INFO(Core_Linker, "======== Load Module to Memory ========");
    LOG_INFO(Core_Linker, "base_virtual_addr ......: {:#018x}", base_virtual_addr);
    LOG_INFO(Core_Linker, "base_size ..............: {:#018x}", base_size);
    LOG_INFO(Core_Linker, "aligned_base_size ......: {:#018x}", aligned_base_size);

    const auto add_segment = [this](const elf_program_header& phdr, bool do_map = true) {
        const VAddr segment_addr = base_virtual_addr + phdr.p_vaddr;
        if (do_map) {
            elf.LoadSegment(segment_addr, phdr.p_offset, phdr.p_filesz);
        }
        auto& segment = info.segments[info.num_segments++];
        segment.address = segment_addr;
        segment.prot = phdr.p_flags;
        segment.size = GetAlignedSize(phdr);
    };

    for (u16 i = 0; i < elf_header.e_phnum; i++) {
        const auto header_type = elf.ElfPheaderTypeStr(elf_pheader[i].p_type);
        switch (elf_pheader[i].p_type) {
        case PT_LOAD:
        case PT_SCE_RELRO: {
            if (elf_pheader[i].p_memsz == 0) {
                LOG_ERROR(Core_Linker, "p_memsz==0 in type {}", header_type);
                continue;
            }

            const u64 segment_addr = elf_pheader[i].p_vaddr + base_virtual_addr;
            const u64 segment_file_size = elf_pheader[i].p_filesz;
            const u64 segment_memory_size = GetAlignedSize(elf_pheader[i]);
            const auto segment_mode = elf.ElfPheaderFlagsStr(elf_pheader[i].p_flags);
            LOG_INFO(Core_Linker, "program header = [{}] type = {}", i, header_type);
            LOG_INFO(Core_Linker, "segment_addr ..........: {:#018x}", segment_addr);
            LOG_INFO(Core_Linker, "segment_file_size .....: {}", segment_file_size);
            LOG_INFO(Core_Linker, "segment_memory_size ...: {}", segment_memory_size);
            LOG_INFO(Core_Linker, "segment_mode ..........: {}", segment_mode);

            add_segment(elf_pheader[i]);
#ifdef ARCH_X86_64
            if (elf_pheader[i].p_flags & PF_EXEC) {
                PrePatchInstructions(segment_addr, segment_file_size);
            }
#endif
            break;
        }
        case PT_DYNAMIC:
            add_segment(elf_pheader[i], false);
            if (elf_pheader[i].p_filesz != 0) {
                m_dynamic.resize(elf_pheader[i].p_filesz);
                const VAddr segment_addr = std::bit_cast<VAddr>(m_dynamic.data());
                elf.LoadSegment(segment_addr, elf_pheader[i].p_offset, elf_pheader[i].p_filesz);
            } else {
                LOG_ERROR(Core_Linker, "p_filesz==0 in type {}", header_type);
            }
            break;
        case PT_SCE_DYNLIBDATA:
            if (elf_pheader[i].p_filesz != 0) {
                m_dynamic_data.resize(elf_pheader[i].p_filesz);
                const VAddr segment_addr = std::bit_cast<VAddr>(m_dynamic_data.data());
                elf.LoadSegment(segment_addr, elf_pheader[i].p_offset, elf_pheader[i].p_filesz);
            } else {
                LOG_ERROR(Core_Linker, "p_filesz==0 in type {}", header_type);
            }
            break;
        case PT_TLS:
            tls.init_image_size = elf_pheader[i].p_filesz;
            tls.align = elf_pheader[i].p_align;
            tls.image_virtual_addr = elf_pheader[i].p_vaddr + base_virtual_addr;
            tls.image_size = GetAlignedSize(elf_pheader[i]);
            if (tls.image_size != 0) {
                tls.modid = ++max_tls_index;
            }
            LOG_INFO(Core_Linker, "TLS virtual address = {:#x}", tls.image_virtual_addr);
            LOG_INFO(Core_Linker, "TLS image size      = {}", tls.image_size);
            break;
        case PT_SCE_PROCPARAM:
            proc_param_virtual_addr = elf_pheader[i].p_vaddr + base_virtual_addr;
            break;
        case PT_GNU_EH_FRAME: {
            eh_frame_hdr_addr = elf_pheader[i].p_vaddr;
            eh_frame_hdr_size = elf_pheader[i].p_memsz;
            const VAddr eh_hdr_start = base_virtual_addr + eh_frame_hdr_addr;
            const VAddr eh_hdr_end = eh_hdr_start + eh_frame_hdr_size;
            Dwarf::EHHeaderInfo hdr_info;
            if (Dwarf::DecodeEHHdr(eh_hdr_start, eh_hdr_end, hdr_info)) {
                eh_frame_addr = hdr_info.eh_frame_ptr - base_virtual_addr;
                if (eh_frame_hdr_addr > eh_frame_addr) {
                    eh_frame_size = (eh_frame_hdr_addr - eh_frame_addr);
                } else {
                    eh_frame_size = (aligned_base_size - eh_frame_hdr_addr);
                }
            }
            break;
        }
        default:
            LOG_ERROR(Core_Linker, "Unimplemented type {}", header_type);
        }
    }

    const VAddr entry_addr = base_virtual_addr + elf.GetElfEntry();
    LOG_INFO(Core_Linker, "program entry addr ..........: {:#018x}", entry_addr);

    if (MemoryPatcher::g_eboot_address == 0) {
        if (name == "eboot") {
            MemoryPatcher::g_eboot_address = base_virtual_addr;
            MemoryPatcher::g_eboot_image_size = base_size;
            MemoryPatcher::OnGameLoaded();
        }
    }
}

void Module::LoadDynamicInfo() {
    for (const auto* dyn = reinterpret_cast<elf_dynamic*>(m_dynamic.data()); dyn->d_tag != DT_NULL;
         dyn++) {
        switch (dyn->d_tag) {
        case DT_SCE_HASH: // Offset of the hash table.
            dynamic_info.hash_table =
                reinterpret_cast<void*>(m_dynamic_data.data() + dyn->d_un.d_ptr);
            break;
        case DT_SCE_HASHSZ: // Size of the hash table
            dynamic_info.hash_table_size = dyn->d_un.d_val;
            break;
        case DT_SCE_STRTAB: // Offset of the string table.
            dynamic_info.str_table =
                reinterpret_cast<char*>(m_dynamic_data.data() + dyn->d_un.d_ptr);
            break;
        case DT_SCE_STRSZ: // Size of the string table.
            dynamic_info.str_table_size = dyn->d_un.d_val;
            break;
        case DT_SCE_SYMTAB: // Offset of the symbol table.
            dynamic_info.symbol_table =
                reinterpret_cast<elf_symbol*>(m_dynamic_data.data() + dyn->d_un.d_ptr);
            break;
        case DT_SCE_SYMTABSZ: // Size of the symbol table.
            dynamic_info.symbol_table_total_size = dyn->d_un.d_val;
            break;
        case DT_INIT:
            dynamic_info.init_virtual_addr = dyn->d_un.d_ptr;
            break;
        case DT_FINI:
            dynamic_info.fini_virtual_addr = dyn->d_un.d_ptr;
            break;
        case DT_SCE_PLTGOT: // Offset of the global offset table.
            dynamic_info.pltgot_virtual_addr = dyn->d_un.d_ptr;
            break;
        case DT_SCE_JMPREL: // Offset of the table containing jump slots.
            dynamic_info.jmp_relocation_table =
                reinterpret_cast<elf_relocation*>(m_dynamic_data.data() + dyn->d_un.d_ptr);
            break;
        case DT_SCE_PLTRELSZ: // Size of the global offset table.
            dynamic_info.jmp_relocation_table_size = dyn->d_un.d_val;
            break;
        case DT_SCE_PLTREL: // The type of relocations in the relocation table. Should be DT_RELA
            dynamic_info.jmp_relocation_type = dyn->d_un.d_val;
            if (dynamic_info.jmp_relocation_type != DT_RELA) {
                LOG_WARNING(Core_Linker, "DT_SCE_PLTREL is NOT DT_RELA should check!");
            }
            break;
        case DT_SCE_RELA: // Offset of the relocation table.
            dynamic_info.relocation_table =
                reinterpret_cast<elf_relocation*>(m_dynamic_data.data() + dyn->d_un.d_ptr);
            break;
        case DT_SCE_RELASZ: // Size of the relocation table.
            dynamic_info.relocation_table_size = dyn->d_un.d_val;
            break;
        case DT_SCE_RELAENT: // The size of relocation table entries.
            dynamic_info.relocation_table_entries_size = dyn->d_un.d_val;
            if (dynamic_info.relocation_table_entries_size != 0x18) {
                LOG_WARNING(Core_Linker, "DT_SCE_RELAENT is NOT 0x18 should check!");
            }
            break;
        case DT_INIT_ARRAY: // Address of the array of pointers to initialization functions
            dynamic_info.init_array_virtual_addr = dyn->d_un.d_ptr;
            break;
        case DT_FINI_ARRAY: // Address of the array of pointers to termination functions
            dynamic_info.fini_array_virtual_addr = dyn->d_un.d_ptr;
            break;
        case DT_INIT_ARRAYSZ: // Size in bytes of the array of initialization functions
            dynamic_info.init_array_size = dyn->d_un.d_val;
            break;
        case DT_FINI_ARRAYSZ: // Size in bytes of the array of terminationfunctions
            dynamic_info.fini_array_size = dyn->d_un.d_val;
            break;
        case DT_PREINIT_ARRAY: // Address of the array of pointers to pre - initialization functions
            dynamic_info.preinit_array_virtual_addr = dyn->d_un.d_ptr;
            break;
        case DT_PREINIT_ARRAYSZ: // Size in bytes of the array of pre - initialization functions
            dynamic_info.preinit_array_size = dyn->d_un.d_val;
            break;
        case DT_SCE_SYMENT: // The size of symbol table entries
            dynamic_info.symbol_table_entries_size = dyn->d_un.d_val;
            if (dynamic_info.symbol_table_entries_size != 0x18) {
                LOG_WARNING(Core_Linker, "DT_SCE_SYMENT is NOT 0x18 should check!");
            }
            break;
        case DT_DEBUG:
            dynamic_info.debug = dyn->d_un.d_val;
            break;
        case DT_TEXTREL:
            dynamic_info.textrel = dyn->d_un.d_val;
            break;
        case DT_FLAGS:
            dynamic_info.flags = dyn->d_un.d_val;
            // This value should always be DF_TEXTREL (0x04)
            if (dynamic_info.flags != 0x04) {
                LOG_WARNING(Core_Linker, "DT_FLAGS is NOT 0x04 should check!");
            }
            break;
        case DT_NEEDED:
            // Offset of the library string in the string table to be linked in.
            // In theory this should already be filled from about just make a test case
            if (dynamic_info.str_table) {
                dynamic_info.needed.push_back(dynamic_info.str_table + dyn->d_un.d_val);
            } else {
                LOG_ERROR(Core_Linker, "DT_NEEDED str table is not loaded should check!");
            }
            break;
        case DT_SCE_NEEDED_MODULE: {
            ModuleInfo& info = dynamic_info.import_modules.emplace_back();
            info.value = dyn->d_un.d_val;
            info.name = dynamic_info.str_table + info.name_offset;
            info.enc_id = EncodeId(info.id);
            break;
        }
        case DT_SCE_IMPORT_LIB: {
            LibraryInfo& info = dynamic_info.import_libs.emplace_back();
            info.value = dyn->d_un.d_val;
            info.name = dynamic_info.str_table + info.name_offset;
            info.enc_id = EncodeId(info.id);
            break;
        }
        case DT_SCE_FINGERPRINT:
            // The fingerprint is a 24 byte (0x18) size buffer that contains a unique identifier for
            // the given app. How exactly this is generated isn't known, however it is not necessary
            // to have a valid fingerprint. While an invalid fingerprint will cause a warning to be
            // printed to the kernel log, the ELF will still load and run.
            LOG_INFO(Core_Linker, "DT_SCE_FINGERPRINT value = {:#018x}", dyn->d_un.d_val);
            std::memcpy(info.fingerprint.data(), &dyn->d_un.d_val, sizeof(SCE_DBG_NUM_FINGERPRINT));
            break;
        case DT_SCE_IMPORT_LIB_ATTR:
            // The upper 32-bits should contain the module index multiplied by 0x10000. The lower
            // 32-bits should be a constant 0x9.
            LOG_INFO(Core_Linker, "unsupported DT_SCE_IMPORT_LIB_ATTR value = ......: {:#018x}",
                     dyn->d_un.d_val);
            break;
        case DT_SCE_ORIGINAL_FILENAME:
            dynamic_info.filename = dynamic_info.str_table + dyn->d_un.d_val;
            break;
        case DT_SCE_MODULE_INFO: {
            ModuleInfo& info = dynamic_info.export_modules.emplace_back();
            info.value = dyn->d_un.d_val;
            info.name = dynamic_info.str_table + info.name_offset;
            info.enc_id = EncodeId(info.id);
            const std::string full_name = info.name + ".sprx";
            full_name.copy(this->info.name.data(), full_name.size());
            break;
        };
        case DT_SCE_MODULE_ATTR:
            LOG_INFO(Core_Linker, "unsupported DT_SCE_MODULE_ATTR value = ..........: {:#018x}",
                     dyn->d_un.d_val);
            break;
        case DT_SCE_EXPORT_LIB: {
            LibraryInfo& info = dynamic_info.export_libs.emplace_back();
            info.value = dyn->d_un.d_val;
            info.name = dynamic_info.str_table + info.name_offset;
            info.enc_id = EncodeId(info.id);
            break;
        }
        default:
            LOG_INFO(Core_Linker, "unsupported dynamic tag ..........: {:#018x}", dyn->d_tag);
        }
    }
    const u32 relabits_num = dynamic_info.relocation_table_size / sizeof(elf_relocation) +
                             dynamic_info.jmp_relocation_table_size / sizeof(elf_relocation);
    rela_bits.resize((relabits_num + 7) / 8);
}

void Module::LoadSymbols() {
    const auto symbol_database = [this](Loader::SymbolsResolver& symbol, bool export_func) {
        if (!dynamic_info.symbol_table || !dynamic_info.str_table ||
            dynamic_info.symbol_table_total_size == 0) {
            LOG_INFO(Core_Linker, "Symbol table not found!");
            return;
        }
        for (auto* sym = dynamic_info.symbol_table;
             reinterpret_cast<u8*>(sym) < reinterpret_cast<u8*>(dynamic_info.symbol_table) +
                                              dynamic_info.symbol_table_total_size;
             sym++) {
            const u8 bind = sym->GetBind();
            const u8 type = sym->GetType();
            const u8 visibility = sym->GetVisibility();
            const auto id = std::string(dynamic_info.str_table + sym->st_name);
            const auto ids = Common::SplitString(id, '#');
            if (ids.size() != 3) {
                continue;
            }

            const auto* library = FindLibrary(ids[1]);
            const auto* module = FindModule(ids[2]);
            ASSERT_MSG(library && module, "Unable to find library and module");
            if ((bind != STB_GLOBAL && bind != STB_WEAK) ||
                (type != STT_FUN && type != STT_OBJECT) || export_func != (sym->st_value != 0)) {
                continue;
            }

            const auto aeronid = AeroLib::FindByNid(ids.at(0).c_str());
            const auto nid_name = aeronid ? aeronid->name : "UNK";

            Loader::SymbolResolver sym_r{};
            sym_r.name = ids.at(0);
            sym_r.nidName = nid_name;
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
            const VAddr sym_addr = export_func ? sym->st_value + base_virtual_addr : 0;
            symbol.AddSymbol(sym_r, sym_addr);
        }
    };
    symbol_database(export_sym, true);
    symbol_database(import_sym, false);
}

OrbisKernelModuleInfoEx Module::GetModuleInfoEx() const {
    return OrbisKernelModuleInfoEx{
        .name = info.name,
        .tls_index = tls.modid,
        .tls_init_addr = tls.image_virtual_addr,
        .tls_init_size = tls.init_image_size,
        .tls_size = tls.image_size,
        .tls_offset = tls.offset,
        .tls_align = tls.align,
        .init_proc_addr = base_virtual_addr + dynamic_info.init_virtual_addr,
        .fini_proc_addr = base_virtual_addr + dynamic_info.fini_virtual_addr,
        .eh_frame_hdr_addr = eh_frame_hdr_addr,
        .eh_frame_addr = eh_frame_addr,
        .eh_frame_hdr_size = eh_frame_hdr_size,
        .eh_frame_size = eh_frame_size,
        .segments = info.segments,
        .segment_count = info.num_segments,
    };
}

const ModuleInfo* Module::FindModule(std::string_view id) {
    const auto& import_modules = dynamic_info.import_modules;
    for (u32 i = 0; const auto& mod : import_modules) {
        if (mod.enc_id == id) {
            return &import_modules[i];
        }
        i++;
    }
    const auto& export_modules = dynamic_info.export_modules;
    for (u32 i = 0; const auto& mod : export_modules) {
        if (mod.enc_id == id) {
            return &export_modules[i];
        }
        i++;
    }
    return nullptr;
}

const LibraryInfo* Module::FindLibrary(std::string_view id) {
    const auto& import_libs = dynamic_info.import_libs;
    for (u32 i = 0; const auto& lib : import_libs) {
        if (lib.enc_id == id) {
            return &import_libs[i];
        }
        i++;
    }
    const auto& export_libs = dynamic_info.export_libs;
    for (u32 i = 0; const auto& lib : export_libs) {
        if (lib.enc_id == id) {
            return &export_libs[i];
        }
        i++;
    }
    return nullptr;
}

} // namespace Core
