// SPDX-FileCopyrightText: Copyright 2025-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/alignment.h"
#include "common/arch.h"
#include "common/assert.h"
#include "common/elf_info.h"
#include "common/logging/log.h"
#include "common/path_util.h"
#include "common/string_util.h"
#include "common/thread.h"
#include "core/aerolib/aerolib.h"
#include "core/aerolib/stubs.h"
#include "core/devtools/widget/module_list.h"
#include "core/emulator_settings.h"
#include "core/libraries/kernel/kernel.h"
#include "core/libraries/kernel/memory.h"
#include "core/libraries/kernel/threads.h"
#include "core/libraries/sysmodule/sysmodule.h"
#include "core/linker.h"
#include "core/memory.h"
#include "core/tls.h"
#include "ipc/ipc.h"

#ifndef _WIN32
#include <signal.h>
#endif
#include <atomic>
#include <set>
#include <cstdio>

namespace Core {

static std::atomic<RuntimePlatform> g_runtime_platform{RuntimePlatform::PS4};

void SetGlobalRuntimePlatform(RuntimePlatform platform) noexcept {
    g_runtime_platform.store(platform, std::memory_order_relaxed);
}

RuntimePlatform GetGlobalRuntimePlatform() noexcept {
    return g_runtime_platform.load(std::memory_order_relaxed);
}

bool IsGlobalPs5RuntimeMode() noexcept {
    return GetGlobalRuntimePlatform() == RuntimePlatform::PS5;
}

static PS4_SYSV_ABI void ProgramExitFunc() {
    LOG_ERROR(Core_Linker, "Exit function called");
}

static bool IsLibcLibraryName(std::string_view name) {
    return name == "libc" || name == "libSceLibcInternal" || name == "libSceLibcInternalExt";
}

static bool IsLibcModuleFileName(std::string_view name) {
    if (name.ends_with(".sprx")) {
        name.remove_suffix(5);
    } else if (name.ends_with(".prx")) {
        name.remove_suffix(4);
    }
    return IsLibcLibraryName(name);
}

static bool IsLibcSymbolRecord(const Loader::SymbolRecord& record) {
    const auto ids = Common::SplitString(record.name, '#');
    return ids.size() >= 2 && IsLibcLibraryName(ids[1]);
}

static bool IsRuntimeSymbolNameMatch(std::string_view symbol, std::string_view candidate) {
    if (symbol == candidate) {
        return true;
    }
    if (!symbol.empty() && symbol.front() == '_' && symbol.substr(1) == candidate) {
        return true;
    }
    return !candidate.empty() && candidate.front() == '_' && candidate.substr(1) == symbol;
}

static Loader::SymbolType LoaderSymbolTypeFromElf(u8 type) {
    switch (type) {
    case STT_FUN:
        return Loader::SymbolType::Function;
    case STT_OBJECT:
        return Loader::SymbolType::Object;
    case STT_NOTYPE:
        return Loader::SymbolType::NoType;
    default:
        return Loader::SymbolType::Unknown;
    }
}

static bool IsRuntimeSymbolTypeCompatible(u8 type, Loader::SymbolType preferred_type) {
    if (preferred_type == Loader::SymbolType::Unknown) {
        return type == STT_FUN || type == STT_OBJECT || type == STT_NOTYPE;
    }
    return LoaderSymbolTypeFromElf(type) == preferred_type ||
           (preferred_type == Loader::SymbolType::Function && type == STT_NOTYPE);
}

#ifdef ARCH_X86_64
static PS4_SYSV_ABI void* RunMainEntry [[noreturn]] (EntryParams* params) {
    // Start shared library modules
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
                 : "r"(params->entry_addr), "r"(params), "r"(ProgramExitFunc)
                 : "rax", "rsi", "rdi");
    UNREACHABLE();
}

