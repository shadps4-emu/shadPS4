// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/jit/arm64_codegen.h"
#include "core/jit/hle_bridge.h"
#include "core/jit/register_mapping.h"
#include <gtest/gtest.h>
#include <sys/mman.h>
#if defined(__APPLE__) && defined(ARCH_ARM64)
#include <pthread.h>
#endif

using namespace Core::Jit;

// Simple test HLE function
extern "C" PS4_SYSV_ABI u64 TestHleFunction(u64 arg1, u64 arg2) {
  return arg1 + arg2;
}

class HleBridgeTest : public ::testing::Test {
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
    hle_bridge = std::make_unique<HleBridge>(*codegen, *register_mapper);
  }

  void TearDown() override {
    hle_bridge.reset();
    register_mapper.reset();
    codegen.reset();
    if (test_code_buffer != MAP_FAILED) {
      munmap(test_code_buffer, 64 * 1024);
    }
  }

  void *test_code_buffer = MAP_FAILED;
  std::unique_ptr<Arm64CodeGenerator> codegen;
  std::unique_ptr<RegisterMapper> register_mapper;
  std::unique_ptr<HleBridge> hle_bridge;
};

// Test that HLE bridge can be constructed
TEST_F(HleBridgeTest, Construction) { EXPECT_NE(hle_bridge, nullptr); }

// Test that we can generate a bridge to an HLE function
TEST_F(HleBridgeTest, GenerateBridge) {
  void *hle_func = reinterpret_cast<void *>(TestHleFunction);

  // Generate bridge code
  hle_bridge->GenerateBridge(hle_func, 2); // 2 integer arguments

  // Should generate some code
  EXPECT_GT(codegen->getSize(), 0) << "HLE bridge should generate code";
}

// Test that bridge preserves caller-saved registers
TEST_F(HleBridgeTest, BridgePreservesRegisters) {
  // This is a placeholder test - full register preservation testing
  // would require execution, which is complex
  void *hle_func = reinterpret_cast<void *>(TestHleFunction);

  size_t size_before = codegen->getSize();
  hle_bridge->GenerateBridge(hle_func, 2);
  size_t size_after = codegen->getSize();

  // Bridge should generate substantial code for register preservation
  EXPECT_GT(size_after - size_before, 8) << "Bridge should preserve registers";
}
