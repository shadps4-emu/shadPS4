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

using HeapApiFunc = PS4_SYSV_ABI void* (*)(size_t);

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

    void SetHeapApiFunc(void* func) {
        heap_api_func = *reinterpret_cast<HeapApiFunc*>(func);
    }

    void AdvanceGenerationCounter() noexcept {
        dtv_generation_counter++;
    }

    void* TlsGetAddr(u64 module_index, u64 offset);
    void InitTlsForThread(bool is_primary = false);

    s32 LoadModule(const std::filesystem::path& elf_name, bool is_dynamic = false);
    Module* FindByAddress(VAddr address);

    void Relocate(Module* module);
    bool Resolve(const std::string& name, Loader::SymbolType type, Module* module,
                 Loader::SymbolRecord* return_info);
    void Execute();
    void DebugDump();

private:
    const Module* FindExportedModule(const ModuleInfo& m, const LibraryInfo& l);

    MemoryManager* memory;
    std::mutex mutex;
    u32 dtv_generation_counter{1};
    size_t static_tls_size{};
    u32 max_tls_index{};
    u32 num_static_modules{};
    HeapApiFunc heap_api_func{};
    std::vector<std::unique_ptr<Module>> m_modules;
    Loader::SymbolsResolver m_hle_symbols{};
};

} // namespace Core
