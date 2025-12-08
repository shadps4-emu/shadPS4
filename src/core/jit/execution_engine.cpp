// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>
#include <sys/mman.h>
#include "common/decoder.h"
#include "common/logging/log.h"
#include "core/memory.h"
#include "execution_engine.h"
#if defined(__APPLE__) && defined(ARCH_ARM64)
#include <pthread.h>
#endif

namespace Core::Jit {

static size_t alignUp(size_t value, size_t alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

static void* AllocateExecutableMemory(size_t size) {
    size = alignUp(size, 4096);
#if defined(__APPLE__) && defined(ARCH_ARM64)
    // On macOS ARM64:
    // 1. Allocate with PROT_READ | PROT_WRITE (no PROT_EXEC initially)
    // 2. Use pthread_jit_write_protect_np to allow writing
    // 3. After writing, use mprotect to add PROT_EXEC
    void* ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED) {
        LOG_CRITICAL(Core, "Failed to allocate executable memory: {} (errno={})", strerror(errno),
                     errno);
        return nullptr;
    }
    // Initially disable write protection so we can write code
    pthread_jit_write_protect_np(0);
    return ptr;
#else
    void* ptr =
        mmap(nullptr, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED) {
        LOG_CRITICAL(Core, "Failed to allocate executable memory: {}", strerror(errno));
        return nullptr;
    }
    return ptr;
#endif
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
    if (IsInitialized()) {
        LOG_DEBUG(Core, "JIT Execution Engine already initialized");
        return;
    }

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
#if defined(__APPLE__) && defined(ARCH_ARM64)
        // On macOS ARM64, ensure write protection is enabled before unmapping
        pthread_jit_write_protect_np(1);
#endif
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
    VAddr fallthrough_target = 0;
    VAddr branch_target = 0;
    void* branch_patch_location = nullptr;

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

        // Track branch target before translation
        if (instruction.mnemonic == ZYDIS_MNEMONIC_JMP &&
            operands[0].type == ZYDIS_OPERAND_TYPE_IMMEDIATE) {
            s64 offset = static_cast<s64>(operands[0].imm.value.s);
            branch_target = current_address + instruction.length + offset;
            branch_patch_location = code_generator->getCurr();
        }

        bool translated = translator->TranslateInstruction(instruction, operands, current_address);
        if (!translated) {
            LOG_WARNING(Core, "Failed to translate instruction at {:#x}", current_address);
            break;
        }

        instruction_count++;
        VAddr next_address = current_address + instruction.length;

        switch (instruction.mnemonic) {
        case ZYDIS_MNEMONIC_RET:
        case ZYDIS_MNEMONIC_CALL:
            block_end = true;
            break;
        case ZYDIS_MNEMONIC_JMP:
            block_end = true;
            break;
        default:
            // Check for conditional branches (they don't end the block, but we track them)
            if (instruction.mnemonic >= ZYDIS_MNEMONIC_JO &&
                instruction.mnemonic <= ZYDIS_MNEMONIC_JZ) {
                // Conditional branch - block continues with fallthrough
                // TODO: Track conditional branch targets for linking
            }
            break;
        }

        current_address = next_address;
    }

    if (instruction_count == 0) {
        return nullptr;
    }

    // Set fallthrough target if block doesn't end with unconditional branch/ret
    if (!block_end || branch_target == 0) {
        fallthrough_target = current_address;
    }

    size_t code_size = code_generator->getSize();
    code_generator->makeExecutable();
    CodeBlock* block =
        block_manager->CreateBlock(start_address, block_start, code_size, instruction_count);

    // Store control flow information
    block->fallthrough_target = fallthrough_target;
    block->branch_target = branch_target;
    block->branch_patch_location = branch_patch_location;

    LOG_DEBUG(Core,
              "Translated basic block at {:#x}, {} instructions, {} bytes, fallthrough: {:#x}, "
              "branch: {:#x}",
              start_address, instruction_count, code_size, fallthrough_target, branch_target);

    // Try to link blocks if targets are available
    if (branch_target != 0) {
        CodeBlock* target_block = block_manager->GetBlock(branch_target);
        if (target_block) {
            LinkBlock(block, branch_target);
        } else {
            // Add dependency for later linking
            block_manager->AddDependency(start_address, branch_target);
        }
    }

