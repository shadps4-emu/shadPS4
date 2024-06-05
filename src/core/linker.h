// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <mutex>
#include <vector>
#include "core/module.h"

namespace Core {

struct DynamicModuleInfo;
class Linker;

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

    VAddr GetProcParam() const {
        return m_modules[0]->GetProcParam();
    }

    Module* GetModule(s32 index) const {
        return m_modules.at(index).get();
    }

    void SetHeapApiFunc(void* func) {
        heap_api_func = *reinterpret_cast<HeapApiFunc*>(func);
    }

    void* TlsGetAddr(u64 module_index, u64 offset);
    void InitTlsForThread(bool is_primary = false);

    s32 LoadModule(const std::filesystem::path& elf_name);

    void Relocate(Module* module);
    void Resolve(const std::string& name, Loader::SymbolType type, Module* module,
                 Loader::SymbolRecord* return_info);
    void Execute();
    void DebugDump();

private:
    const Module* FindExportedModule(const ModuleInfo& m, const LibraryInfo& l);

    std::mutex mutex;
    u32 dtv_generation_counter{1};
    size_t static_tls_size{};
    size_t max_tls_index{};
    HeapApiFunc heap_api_func{};
    std::vector<std::unique_ptr<Module>> m_modules;
    Loader::SymbolsResolver m_hle_symbols{};
};

} // namespace Core