static PS4_SYSV_ABI void* RunMainEntryPs5 [[noreturn]] (Ps5EntryParams* params) {
    // PS5 entry frame process bootstrap:
    // [0x00]=argc(u32), [0x04]=padding(u32), [0x08]=argv0(char*), [0x10]=0, [0x18]=0.
    // Guest receives rdi=&entry_params and rsi=exit handler.
    asm volatile("andq $-16, %%rsp\n"
                 "subq $8, %%rsp\n"
                 "pushq 8(%1)\n" // argv0
                 "pushq 0(%1)\n" // argc|padding
                 "movq %1, %%rdi\n"
                 "movq %2, %%rsi\n"
                 "jmp *%0\n"
                 :
                 : "r"(params->entry_addr), "r"(params), "r"(ProgramExitFunc)
                 : "rax", "rsi", "rdi");
    UNREACHABLE();
}
#endif

Linker::Linker() : memory{Memory::Instance()} {}

Linker::~Linker() = default;

void Linker::SetRuntimePlatform(RuntimePlatform platform) noexcept {
    runtime_platform = platform;
    SetGlobalRuntimePlatform(platform);
}

void Linker::Execute(const std::vector<std::string>& args) {
    if (EmulatorSettings.IsDebugDump()) {
        DebugDump();
    }

    if (IsPs5RuntimeMode()) {
        PreloadPs5AdjacentModules();
    }

    // Calculate static TLS size.
    Module* module = m_modules[0].get();
    static_tls_size = module->tls.offset = module->tls.image_size;

    // Relocate all modules
    for (const auto& m : m_modules) {
        Relocate(m.get());
    }

    // Configure the direct and flexible memory regions.
    u64 fmem_size = ORBIS_KERNEL_FLEXIBLE_MEMORY_SIZE;
    bool use_extended_mem1 = true, use_extended_mem2 = true;

    if (!IsPs5RuntimeMode()) {
        const auto* proc_param = GetProcParam();
        ASSERT(proc_param);

        Core::OrbisKernelMemParam mem_param{};
        if (proc_param->size >= offsetof(OrbisProcParam, mem_param) + sizeof(OrbisKernelMemParam*)) {
            if (proc_param->mem_param) {
                mem_param = *proc_param->mem_param;
                if (mem_param.size >=
                    offsetof(OrbisKernelMemParam, flexible_memory_size) + sizeof(u64*)) {
                    if (const auto* flexible_size = mem_param.flexible_memory_size) {
                        fmem_size = *flexible_size + ORBIS_KERNEL_FLEXIBLE_MEMORY_BASE;
                    }
                }
            }
        }

        if (mem_param.size < offsetof(OrbisKernelMemParam, extended_memory_1) + sizeof(u64*)) {
            mem_param.extended_memory_1 = nullptr;
        }
        if (mem_param.size < offsetof(OrbisKernelMemParam, extended_memory_2) + sizeof(u64*)) {
            mem_param.extended_memory_2 = nullptr;
        }

        const u64 sdk_ver = proc_param->sdk_version;
        if (sdk_ver < Common::ElfInfo::FW_50) {
            use_extended_mem1 = mem_param.extended_memory_1 ? *mem_param.extended_memory_1 : false;
            use_extended_mem2 = mem_param.extended_memory_2 ? *mem_param.extended_memory_2 : false;
        }
    } else {
        LOG_INFO(Core_Linker, "PS5 mode enabled: using default memory region layout");
    }

    memory->SetupMemoryRegions(fmem_size, use_extended_mem1, use_extended_mem2);

    main_thread.Run([this, module, &args](std::stop_token) {
        Common::SetCurrentThreadName("Game:Main");
#ifndef _WIN32 // Clear any existing signal mask for game threads.
        sigset_t emptyset;
        sigemptyset(&emptyset);
        pthread_sigmask(SIG_SETMASK, &emptyset, nullptr);
#endif
        if (auto& ipc = IPC::Instance()) {
            ipc.WaitForStart();
        }

        if (!IsPs5RuntimeMode()) {
            // Have libSceSysmodule preload our libraries.
            Libraries::SysModule::sceSysmodulePreloadModuleForLibkernel();

            // Simulate libSceGnmDriver initialization, which maps a chunk of direct memory.
            // Some games fail without accurately emulating this behavior.
            s64 phys_addr{};
            s32 result = Libraries::Kernel::sceKernelAllocateDirectMemory(
                0, Libraries::Kernel::sceKernelGetDirectMemorySize(), 0x10000, 0x10000, 3,
                &phys_addr);
            if (result == 0) {
                void* addr{reinterpret_cast<void*>(0xfe0000000)};
                result = Libraries::Kernel::sceKernelMapNamedDirectMemory(
                    &addr, 0x10000, 0x13, 0, phys_addr, 0x10000, "SceGnmDriver");
            }
            ASSERT_MSG(result == 0, "Unable to emulate libSceGnmDriver initialization");
        } else {
            LOG_INFO(Core_Linker,
                     "PS5 mode enabled: skipping PS4 sysmodule preload and SceGnmDriver init");
            StartPs5PreloadedModules();
        }

        // Start main module.
        EntryParams& params = Libraries::Kernel::entry_params;
        params.argc = 1;
        std::fill(std::begin(params.argv_storage), std::end(params.argv_storage), nullptr);
        params.argv_storage[0] = "eboot.bin";
        params.argv = params.argv_storage;
        if (!args.empty()) {
            constexpr int MaxArgs =
                static_cast<int>(sizeof(params.argv_storage) / sizeof(params.argv_storage[0]));
            params.argc = std::min<int>(args.size(), MaxArgs);
            for (int i = 0; i < params.argc; i++) {
                params.argv_storage[i] = args[i].c_str();
            }
        }
        params.entry_addr = module->GetEntryAddress();
        if (IsPs5RuntimeMode()) {
            Ps5EntryParams ps5_params{};
            ps5_params.argc = 1;
            ps5_params.padding = 0;
            ps5_params.argv0 = "eboot.bin";
            ps5_params.reserved0 = 0;
            ps5_params.reserved1 = 0;
            ps5_params.entry_addr = params.entry_addr;
            LOG_INFO(Core_Linker,
                     "PS5 mode: entering guest");
            RunMainEntryPs5(&ps5_params);
            return;
        } else {
            RunMainEntry(&params);
        }
    });
}

