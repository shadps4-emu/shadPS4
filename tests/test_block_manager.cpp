// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/jit/block_manager.h"
#include <gtest/gtest.h>
#include <sys/mman.h>
#if defined(__APPLE__) && defined(ARCH_ARM64)
#include <pthread.h>
#endif

using namespace Core::Jit;

class BlockManagerTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Allocate executable memory for test code blocks
#if defined(__APPLE__) && defined(ARCH_ARM64)
    // On macOS ARM64, use the JIT API approach
    test_code = mmap(nullptr, 64 * 1024, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    ASSERT_NE(test_code, MAP_FAILED)
        << "Failed to allocate executable memory for test";
    pthread_jit_write_protect_np(0); // Disable write protection for writing
                                     // Will make executable later if needed
#else
    test_code = mmap(nullptr, 64 * 1024, PROT_READ | PROT_WRITE | PROT_EXEC,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    ASSERT_NE(test_code, MAP_FAILED)
        << "Failed to allocate executable memory for test";
#endif
  }

  void TearDown() override {
    if (test_code != MAP_FAILED) {
      munmap(test_code, 64 * 1024);
    }
  }

  void *test_code = MAP_FAILED;
};

TEST_F(BlockManagerTest, Constructor) {
  BlockManager manager;
  EXPECT_EQ(manager.GetBlockCount(), 0);
  EXPECT_EQ(manager.GetTotalCodeSize(), 0);
}

TEST_F(BlockManagerTest, CreateBlock) {
  BlockManager manager;
  VAddr ps4_addr = 0x400000;
  void *arm64_code = test_code;
  size_t code_size = 1024;
  size_t instruction_count = 10;

  CodeBlock *block =
      manager.CreateBlock(ps4_addr, arm64_code, code_size, instruction_count);
  ASSERT_NE(block, nullptr);
  EXPECT_EQ(block->ps4_address, ps4_addr);
  EXPECT_EQ(block->arm64_code, arm64_code);
  EXPECT_EQ(block->code_size, code_size);
  EXPECT_EQ(block->instruction_count, instruction_count);
  EXPECT_FALSE(block->is_linked);
  EXPECT_EQ(manager.GetBlockCount(), 1);
  EXPECT_EQ(manager.GetTotalCodeSize(), code_size);
}

TEST_F(BlockManagerTest, GetBlock) {
  BlockManager manager;
  VAddr ps4_addr = 0x400000;
  void *arm64_code = test_code;

  // Block doesn't exist yet
  CodeBlock *block = manager.GetBlock(ps4_addr);
  EXPECT_EQ(block, nullptr);

  manager.CreateBlock(ps4_addr, arm64_code, 1024, 10);

  // Now it should exist
  block = manager.GetBlock(ps4_addr);
  ASSERT_NE(block, nullptr);
  EXPECT_EQ(block->ps4_address, ps4_addr);
}

TEST_F(BlockManagerTest, MultipleBlocks) {
  BlockManager manager;

  // Create multiple blocks
  for (int i = 0; i < 10; ++i) {
    VAddr ps4_addr = 0x400000 + (i * 0x1000);
    void *arm64_code = static_cast<char *>(test_code) + (i * 1024);
    manager.CreateBlock(ps4_addr, arm64_code, 1024, 10);
  }

  EXPECT_EQ(manager.GetBlockCount(), 10);
  EXPECT_EQ(manager.GetTotalCodeSize(), 10 * 1024);
}

TEST_F(BlockManagerTest, InvalidateBlock) {
  BlockManager manager;
  VAddr ps4_addr = 0x400000;

  // Create and verify block exists
  manager.CreateBlock(ps4_addr, test_code, 1024, 10);
  EXPECT_NE(manager.GetBlock(ps4_addr), nullptr);

  // Invalidate block
  manager.InvalidateBlock(ps4_addr);

  // Block should no longer exist
  EXPECT_EQ(manager.GetBlock(ps4_addr), nullptr);
  EXPECT_EQ(manager.GetBlockCount(), 0);
  EXPECT_EQ(manager.GetTotalCodeSize(), 0);
}

TEST_F(BlockManagerTest, InvalidateRange) {
  BlockManager manager;

  // Create blocks at different addresses
  manager.CreateBlock(0x400000, test_code, 1024, 10);
  manager.CreateBlock(0x401000, static_cast<char *>(test_code) + 1024, 1024,
                      10);
  manager.CreateBlock(0x402000, static_cast<char *>(test_code) + 2048, 1024,
                      10);
  manager.CreateBlock(0x500000, static_cast<char *>(test_code) + 3072, 1024,
                      10);

  EXPECT_EQ(manager.GetBlockCount(), 4);

  // Invalidate range that covers first 3 blocks
  manager.InvalidateRange(0x400000, 0x403000);

  // First 3 blocks should be gone, last one should remain
  EXPECT_EQ(manager.GetBlock(0x400000), nullptr);
  EXPECT_EQ(manager.GetBlock(0x401000), nullptr);
  EXPECT_EQ(manager.GetBlock(0x402000), nullptr);
  EXPECT_NE(manager.GetBlock(0x500000), nullptr);
  EXPECT_EQ(manager.GetBlockCount(), 1);
}

TEST_F(BlockManagerTest, AddDependency) {
  BlockManager manager;
  VAddr block_addr = 0x400000;
  VAddr dep_addr = 0x500000;

  CodeBlock *block = manager.CreateBlock(block_addr, test_code, 1024, 10);
  manager.AddDependency(block_addr, dep_addr);

  EXPECT_EQ(block->dependencies.size(), 1);
  EXPECT_NE(block->dependencies.find(dep_addr), block->dependencies.end());
}

TEST_F(BlockManagerTest, MultipleDependencies) {
  BlockManager manager;
  VAddr block_addr = 0x400000;

  CodeBlock *block = manager.CreateBlock(block_addr, test_code, 1024, 10);
  manager.AddDependency(block_addr, 0x500000);
  manager.AddDependency(block_addr, 0x600000);
  manager.AddDependency(block_addr, 0x700000);

  EXPECT_EQ(block->dependencies.size(), 3);
}

TEST_F(BlockManagerTest, Clear) {
  BlockManager manager;

  // Create multiple blocks
  for (int i = 0; i < 5; ++i) {
    VAddr ps4_addr = 0x400000 + (i * 0x1000);
    void *arm64_code = static_cast<char *>(test_code) + (i * 1024);
    manager.CreateBlock(ps4_addr, arm64_code, 1024, 10);
  }

  EXPECT_EQ(manager.GetBlockCount(), 5);

  manager.Clear();

  EXPECT_EQ(manager.GetBlockCount(), 0);
  EXPECT_EQ(manager.GetTotalCodeSize(), 0);
}
