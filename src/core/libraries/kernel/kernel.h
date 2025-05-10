// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <algorithm>
#include <fmt/core.h>
#include "common/string_literal.h"
#include "common/types.h"
#include "core/libraries/kernel/orbis_error.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Kernel {

void ErrSceToPosix(int result);
int ErrnoToSceKernelError(int e);
void SetPosixErrno(int e);
int* PS4_SYSV_ABI __Error();

template <StringLiteral name, class F, F f>
struct WrapperImpl;

template <StringLiteral name, class R, class... Args, PS4_SYSV_ABI R (*f)(Args...)>
struct WrapperImpl<name, PS4_SYSV_ABI R (*)(Args...), f> {
    static constexpr StringLiteral Name{name};
    static R PS4_SYSV_ABI wrap(Args... args) {
        u32 ret = f(args...);
        if (ret != 0) {
            // LOG_ERROR(Lib_Kernel, "Function {} returned {}", std::string_view{name.value}, ret);
            ret += ORBIS_KERNEL_ERROR_UNKNOWN;
        }
        return ret;
    }
};

template <StringLiteral name, class F, F f>
constexpr auto OrbisWrapper = WrapperImpl<name, F, f>::wrap;

#define ORBIS(func) WrapperImpl<#func, decltype(&func), func>::wrap

int* PS4_SYSV_ABI __Error();

void RegisterKernel(Core::Loader::SymbolsResolver* sym);

} // namespace Libraries::Kernel
