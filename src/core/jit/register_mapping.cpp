// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>
#include "arm64_codegen.h"
#include "common/assert.h"
#include "register_mapping.h"

namespace Core::Jit {

RegisterMapper::RegisterMapper() : register_save_area(nullptr) {
    x86_to_arm64_map.fill(INVALID_MAPPING);
    spilled_registers.fill(false);

    x86_to_arm64_map[static_cast<size_t>(X86_64Register::RAX)] =
        GetArm64RegisterNumber(Arm64Register::X0);
    x86_to_arm64_map[static_cast<size_t>(X86_64Register::RCX)] =
        GetArm64RegisterNumber(Arm64Register::X1);
    x86_to_arm64_map[static_cast<size_t>(X86_64Register::RDX)] =
        GetArm64RegisterNumber(Arm64Register::X2);
    x86_to_arm64_map[static_cast<size_t>(X86_64Register::RBX)] =
        GetArm64RegisterNumber(Arm64Register::X19);
    x86_to_arm64_map[static_cast<size_t>(X86_64Register::RSP)] =
        GetArm64RegisterNumber(Arm64Register::SP);
    x86_to_arm64_map[static_cast<size_t>(X86_64Register::RBP)] =
        GetArm64RegisterNumber(Arm64Register::X29);
    x86_to_arm64_map[static_cast<size_t>(X86_64Register::RSI)] =
        GetArm64RegisterNumber(Arm64Register::X3);
    x86_to_arm64_map[static_cast<size_t>(X86_64Register::RDI)] =
        GetArm64RegisterNumber(Arm64Register::X0);
    x86_to_arm64_map[static_cast<size_t>(X86_64Register::R8)] =
        GetArm64RegisterNumber(Arm64Register::X4);
    x86_to_arm64_map[static_cast<size_t>(X86_64Register::R9)] =
        GetArm64RegisterNumber(Arm64Register::X5);
    x86_to_arm64_map[static_cast<size_t>(X86_64Register::R10)] =
        GetArm64RegisterNumber(Arm64Register::X6);
    x86_to_arm64_map[static_cast<size_t>(X86_64Register::R11)] =
        GetArm64RegisterNumber(Arm64Register::X7);
    x86_to_arm64_map[static_cast<size_t>(X86_64Register::R12)] =
        GetArm64RegisterNumber(Arm64Register::X20);
    x86_to_arm64_map[static_cast<size_t>(X86_64Register::R13)] =
        GetArm64RegisterNumber(Arm64Register::X21);
    x86_to_arm64_map[static_cast<size_t>(X86_64Register::R14)] =
        GetArm64RegisterNumber(Arm64Register::X22);
    x86_to_arm64_map[static_cast<size_t>(X86_64Register::R15)] =
        GetArm64RegisterNumber(Arm64Register::X23);

    x86_to_arm64_map[static_cast<size_t>(X86_64Register::XMM0)] =
        GetArm64RegisterNumber(Arm64Register::V0);
    x86_to_arm64_map[static_cast<size_t>(X86_64Register::XMM1)] =
        GetArm64RegisterNumber(Arm64Register::V1);
    x86_to_arm64_map[static_cast<size_t>(X86_64Register::XMM2)] =
        GetArm64RegisterNumber(Arm64Register::V2);
    x86_to_arm64_map[static_cast<size_t>(X86_64Register::XMM3)] =
        GetArm64RegisterNumber(Arm64Register::V3);
    x86_to_arm64_map[static_cast<size_t>(X86_64Register::XMM4)] =
        GetArm64RegisterNumber(Arm64Register::V4);
    x86_to_arm64_map[static_cast<size_t>(X86_64Register::XMM5)] =
        GetArm64RegisterNumber(Arm64Register::V5);
    x86_to_arm64_map[static_cast<size_t>(X86_64Register::XMM6)] =
        GetArm64RegisterNumber(Arm64Register::V6);
    x86_to_arm64_map[static_cast<size_t>(X86_64Register::XMM7)] =
        GetArm64RegisterNumber(Arm64Register::V7);
    x86_to_arm64_map[static_cast<size_t>(X86_64Register::XMM8)] =
        GetArm64RegisterNumber(Arm64Register::V8);
    x86_to_arm64_map[static_cast<size_t>(X86_64Register::XMM9)] =
        GetArm64RegisterNumber(Arm64Register::V9);
    x86_to_arm64_map[static_cast<size_t>(X86_64Register::XMM10)] =
        GetArm64RegisterNumber(Arm64Register::V10);
    x86_to_arm64_map[static_cast<size_t>(X86_64Register::XMM11)] =
        GetArm64RegisterNumber(Arm64Register::V11);
    x86_to_arm64_map[static_cast<size_t>(X86_64Register::XMM12)] =
        GetArm64RegisterNumber(Arm64Register::V12);
    x86_to_arm64_map[static_cast<size_t>(X86_64Register::XMM13)] =
        GetArm64RegisterNumber(Arm64Register::V13);
    x86_to_arm64_map[static_cast<size_t>(X86_64Register::XMM14)] =
        GetArm64RegisterNumber(Arm64Register::V14);
    x86_to_arm64_map[static_cast<size_t>(X86_64Register::XMM15)] =
        GetArm64RegisterNumber(Arm64Register::V15);

    x86_to_arm64_map[static_cast<size_t>(X86_64Register::FLAGS)] =
        GetArm64RegisterNumber(Arm64Register::X11);
}

int RegisterMapper::MapX86_64ToArm64(X86_64Register x86_reg) {
    size_t index = static_cast<size_t>(x86_reg);
    ASSERT_MSG(index < static_cast<size_t>(X86_64Register::COUNT), "Invalid x86_64 register");
    return x86_to_arm64_map[index];
}

int RegisterMapper::MapX86_64XmmToArm64Neon(X86_64Register xmm_reg) {
    if (!IsXmmRegister(xmm_reg)) {
        return INVALID_MAPPING;
    }
    return MapX86_64ToArm64(xmm_reg);
}

bool RegisterMapper::IsXmmRegister(X86_64Register reg) {
    return reg >= X86_64Register::XMM0 && reg <= X86_64Register::XMM15;
}

void RegisterMapper::SpillRegister(X86_64Register x86_reg) {
    size_t index = static_cast<size_t>(x86_reg);
    ASSERT_MSG(index < static_cast<size_t>(X86_64Register::COUNT), "Invalid x86_64 register");
    spilled_registers[index] = true;
}

void RegisterMapper::ReloadRegister(X86_64Register x86_reg) {
    size_t index = static_cast<size_t>(x86_reg);
    ASSERT_MSG(index < static_cast<size_t>(X86_64Register::COUNT), "Invalid x86_64 register");
    spilled_registers[index] = false;
}

bool RegisterMapper::IsRegisterSpilled(X86_64Register x86_reg) const {
    size_t index = static_cast<size_t>(x86_reg);
    ASSERT_MSG(index < static_cast<size_t>(X86_64Register::COUNT), "Invalid x86_64 register");
    return spilled_registers[index];
}

void RegisterMapper::SaveRegister(Arm64CodeGenerator& codegen, X86_64Register x86_reg,
                                  RegisterContext* ctx) {
    if (!ctx) {
        return;
    }

    int arm64_reg = MapX86_64ToArm64(x86_reg);
    if (arm64_reg == INVALID_MAPPING) {
        return;
    }

    size_t index = static_cast<size_t>(x86_reg);
    if (IsXmmRegister(x86_reg)) {
        int vreg = MapX86_64XmmToArm64Neon(x86_reg);
        if (vreg != INVALID_MAPPING) {
            codegen.movz(SCRATCH_REG,
                         reinterpret_cast<u64>(&ctx->xmm_regs[index - 16][0]) & 0xFFFF);
            codegen.movk(SCRATCH_REG,
                         (reinterpret_cast<u64>(&ctx->xmm_regs[index - 16][0]) >> 16) & 0xFFFF, 16);
            codegen.movk(SCRATCH_REG,
                         (reinterpret_cast<u64>(&ctx->xmm_regs[index - 16][0]) >> 32) & 0xFFFF, 32);
            codegen.movk(SCRATCH_REG,
                         (reinterpret_cast<u64>(&ctx->xmm_regs[index - 16][0]) >> 48) & 0xFFFF, 48);
            codegen.str_v(vreg, SCRATCH_REG, 0);
        }
    } else if (x86_reg == X86_64Register::FLAGS) {
        codegen.movz(SCRATCH_REG, reinterpret_cast<u64>(&ctx->flags) & 0xFFFF);
        codegen.movk(SCRATCH_REG, (reinterpret_cast<u64>(&ctx->flags) >> 16) & 0xFFFF, 16);
        codegen.movk(SCRATCH_REG, (reinterpret_cast<u64>(&ctx->flags) >> 32) & 0xFFFF, 32);
        codegen.movk(SCRATCH_REG, (reinterpret_cast<u64>(&ctx->flags) >> 48) & 0xFFFF, 48);
        codegen.str(arm64_reg, SCRATCH_REG, 0);
    } else if (x86_reg == X86_64Register::RSP || x86_reg == X86_64Register::RBP) {
        if (arm64_reg == STACK_POINTER) {
            codegen.mov(SCRATCH_REG, STACK_POINTER);
            codegen.movz(SCRATCH_REG2, reinterpret_cast<u64>(&ctx->rsp) & 0xFFFF);
            codegen.movk(SCRATCH_REG2, (reinterpret_cast<u64>(&ctx->rsp) >> 16) & 0xFFFF, 16);
            codegen.movk(SCRATCH_REG2, (reinterpret_cast<u64>(&ctx->rsp) >> 32) & 0xFFFF, 32);
            codegen.movk(SCRATCH_REG2, (reinterpret_cast<u64>(&ctx->rsp) >> 48) & 0xFFFF, 48);
            codegen.str(SCRATCH_REG, SCRATCH_REG2, 0);
        } else {
            codegen.movz(SCRATCH_REG, reinterpret_cast<u64>(&ctx->rbp) & 0xFFFF);
            codegen.movk(SCRATCH_REG, (reinterpret_cast<u64>(&ctx->rbp) >> 16) & 0xFFFF, 16);
            codegen.movk(SCRATCH_REG, (reinterpret_cast<u64>(&ctx->rbp) >> 32) & 0xFFFF, 32);
            codegen.movk(SCRATCH_REG, (reinterpret_cast<u64>(&ctx->rbp) >> 48) & 0xFFFF, 48);
            codegen.str(arm64_reg, SCRATCH_REG, 0);
        }
    } else {
        if (index < 16) {
            codegen.movz(SCRATCH_REG, reinterpret_cast<u64>(&ctx->gp_regs[index]) & 0xFFFF);
            codegen.movk(SCRATCH_REG, (reinterpret_cast<u64>(&ctx->gp_regs[index]) >> 16) & 0xFFFF,
                         16);
            codegen.movk(SCRATCH_REG, (reinterpret_cast<u64>(&ctx->gp_regs[index]) >> 32) & 0xFFFF,
                         32);
            codegen.movk(SCRATCH_REG, (reinterpret_cast<u64>(&ctx->gp_regs[index]) >> 48) & 0xFFFF,
                         48);
            codegen.str(arm64_reg, SCRATCH_REG, 0);
        }
    }
}

void RegisterMapper::RestoreRegister(Arm64CodeGenerator& codegen, X86_64Register x86_reg,
                                     RegisterContext* ctx) {
    if (!ctx) {
        return;
    }

    int arm64_reg = MapX86_64ToArm64(x86_reg);
    if (arm64_reg == INVALID_MAPPING) {
        return;
    }

    size_t index = static_cast<size_t>(x86_reg);
    if (IsXmmRegister(x86_reg)) {
        int vreg = MapX86_64XmmToArm64Neon(x86_reg);
        if (vreg != INVALID_MAPPING) {
            codegen.movz(SCRATCH_REG,
                         reinterpret_cast<u64>(&ctx->xmm_regs[index - 16][0]) & 0xFFFF);
            codegen.movk(SCRATCH_REG,
                         (reinterpret_cast<u64>(&ctx->xmm_regs[index - 16][0]) >> 16) & 0xFFFF, 16);
            codegen.movk(SCRATCH_REG,
                         (reinterpret_cast<u64>(&ctx->xmm_regs[index - 16][0]) >> 32) & 0xFFFF, 32);
            codegen.movk(SCRATCH_REG,
                         (reinterpret_cast<u64>(&ctx->xmm_regs[index - 16][0]) >> 48) & 0xFFFF, 48);
            codegen.ldr_v(vreg, SCRATCH_REG, 0);
        }
    } else if (x86_reg == X86_64Register::FLAGS) {
        codegen.movz(SCRATCH_REG, reinterpret_cast<u64>(&ctx->flags) & 0xFFFF);
        codegen.movk(SCRATCH_REG, (reinterpret_cast<u64>(&ctx->flags) >> 16) & 0xFFFF, 16);
        codegen.movk(SCRATCH_REG, (reinterpret_cast<u64>(&ctx->flags) >> 32) & 0xFFFF, 32);
        codegen.movk(SCRATCH_REG, (reinterpret_cast<u64>(&ctx->flags) >> 48) & 0xFFFF, 48);
        codegen.ldr(arm64_reg, SCRATCH_REG, 0);
    } else if (x86_reg == X86_64Register::RSP || x86_reg == X86_64Register::RBP) {
        if (arm64_reg == STACK_POINTER) {
            codegen.movz(SCRATCH_REG, reinterpret_cast<u64>(&ctx->rsp) & 0xFFFF);
            codegen.movk(SCRATCH_REG, (reinterpret_cast<u64>(&ctx->rsp) >> 16) & 0xFFFF, 16);
            codegen.movk(SCRATCH_REG, (reinterpret_cast<u64>(&ctx->rsp) >> 32) & 0xFFFF, 32);
            codegen.movk(SCRATCH_REG, (reinterpret_cast<u64>(&ctx->rsp) >> 48) & 0xFFFF, 48);
            codegen.ldr(SCRATCH_REG2, SCRATCH_REG, 0);
            codegen.mov(STACK_POINTER, SCRATCH_REG2);
        } else {
            codegen.movz(SCRATCH_REG, reinterpret_cast<u64>(&ctx->rbp) & 0xFFFF);
            codegen.movk(SCRATCH_REG, (reinterpret_cast<u64>(&ctx->rbp) >> 16) & 0xFFFF, 16);
            codegen.movk(SCRATCH_REG, (reinterpret_cast<u64>(&ctx->rbp) >> 32) & 0xFFFF, 32);
            codegen.movk(SCRATCH_REG, (reinterpret_cast<u64>(&ctx->rbp) >> 48) & 0xFFFF, 48);
            codegen.ldr(arm64_reg, SCRATCH_REG, 0);
        }
    } else {
        if (index < 16) {
            codegen.movz(SCRATCH_REG, reinterpret_cast<u64>(&ctx->gp_regs[index]) & 0xFFFF);
            codegen.movk(SCRATCH_REG, (reinterpret_cast<u64>(&ctx->gp_regs[index]) >> 16) & 0xFFFF,
                         16);
            codegen.movk(SCRATCH_REG, (reinterpret_cast<u64>(&ctx->gp_regs[index]) >> 32) & 0xFFFF,
                         32);
            codegen.movk(SCRATCH_REG, (reinterpret_cast<u64>(&ctx->gp_regs[index]) >> 48) & 0xFFFF,
                         48);
            codegen.ldr(arm64_reg, SCRATCH_REG, 0);
        }
    }
}

void RegisterMapper::SaveAllRegisters(Arm64CodeGenerator& codegen, RegisterContext* ctx) {
    if (!ctx) {
        return;
    }

    for (int i = 0; i < 16; i++) {
        SaveRegister(codegen, static_cast<X86_64Register>(i), ctx);
    }
    for (int i = 16; i < 32; i++) {
        SaveRegister(codegen, static_cast<X86_64Register>(i), ctx);
    }
    SaveRegister(codegen, X86_64Register::FLAGS, ctx);
}

void RegisterMapper::RestoreAllRegisters(Arm64CodeGenerator& codegen, RegisterContext* ctx) {
    if (!ctx) {
        return;
    }

    RestoreRegister(codegen, X86_64Register::FLAGS, ctx);
    for (int i = 16; i < 32; i++) {
        RestoreRegister(codegen, static_cast<X86_64Register>(i), ctx);
    }
    for (int i = 0; i < 16; i++) {
        RestoreRegister(codegen, static_cast<X86_64Register>(i), ctx);
    }
}

} // namespace Core::Jit
