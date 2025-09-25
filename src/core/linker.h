// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <algorithm>
#include <mutex>
#include <vector>
#include "core/libraries/kernel/threads.h"
#include "core/module.h"

namespace Core {

struct DynamicModuleInfo;
class Linker;
class MemoryManager;

struct OrbisKernelMemParam {
    u64 size;
    u64* extended_page_table;
    u64* flexible_memory_size;
    u8* extended_memory_1;
    u64* extended_gpu_page_table;
    u8* extended_memory_2;
    u64* extended_cpu_page_table;
};
static_assert(sizeof(OrbisKernelMemParam) == 0x38);

struct OrbisProcParam {
    u64 size;
    u32 magic;
    u32 entry_count;
    u64 sdk_version;
    char* process_name;
    char* main_thread_name;
    u32* main_thread_prio;
    u32* main_thread_stack_size;
    void* libc_param;
    OrbisKernelMemParam* mem_param;
    void* fs_param;
    u32* process_preload_enable;
    u64 unknown1;
};

using ExitFunc = PS4_SYSV_ABI void (*)();

class Linker;

struct EntryParams {
    int argc;
    u32 padding;
    const char* argv[33];
    VAddr entry_addr;
};

struct HeapAPI {
    PS4_SYSV_ABI void* (*heap_malloc)(size_t);
    PS4_SYSV_ABI void (*heap_free)(void*);
    PS4_SYSV_ABI void* (*heap_calloc)(size_t, size_t);
    PS4_SYSV_ABI void* (*heap_realloc)(void*, size_t);
    PS4_SYSV_ABI void* (*heap_memalign)(size_t, size_t);
    PS4_SYSV_ABI int (*heap_posix_memalign)(void**, size_t, size_t);
    // NOTE: Fields below may be inaccurate
    PS4_SYSV_ABI int (*heap_reallocalign)(void);
    PS4_SYSV_ABI void (*heap_malloc_stats)(void);
    PS4_SYSV_ABI int (*heap_malloc_stats_fast)(void);
    PS4_SYSV_ABI size_t (*heap_malloc_usable_size)(void*);
};

using AppHeapAPI = HeapAPI*;

class Linker {
public:
    explicit Linker();
    ~Linker();

    Loader::SymbolsResolver& GetHLESymbols() {
        return m_hle_symbols;
    }

    OrbisProcParam* GetProcParam() const {
        return m_modules[0]->GetProcParam<OrbisProcParam*>();
    }

    Module* GetModule(s32 index) const {
        if (index >= 0 && index < m_modules.size()) {
            return m_modules.at(index).get();
        }
        return nullptr;
    }

    u32 FindByName(const std::filesystem::path& name) const {
        for (u32 i = 0; i < m_modules.size(); i++) {
            if (name == m_modules[i]->file) {
                return i;
            }
        }
        return -1;
    }

    u32 MaxTlsIndex() const {
        return max_tls_index;
    }

    u32 GenerationCounter() const {
        return dtv_generation_counter;
    }

    size_t StaticTlsSize() const noexcept {
        return static_tls_size;
    }

    void RelocateAnyImports(Module* m) {
        Relocate(m);
        const auto exports = m->GetExportModules();
        for (auto& export_mod : exports) {
            for (auto& module : m_modules) {
                const auto imports = module->GetImportModules();
                if (std::ranges::contains(imports, export_mod.name, &ModuleInfo::name)) {
                    Relocate(module.get());
                }
            }
        }
    }

    void LoadSharedLibraries() {
        for (auto& module : m_modules) {
            if (module->IsSharedLib()) {
                module->Start(0, nullptr, nullptr);
            }
        }
    }

    void SetHeapAPI(void* func[]) {
        heap_api = reinterpret_cast<AppHeapAPI>(func);
    }

    void AdvanceGenerationCounter() noexcept {
        dtv_generation_counter++;
    }

    void* TlsGetAddr(u64 module_index, u64 offset);
    void* AllocateTlsForThread(bool is_primary);
    void FreeTlsForNonPrimaryThread(void* pointer);

    s32 LoadModule(const std::filesystem::path& elf_name, bool is_dynamic = false);
    s32 LoadAndStartModule(const std::filesystem::path& path, u64 args, const void* argp,
                           int* pRes);
    Module* FindByAddress(VAddr address);

    void Relocate(Module* module);
    bool Resolve(const std::string& name, Loader::SymbolType type, Module* module,
                 Loader::SymbolRecord* return_info);
    void Execute(const std::vector<std::string>& args = {});
    void DebugDump();

private:
    const Module* FindExportedModule(const ModuleInfo& m, const LibraryInfo& l);

    MemoryManager* memory;
    Libraries::Kernel::Thread main_thread;
    std::mutex mutex;
    u32 dtv_generation_counter{1};
    size_t static_tls_size{};
    u32 max_tls_index{};
    u32 num_static_modules{};
    AppHeapAPI heap_api{};
    std::vector<std::unique_ptr<Module>> m_modules;
    Loader::SymbolsResolver m_hle_symbols{};
};

} // namespace Core
