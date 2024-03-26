// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <mutex>
#include <vector>
#include "core/loader/elf.h"
#include "core/loader/symbols_resolver.h"

namespace Core {
using module_func_t = int (*)(size_t args, const void* argp);
struct DynamicModuleInfo;
class Linker;

struct EntryParams {
    int argc;
    u32 padding;
    const char* argv[3];
};

struct ModuleInfo {
    bool operator==(const ModuleInfo& other) const {
        return version_major == other.version_major && version_minor == other.version_minor &&
               name == other.name;
    }
    std::string name;
    union {
        u64 value;
        struct {
            u32 name_offset;
            u8 version_minor;
            u8 version_major;
            u16 id;
        };
    };
    std::string enc_id;
};

struct LibraryInfo {
    bool operator==(const LibraryInfo& other) const {
        return version == other.version && name == other.name;
    }
    std::string name;
    union {
        u64 value;
        struct {
            u32 name_offset;
            u16 version;
            u16 id;
        };
    };
    std::string enc_id;
};

struct PS4ThreadLocal {
    u64 image_virtual_addr = 0;
    u64 image_size = 0;
    u64 handler_virtual_addr = 0;
};

struct DynamicModuleInfo {
    void* hash_table = nullptr;
    u64 hash_table_size = 0;

    char* str_table = nullptr;
    u64 str_table_size = 0;

    elf_symbol* symbol_table = nullptr;
    u64 symbol_table_total_size = 0;
    u64 symbol_table_entries_size = 0;

    u64 init_virtual_addr = 0;
    u64 fini_virtual_addr = 0;
    u64 pltgot_virtual_addr = 0;
    u64 init_array_virtual_addr = 0;
    u64 fini_array_virtual_addr = 0;
    u64 preinit_array_virtual_addr = 0;
    u64 init_array_size = 0;
    u64 fini_array_size = 0;
    u64 preinit_array_size = 0;

    elf_relocation* jmp_relocation_table = nullptr;
    u64 jmp_relocation_table_size = 0;
    s64 jmp_relocation_type = 0;

    elf_relocation* relocation_table = nullptr;
    u64 relocation_table_size = 0;
    u64 relocation_table_entries_size = 0;

    u64 debug = 0;
    u64 textrel = 0;
    u64 flags = 0;

    std::vector<const char*> needed;
    std::vector<ModuleInfo> import_modules;
    std::vector<ModuleInfo> export_modules;
    std::vector<LibraryInfo> import_libs;
    std::vector<LibraryInfo> export_libs;

    std::string filename; // Filename with absolute path
};

// This struct keeps neccesary info about loaded modules. Main executeable is included too as well
struct Module {
    Loader::Elf elf;
    u64 aligned_base_size = 0;
    u64 base_virtual_addr = 0;
    u64 proc_param_virtual_addr = 0;

    std::string file_name;

    std::vector<u8> m_dynamic;
    std::vector<u8> m_dynamic_data;
    DynamicModuleInfo dynamic_info{};

    Loader::SymbolsResolver export_sym;
    Loader::SymbolsResolver import_sym;

    PS4ThreadLocal tls;
};

class Linker {
public:
    Linker();
    virtual ~Linker();

    Module* LoadModule(const std::filesystem::path& elf_name);
    Module* FindModule(u32 id = 0);
    void LoadModuleToMemory(Module* m);
    void LoadDynamicInfo(Module* m);
    void LoadSymbols(Module* m);
    Loader::SymbolsResolver& getHLESymbols() {
        return m_hle_symbols;
    }
    void Relocate(Module* m);
    void Resolve(const std::string& name, Loader::SymbolType Symtype, Module* m,
                 Loader::SymbolRecord* return_info);
    void Execute();
    void DebugDump();
    u64 GetProcParam();

private:
    const ModuleInfo* FindModule(const Module& m, const std::string& id);
    const LibraryInfo* FindLibrary(const Module& program, const std::string& id);
    Module* FindExportedModule(const ModuleInfo& m, const LibraryInfo& l);
    int StartModule(Module* m, size_t args, const void* argp, module_func_t func);
    void StartAllModules();

    std::vector<std::unique_ptr<Module>> m_modules;
    Loader::SymbolsResolver m_hle_symbols{};
    std::mutex m_mutex;
};

} // namespace Core
