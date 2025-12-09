// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>
#include "common/logging/log.h"
#include "hle_bridge.h"

namespace Core::Jit {

HleBridge::HleBridge(Arm64CodeGenerator& codegen, RegisterMapper& reg_mapper)
    : codegen(codegen), reg_mapper(reg_mapper), calling_conv(codegen, reg_mapper) {}

void HleBridge::GenerateBridge(void* hle_func, int int_arg_count, int float_arg_count) {
    // Save caller-saved registers (x86_64: RAX, RCX, RDX, RSI, RDI, R8-R11)
    // These correspond to ARM64: X0-X7, X9-X15 (some are callee-saved, but we save all to be safe)
    SaveCallerSavedRegisters();

    // Map x86_64 arguments to ARM64 calling convention
    // x86_64 System V ABI: RDI, RSI, RDX, RCX, R8, R9 (integer), XMM0-XMM7 (float)
    // ARM64: X0-X7 (integer), V0-V7 (float)
    MapArguments(int_arg_count, float_arg_count);

    // Call the HLE function
    calling_conv.CallFunction(hle_func);

    // Map return value from ARM64 X0 to x86_64 RAX
    MapReturnValue();

    // Restore caller-saved registers
    RestoreCallerSavedRegisters();
}

void HleBridge::SaveCallerSavedRegisters() {
    // x86_64 caller-saved registers: RAX, RCX, RDX, RSI, RDI, R8, R9, R10, R11
    // Map to ARM64 and save them
    // Note: We need to be careful about which registers are actually caller-saved in ARM64
    // ARM64 caller-saved: X0-X7, X9-X15, V0-V7, V16-V31
    // We'll save the x86_64 registers that map to ARM64 caller-saved registers

    // Save integer registers that are caller-saved
    // RAX -> X0, RCX -> X1, RDX -> X2, RSI -> X3, RDI -> X0 (reused), R8 -> X4, R9 -> X5
    // We'll save X0-X7 to be safe (they're all caller-saved in ARM64)
    for (int i = 0; i < 8; ++i) {
        codegen.push(i); // Save X0-X7
    }

    // Save XMM registers (V0-V7 in ARM64)
    // x86_64 XMM0-XMM7 map to ARM64 V0-V7
    for (int i = 0; i < 8; ++i) {
        codegen.sub_imm(31, 31, 16); // Decrement stack pointer by 16 bytes
        codegen.str_v(i, 31, 0);     // Store V0-V7
    }
}

void HleBridge::RestoreCallerSavedRegisters() {
    // Restore XMM registers first (reverse order)
    for (int i = 7; i >= 0; --i) {
        codegen.ldr_v(i, 31, 0);     // Load V0-V7
        codegen.add_imm(31, 31, 16); // Increment stack pointer by 16 bytes
    }

    // Restore integer registers (reverse order)
    for (int i = 7; i >= 0; --i) {
        codegen.pop(i); // Restore X0-X7
    }
}

void HleBridge::MapArguments(int int_arg_count, int float_arg_count) {
    // x86_64 System V ABI argument registers:
    // Integer: RDI (arg1), RSI (arg2), RDX (arg3), RCX (arg4), R8 (arg5), R9 (arg6)
    // Float: XMM0 (arg1), XMM1 (arg2), XMM2 (arg3), XMM3 (arg4), XMM4 (arg5), XMM5 (arg6), XMM6
    // (arg7), XMM7 (arg8)

    // ARM64 calling convention:
    // Integer: X0 (arg1), X1 (arg2), X2 (arg3), X3 (arg4), X4 (arg5), X5 (arg6), X6 (arg7), X7
    // (arg8) Float: V0 (arg1), V1 (arg2), V2 (arg3), V3 (arg4), V4 (arg5), V5 (arg6), V6 (arg7), V7
    // (arg8)

    // Map integer arguments
    static constexpr X86_64Register x86_int_args[] = {
        X86_64Register::RDI, // arg1
        X86_64Register::RSI, // arg2
        X86_64Register::RDX, // arg3
        X86_64Register::RCX, // arg4
        X86_64Register::R8,  // arg5
        X86_64Register::R9,  // arg6
    };

    for (int i = 0; i < int_arg_count && i < 6; ++i) {
        int x86_reg = reg_mapper.MapX86_64ToArm64(x86_int_args[i]);
        int arm64_arg_reg = i; // X0, X1, X2, etc.
        if (x86_reg != arm64_arg_reg) {
            codegen.mov(arm64_arg_reg, x86_reg);
        }
    }

    // Map floating point arguments
    static constexpr X86_64Register x86_float_args[] = {
        X86_64Register::XMM0, // arg1
        X86_64Register::XMM1, // arg2
        X86_64Register::XMM2, // arg3
        X86_64Register::XMM3, // arg4
        X86_64Register::XMM4, // arg5
        X86_64Register::XMM5, // arg6
        X86_64Register::XMM6, // arg7
        X86_64Register::XMM7, // arg8
    };

    for (int i = 0; i < float_arg_count && i < 8; ++i) {
        int x86_xmm_reg = reg_mapper.MapX86_64XmmToArm64Neon(x86_float_args[i]);
        int arm64_arg_reg = i; // V0, V1, V2, etc.
        if (x86_xmm_reg != arm64_arg_reg) {
            codegen.mov_v(arm64_arg_reg, x86_xmm_reg);
        }
    }
}

void HleBridge::MapReturnValue() {
    // Return value: ARM64 X0 -> x86_64 RAX
    int arm64_return = 0; // X0
    int x86_return = reg_mapper.MapX86_64ToArm64(X86_64Register::RAX);
    if (x86_return != arm64_return) {
        codegen.mov(x86_return, arm64_return);
    }
}

bool HleBridge::IsHleAddress(VAddr address) {
    // TODO: Implement HLE address lookup
    (void)address;
    return false;
}

void* HleBridge::GetHleFunction(VAddr address) {
    // TODO: Implement HLE function lookup
    (void)address;
    return nullptr;
}

} // namespace Core::Jit
