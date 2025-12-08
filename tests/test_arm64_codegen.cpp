// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/jit/arm64_codegen.h"
#include <cstring>
#include <gtest/gtest.h>
#include <sys/mman.h>

using namespace Core::Jit;

class Arm64CodeGenTest : public ::testing::Test {
protected:
  void SetUp() override { test_gen = std::make_unique<Arm64CodeGenerator>(); }

  void TearDown() override { test_gen.reset(); }

  std::unique_ptr<Arm64CodeGenerator> test_gen;
};

TEST_F(Arm64CodeGenTest, Constructor) {
  EXPECT_NE(test_gen->getCode(), nullptr);
  EXPECT_EQ(test_gen->getSize(), 0);
}

TEST_F(Arm64CodeGenTest, Reset) {
  test_gen->add(0, 1, 2);
  size_t size_after_add = test_gen->getSize();
  EXPECT_GT(size_after_add, 0);

  test_gen->reset();
  EXPECT_EQ(test_gen->getSize(), 0);
}

TEST_F(Arm64CodeGenTest, AddInstruction) {
  test_gen->add(0, 1, 2); // X0 = X1 + X2
  EXPECT_GT(test_gen->getSize(), 0);
  EXPECT_LE(test_gen->getSize(), 4); // Should be 4 bytes (one instruction)
}

TEST_F(Arm64CodeGenTest, AddImmediate) {
  test_gen->add_imm(0, 1, 42); // X0 = X1 + 42
  EXPECT_GT(test_gen->getSize(), 0);
}

TEST_F(Arm64CodeGenTest, MovRegister) {
  test_gen->mov(0, 1); // X0 = X1
  EXPECT_GT(test_gen->getSize(), 0);
}

TEST_F(Arm64CodeGenTest, MovImmediate) {
  test_gen->mov(0, 0x1234LL); // X0 = 0x1234
  EXPECT_GT(test_gen->getSize(), 0);
  // Large immediate may require multiple instructions
  EXPECT_LE(test_gen->getSize(),
            16); // Up to 4 instructions for 64-bit immediate
}

TEST_F(Arm64CodeGenTest, LoadStore) {
  test_gen->ldr(0, 1, 0);            // X0 = [X1]
  test_gen->str(0, 1, 0);            // [X1] = X0
  EXPECT_GE(test_gen->getSize(), 8); // At least 2 instructions
}

TEST_F(Arm64CodeGenTest, Branch) {
  void *target = test_gen->getCode(); // Branch to start of code
  test_gen->b(target);
  EXPECT_GT(test_gen->getSize(), 0);
}

TEST_F(Arm64CodeGenTest, ConditionalBranch) {
  void *target = test_gen->getCode(); // Branch to start of code
  test_gen->b(0, target);             // Branch if equal
  EXPECT_GT(test_gen->getSize(), 0);
}

TEST_F(Arm64CodeGenTest, Compare) {
  test_gen->cmp(0, 1); // Compare X0 and X1
  EXPECT_GT(test_gen->getSize(), 0);
}

TEST_F(Arm64CodeGenTest, ArithmeticOperations) {
  test_gen->add(0, 1, 2);
  test_gen->sub(0, 1, 2);
  test_gen->mul(0, 1, 2);
  test_gen->and_(0, 1, 2);
  test_gen->orr(0, 1, 2);
  test_gen->eor(0, 1, 2);
  EXPECT_GE(test_gen->getSize(), 24); // At least 6 instructions
}

TEST_F(Arm64CodeGenTest, SIMDOperations) {
  test_gen->mov_v(0, 1);              // V0 = V1
  test_gen->add_v(0, 1, 2);           // V0 = V1 + V2
  test_gen->sub_v(0, 1, 2);           // V0 = V1 - V2
  test_gen->mul_v(0, 1, 2);           // V0 = V1 * V2
  EXPECT_GE(test_gen->getSize(), 16); // At least 4 instructions
}

TEST_F(Arm64CodeGenTest, SetSize) {
  test_gen->add(0, 1, 2);
  size_t original_size = test_gen->getSize();
  EXPECT_GT(original_size, 0);

  // Test setting size to 0
  test_gen->setSize(0);
  EXPECT_EQ(test_gen->getSize(), 0);

  // Test setting size back (this should work without throwing)
  test_gen->setSize(original_size);
  EXPECT_EQ(test_gen->getSize(), original_size);
}
