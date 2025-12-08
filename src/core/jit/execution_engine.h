// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include "arm64_codegen.h"
#include "block_manager.h"
#include "common/singleton.h"
#include "common/types.h"
#include "register_mapping.h"
#include "x86_64_translator.h"

namespace Core::Jit {

class ExecutionEngine {
public:
    ExecutionEngine();
    ~ExecutionEngine();

    bool ExecuteBlock(VAddr ps4_address);
    CodeBlock* TranslateBlock(VAddr ps4_address);
    void InvalidateBlock(VAddr ps4_address);
    void InvalidateRange(VAddr start, VAddr end);

    bool IsJitCode(void* code_ptr) const;
    VAddr GetPs4AddressForJitCode(void* code_ptr) const;

    void Initialize();
    void Shutdown();
    bool IsInitialized() const {
        return code_buffer != nullptr;
    }

private:
    CodeBlock* TranslateBasicBlock(VAddr start_address, size_t max_instructions = 100);
    void* AllocateCodeBuffer(size_t size);
    void LinkBlock(CodeBlock* block, VAddr target_address);

    std::unique_ptr<BlockManager> block_manager;
    std::unique_ptr<RegisterMapper> register_mapper;
    std::unique_ptr<Arm64CodeGenerator> code_generator;
    std::unique_ptr<X86_64Translator> translator;

    void* code_buffer;
    size_t code_buffer_size;
    size_t code_buffer_used;

    static constexpr size_t DEFAULT_CODE_BUFFER_SIZE = 64_MB;

    friend class BlockManager;
};

using JitEngine = Common::Singleton<ExecutionEngine>;

} // namespace Core::Jit