    if (fallthrough_target != 0 && branch_target == 0) {
        // Try to link fallthrough
        CodeBlock* target_block = block_manager->GetBlock(fallthrough_target);
        if (target_block) {
            // For fallthrough, we need to append a branch at the end
            // This will be handled by linking logic
            block_manager->AddDependency(start_address, fallthrough_target);
        }
    }

    return block;
}

CodeBlock* ExecutionEngine::TranslateBlock(VAddr ps4_address) {
    CodeBlock* existing = block_manager->GetBlock(ps4_address);
    if (existing) {
        return existing;
    }

    CodeBlock* new_block = TranslateBasicBlock(ps4_address);
    if (!new_block) {
        return nullptr;
    }

    // After creating a new block, check if any existing blocks can link to it
    // This handles the case where we translate a target block after the source
    for (auto& [addr, block] : block_manager->blocks) {
        if (block->branch_target == ps4_address && !block->is_linked) {
            LinkBlock(block.get(), ps4_address);
        }
        if (block->fallthrough_target == ps4_address && block->branch_target == 0 &&
            !block->is_linked) {
            LinkBlock(block.get(), ps4_address);
        }
    }

    return new_block;
}

void ExecutionEngine::LinkBlock(CodeBlock* block, VAddr target_address) {
    CodeBlock* target_block = block_manager->GetBlock(target_address);
    if (!target_block) {
        return;
    }

    // Patch the branch instruction if we have a patch location
    if (block->branch_patch_location && block->branch_target == target_address) {
#if defined(__APPLE__) && defined(ARCH_ARM64)
        pthread_jit_write_protect_np(0);
#endif
        // Calculate offset from patch location to target
        s64 offset = reinterpret_cast<s64>(target_block->arm64_code) -
                     reinterpret_cast<s64>(block->branch_patch_location);

        // Check if we can use a relative branch (within ±128MB)
        if (offset >= -0x8000000 && offset < 0x8000000) {
            s32 imm26 = static_cast<s32>(offset / 4);
            u32* patch_ptr = reinterpret_cast<u32*>(block->branch_patch_location);
            // Patch the branch instruction: 0x14000000 | (imm26 & 0x3FFFFFF)
            *patch_ptr = 0x14000000 | (imm26 & 0x3FFFFFF);
        } else {
            // Far branch - need to use indirect branch
            // For now, leave as-is (will use the placeholder branch)
            LOG_DEBUG(Core, "Branch target too far for direct linking: offset={}", offset);
        }
#if defined(__APPLE__) && defined(ARCH_ARM64)
        pthread_jit_write_protect_np(1);
        __builtin___clear_cache(static_cast<char*>(block->branch_patch_location),
                                static_cast<char*>(block->branch_patch_location) + 4);
#endif
        block->is_linked = true;
        LOG_DEBUG(Core, "Linked block {:#x} to {:#x}", block->ps4_address, target_address);
    } else if (block->fallthrough_target == target_address && block->branch_target == 0) {
        // For fallthrough, append a branch at the end of the block
#if defined(__APPLE__) && defined(ARCH_ARM64)
        pthread_jit_write_protect_np(0);
#endif
        void* link_location = static_cast<u8*>(block->arm64_code) + block->code_size;
        s64 offset =
            reinterpret_cast<s64>(target_block->arm64_code) - reinterpret_cast<s64>(link_location);

        if (offset >= -0x8000000 && offset < 0x8000000) {
            s32 imm26 = static_cast<s32>(offset / 4);
            u32* patch_ptr = reinterpret_cast<u32*>(link_location);
            *patch_ptr = 0x14000000 | (imm26 & 0x3FFFFFF);
            block->code_size += 4; // Update block size
        }
#if defined(__APPLE__) && defined(ARCH_ARM64)
        pthread_jit_write_protect_np(1);
        __builtin___clear_cache(static_cast<char*>(link_location),
                                static_cast<char*>(link_location) + 4);
#endif
        block->is_linked = true;
        LOG_DEBUG(Core, "Linked fallthrough from block {:#x} to {:#x}", block->ps4_address,
                  target_address);
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
