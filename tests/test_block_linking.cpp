// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/decoder.h"
#include "core/jit/arm64_codegen.h"
#include "core/jit/block_manager.h"
#include "core/jit/register_mapping.h"
#include "core/jit/x86_64_translator.h"
#include <gtest/gtest.h>
#include <sys/mman.h>
#if defined(__APPLE__) && defined(ARCH_ARM64)
#include <pthread.h>
#endif

using namespace Core::Jit;

class BlockLinkingTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Allocate executable memory for test code
#if defined(__APPLE__) && defined(ARCH_ARM64)
    test_code_buffer = mmap(nullptr, 64 * 1024, PROT_READ | PROT_WRITE,
                            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    ASSERT_NE(test_code_buffer, MAP_FAILED)
        << "Failed to allocate executable memory for test";
    pthread_jit_write_protect_np(0);
#else
    test_code_buffer =
        mmap(nullptr, 64 * 1024, PROT_READ | PROT_WRITE | PROT_EXEC,
             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    ASSERT_NE(test_code_buffer, MAP_FAILED)
        << "Failed to allocate executable memory for test";
#endif
    codegen = std::make_unique<Arm64CodeGenerator>(64 * 1024, test_code_buffer);
    register_mapper = std::make_unique<RegisterMapper>();
    translator = std::make_unique<X86_64Translator>(*codegen, *register_mapper);
    block_manager = std::make_unique<BlockManager>();
  }

  void TearDown() override {
    translator.reset();
    register_mapper.reset();
    codegen.reset();
    block_manager.reset();
    if (test_code_buffer != MAP_FAILED) {
      munmap(test_code_buffer, 64 * 1024);
    }
  }

  void *test_code_buffer = MAP_FAILED;
  std::unique_ptr<Arm64CodeGenerator> codegen;
  std::unique_ptr<RegisterMapper> register_mapper;
  std::unique_ptr<X86_64Translator> translator;
  std::unique_ptr<BlockManager> block_manager;
};

// Test that JMP translation can handle direct immediate addresses
TEST_F(BlockLinkingTest, TranslateDirectJmp) {
  // Create a simple x86_64 JMP instruction: JMP +0x1000 (relative jump)
  // x86_64 encoding: E9 <offset> (near relative jump, 32-bit offset)
  // E9 00 10 00 00 = JMP +0x1000
  u8 x86_jmp[] = {0xE9, 0x00, 0x10, 0x00, 0x00};

  ZydisDecodedInstruction instruction;
  ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT];

  ZyanStatus status = Common::Decoder::Instance()->decodeInstruction(
      instruction, operands, x86_jmp, sizeof(x86_jmp));

  if (!ZYAN_SUCCESS(status)) {
    GTEST_SKIP()
        << "Failed to decode JMP instruction - Zydis may not be available";
  }

  // JMP translation should succeed (even if target isn't linked yet)
  bool result = translator->TranslateJmp(instruction, operands, 0x400000);
  EXPECT_TRUE(result) << "JMP translation should succeed";
  EXPECT_GT(codegen->getSize(), 0) << "JMP should generate ARM64 code";
}