s32 Linker::LoadModule(const std::filesystem::path& elf_name, bool is_dynamic) {
    std::scoped_lock lk{mutex};

    if (!std::filesystem::exists(elf_name)) {
        LOG_ERROR(Core_Linker, "Provided file {} does not exist", elf_name.string());
        return -1;
    }

    auto module = std::make_unique<Module>(memory, elf_name, max_tls_index, IsPs5RuntimeMode());
    if (!module->IsValid()) {
        LOG_ERROR(Core_Linker, "Provided file {} is not valid ELF file", elf_name.string());
        return -1;
    }

    num_static_modules += !is_dynamic;
    m_modules.emplace_back(std::move(module));

    Core::Devtools::Widget::ModuleList::AddModule(elf_name.filename().string(), elf_name);

    return m_modules.size() - 1;
}

s32 Linker::LoadAndStartModule(const std::filesystem::path& path, u64 args, const void* argp,
                               int* pRes) {
    u32 handle = FindByName(path);
    if (handle != -1) {
        return handle;
    }
    handle = LoadModule(path, true);
    if (handle == -1) {
        return -1;
    }
    auto* module = GetModule(handle);
    RelocateAnyImports(module);

    // If the new module has a TLS image, trigger its load when TlsGetAddr is called.
    if (module->tls.image_size != 0) {
        AdvanceGenerationCounter();
    }

    // Retrieve and verify proc param according to libkernel.
    auto* param = module->GetProcParam<OrbisProcParam*>();
    ASSERT_MSG(!param || param->size >= 0x18, "Invalid module param size: {}", param->size);
    s32 ret = module->Start(args, argp, param);
    if (pRes) {
        *pRes = ret;
    }

    return handle;
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
    module->ForEachRelocation([&](elf_relocation* rel, u32 i, bool is_jmp_rel) {
        const u32 num_relocs = module->dynamic_info.relocation_table_size / sizeof(elf_relocation);
        const u32 bit_idx = (is_jmp_rel ? num_relocs : 0) + i;
        if (module->TestRelaBit(bit_idx)) {
            return;
        }
        auto type = rel->GetType();
        auto symbol = rel->GetSymbol();
        auto addend = rel->rel_addend;
        auto* symbol_table = module->dynamic_info.symbol_table;
        auto* names_tlb = module->dynamic_info.str_table;

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
                rel_name = names_tlb + sym.st_name;
                if (Resolve(rel_name, rel_sym_type, module, &symrec)) {
                    // Only set the rela bit if the symbol was actually resolved and not stubbed.
                    module->SetRelaBit(bit_idx);
                }
                symbol_virtual_addr = symrec.virtual_address;
                break;
            }
            default:
                UNREACHABLE_MSG("Unknown bind type {}", sym_bind);
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
            std::memcpy(reinterpret_cast<void*>(rel_virtual_addr), &rel_value, sizeof(rel_value));
        } else {
            LOG_INFO(Core_Linker, "Function not patched! {}", rel_name);
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
    const auto resolve_from_loaded_exports_by_nid =
        [this, sym_type](std::string_view nid) -> const Loader::SymbolRecord* {
        for (const auto& loaded_module : m_modules) {
            if (!loaded_module || loaded_module->export_sym.GetSize() == 0) {
                continue;
            }
            if (const auto* record = loaded_module->export_sym.FindSymbolByNid(nid, sym_type)) {
                return record;
            }
        }
        return nullptr;
    };

    const auto resolve_from_loaded_runtime_symbol =
        [this, sym_type](std::string_view nid, Loader::SymbolRecord* out,
                         bool libc_modules_only) -> bool {
        const auto* aeronid = AeroLib::FindByNid(std::string{nid}.c_str());
        const std::string_view export_name = aeronid ? std::string_view{aeronid->name} : nid;

        for (const auto& loaded_module : m_modules) {
            if (!loaded_module || !loaded_module->dynamic_info.symbol_table ||
                !loaded_module->dynamic_info.str_table ||
                loaded_module->dynamic_info.symbol_table_total_size == 0) {
                continue;
            }
            if (libc_modules_only && !IsLibcModuleFileName(loaded_module->name)) {
                continue;
            }

            for (auto* sym = loaded_module->dynamic_info.symbol_table;
                 reinterpret_cast<u8*>(sym) <
                 reinterpret_cast<u8*>(loaded_module->dynamic_info.symbol_table) +
                     loaded_module->dynamic_info.symbol_table_total_size;
                 sym++) {
                const u8 bind = sym->GetBind();
                const u8 type = sym->GetType();
                if ((bind != STB_GLOBAL && bind != STB_WEAK) || sym->st_value == 0 ||
                    sym->st_name == 0 || !IsRuntimeSymbolTypeCompatible(type, sym_type)) {
                    continue;
                }

                const std::string_view symbol_name{
                    loaded_module->dynamic_info.str_table + sym->st_name};
                const size_t nid_sep = symbol_name.find('#');
                const std::string_view symbol_nid =
                    nid_sep == std::string_view::npos ? symbol_name : symbol_name.substr(0, nid_sep);
                if (symbol_nid != nid && !IsRuntimeSymbolNameMatch(symbol_name, export_name)) {
                    continue;
                }

                const VAddr address = sym->st_value >= loaded_module->GetBaseAddress()
                                          ? sym->st_value
                                          : sym->st_value + loaded_module->GetBaseAddress();
                if (address < 0x10000) {
                    continue;
                }

                const auto loader_type = LoaderSymbolTypeFromElf(type);
                out->name = std::string(symbol_name) + "#runtime#0#" + loaded_module->name + "#" +
                            std::string(Loader::SymbolsResolver::SymbolTypeToS(loader_type));
                out->nid_name = std::string(export_name);
                out->virtual_address = address;
                return true;
            }
        }
        return false;
    };

    const auto ids = Common::SplitString(name, '#');
    if (ids.size() != 3) {
        return_info->virtual_address = 0;
        return_info->name = name;
        LOG_ERROR(Core_Linker, "Not Resolved {}", name);
        return false;
    }

    const LibraryInfo* library = m->FindLibrary(ids[1]);
    const ModuleInfo* module = m->FindModule(ids[2]);
    if ((!library || !module) && IsPs5RuntimeMode()) {
        const auto* hle_record = m_hle_symbols.FindSymbolByNid(ids.at(0), sym_type);
        {
            if (const auto* export_record = resolve_from_loaded_exports_by_nid(ids.at(0));
                export_record &&
                (IsLibcSymbolRecord(*export_record) ||
                 (hle_record && IsLibcSymbolRecord(*hle_record)))) {
                *return_info = *export_record;
                LOG_INFO(Core_Linker,
                         "PS5 mode: resolved libc by module-export NID fallback {} -> {} "
                         "(missing lib/module metadata: lib_id='{}', mod_id='{}')",
                         ids.at(0), return_info->name, ids.at(1), ids.at(2));
                return true;
            }
            if (hle_record && IsLibcSymbolRecord(*hle_record) &&
                resolve_from_loaded_runtime_symbol(ids.at(0), return_info, true)) {
                LOG_INFO(Core_Linker,
                         "PS5 mode: resolved libc by runtime symbol {} -> {} "
                         "(missing lib/module metadata: lib_id='{}', mod_id='{}')",
                         ids.at(0), return_info->name, ids.at(1), ids.at(2));
                return true;
            }
            if (!hle_record &&
                resolve_from_loaded_runtime_symbol(ids.at(0), return_info, true)) {
                LOG_INFO(Core_Linker,
                         "PS5 mode: resolved libc by runtime symbol {} -> {} "
                         "(missing lib/module metadata: lib_id='{}', mod_id='{}')",
                         ids.at(0), return_info->name, ids.at(1), ids.at(2));
                return true;
            }
        }
        if (hle_record) {
            *return_info = *hle_record;
            LOG_INFO(Core_Linker,
                     "PS5 mode: resolved by NID-only fallback {} -> {} "
                     "(missing lib/module metadata: lib_id='{}', mod_id='{}')",
                     ids.at(0), return_info->name, ids.at(1), ids.at(2));
            return true;
        }
        if (const auto* export_record = resolve_from_loaded_exports_by_nid(ids.at(0))) {
            *return_info = *export_record;
            LOG_INFO(Core_Linker,
                     "PS5 mode: resolved by module-export NID fallback {} -> {} "
                     "(missing lib/module metadata: lib_id='{}', mod_id='{}')",
                     ids.at(0), return_info->name, ids.at(1), ids.at(2));
            return true;
        }
        return_info->virtual_address = AeroLib::GetStub(ids.at(0).c_str());
        return_info->name = "Unknown !!!";
        return false;
    }
    ASSERT_MSG(library && module, "Unable to find library and module");

    Loader::SymbolResolver sr{};
    sr.name = ids.at(0);
    sr.library = library->name;
    sr.library_version = library->version;
    sr.module = module->name;
    sr.type = sym_type;

    if (IsPs5RuntimeMode() && IsLibcLibraryName(sr.library)) {
        if (const auto* p = FindExportedModule(*module, *library); p && p->export_sym.GetSize() > 0) {
            if (const auto* record = p->export_sym.FindSymbol(sr)) {
                *return_info = *record;
                LOG_INFO(Core_Linker, "PS5 mode: resolved libc from loaded module {} -> {}",
                         sr.name, return_info->name);
                return true;
            }
        }
        if (const auto* export_record = resolve_from_loaded_exports_by_nid(sr.name);
            export_record && IsLibcSymbolRecord(*export_record)) {
            *return_info = *export_record;
            LOG_INFO(Core_Linker,
                     "PS5 mode: resolved libc by module-export NID fallback {} -> {} "
                     "(requested lib={}, mod={})",
                     sr.name, return_info->name, sr.library, sr.module);
            return true;
        }
        if (resolve_from_loaded_runtime_symbol(sr.name, return_info, true)) {
            LOG_INFO(Core_Linker,
                     "PS5 mode: resolved libc by runtime symbol {} -> {} "
                     "(requested lib={}, mod={})",
                     sr.name, return_info->name, sr.library, sr.module);
            return true;
        }
    }

    const auto* record = m_hle_symbols.FindSymbol(sr);
    if (record) {
        *return_info = *record;
        Core::Devtools::Widget::ModuleList::AddModule(sr.library);
        return true;
    }

    if (IsPs5RuntimeMode()) {
        if (const auto* nid_record = m_hle_symbols.FindSymbolByNid(sr.name, sym_type)) {
            *return_info = *nid_record;
            LOG_INFO(Core_Linker,
                     "PS5 mode: resolved by NID-only fallback {} -> {} "
                     "(requested lib={}, mod={})",
                     sr.name, return_info->name, sr.library, sr.module);
            return true;
        }
        if (const auto* export_record = resolve_from_loaded_exports_by_nid(sr.name)) {
            *return_info = *export_record;
            LOG_INFO(Core_Linker,
                     "PS5 mode: resolved by module-export NID fallback {} -> {} "
                     "(requested lib={}, mod={})",
                     sr.name, return_info->name, sr.library, sr.module);
            return true;
        }
    }

    if (IsLibcLibraryName(sr.library) &&
        resolve_from_loaded_runtime_symbol(sr.name, return_info, true)) {
        LOG_INFO(Core_Linker,
                 "Resolved libc by loaded runtime symbol {} -> {} (requested lib={}, mod={})",
                 sr.name, return_info->name, sr.library, sr.module);
        return true;
    }

    // Check if it an export function
    const auto* p = FindExportedModule(*module, *library);
    if (p && p->export_sym.GetSize() > 0) {
        record = p->export_sym.FindSymbol(sr);
        if (record) {
            *return_info = *record;
            return true;
        }
    }

    const auto aeronid = AeroLib::FindByNid(sr.name.c_str());
    if (aeronid) {
        return_info->name = aeronid->name;
        return_info->virtual_address = AeroLib::GetStub(aeronid->nid);
    } else {
        return_info->virtual_address = AeroLib::GetStub(sr.name.c_str());
        return_info->name = "Unknown !!!";
    }
    if (library->name != "libc" && library->name != "libSceFios2") {
        LOG_WARNING(Core_Linker, "Linker: Stub resolved {} as {} (lib: {}, mod: {})", sr.name,
                    return_info->name, library->name, module->name);
    }
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
        DtvEntry* new_dtv_table = new DtvEntry[max_tls_index + 2]{};
        std::memcpy(new_dtv_table + 2, dtv_table + 2, old_num_dtvs * sizeof(DtvEntry));
        new_dtv_table[0].counter = dtv_generation_counter;
        new_dtv_table[1].counter = max_tls_index;
        delete[] dtv_table;

        // Update TCB pointer.
        GetTcbBase()->tcb_dtv = new_dtv_table;
        dtv_table = new_dtv_table;
    }

    u8* addr = dtv_table[module_index + 1].pointer;
    Module* module = m_modules[module_index - 1].get();
    if (!addr) {
        // Module was just loaded by above code. Allocate TLS block for it.
        const u32 init_image_size = module->tls.init_image_size;
        u8* dest = reinterpret_cast<u8*>(heap_api->heap_malloc(module->tls.image_size));
        const u8* src = reinterpret_cast<const u8*>(module->tls.image_virtual_addr);
        std::memcpy(dest, src, init_image_size);
        std::memset(dest + init_image_size, 0, module->tls.image_size - init_image_size);
        dtv_table[module_index + 1].pointer = dest;
        addr = dest;
    }
    return addr + offset;
}

