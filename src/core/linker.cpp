// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/alignment.h"
#include "common/arch.h"
#include "common/assert.h"
#include "common/config.h"
#include "common/logging/log.h"
#include "common/path_util.h"
#include "common/string_util.h"
#include "common/thread.h"
#include "core/aerolib/aerolib.h"
#include "core/aerolib/stubs.h"
#include "core/cpu_patches.h"
#include "core/libraries/kernel/memory_management.h"
#include "core/libraries/kernel/thread_management.h"
#include "core/linker.h"
#include "core/memory.h"
#include "core/tls.h"
#include "core/virtual_memory.h"
#include "debug_state.h"

namespace Core {

using ExitFunc = PS4_SYSV_ABI void (*)();

static PS4_SYSV_ABI void ProgramExitFunc() {
    fmt::print("exit function called\n");
}

#ifdef ARCH_X86_64
static PS4_SYSV_ABI void RunMainEntry(VAddr addr, EntryParams* params, ExitFunc exit_func) {
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
                 : "rax", "rsi", "rdi");
}
#endif

Linker::Linker() : memory{Memory::Instance()} {}

Linker::~Linker() = default;

void Linker::Execute() {
    if (Config::debugDump()) {
        DebugDump();
    }

    // Calculate static TLS size.
    for (const auto& module : m_modules) {
        static_tls_size += module->tls.image_size;
        module->tls.offset = static_tls_size;
    }

    // Relocate all modules
    for (const auto& m : m_modules) {
        Relocate(m.get());
    }

    // Configure used flexible memory size.
    if (const auto* proc_param = GetProcParam()) {
        if (proc_param->size >=
            offsetof(OrbisProcParam, mem_param) + sizeof(OrbisKernelMemParam*)) {
            if (const auto* mem_param = proc_param->mem_param) {
                if (mem_param->size >=
                    offsetof(OrbisKernelMemParam, flexible_memory_size) + sizeof(u64*)) {
                    if (const auto* flexible_size = mem_param->flexible_memory_size) {
                        memory->SetupMemoryRegions(*flexible_size);
                    }
                }
            }
        }
    }

    // Init primary thread.
    Common::SetCurrentThreadName("GAME_MainThread");
    DebugState.AddCurrentThreadToGuestList();
    Libraries::Kernel::pthreadInitSelfMainThread();
    EnsureThreadInitialized(true);

    // Start shared library modules
    for (auto& m : m_modules) {
        if (m->IsSharedLib()) {
            m->Start(0, nullptr, nullptr);
        }
    }

    // Start main module.
    EntryParams p{};
    p.argc = 1;
    p.argv[0] = "eboot.bin";

    for (auto& m : m_modules) {
        if (!m->IsSharedLib()) {
#ifdef ARCH_X86_64
            ExecuteGuest(RunMainEntry, m->GetEntryAddress(), &p, ProgramExitFunc);
#else
            UNIMPLEMENTED_MSG(
                "Missing guest entrypoint implementation for target CPU architecture.");
#endif
        }
    }

    SetTcbBase(nullptr);
}

s32 Linker::LoadModule(const std::filesystem::path& elf_name, bool is_dynamic) {
    std::scoped_lock lk{mutex};

    if (!std::filesystem::exists(elf_name)) {
        LOG_ERROR(Core_Linker, "Provided file {} does not exist", elf_name.string());
        return -1;
    }

    auto module = std::make_unique<Module>(memory, elf_name, max_tls_index);
    if (!module->IsValid()) {
        LOG_ERROR(Core_Linker, "Provided file {} is not valid ELF file", elf_name.string());
        return -1;
    }

    num_static_modules += !is_dynamic;
    m_modules.emplace_back(std::move(module));
    return m_modules.size() - 1;
}

Module* Linker::FindByAddress(VAddr address) {
    for (auto& module : m_modules) {
        const VAddr base = module->GetBaseAddress();
        if (address >= base && address < base + module->aligned_base_size) {
            return module.get();
        }
    }
    return nullptr;
}

