// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <vector>
#include "arm64_codegen.h"
#include "register_mapping.h"

namespace Core::Jit {

class CallingConvention {
public:
    explicit CallingConvention(Arm64CodeGenerator& codegen, RegisterMapper& reg_mapper);

    void PrepareCall(int arg_count, const std::vector<int>& arg_regs);
    void CallFunction(void* function_ptr);
    void CallFunction(int reg);
    void Return(int return_reg = -1);

    void SaveCallerSavedRegisters();
    void RestoreCallerSavedRegisters();

    static constexpr int MAX_INT_ARGS = 8;
    static constexpr int MAX_FLOAT_ARGS = 8;

private:
    Arm64CodeGenerator& codegen;
    RegisterMapper& reg_mapper;
    std::vector<int> saved_registers;
};

} // namespace Core::Jit
