// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include <vector>
#include "common/types.h"
#include "core/loader/elf.h"
#include "core/loader/symbols_resolver.h"

namespace Core {

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

struct ThreadLocalImage {
    u64 align;
    u64 image_size;
    u64 offset;
    u32 modid;
    VAddr image_virtual_addr;
    u64 init_image_size;
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

    std::string filename;
};

using ModuleFunc = int (*)(size_t, const void*);

class Module {
public:
    explicit Module(const std::filesystem::path& file);
    ~Module();

    VAddr GetBaseAddress() const noexcept {
        return base_virtual_addr;
    }

    VAddr GetEntryAddress() const noexcept {
        return base_virtual_addr + elf.GetElfEntry();
    }

    bool IsValid() const noexcept {
        return base_virtual_addr != 0;
    }

    bool IsSharedLib() const noexcept {
        return elf.IsSharedLib();
    }

    void* FindByName(std::string_view name) {
        const auto symbols = export_sym.GetSymbols();
        const auto it = std::ranges::find(symbols, name, &Loader::SymbolRecord::nid_name);
        if (it != symbols.end()) {
            return reinterpret_cast<void*>(it->virtual_address);
        }
        return nullptr;
    }

    template <typename T = VAddr>
    T GetProcParam() const noexcept {
        return reinterpret_cast<T>(proc_param_virtual_addr);
    }

    std::span<const ModuleInfo> GetImportModules() const {
        return dynamic_info.import_modules;
    }

    std::span<const ModuleInfo> GetExportModules() const {
        return dynamic_info.export_modules;
    }

    std::span<const LibraryInfo> GetImportLibs() const {
        return dynamic_info.import_libs;
    }

    std::span<const LibraryInfo> GetExportLibs() const {
        return dynamic_info.export_libs;
    }

    void ForEachRelocation(auto&& func) {
        for (u32 i = 0; i < dynamic_info.relocation_table_size / sizeof(elf_relocation); i++) {
            func(&dynamic_info.relocation_table[i], false);
        }
        for (u32 i = 0; i < dynamic_info.jmp_relocation_table_size / sizeof(elf_relocation); i++) {
            func(&dynamic_info.jmp_relocation_table[i], true);
        }
    }

    void Start(size_t args, const void* argp, void* param);
    void LoadModuleToMemory();
    void LoadDynamicInfo();
    void LoadSymbols();

    const ModuleInfo* FindModule(std::string_view id);
    const LibraryInfo* FindLibrary(std::string_view id);

public:
    std::filesystem::path file;
    Loader::Elf elf;
    u64 aligned_base_size{};
    VAddr base_virtual_addr{};
    VAddr proc_param_virtual_addr{};
    DynamicModuleInfo dynamic_info{};
    std::vector<u8> m_dynamic;
    std::vector<u8> m_dynamic_data;
    Loader::SymbolsResolver export_sym;
    Loader::SymbolsResolver import_sym;
    ThreadLocalImage tls{};
};

} // namespace Core