void Linker::Relocate(Module* module) {
    module->ForEachRelocation([&](elf_relocation* rel, u32 i, bool isJmpRel) {
        const u32 bit_idx =
            (isJmpRel ? module->dynamic_info.relocation_table_size / sizeof(elf_relocation) : 0) +
            i;
        if (module->TestRelaBit(bit_idx)) {
            return;
        }
        auto type = rel->GetType();
        auto symbol = rel->GetSymbol();
        auto addend = rel->rel_addend;
        auto* symbol_table = module->dynamic_info.symbol_table;
        auto* namesTlb = module->dynamic_info.str_table;

        const VAddr rel_base_virtual_addr = module->GetBaseAddress();
        const VAddr rel_virtual_addr = rel_base_virtual_addr + rel->rel_offset;
        bool rel_is_resolved = false;
        u64 rel_value = 0;
        Loader::SymbolType rel_sym_type = Loader::SymbolType::Unknown;
        std::string rel_name;

        switch (type) {
        case R_X86_64_RELATIVE:
            rel_value = rel_base_virtual_addr + addend;
            rel_is_resolved = true;
            module->SetRelaBit(bit_idx);
            break;
        case R_X86_64_DTPMOD64:
            rel_value = static_cast<u64>(module->tls.modid);
            rel_is_resolved = true;
            rel_sym_type = Loader::SymbolType::Tls;
            module->SetRelaBit(bit_idx);
            break;
        case R_X86_64_GLOB_DAT:
        case R_X86_64_JUMP_SLOT:
            addend = 0;
        case R_X86_64_64: {
            auto sym = symbol_table[symbol];
            auto sym_bind = sym.GetBind();
            auto sym_type = sym.GetType();
            auto sym_visibility = sym.GetVisibility();
            u64 symbol_virtual_addr = 0;
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

            if (sym_visibility != 0) {
                LOG_INFO(Core_Linker, "symbol visibility !=0");
            }

            switch (sym_bind) {
            case STB_LOCAL:
                symbol_virtual_addr = rel_base_virtual_addr + sym.st_value;
                module->SetRelaBit(bit_idx);
                break;
            case STB_GLOBAL:
            case STB_WEAK: {
                rel_name = namesTlb + sym.st_name;
                if (Resolve(rel_name, rel_sym_type, module, &symrec)) {
                    // Only set the rela bit if the symbol was actually resolved and not stubbed.
                    module->SetRelaBit(bit_idx);
                }
                symbol_virtual_addr = symrec.virtual_address;
                break;
            }
            default:
                ASSERT_MSG(0, "unknown bind type {}", sym_bind);
            }
            rel_is_resolved = (symbol_virtual_addr != 0);
            rel_value = (rel_is_resolved ? symbol_virtual_addr + addend : 0);
            rel_name = symrec.name;
            break;
        }
        default:
            LOG_INFO(Core_Linker, "UNK type {:#010x} rel symbol : {:#010x}", type, symbol);
        }

        if (rel_is_resolved) {
            VirtualMemory::memory_patch(rel_virtual_addr, rel_value);
        } else {
            LOG_INFO(Core_Linker, "function not patched! {}", rel_name);
        }
    });
}

const Module* Linker::FindExportedModule(const ModuleInfo& module, const LibraryInfo& library) {
    const auto it = std::ranges::find_if(m_modules, [&](const auto& m) {
        return std::ranges::contains(m->GetExportLibs(), library) &&
               std::ranges::contains(m->GetExportModules(), module);
    });
    return it == m_modules.end() ? nullptr : it->get();
}

