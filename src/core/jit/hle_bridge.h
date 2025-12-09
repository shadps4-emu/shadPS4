// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "arm64_codegen.h"
#include "calling_convention.h"
#include "register_mapping.h"

namespace Core::Jit {

class HleBridge {
public:
    explicit HleBridge(Arm64CodeGenerator& codegen, RegisterMapper& reg_mapper);
    ~HleBridge() = default;

    // Generate bridge code to call an HLE function
    // hle_func: Pointer to the HLE function
    // int_arg_count: Number of integer arguments (0-6 for x86_64 System V ABI)
    // float_arg_count: Number of floating point arguments (0-8 for x86_64 System V ABI)
    void GenerateBridge(void* hle_func, int int_arg_count = 0, int float_arg_count = 0);

    // Check if an address is an HLE function
    static bool IsHleAddress(VAddr address);

    // Get HLE function pointer for an address
    static void* GetHleFunction(VAddr address);

private:
    void SaveCallerSavedRegisters();
    void RestoreCallerSavedRegisters();
    void MapArguments(int int_arg_count, int float_arg_count);
    void MapReturnValue();

    Arm64CodeGenerator& codegen;
    RegisterMapper& reg_mapper;
    CallingConvention calling_conv;
};

} // namespace Core::Jit
