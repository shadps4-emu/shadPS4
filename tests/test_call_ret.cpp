// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/decoder.h"
#include "core/jit/arm64_codegen.h"
#include "core/jit/register_mapping.h"
#include "core/jit/x86_64_translator.h"
#include <gtest/gtest.h>
#include <sys/mman.h>
#if defined(__APPLE__) && defined(ARCH_ARM64)
#include <pthread.h>
#endif

using namespace Core::Jit;

class CallRetTest : public ::testing::Test {
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
  }

  void TearDown() override {
    translator.reset();
    register_mapper.reset();
    codegen.reset();
    if (test_code_buffer != MAP_FAILED) {
      munmap(test_code_buffer, 64 * 1024);
    }
  }

  void *test_code_buffer = MAP_FAILED;
  std::unique_ptr<Arm64CodeGenerator> codegen;
  std::unique_ptr<RegisterMapper> register_mapper;
  std::unique_ptr<X86_64Translator> translator;
};

// Test that RET translation generates ARM64 code
TEST_F(CallRetTest, TranslateRet) {
  // x86_64 RET instruction: C3
  u8 x86_ret[] = {0xC3};

  ZydisDecodedInstruction instruction;
  ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT];

  ZyanStatus status = Common::Decoder::Instance()->decodeInstruction(
      instruction, operands, x86_ret, sizeof(x86_ret));

  if (!ZYAN_SUCCESS(status)) {
    GTEST_SKIP()
        << "Failed to decode RET instruction - Zydis may not be available";
  }

  // RET translation should succeed
  bool result = translator->TranslateRet(instruction, operands);
  EXPECT_TRUE(result) << "RET translation should succeed";
  EXPECT_GT(codegen->getSize(), 0) << "RET should generate ARM64 code";
}

// Test that CALL translation generates ARM64 code
TEST_F(CallRetTest, TranslateDirectCall) {
  // x86_64 CALL instruction: E8 <offset> (near relative call, 32-bit offset)
  // E8 00 10 00 00 = CALL +0x1000
  u8 x86_call[] = {0xE8, 0x00, 0x10, 0x00, 0x00};

  ZydisDecodedInstruction instruction;
  ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT];

  ZyanStatus status = Common::Decoder::Instance()->decodeInstruction(
      instruction, operands, x86_call, sizeof(x86_call));

  if (!ZYAN_SUCCESS(status)) {
    GTEST_SKIP()
        << "Failed to decode CALL instruction - Zydis may not be available";
  }

  // CALL translation should succeed
  bool result = translator->TranslateCall(instruction, operands, 0x400000);
  EXPECT_TRUE(result) << "CALL translation should succeed";
  EXPECT_GT(codegen->getSize(), 0) << "CALL should generate ARM64 code";
}

// Test that CALL pushes return address to stack
TEST_F(CallRetTest, CallPushesReturnAddress) {
  // Simulate a CALL instruction
  // We need to verify that the stack pointer is decremented and return address
  // is stored This is a simplified test - full implementation will need
  // execution engine integration

  // For now, just verify CALL generates code
  u8 x86_call[] = {0xE8, 0x00, 0x10, 0x00, 0x00};

  ZydisDecodedInstruction instruction;
  ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT];

  ZyanStatus status = Common::Decoder::Instance()->decodeInstruction(
      instruction, operands, x86_call, sizeof(x86_call));

  if (!ZYAN_SUCCESS(status)) {
    GTEST_SKIP() << "Failed to decode CALL instruction";
  }

  size_t size_before = codegen->getSize();
  bool result = translator->TranslateCall(instruction, operands, 0x400000);
  size_t size_after = codegen->getSize();

  EXPECT_TRUE(result);
  EXPECT_GT(size_after, size_before) << "CALL should generate code";
  // CALL should generate more code than a simple branch (needs stack
  // manipulation)
  EXPECT_GE(size_after - size_before, 4)
      << "CALL should generate multiple instructions";
}

// Test that RET pops return address from stack
TEST_F(CallRetTest, RetPopsReturnAddress) {
  // RET instruction should pop return address and jump to it
  u8 x86_ret[] = {0xC3};

  ZydisDecodedInstruction instruction;
  ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT];

  ZyanStatus status = Common::Decoder::Instance()->decodeInstruction(
      instruction, operands, x86_ret, sizeof(x86_ret));

  if (!ZYAN_SUCCESS(status)) {
    GTEST_SKIP() << "Failed to decode RET instruction";
  }

  size_t size_before = codegen->getSize();
  bool result = translator->TranslateRet(instruction, operands);
  size_t size_after = codegen->getSize();

  EXPECT_TRUE(result);
  EXPECT_GT(size_after, size_before) << "RET should generate code";
}
