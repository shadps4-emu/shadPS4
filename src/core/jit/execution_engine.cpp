// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>
#include <sys/mman.h>
#include "common/decoder.h"
#include "common/logging/log.h"
#include "core/memory.h"
#include "execution_engine.h"

namespace Core::Jit {

static void* AllocateExecutableMemory(size_t size) {
    size = (size + 4095) & ~4095;
    void* ptr =
        mmap(nullptr, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED) {
        LOG_CRITICAL(Core, "Failed to allocate executable memory: {}", strerror(errno));
        return nullptr;
    }
    return ptr;
}

ExecutionEngine::ExecutionEngine()
    : code_buffer(nullptr), code_buffer_size(DEFAULT_CODE_BUFFER_SIZE), code_buffer_used(0) {
    block_manager = std::make_unique<BlockManager>();
    register_mapper = std::make_unique<RegisterMapper>();
}

ExecutionEngine::~ExecutionEngine() {
    Shutdown();
}

void ExecutionEngine::Initialize() {
    code_buffer = AllocateExecutableMemory(code_buffer_size);
    if (!code_buffer) {
        throw std::bad_alloc();
    }

    code_generator = std::make_unique<Arm64CodeGenerator>(code_buffer_size, code_buffer);
    translator = std::make_unique<X86_64Translator>(*code_generator, *register_mapper);

    LOG_INFO(Core, "JIT Execution Engine initialized");
}

void ExecutionEngine::Shutdown() {
    if (code_buffer) {
        munmap(code_buffer, code_buffer_size);
        code_buffer = nullptr;
    }
    code_generator.reset();
    translator.reset();
    block_manager.reset();
    register_mapper.reset();
}

void* ExecutionEngine::AllocateCodeBuffer(size_t size) {
    size = (size + 15) & ~15;
    if (code_buffer_used + size > code_buffer_size) {
        LOG_WARNING(Core, "Code buffer exhausted, need to allocate more");
        return nullptr;
    }
    void* result = static_cast<u8*>(code_buffer) + code_buffer_used;
    code_buffer_used += size;
    return result;
}

CodeBlock* ExecutionEngine::TranslateBasicBlock(VAddr start_address, size_t max_instructions) {
    auto* memory = Core::Memory::Instance();
    auto& address_space = memory->GetAddressSpace();
    void* ps4_code_ptr = address_space.TranslateAddress(start_address);
    if (!ps4_code_ptr) {
        LOG_ERROR(Core, "Invalid PS4 address for translation: {:#x}", start_address);
        return nullptr;
    }

    code_generator->reset();
    void* block_start = code_generator->getCurr();

    VAddr current_address = start_address;
    size_t instruction_count = 0;
    bool block_end = false;

    while (instruction_count < max_instructions && !block_end) {
        ZydisDecodedInstruction instruction;
        ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT];

        void* code_ptr = address_space.TranslateAddress(current_address);
        if (!code_ptr) {
            break;
        }

        ZyanStatus status =
            Common::Decoder::Instance()->decodeInstruction(instruction, operands, code_ptr, 15);
        if (!ZYAN_SUCCESS(status)) {
            LOG_WARNING(Core, "Failed to decode instruction at {:#x}", current_address);
            break;
        }

        bool translated = translator->TranslateInstruction(instruction, operands, current_address);
        if (!translated) {
            LOG_WARNING(Core, "Failed to translate instruction at {:#x}", current_address);
            break;
        }

        instruction_count++;
        current_address += instruction.length;

        switch (instruction.mnemonic) {
        case ZYDIS_MNEMONIC_RET:
        case ZYDIS_MNEMONIC_JMP:
        case ZYDIS_MNEMONIC_CALL:
            block_end = true;
            break;
        default:
            break;
        }
    }

    if (instruction_count == 0) {
        return nullptr;
    }

    size_t code_size = code_generator->getSize();
    CodeBlock* block =
        block_manager->CreateBlock(start_address, block_start, code_size, instruction_count);

    LOG_DEBUG(Core, "Translated basic block at {:#x}, {} instructions, {} bytes", start_address,
              instruction_count, code_size);

    return block;
}

CodeBlock* ExecutionEngine::TranslateBlock(VAddr ps4_address) {
    CodeBlock* existing = block_manager->GetBlock(ps4_address);
    if (existing) {
        return existing;
    }

    return TranslateBasicBlock(ps4_address);
}

void ExecutionEngine::LinkBlock(CodeBlock* block, VAddr target_address) {
    CodeBlock* target_block = block_manager->GetBlock(target_address);
    if (target_block && !block->is_linked) {
        void* link_location = static_cast<u8*>(block->arm64_code) + block->code_size - 4;
        code_generator->setSize(reinterpret_cast<u8*>(link_location) -
                                static_cast<u8*>(code_generator->getCode()));
        code_generator->b(target_block->arm64_code);
        block->is_linked = true;
    }
}

bool ExecutionEngine::ExecuteBlock(VAddr ps4_address) {
    CodeBlock* block = TranslateBlock(ps4_address);
    if (!block) {
        LOG_ERROR(Core, "Failed to translate or find block at {:#x}", ps4_address);
        return false;
    }

    typedef void (*BlockFunc)();
    BlockFunc func = reinterpret_cast<BlockFunc>(block->arm64_code);
    func();

    return true;
}

void ExecutionEngine::InvalidateBlock(VAddr ps4_address) {
    block_manager->InvalidateBlock(ps4_address);
}

void ExecutionEngine::InvalidateRange(VAddr start, VAddr end) {
    block_manager->InvalidateRange(start, end);
}

bool ExecutionEngine::IsJitCode(void* code_ptr) const {
    if (!code_buffer) {
        return false;
    }
    u8* ptr = static_cast<u8*>(code_ptr);
    u8* start = static_cast<u8*>(code_buffer);
    u8* end = start + code_buffer_size;
    return ptr >= start && ptr < end;
}

VAddr ExecutionEngine::GetPs4AddressForJitCode(void* code_ptr) const {
    if (!IsJitCode(code_ptr)) {
        return 0;
    }
    std::lock_guard<std::mutex> lock(block_manager->mutex);
    for (const auto& [ps4_addr, block] : block_manager->blocks) {
        u8* block_start = static_cast<u8*>(block->arm64_code);
        u8* block_end = block_start + block->code_size;
        u8* ptr = static_cast<u8*>(code_ptr);
        if (ptr >= block_start && ptr < block_end) {
            return ps4_addr;
        }
    }
    return 0;
}

} // namespace Core::Jit
