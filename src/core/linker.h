// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <algorithm>
#include <mutex>
#include <vector>
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
    u64* exnteded_cpu_page_table;
};

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

struct EntryParams {
    int argc;
    u32 padding;
    const char* argv[3];
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
        return m_modules.at(index).get();
    }

    void RelocateAnyImports(Module* m) {
        Relocate(m);
        for (auto& module : m_modules) {
            const auto imports = module->GetImportModules();
            if (std::ranges::contains(imports, m->name, &ModuleInfo::name)) {
                Relocate(module.get());
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

    s32 LoadModule(const std::filesystem::path& elf_name, bool is_dynamic = false);
    Module* FindByAddress(VAddr address);

    void Relocate(Module* module);
    bool Resolve(const std::string& name, Loader::SymbolType type, Module* module,
                 Loader::SymbolRecord* return_info);
    void Execute();
    void DebugDump();

    template <class ReturnType, class... FuncArgs, class... CallArgs>
    ReturnType ExecuteGuest(PS4_SYSV_ABI ReturnType (*func)(FuncArgs...),
                            CallArgs&&... args) const {
        // Make sure TLS is initialized for the thread before entering guest.
        EnsureThreadInitialized();
        return ExecuteGuestWithoutTls(func, args...);
    }

private:
    const Module* FindExportedModule(const ModuleInfo& m, const LibraryInfo& l);
    void EnsureThreadInitialized(bool is_primary = false) const;
    void InitTlsForThread(bool is_primary) const;

    template <class ReturnType, class... FuncArgs, class... CallArgs>
    ReturnType ExecuteGuestWithoutTls(PS4_SYSV_ABI ReturnType (*func)(FuncArgs...),
                                      CallArgs&&... args) const {
        return func(std::forward<CallArgs>(args)...);
    }

    MemoryManager* memory;
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
