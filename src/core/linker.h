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
    std::vector<std::unique_ptr<Module>> m_modules;
    Loader::SymbolsResolver m_hle_symbols{};
};

} // namespace Core
