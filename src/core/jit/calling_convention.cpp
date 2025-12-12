// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "calling_convention.h"
#include "common/assert.h"

namespace Core::Jit {

CallingConvention::CallingConvention(Arm64CodeGenerator& codegen, RegisterMapper& reg_mapper)
    : codegen(codegen), reg_mapper(reg_mapper) {}

void CallingConvention::PrepareCall(int arg_count, const std::vector<int>& arg_regs) {
    ASSERT_MSG(arg_count <= MAX_INT_ARGS, "Too many arguments");
    ASSERT_MSG(arg_regs.size() >= static_cast<size_t>(arg_count), "Not enough argument registers");

    for (int i = 0; i < arg_count && i < MAX_INT_ARGS; i++) {
        int arm64_arg_reg = i;
        int x86_arg_reg = arg_regs[i];
        int mapped_reg = reg_mapper.MapX86_64ToArm64(static_cast<X86_64Register>(x86_arg_reg));
        if (mapped_reg != arm64_arg_reg) {
            codegen.mov(arm64_arg_reg, mapped_reg);
        }
    }
}

void CallingConvention::CallFunction(void* function_ptr) {
    codegen.movz(16, reinterpret_cast<u64>(function_ptr) & 0xFFFF);
    codegen.movk(16, (reinterpret_cast<u64>(function_ptr) >> 16) & 0xFFFF, 16);
    codegen.movk(16, (reinterpret_cast<u64>(function_ptr) >> 32) & 0xFFFF, 32);
    codegen.movk(16, (reinterpret_cast<u64>(function_ptr) >> 48) & 0xFFFF, 48);
    codegen.blr(16);
}

void CallingConvention::CallFunction(int reg) {
    codegen.blr(reg);
}

void CallingConvention::Return(int return_reg) {
    if (return_reg >= 0) {
        int arm64_return = reg_mapper.MapX86_64ToArm64(X86_64Register::RAX);
        if (return_reg != arm64_return) {
            codegen.mov(arm64_return, return_reg);
        }
    }
    codegen.ret();
}

void CallingConvention::SaveCallerSavedRegisters() {
    saved_registers.clear();
    for (int i = 0; i < 8; i++) {
        codegen.push(i);
        saved_registers.push_back(i);
    }
}

void CallingConvention::RestoreCallerSavedRegisters() {
    for (auto it = saved_registers.rbegin(); it != saved_registers.rend(); ++it) {
        codegen.pop(*it);
    }
    saved_registers.clear();
}

} // namespace Core::Jit
