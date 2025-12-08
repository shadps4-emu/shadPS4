// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include "common/types.h"
#include "core/jit/arm64_codegen.h"

namespace Core::Jit {

enum class X86_64Register : u8 {
    RAX = 0,
    RCX = 1,
    RDX = 2,
    RBX = 3,
    RSP = 4,
    RBP = 5,
    RSI = 6,
    RDI = 7,
    R8 = 8,
    R9 = 9,
    R10 = 10,
    R11 = 11,
    R12 = 12,
    R13 = 13,
    R14 = 14,
    R15 = 15,
    XMM0 = 16,
    XMM1 = 17,
    XMM2 = 18,
    XMM3 = 19,
    XMM4 = 20,
    XMM5 = 21,
    XMM6 = 22,
    XMM7 = 23,
    XMM8 = 24,
    XMM9 = 25,
    XMM10 = 26,
    XMM11 = 27,
    XMM12 = 28,
    XMM13 = 29,
    XMM14 = 30,
    XMM15 = 31,
    FLAGS = 32,
    COUNT = 33
};

enum class Arm64Register : u8 {
    X0 = 0,
    X1 = 1,
    X2 = 2,
    X3 = 3,
    X4 = 4,
    X5 = 5,
    X6 = 6,
    X7 = 7,
    X8 = 8,
    X9 = 9,
    X10 = 10,
    X11 = 11,
    X12 = 12,
    X13 = 13,
    X14 = 14,
    X15 = 15,
    X16 = 16,
    X17 = 17,
    X18 = 18,
    X19 = 19,
    X20 = 20,
    X21 = 21,
    X22 = 22,
    X23 = 23,
    X24 = 24,
    X25 = 25,
    X26 = 26,
    X27 = 27,
    X28 = 28,
    X29 = 29,
    X30 = 30,
    SP = 31,
    V0 = 32,
    V1 = 33,
    V2 = 34,
    V3 = 35,
    V4 = 36,
    V5 = 37,
    V6 = 38,
    V7 = 39,
    V8 = 40,
    V9 = 41,
    V10 = 42,
    V11 = 43,
    V12 = 44,
    V13 = 45,
    V14 = 46,
    V15 = 47,
    COUNT = 48
};

struct RegisterContext {
    u64 gp_regs[16];
    u64 xmm_regs[16][2];
    u64 flags;
    u64 rsp;
    u64 rbp;
};

class RegisterMapper {
public:
    RegisterMapper();

    int MapX86_64ToArm64(X86_64Register x86_reg);
    int MapX86_64XmmToArm64Neon(X86_64Register xmm_reg);
    bool IsXmmRegister(X86_64Register reg);

    void SpillRegister(X86_64Register x86_reg);
    void ReloadRegister(X86_64Register x86_reg);
    bool IsRegisterSpilled(X86_64Register x86_reg) const;

    void SaveAllRegisters(Arm64CodeGenerator& codegen, RegisterContext* ctx);
    void RestoreAllRegisters(Arm64CodeGenerator& codegen, RegisterContext* ctx);
    void SaveRegister(Arm64CodeGenerator& codegen, X86_64Register x86_reg, RegisterContext* ctx);
    void RestoreRegister(Arm64CodeGenerator& codegen, X86_64Register x86_reg, RegisterContext* ctx);

    static constexpr int SCRATCH_REG = 9;
    static constexpr int SCRATCH_REG2 = 10;
    static constexpr int FLAGS_REG = 11;
    static constexpr int STACK_POINTER = 31;

private:
    static constexpr int INVALID_MAPPING = -1;

    std::array<int, static_cast<size_t>(X86_64Register::COUNT)> x86_to_arm64_map;
    std::array<bool, static_cast<size_t>(X86_64Register::COUNT)> spilled_registers;
    void* register_save_area;
};

inline int GetArm64RegisterNumber(Arm64Register reg) {
    return static_cast<int>(reg);
}

inline int GetX86_64RegisterNumber(X86_64Register reg) {
    return static_cast<int>(reg);
}

} // namespace Core::Jit