void* Linker::AllocateTlsForThread(bool is_primary) {
    static constexpr size_t TcbSize = 0x40;
    static constexpr size_t TlsAllocAlign = 0x20;
    const size_t total_tls_size = Common::AlignUp(static_tls_size, TlsAllocAlign) + TcbSize;

    // If sceKernelMapNamedFlexibleMemory is being called from libkernel and addr = 0
    // it automatically places mappings in system reserved area instead of managed.
    // Since the system reserved area already has a mapping in it, this address is slightly higher.
    static constexpr VAddr KernelAllocBase = 0x881000000ULL;

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
            addr_out = heap_api->heap_malloc(total_tls_size);
        } else {
            addr_out = std::malloc(total_tls_size);
        }
    }
    return addr_out;
}

void Linker::FreeTlsForNonPrimaryThread(void* pointer) {
    if (heap_api) {
        heap_api->heap_free(pointer);
    } else {
        std::free(pointer);
    }
}

void Linker::DebugDump() {
    const auto& log_dir = Common::FS::GetUserPath(Common::FS::PathType::LogDir);
    const std::filesystem::path debug(log_dir / "debugdump");
    std::filesystem::create_directory(debug);
    for (const auto& m : m_modules) {
        Module* module = m.get();
        auto& elf = module->elf;
        const std::filesystem::path filepath(debug / module->file.stem());
        std::filesystem::create_directory(filepath);
        module->import_sym.DebugDump(filepath / "imports.txt");
        module->export_sym.DebugDump(filepath / "exports.txt");
        if (elf.IsSelfFile()) {
            elf.SelfHeaderDebugDump(filepath / "selfHeader.txt");
            elf.SelfSegHeaderDebugDump(filepath / "selfSegHeaders.txt");
        }
        elf.ElfHeaderDebugDump(filepath / "elfHeader.txt");
        elf.PHeaderDebugDump(filepath / "elfPHeaders.txt");
    }
}

