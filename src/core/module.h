// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include <vector>
#include "common/config.h"
#include "common/types.h"
#include "core/loader/elf.h"
#include "core/loader/symbols_resolver.h"

namespace Core {

static constexpr size_t SCE_DBG_MAX_NAME_LENGTH = 256;
static constexpr size_t SCE_DBG_MAX_SEGMENTS = 4;
static constexpr size_t SCE_DBG_NUM_FINGERPRINT = 20;

struct OrbisKernelModuleSegmentInfo {
    VAddr address;
    u32 size;
    s32 prot;
};

struct OrbisKernelModuleInfo {
    u64 st_size = sizeof(OrbisKernelModuleInfo);
    std::array<char, SCE_DBG_MAX_NAME_LENGTH> name;
    std::array<OrbisKernelModuleSegmentInfo, SCE_DBG_MAX_SEGMENTS> segments;
    u32 num_segments;
    std::array<u8, SCE_DBG_NUM_FINGERPRINT> fingerprint;
};

struct OrbisKernelModuleInfoEx {
    u64 st_size = sizeof(OrbisKernelModuleInfoEx);
    std::array<char, SCE_DBG_MAX_NAME_LENGTH> name;
    s32 id;
    u32 tls_index;
    VAddr tls_init_addr;
    u32 tls_init_size;
    u32 tls_size;
    u32 tls_offset;
    u32 tls_align;
    VAddr init_proc_addr;
    VAddr fini_proc_addr;
    u64 reserved1;
    u64 reserved2;
    VAddr eh_frame_hdr_addr;
    VAddr eh_frame_addr;
    u32 eh_frame_hdr_size;
    u32 eh_frame_size;
    std::array<OrbisKernelModuleSegmentInfo, SCE_DBG_MAX_SEGMENTS> segments;
    u32 segment_count;
};

struct ModuleInfo {
    bool operator==(const ModuleInfo& other) const {
        return name == other.name;
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
    u32 align;
    u32 image_size;
    u32 offset;
    u32 modid;
    VAddr image_virtual_addr;
    u32 init_image_size;
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
class MemoryManager;

class Module {
public:
    explicit Module(Core::MemoryManager* memory, const std::filesystem::path& file,
                    u32& max_tls_index);
    ~Module();

    VAddr GetBaseAddress() const noexcept {
        return base_virtual_addr;
    }

    VAddr GetEntryAddress() const noexcept {
        return base_virtual_addr + elf.GetElfEntry();
    }

    OrbisKernelModuleInfo GetModuleInfo() const noexcept {
        return info;
    }

    bool IsValid() const noexcept {
        return base_virtual_addr != 0;
    }

    bool IsSharedLib() const noexcept {
        return elf.IsSharedLib();
    }

    bool IsSystemLib() {
        auto system_path = Config::getSysModulesPath();
        if (file.string().starts_with(system_path.string().c_str())) {
            return true;
        }
        return false;
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
            func(&dynamic_info.relocation_table[i], i, false);
        }
        for (u32 i = 0; i < dynamic_info.jmp_relocation_table_size / sizeof(elf_relocation); i++) {
            func(&dynamic_info.jmp_relocation_table[i], i, true);
        }
    }

    void SetRelaBit(u32 index) {
        rela_bits[index >> 3] |= (1 << (index & 7));
    }

    bool TestRelaBit(u32 index) const {
        return (rela_bits[index >> 3] >> (index & 7)) & 1;
    }

    s32 Start(u64 args, const void* argp, void* param);
    void LoadModuleToMemory(u32& max_tls_index);
    void LoadDynamicInfo();
    void LoadSymbols();

    void* FindByName(std::string_view name);

    OrbisKernelModuleInfoEx GetModuleInfoEx() const;
    const ModuleInfo* FindModule(std::string_view id);
    const LibraryInfo* FindLibrary(std::string_view id);

public:
    Core::MemoryManager* memory;
    std::filesystem::path file;
    std::string name;
    Loader::Elf elf;
    u64 aligned_base_size{};
    VAddr base_virtual_addr{};
    VAddr proc_param_virtual_addr{};
    VAddr eh_frame_hdr_addr{};
    VAddr eh_frame_addr{};
    u32 eh_frame_hdr_size{};
    u32 eh_frame_size{};
    DynamicModuleInfo dynamic_info{};
    std::vector<u8> m_dynamic;
    std::vector<u8> m_dynamic_data;
    Loader::SymbolsResolver export_sym;
    Loader::SymbolsResolver import_sym;
    ThreadLocalImage tls{};
    OrbisKernelModuleInfo info{};
    std::vector<u8> rela_bits;
};

} // namespace Core
