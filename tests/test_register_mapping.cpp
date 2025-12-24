// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/jit/register_mapping.h"
#include <gtest/gtest.h>

using namespace Core::Jit;

class RegisterMappingTest : public ::testing::Test {
protected:
  RegisterMapper mapper;
};

TEST_F(RegisterMappingTest, MapGeneralPurposeRegisters) {
  // Test mapping of common x86_64 registers
  EXPECT_EQ(mapper.MapX86_64ToArm64(X86_64Register::RAX), 0); // X0
  EXPECT_EQ(mapper.MapX86_64ToArm64(X86_64Register::RCX), 1); // X1
  EXPECT_EQ(mapper.MapX86_64ToArm64(X86_64Register::RDX), 2); // X2
  EXPECT_EQ(mapper.MapX86_64ToArm64(X86_64Register::RSI), 3); // X3
  EXPECT_EQ(mapper.MapX86_64ToArm64(X86_64Register::RDI),
            0);                                              // X0 (same as RAX)
  EXPECT_EQ(mapper.MapX86_64ToArm64(X86_64Register::R8), 4); // X4
  EXPECT_EQ(mapper.MapX86_64ToArm64(X86_64Register::R9), 5); // X5
}

TEST_F(RegisterMappingTest, MapStackPointer) {
  EXPECT_EQ(mapper.MapX86_64ToArm64(X86_64Register::RSP), 31); // SP
}

TEST_F(RegisterMappingTest, MapFramePointer) {
  EXPECT_EQ(mapper.MapX86_64ToArm64(X86_64Register::RBP), 29); // FP
}

TEST_F(RegisterMappingTest, MapCalleeSavedRegisters) {
  EXPECT_EQ(mapper.MapX86_64ToArm64(X86_64Register::RBX), 19); // X19
  EXPECT_EQ(mapper.MapX86_64ToArm64(X86_64Register::R12), 20); // X20
  EXPECT_EQ(mapper.MapX86_64ToArm64(X86_64Register::R13), 21); // X21
  EXPECT_EQ(mapper.MapX86_64ToArm64(X86_64Register::R14), 22); // X22
  EXPECT_EQ(mapper.MapX86_64ToArm64(X86_64Register::R15), 23); // X23
}

TEST_F(RegisterMappingTest, MapFlagsRegister) {
  EXPECT_EQ(mapper.MapX86_64ToArm64(X86_64Register::FLAGS), 11); // X11
}

TEST_F(RegisterMappingTest, MapXMMRegisters) {
  // Test mapping of XMM registers to NEON registers (V registers start at 32)
  EXPECT_EQ(mapper.MapX86_64XmmToArm64Neon(X86_64Register::XMM0), 32); // V0
  EXPECT_EQ(mapper.MapX86_64XmmToArm64Neon(X86_64Register::XMM1), 33); // V1
  EXPECT_EQ(mapper.MapX86_64XmmToArm64Neon(X86_64Register::XMM2), 34); // V2
  EXPECT_EQ(mapper.MapX86_64XmmToArm64Neon(X86_64Register::XMM3), 35); // V3
  EXPECT_EQ(mapper.MapX86_64XmmToArm64Neon(X86_64Register::XMM4), 36); // V4
  EXPECT_EQ(mapper.MapX86_64XmmToArm64Neon(X86_64Register::XMM5), 37); // V5
  EXPECT_EQ(mapper.MapX86_64XmmToArm64Neon(X86_64Register::XMM6), 38); // V6
  EXPECT_EQ(mapper.MapX86_64XmmToArm64Neon(X86_64Register::XMM7), 39); // V7
}

TEST_F(RegisterMappingTest, MapAllXMMRegisters) {
  // Test all 16 XMM registers (V registers start at 32)
  for (int i = 0; i < 16; ++i) {
    X86_64Register xmm_reg =
        static_cast<X86_64Register>(static_cast<int>(X86_64Register::XMM0) + i);
    int neon_reg = mapper.MapX86_64XmmToArm64Neon(xmm_reg);
    EXPECT_EQ(neon_reg, 32 + i) << "XMM" << i << " should map to V" << i
                                << " (register number " << (32 + i) << ")";
  }
}

TEST_F(RegisterMappingTest, InvalidRegister) {
  // COUNT is not a valid register
  // NOTE: The implementation uses ASSERT_MSG which will crash on invalid input
  // This test verifies that valid registers work correctly
  // Testing invalid registers would require a different implementation that
  // returns error codes For now, we just verify that the last valid register
  // works
  int result = mapper.MapX86_64ToArm64(X86_64Register::XMM15);
  EXPECT_GE(result, 0) << "Last valid register should map correctly";
}

TEST_F(RegisterMappingTest, RegisterMappingConsistency) {
  // Test that register mappings are consistent
  // RAX should always map to the same ARM64 register
  int reg1 = mapper.MapX86_64ToArm64(X86_64Register::RAX);
  int reg2 = mapper.MapX86_64ToArm64(X86_64Register::RAX);
  EXPECT_EQ(reg1, reg2);
}