bool Linker::Resolve(const std::string& name, Loader::SymbolType sym_type, Module* m,
                     Loader::SymbolRecord* return_info) {
    const auto ids = Common::SplitString(name, '#');
    if (ids.size() != 3) {
        return_info->virtual_address = 0;
        return_info->name = name;
        LOG_ERROR(Core_Linker, "Not Resolved {}", name);
        return false;
    }

    const LibraryInfo* library = m->FindLibrary(ids[1]);
    const ModuleInfo* module = m->FindModule(ids[2]);
    ASSERT_MSG(library && module, "Unable to find library and module");

    Loader::SymbolResolver sr{};
    sr.name = ids.at(0);
    sr.library = library->name;
    sr.library_version = library->version;
    sr.module = module->name;
    sr.module_version_major = module->version_major;
    sr.module_version_minor = module->version_minor;
    sr.type = sym_type;

    const auto* record = m_hle_symbols.FindSymbol(sr);
    if (!record) {
        // Check if it an export function
        const auto* p = FindExportedModule(*module, *library);
        if (p && p->export_sym.GetSize() > 0) {
            record = p->export_sym.FindSymbol(sr);
        }
    }
    if (record) {
        *return_info = *record;
        return true;
    }

    const auto aeronid = AeroLib::FindByNid(sr.name.c_str());
    if (aeronid) {
        return_info->name = aeronid->name;
        return_info->virtual_address = AeroLib::GetStub(aeronid->nid);
    } else {
        return_info->virtual_address = AeroLib::GetStub(sr.name.c_str());
        return_info->name = "Unknown !!!";
    }
    LOG_ERROR(Core_Linker, "Linker: Stub resolved {} as {} (lib: {}, mod: {})", sr.name,
              return_info->name, library->name, module->name);
    return false;
}

void* Linker::TlsGetAddr(u64 module_index, u64 offset) {
    std::scoped_lock lk{mutex};

    DtvEntry* dtv_table = GetTcbBase()->tcb_dtv;
    if (dtv_table[0].counter != dtv_generation_counter) {
        // Generation counter changed, a dynamic module was either loaded or unloaded.
        const u32 old_num_dtvs = dtv_table[1].counter;
        ASSERT_MSG(max_tls_index > old_num_dtvs, "Module unloading unsupported");
        // Module was loaded, increase DTV table size.
        DtvEntry* new_dtv_table = new DtvEntry[max_tls_index + 2];
        std::memcpy(new_dtv_table + 2, dtv_table + 2, old_num_dtvs * sizeof(DtvEntry));
        new_dtv_table[0].counter = dtv_generation_counter;
        new_dtv_table[1].counter = max_tls_index;
        delete[] dtv_table;

        // Update TCB pointer.
        GetTcbBase()->tcb_dtv = new_dtv_table;
        dtv_table = new_dtv_table;
    }

    u8* addr = dtv_table[module_index + 1].pointer;
    if (!addr) {
        // Module was just loaded by above code. Allocate TLS block for it.
        Module* module = m_modules[module_index - 1].get();
        const u32 init_image_size = module->tls.init_image_size;
        // TODO: Determine if Windows will crash from this
        u8* dest =
            reinterpret_cast<u8*>(ExecuteGuest(heap_api->heap_malloc, module->tls.image_size));
        const u8* src = reinterpret_cast<const u8*>(module->tls.image_virtual_addr);
        std::memcpy(dest, src, init_image_size);
        std::memset(dest + init_image_size, 0, module->tls.image_size - init_image_size);
        dtv_table[module_index + 1].pointer = dest;
        addr = dest;
    }
    return addr + offset;
}

thread_local std::once_flag init_tls_flag;

void Linker::EnsureThreadInitialized(bool is_primary) const {
    std::call_once(init_tls_flag, [this, is_primary] {
#ifdef ARCH_X86_64
        InitializeThreadPatchStack();
#endif
        InitTlsForThread(is_primary);
    });
}