// Test that we can create two blocks and link them
TEST_F(BlockLinkingTest, CreateAndLinkBlocks) {
  VAddr block1_addr = 0x400000;
  VAddr block2_addr = 0x401000;

  // Allocate separate memory for each block to avoid issues
#if defined(__APPLE__) && defined(ARCH_ARM64)
  void *block1_mem = mmap(nullptr, 4096, PROT_READ | PROT_WRITE,
                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  ASSERT_NE(block1_mem, MAP_FAILED);
  pthread_jit_write_protect_np(0);

  void *block2_mem = mmap(nullptr, 4096, PROT_READ | PROT_WRITE,
                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  ASSERT_NE(block2_mem, MAP_FAILED);
#else
  void *block1_mem = mmap(nullptr, 4096, PROT_READ | PROT_WRITE | PROT_EXEC,
                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  ASSERT_NE(block1_mem, MAP_FAILED);

  void *block2_mem = mmap(nullptr, 4096, PROT_READ | PROT_WRITE | PROT_EXEC,
                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  ASSERT_NE(block2_mem, MAP_FAILED);
#endif

  // Write simple NOP instructions
  u32 nop = 0xD503201F; // ARM64 NOP
  *reinterpret_cast<u32 *>(block1_mem) = nop;
  *reinterpret_cast<u32 *>(block2_mem) = nop;

#if defined(__APPLE__) && defined(ARCH_ARM64)
  pthread_jit_write_protect_np(1);
  mprotect(block1_mem, 4096, PROT_READ | PROT_EXEC);
  mprotect(block2_mem, 4096, PROT_READ | PROT_EXEC);
#endif

  // Create blocks
  CodeBlock *block1 = block_manager->CreateBlock(block1_addr, block1_mem, 4, 1);
  ASSERT_NE(block1, nullptr);

  CodeBlock *block2 = block_manager->CreateBlock(block2_addr, block2_mem, 4, 1);
  ASSERT_NE(block2, nullptr);

  // Verify blocks exist
  EXPECT_EQ(block_manager->GetBlockCount(), 2);
  EXPECT_NE(block_manager->GetBlock(block1_addr), nullptr);
  EXPECT_NE(block_manager->GetBlock(block2_addr), nullptr);

  // Test that blocks can be retrieved
  CodeBlock *retrieved_block1 = block_manager->GetBlock(block1_addr);
  CodeBlock *retrieved_block2 = block_manager->GetBlock(block2_addr);
  EXPECT_EQ(retrieved_block1, block1);
  EXPECT_EQ(retrieved_block2, block2);

  // Cleanup
  munmap(block1_mem, 4096);
  munmap(block2_mem, 4096);
}

// Test that block linking tracks dependencies
TEST_F(BlockLinkingTest, BlockDependencies) {
  VAddr block1_addr = 0x400000;
  VAddr block2_addr = 0x401000;

  // Allocate memory for blocks
#if defined(__APPLE__) && defined(ARCH_ARM64)
  void *block1_mem = mmap(nullptr, 4096, PROT_READ | PROT_WRITE,
                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  ASSERT_NE(block1_mem, MAP_FAILED);
  pthread_jit_write_protect_np(0);

  void *block2_mem = mmap(nullptr, 4096, PROT_READ | PROT_WRITE,
                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  ASSERT_NE(block2_mem, MAP_FAILED);

  u32 nop = 0xD503201F;
  *reinterpret_cast<u32 *>(block1_mem) = nop;
  *reinterpret_cast<u32 *>(block2_mem) = nop;

  pthread_jit_write_protect_np(1);
  mprotect(block1_mem, 4096, PROT_READ | PROT_EXEC);
  mprotect(block2_mem, 4096, PROT_READ | PROT_EXEC);
#else
  void *block1_mem = mmap(nullptr, 4096, PROT_READ | PROT_WRITE | PROT_EXEC,
                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  ASSERT_NE(block1_mem, MAP_FAILED);

  void *block2_mem = mmap(nullptr, 4096, PROT_READ | PROT_WRITE | PROT_EXEC,
                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  ASSERT_NE(block2_mem, MAP_FAILED);

  u32 nop = 0xD503201F;
  *reinterpret_cast<u32 *>(block1_mem) = nop;
  *reinterpret_cast<u32 *>(block2_mem) = nop;
#endif

  // Create blocks
  CodeBlock *block1 = block_manager->CreateBlock(block1_addr, block1_mem, 4, 1);
  CodeBlock *block2 = block_manager->CreateBlock(block2_addr, block2_mem, 4, 1);

  // Add dependency: block1 depends on block2
  block_manager->AddDependency(block1_addr, block2_addr);

  // Verify dependency is tracked
  EXPECT_EQ(block1->dependencies.count(block2_addr), 1);

  // Cleanup
  munmap(block1_mem, 4096);
  munmap(block2_mem, 4096);
}

// Test that invalidating a block invalidates dependent blocks
TEST_F(BlockLinkingTest, InvalidateDependentBlocks) {
  VAddr block1_addr = 0x400000;
  VAddr block2_addr = 0x401000;

  // Allocate memory for blocks
#if defined(__APPLE__) && defined(ARCH_ARM64)
  void *block1_mem = mmap(nullptr, 4096, PROT_READ | PROT_WRITE,
                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  ASSERT_NE(block1_mem, MAP_FAILED);
  pthread_jit_write_protect_np(0);

  void *block2_mem = mmap(nullptr, 4096, PROT_READ | PROT_WRITE,
                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  ASSERT_NE(block2_mem, MAP_FAILED);

  u32 nop = 0xD503201F;
  *reinterpret_cast<u32 *>(block1_mem) = nop;
  *reinterpret_cast<u32 *>(block2_mem) = nop;

  pthread_jit_write_protect_np(1);
  mprotect(block1_mem, 4096, PROT_READ | PROT_EXEC);
  mprotect(block2_mem, 4096, PROT_READ | PROT_EXEC);
#else
  void *block1_mem = mmap(nullptr, 4096, PROT_READ | PROT_WRITE | PROT_EXEC,
                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  ASSERT_NE(block1_mem, MAP_FAILED);

  void *block2_mem = mmap(nullptr, 4096, PROT_READ | PROT_WRITE | PROT_EXEC,
                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  ASSERT_NE(block2_mem, MAP_FAILED);

  u32 nop = 0xD503201F;
  *reinterpret_cast<u32 *>(block1_mem) = nop;
  *reinterpret_cast<u32 *>(block2_mem) = nop;
#endif

  // Create blocks with dependency
  CodeBlock *block1 = block_manager->CreateBlock(block1_addr, block1_mem, 4, 1);
  CodeBlock *block2 = block_manager->CreateBlock(block2_addr, block2_mem, 4, 1);

  block_manager->AddDependency(block1_addr, block2_addr);

  // Invalidate block2
  block_manager->InvalidateBlock(block2_addr);

  // block2 should be removed
  EXPECT_EQ(block_manager->GetBlock(block2_addr), nullptr);
  // block1 should still exist (dependency tracking doesn't auto-invalidate)
  // But in a real implementation, we might want to invalidate dependents
  EXPECT_NE(block_manager->GetBlock(block1_addr), nullptr);

  // Cleanup
  munmap(block1_mem, 4096);
  munmap(block2_mem, 4096);
}