void Linker::PreloadPs5AdjacentModules() {
    if (m_modules.empty()) {
        return;
    }

    const std::filesystem::path game_root = m_modules[0]->file.parent_path();
    if (game_root.empty()) {
        return;
    }

    static constexpr std::array<const char*, 2> CandidateDirs = {"sce_module", "sce_modules"};
    static constexpr std::array<const char*, 2> SkipModules = {"libkernel.prx", "libkernel_sys.prx"};

    std::vector<std::filesystem::path> preload_paths{};
    for (const char* dir_name : CandidateDirs) {
        const auto dir_path = game_root / dir_name;
        if (!std::filesystem::exists(dir_path) || !std::filesystem::is_directory(dir_path)) {
            continue;
        }
        for (const auto& entry : std::filesystem::directory_iterator(dir_path)) {
            if (!entry.is_regular_file()) {
                continue;
            }
            const auto ext = Common::ToLower(entry.path().extension().string());
            if (ext != ".prx" && ext != ".sprx") {
                continue;
            }
            const auto file_name = Common::ToLower(entry.path().filename().string());
            if (std::ranges::contains(SkipModules, file_name)) {
                continue;
            }
            if (!std::ranges::contains(preload_paths, entry.path())) {
                preload_paths.emplace_back(entry.path());
            }
        }
    }
    std::ranges::sort(preload_paths, [](const auto& lhs, const auto& rhs) {
        return Common::ToLower(lhs.filename().string()) < Common::ToLower(rhs.filename().string());
    });

    if (preload_paths.empty()) {
        LOG_INFO(Core_Linker, "PS5 mode: no adjacent sce_module PRX/SPRX to preload");
        return;
    }

    u32 loaded_count = 0;
    for (const auto& module_path : preload_paths) {
        if (FindByName(module_path) != static_cast<u32>(-1)) {
            continue;
        }

        const s32 handle = LoadModule(module_path, true);
        if (handle < 0) {
            LOG_WARNING(Core_Linker, "PS5 mode: failed to preload {}", module_path.string());
            continue;
        }

        Module* module = GetModule(handle);
        if (!module) {
            continue;
        }

        if (module->tls.image_size != 0) {
            AdvanceGenerationCounter();
        }
        loaded_count++;
    }

    LOG_INFO(Core_Linker,
             "PS5 mode: adjacent module preload summary loaded={}", loaded_count);
}

void Linker::StartPs5PreloadedModules() {
    u32 started_count = 0;
    u32 start_failures = 0;
    for (size_t i = 1; i < m_modules.size(); i++) {
        Module* module = m_modules[i].get();
        if (!module || module->dynamic_info.init_virtual_addr == 0) {
            continue;
        }

        auto* param = module->GetProcParam<OrbisProcParam*>();
        ASSERT_MSG(!param || param->size >= 0x18, "Invalid module param size: {}", param->size);
        const s32 start_result = module->Start(0, nullptr, param);
        if (start_result != ORBIS_OK) {
            start_failures++;
            LOG_WARNING(Core_Linker, "PS5 mode: module {} start returned {}", module->name,
                        start_result);
        }
        started_count++;
    }

    LOG_INFO(Core_Linker,
             "PS5 mode: preloaded module start summary started={}, start_failures={}",
             started_count, start_failures);
}

} // namespace Core
