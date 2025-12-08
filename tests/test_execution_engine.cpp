// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/jit/arm64_codegen.h"
#include "core/jit/block_manager.h"
#include "core/jit/register_mapping.h"
#include <gtest/gtest.h>
#include <sys/mman.h>

using namespace Core::Jit;

// NOTE: ExecutionEngine requires MemoryManager and AddressSpace which have
// heavy dependencies. These tests focus on the components that can be tested in
// isolation. Full integration tests would require the complete emulator system
// to be initialized. Let's just skip them for now.

class ExecutionEngineComponentTest : public ::testing::Test {
protected:
  void SetUp() override {}

  void TearDown() override {}
};

// Test that the components used by ExecutionEngine can be constructed
TEST_F(ExecutionEngineComponentTest, ComponentConstruction) {
  BlockManager block_manager;
  RegisterMapper register_mapper;
  Arm64CodeGenerator code_generator;

  // All components should construct successfully
  EXPECT_EQ(block_manager.GetBlockCount(), 0);
  EXPECT_NE(code_generator.getCode(), nullptr);
}

// Test block invalidation through BlockManager (used by ExecutionEngine)
TEST_F(ExecutionEngineComponentTest, BlockInvalidation) {
  BlockManager block_manager;
  VAddr test_addr = 0x400000;

  // Invalidate should not crash even if block doesn't exist
  EXPECT_NO_THROW(block_manager.InvalidateBlock(test_addr));
}

TEST_F(ExecutionEngineComponentTest, BlockInvalidateRange) {
  BlockManager block_manager;

  // Invalidate range should not crash
  EXPECT_NO_THROW(block_manager.InvalidateRange(0x400000, 0x500000));
}
