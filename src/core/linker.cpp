// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <windows.h>
#include "common/assert.h"
#include "common/config.h"
#include "common/logging/log.h"
#include "common/path_util.h"
#include "common/string_util.h"
#include "common/thread.h"
#include "core/aerolib/aerolib.h"
#include "core/aerolib/stubs.h"
#include "core/libraries/kernel/thread_management.h"
#include "core/linker.h"
#include "core/virtual_memory.h"

namespace Core {

using ExitFunc = PS4_SYSV_ABI void (*)();

static PS4_SYSV_ABI void ProgramExitFunc() {
    fmt::print("exit function called\n");
}

static void RunMainEntry(VAddr addr, EntryParams* params, ExitFunc exit_func) {
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

Linker::Linker() = default;

Linker::~Linker() = default;

void Linker::Execute() {
    if (Config::debugDump()) {
        DebugDump();
    }

    // Calculate static TLS size.
    for (const auto& module : m_modules) {
        if (module->tls.image_size != 0) {
            module->tls.modid = ++max_tls_index;
        }
        static_tls_size += module->tls.image_size;
        module->tls.offset = static_tls_size;
    }

    // Relocate all modules
    for (const auto& m : m_modules) {
        Relocate(m.get());
    }

    // Init primary thread.
    Common::SetCurrentThreadName("GAME_MainThread");
    Libraries::Kernel::pthreadInitSelfMainThread();

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
            RunMainEntry(m->GetEntryAddress(), &p, ProgramExitFunc);
        }
    }
}

s32 Linker::LoadModule(const std::filesystem::path& elf_name) {
    std::scoped_lock lk{mutex};

    if (!std::filesystem::exists(elf_name)) {
        LOG_ERROR(Core_Linker, "Provided file {} does not exist", elf_name.string());
        return -1;
    }

    auto module = std::make_unique<Module>(elf_name);
    if (!module->IsValid()) {
        LOG_ERROR(Core_Linker, "Provided file {} is not valid ELF file", elf_name.string());
        return -1;
    }

    m_modules.emplace_back(std::move(module));
    return m_modules.size() - 1;
}

void Linker::Relocate(Module* module) {
    module->ForEachRelocation([&](elf_relocation* rel, bool isJmpRel) {
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
            break;
        case R_X86_64_DTPMOD64:
            rel_value = static_cast<u64>(module->tls.modid);
            rel_is_resolved = true;
            rel_sym_type = Loader::SymbolType::Tls;
            break;
        case R_X86_64_GLOB_DAT:
        case R_X86_64_JUMP_SLOT:
            addend = 0;
        case R_X86_64_64: {
            auto sym = symbol_table[symbol];
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

            if (sym_visibility != 0) {
                LOG_INFO(Core_Linker, "symbol visilibity !=0");
            }

            switch (sym_bind) {
            case STB_LOCAL:
                symbol_vitrual_addr = rel_base_virtual_addr + sym.st_value;
                break;
            case STB_GLOBAL:
            case STB_WEAK: {
                rel_name = namesTlb + sym.st_name;
                Resolve(rel_name, rel_sym_type, module, &symrec);
                symbol_vitrual_addr = symrec.virtual_address;
                break;
            }
            default:
                ASSERT_MSG(0, "unknown bind type {}", sym_bind);
            }
            rel_is_resolved = (symbol_vitrual_addr != 0);
            rel_value = (rel_is_resolved ? symbol_vitrual_addr + addend : 0);
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

void Linker::Resolve(const std::string& name, Loader::SymbolType sym_type, Module* m,
                     Loader::SymbolRecord* return_info) {
    const auto ids = Common::SplitString(name, '#');
    if (ids.size() != 3) {
        return_info->virtual_address = 0;
        return_info->name = name;
        LOG_ERROR(Core_Linker, "Not Resolved {}", name);
        return;
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
        return;
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
    }
}

} // namespace Core