void Linker::InitTlsForThread(bool is_primary) const {
    static constexpr size_t TcbSize = 0x40;
    static constexpr size_t TlsAllocAlign = 0x20;
    const size_t total_tls_size = Common::AlignUp(static_tls_size, TlsAllocAlign) + TcbSize;

    // If sceKernelMapNamedFlexibleMemory is being called from libkernel and addr = 0
    // it automatically places mappings in system reserved area instead of managed.
    static constexpr VAddr KernelAllocBase = 0x880000000ULL;

    // The kernel module has a few different paths for TLS allocation.
    // For SDK < 1.7 it allocates both main and secondary thread blocks using libc mspace/malloc.
    // In games compiled with newer SDK, the main thread gets mapped from flexible memory,
    // with addr = 0, so system managed area. Here we will only implement the latter.
    void* addr_out{reinterpret_cast<void*>(KernelAllocBase)};
    if (is_primary) {
        const size_t tls_aligned = Common::AlignUp(total_tls_size, 16_KB);
        const int ret = Libraries::Kernel::sceKernelMapNamedFlexibleMemory(
            &addr_out, tls_aligned, 3, 0, "SceKernelPrimaryTcbTls");
        ASSERT_MSG(ret == 0, "Unable to allocate TLS+TCB for the primary thread");
    } else {
        if (heap_api) {
#ifndef WIN32
            addr_out = ExecuteGuestWithoutTls(heap_api->heap_malloc, total_tls_size);
        } else {
            addr_out = std::malloc(total_tls_size);
#else
            // TODO: Windows tls malloc replacement, refer to rtld_tls_block_malloc
            LOG_ERROR(Core_Linker, "TLS user malloc called, using std::malloc");
            addr_out = std::malloc(total_tls_size);
            if (!addr_out) {
                auto pth_id = pthread_self();
                auto handle = pthread_gethandle(pth_id);
                ASSERT_MSG(addr_out,
                           "Cannot allocate TLS block defined for handle=%x, index=%d size=%d",
                           handle, pth_id, total_tls_size);
            }
#endif
        }
    }

    // Initialize allocated memory and allocate DTV table.
    const u32 num_dtvs = max_tls_index;
    std::memset(addr_out, 0, total_tls_size);
    DtvEntry* dtv_table = new DtvEntry[num_dtvs + 2];

    // Initialize thread control block
    u8* addr = reinterpret_cast<u8*>(addr_out);
    Tcb* tcb = reinterpret_cast<Tcb*>(addr + static_tls_size);
    tcb->tcb_self = tcb;
    tcb->tcb_dtv = dtv_table;

    // Dtv[0] is the generation counter. libkernel puts their number into dtv[1] (why?)
    dtv_table[0].counter = dtv_generation_counter;
    dtv_table[1].counter = num_dtvs;

    // Copy init images to TLS thread blocks and map them to DTV slots.
    for (u32 i = 0; i < num_static_modules; i++) {
        auto* module = m_modules[i].get();
        if (module->tls.image_size == 0) {
            continue;
        }
        u8* dest = reinterpret_cast<u8*>(addr + static_tls_size - module->tls.offset);
        const u8* src = reinterpret_cast<const u8*>(module->tls.image_virtual_addr);
        std::memcpy(dest, src, module->tls.init_image_size);
        tcb->tcb_dtv[module->tls.modid + 1].pointer = dest;
    }

    // Set pointer to FS base
    SetTcbBase(tcb);
}

void Linker::DebugDump() {
    const auto& log_dir = Common::FS::GetUserPath(Common::FS::PathType::LogDir);
    const std::filesystem::path debug(log_dir / "debugdump");
    std::filesystem::create_directory(debug);
    for (const auto& m : m_modules) {
        // TODO make a folder with game id for being more unique?
        const std::filesystem::path filepath(debug / m.get()->file.stem());
        std::filesystem::create_directory(filepath);
        m.get()->import_sym.DebugDump(filepath / "imports.txt");
        m.get()->export_sym.DebugDump(filepath / "exports.txt");
        if (m.get()->elf.IsSelfFile()) {
            m.get()->elf.SelfHeaderDebugDump(filepath / "selfHeader.txt");
            m.get()->elf.SelfSegHeaderDebugDump(filepath / "selfSegHeaders.txt");
        }
        m.get()->elf.ElfHeaderDebugDump(filepath / "elfHeader.txt");
        m.get()->elf.PHeaderDebugDump(filepath / "elfPHeaders.txt");
    }
}

} // namespace Core
